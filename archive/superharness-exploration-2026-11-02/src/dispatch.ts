import { mkdirSync } from "node:fs";
import { join } from "node:path";
import type { Database } from "bun:sqlite";
import {
  dispatchById,
  heartbeat,
  markLedgerExited,
  nextPendingDispatch,
  reportTicket,
  ticketById,
  triggerById,
} from "./db";
import { assertNotNested } from "./guards";
import { assertThreadBudget, loadConfig, readThreadStartTicks, threadCount, type HarnessConfig } from "./config";
import { killOwned, spawnOwned, workerCommand } from "./process";
import type { DispatchRow, TicketRow } from "./types";

export interface DispatchOnceResult {
  dryRun: boolean;
  dispatched: boolean;
  ticketId?: string;
  dispatchId?: string;
  ledgerId?: string;
  pid?: number | null;
  elapsedMs: number;
  tokens: number;
  threadsBefore: number;
  threadsAfter: number;
  timedOut: boolean;
  tokenExceeded: boolean;
  residual: boolean;
  dispatch?: DispatchRow;
  ticket?: TicketRow;
  outbox: number;
}

function readUsageTokens(value: unknown): number {
  if (!value || typeof value !== "object") return 0;
  const record = value as Record<string, unknown>;
  const usage = record.usage;
  if (usage && typeof usage === "object") {
    const total = (usage as Record<string, unknown>).totalTokens;
    if (typeof total === "number" && Number.isFinite(total)) return total;
  }
  let max = 0;
  for (const child of Object.values(record)) max = Math.max(max, readUsageTokens(child));
  return max;
}

async function readTokens(
  stream: ReadableStream<Uint8Array> | undefined,
  onTokens: (tokens: number) => void,
): Promise<number> {
  if (!stream) return 0;
  const reader = stream.getReader();
  const decoder = new TextDecoder();
  let buffer = "";
  let max = 0;
  try {
    while (true) {
      const { done, value } = await reader.read();
      if (done) break;
      buffer += decoder.decode(value, { stream: true });
      let newline = buffer.indexOf("\n");
      while (newline >= 0) {
        const line = buffer.slice(0, newline);
        buffer = buffer.slice(newline + 1);
        if (line.trim() !== "") {
          try {
            const tokens = readUsageTokens(JSON.parse(line));
            if (tokens > max) {
              max = tokens;
              onTokens(tokens);
            }
          } catch {
            // Non-JSON diagnostics do not affect usage accounting.
          }
        }
        newline = buffer.indexOf("\n");
      }
    }
  } finally {
    reader.releaseLock();
  }
  return max;
}

function workerPrompt(ticketId: string, dispatchId: string, task: string): string {
  return [
    "You are a read-only superharness worker. Only read, grep and glob are available; writing, editing and shell commands are unavailable.",
    "Read the task and the files it names, then summarize. Do not modify any file.",
    "Call the harness_report tool exactly once:",
    `ticket_id=${JSON.stringify(ticketId)}`,
    `dispatch_id=${JSON.stringify(dispatchId)}`,
    'status="ready", summary="<concise result>", outcome="succeeded", files_modified=[], tokens_used=0',
    'If the task cannot be done read-only, report status="blocked" with a question instead.',
    "Reply DONE after the report.",
    "",
    `Task: ${task}`,
  ].join("\n");
}

/**
 * Decision-protocol claim (omp-workflow adapter): reserve one pending dispatch
 * for a named worker without spawning anything. The caller owns the process.
 * Enforces "one active dispatch per ticket" and persists pid/pgid later via
 * spawnOwned; here only the state transition and the worker id are recorded.
 */
export function claimDispatch(
  db: Database,
  dispatchId: string,
  workerId: string,
  workdir: string | null = null,
): { dispatchId: string; ticketId: string; attempt: number; workdir: string | null } {
  const dispatch = dispatchById(db, dispatchId);
  if (!dispatch) throw new Error(`dispatch not found: ${dispatchId}`);
  if (dispatch.state !== "pending") throw new Error(`dispatch not pending: ${dispatch.state}`);
  const busy = db
    .query("select id from dispatch where ticket_id = ? and state = 'running' limit 1")
    .get(dispatch.ticket_id) as { id: string } | null;
  if (busy) throw new Error(`ticket already has a running dispatch: ${busy.id}`);
  const now = Date.now();
  db.transaction(() => {
    db.query("update dispatch set state = 'running', started_at = ?, heartbeat_at = ? where id = ?").run(
      now,
      now,
      dispatchId,
    );
    db.query("update ticket set status = 'running', updated_at = ? where id = ?").run(now, dispatch.ticket_id);
    db.query(
      "insert into worker(id, ticket_id, state, session_dir, workdir, started_at, last_heartbeat_at) " +
        "values (?, ?, 'running', ?, ?, ?, ?) " +
        "on conflict(id) do update set state = 'running', workdir = excluded.workdir, last_heartbeat_at = excluded.last_heartbeat_at",
    ).run(workerId, dispatch.ticket_id, workdir ?? "", workdir ?? "", now, now);
  })();
  return { dispatchId, ticketId: dispatch.ticket_id, attempt: dispatch.attempt, workdir };
}

/**
 * D4: turn one pending dispatch into exactly one worker process. The worker is
 * read-only by `--tools`; heartbeat, timeout, token cap and cleanup are owned by
 * the harness, not by the model.
 */
export async function dispatchOnce(
  db: Database,
  config: HarnessConfig = loadConfig(),
): Promise<DispatchOnceResult> {
  assertNotNested(config);
  assertThreadBudget(config);
  const pending = nextPendingDispatch(db);
  const base: DispatchOnceResult = {
    dryRun: config.dryRun,
    dispatched: false,
    elapsedMs: 0,
    tokens: 0,
    threadsBefore: threadCount(),
    threadsAfter: 0,
    timedOut: false,
    tokenExceeded: false,
    residual: false,
    outbox: 0,
  };
  if (!pending) return { ...base, threadsAfter: base.threadsBefore };
  const { dispatch, ticket } = pending;
  const trigger = triggerById(db, ticket.trigger_id);
  const workdir = join(import.meta.dir, "..", "work", ticket.id);
  mkdirSync(workdir, { recursive: true });
  const startedAt = Date.now();
  const prompt = workerPrompt(ticket.id, dispatch.id, trigger?.text ?? ticket.summary ?? "Summarize the requested input.");
  const result = spawnOwned(
    db,
    {
      ticketId: ticket.id,
      dispatchId: dispatch.id,
      command: workerCommand(config, prompt),
      cwd: workdir,
      env: { CODEBUDDY_LOCAL_KEY: "local" },
      dryRun: config.dryRun,
    },
    config,
  );
  if (result.dryRun || !result.child) {
    return {
      ...base,
      dispatched: false,
      ticketId: ticket.id,
      dispatchId: dispatch.id,
      ledgerId: result.ledgerId,
      pid: result.pid,
      elapsedMs: Date.now() - startedAt,
      threadsAfter: threadCount(),
    };
  }
  const child = result.child;
  let tokens = 0;
  let timedOut = false;
  let tokenExceeded = false;
  const heartbeatTimer = setInterval(() => {
    try {
      heartbeat(db, dispatch.id);
    } catch {
      // The dispatch may have reached a terminal state.
    }
  }, 5_000);
  const timeoutTimer = setTimeout(() => {
    timedOut = true;
    void killOwned(db, result.ledgerId).catch(() => undefined);
  }, config.workerTimeoutMs);
  const tokenReader = readTokens(child.stdout, (observed) => {
    tokens = observed;
    if (!tokenExceeded && observed > config.tokenBudgetPerWorker) {
      tokenExceeded = true;
      void killOwned(db, result.ledgerId).catch(() => undefined);
    }
  });
  try {
    await child.exited;
    tokens = Math.max(tokens, await tokenReader);
    clearInterval(heartbeatTimer);
    clearTimeout(timeoutTimer);
    const current = dispatchById(db, dispatch.id);
    if (current && (current.state === "pending" || current.state === "running")) {
      const reason = tokenExceeded
        ? `token budget exceeded: ${String(tokens)}/${String(config.tokenBudgetPerWorker)}`
        : timedOut
          ? `worker timeout after ${String(config.workerTimeoutMs)}ms`
          : "worker exited without a terminal report";
      try {
        reportTicket(db, ticket.id, "failed", reason, undefined, {
          dispatchId: dispatch.id,
          outcome: "failed",
          tokensUsed: tokens,
          tokenBudget: config.tokenBudgetPerWorker,
        });
      } catch {
        // reportTicket marks the failed budget itself; never override a terminal report.
      }
    }
    markLedgerExited(db, result.ledgerId, "exited", threadCount());
    const finalDispatch = dispatchById(db, dispatch.id) ?? undefined;
    const finalTicket = ticketById(db, ticket.id) ?? undefined;
    const residual = result.pid !== null && readThreadStartTicks(result.pid) !== null;
    return {
      ...base,
      dispatched: true,
      ticketId: ticket.id,
      dispatchId: dispatch.id,
      ledgerId: result.ledgerId,
      pid: result.pid,
      elapsedMs: Date.now() - startedAt,
      tokens,
      threadsAfter: threadCount(),
      timedOut,
      tokenExceeded,
      residual: false,
      dispatch: finalDispatch,
      ticket: finalTicket,
      outbox: (db.query("select count(*) as n from outbox where ticket_id = ?").get(ticket.id) as { n: number }).n,
    };
  } finally {
    clearInterval(heartbeatTimer);
    clearTimeout(timeoutTimer);
  }
}
