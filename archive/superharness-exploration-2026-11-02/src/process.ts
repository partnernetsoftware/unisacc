import type { Database } from "bun:sqlite";
import {
  consumeSpawnBudget,
  dispatchById,
  ledgerById,
  listLedger,
  markLedgerExited,
  recordLedger,
} from "./db";
import { assertNotNested, assertThreadBudget, loadConfig, readThreadStartTicks, threadCount, type HarnessConfig } from "./config";
import { assertWorkerSlot } from "./guards";
import type { LedgerRow } from "./types";

export interface SpawnOptions {
  ticketId: string;
  dispatchId: string;
  command: string[];
  cwd?: string;
  env?: Record<string, string>;
  dryRun?: boolean;
}

export interface SpawnResult {
  dryRun: boolean;
  ledgerId: string;
  pid: number | null;
  pgid: number | null;
  child?: Bun.Subprocess;
}

function shortId(prefix: string): string {
  return `${prefix}_${crypto.randomUUID().slice(0, 8)}`;
}

/**
 * Spawn one worker. Every guard runs before Bun.spawn: depth, thread budget,
 * worker slot, hourly spawn budget. Dry-run records the intent and never
 * creates a process or consumes the spawn budget.
 */
export function spawnOwned(db: Database, options: SpawnOptions, config: HarnessConfig = loadConfig()): SpawnResult {
  assertNotNested(config);
  assertThreadBudget(config);
  assertWorkerSlot(db);
  const dryRun = options.dryRun ?? config.dryRun;
  const ledgerId = shortId("pl");
  if (dryRun) {
    const row: LedgerRow = {
      id: ledgerId,
      ticket_id: options.ticketId,
      dispatch_id: options.dispatchId,
      pid: 0,
      pgid: 0,
      start_ticks: "dry-run",
      command: options.command.join(" "),
      state: "dry-run",
      started_at: Date.now(),
      exited_at: null,
      threads_before: threadCount(),
      threads_after: null,
    };
    recordLedger(db, row);
    return { dryRun: true, ledgerId, pid: null, pgid: null };
  }
  consumeSpawnBudget(db, config.spawnBudgetPerHour);
  const child = Bun.spawn(options.command, {
    cwd: options.cwd,
    env: { ...process.env, ...(options.env ?? {}) },
    detached: true,
    stdio: ["ignore", "pipe", "pipe"],
  });
  const pid = child.pid;
  const pgid = pid;
  const startTicks = readThreadStartTicks(pid);
  if (!startTicks) {
    child.kill("SIGKILL");
    throw new Error(`spawned pid ${String(pid)} has no /proc start time; killed`);
  }
  recordLedger(db, {
    id: ledgerId,
    ticket_id: options.ticketId,
    dispatch_id: options.dispatchId,
    pid,
    pgid,
    start_ticks: startTicks,
    command: options.command.join(" "),
    state: "running",
    started_at: Date.now(),
    exited_at: null,
    threads_before: threadCount(),
    threads_after: null,
  });
  db.query("update dispatch set state = 'running', pid = ?, pgid = ?, started_at = ? where id = ?").run(
    pid,
    pgid,
    Date.now(),
    options.dispatchId,
  );
  db.query("update worker set pid = ?, pgid = ?, state = 'running', started_at = ? where ticket_id = ?").run(
    pid,
    pgid,
    Date.now(),
    options.ticketId,
  );
  return { dryRun: false, ledgerId, pid, pgid, child };
}

function sameProcess(row: LedgerRow): boolean {
  if (row.pid <= 0) return false;
  return readThreadStartTicks(row.pid) === row.start_ticks;
}

/**
 * Kill only the process group recorded in our ledger. The start-time check
 * prevents killing a recycled PID. Never uses a command-line pattern.
 */
export async function killOwned(db: Database, ledgerId: string): Promise<"killed" | "already-exited" | "pid-recycled"> {
  const row = ledgerById(db, ledgerId);
  if (!row) throw new Error(`ledger not found: ${ledgerId}`);
  if (row.state !== "running") return "already-exited";
  if (!sameProcess(row)) {
    markLedgerExited(db, row.id, "exited", threadCount());
    return "pid-recycled";
  }
  try {
    process.kill(-row.pgid, "SIGTERM");
  } catch {
    // It may have exited between the start-time check and the signal.
  }
  for (let i = 0; i < 20; i++) {
    await Bun.sleep(100);
    if (!sameProcess(row)) {
      markLedgerExited(db, row.id, "killed", threadCount());
      db.query("update dispatch set state = 'stopped', ended_at = ? where id = ?").run(Date.now(), row.dispatch_id);
      db.query("update worker set state = 'stopped' where ticket_id = ?").run(row.ticket_id);
      return "killed";
    }
  }
  try {
    process.kill(-row.pgid, "SIGKILL");
  } catch {
    // Already gone.
  }
  markLedgerExited(db, row.id, "killed", threadCount());
  if (row.dispatch_id) db.query("update dispatch set state = 'stopped', ended_at = ? where id = ?").run(Date.now(), row.dispatch_id);
  db.query("update worker set state = 'stopped' where ticket_id = ?").run(row.ticket_id);
  return "killed";
}

/**
 * On startup, reconcile our own ledger. A live process is ours only when both
 * PID and /proc start time match; then it is killed by exact PGID. A dead or
 * recycled PID is marked exited, never signalled.
 */
export async function reclaimOrphans(db: Database): Promise<Array<{ id: string; result: string }>> {
  const results: Array<{ id: string; result: string }> = [];
  for (const row of listLedger(db, "running", 1_000_000)) {
    if (sameProcess(row)) {
      results.push({ id: row.id, result: await killOwned(db, row.id) });
    } else {
      markLedgerExited(db, row.id, "exited", threadCount());
      if (row.dispatch_id) db.query("update dispatch set state = 'stopped', ended_at = ? where id = ?").run(Date.now(), row.dispatch_id);
      db.query("update worker set state = 'exited' where ticket_id = ?").run(row.ticket_id);
      results.push({ id: row.id, result: "marked-exited" });
    }
  }
  return results;
}

export interface SweepResult {
  checked: number;
  killed: string[];
  marked: string[];
  failedDispatches: string[];
  skipped: string[];
}

/**
 * D5 bounded watchdog sweep. It never matches command lines and never signals a
 * process whose PID+starttime do not match its ledger row. A live process is
 * left alone while its dispatch heartbeat is fresh; terminal/missing/stale
 * dispatches are killed by their exact PGID and recorded failed.
 */
export async function sweepOrphans(
  db: Database,
  config: HarnessConfig = loadConfig(),
): Promise<SweepResult> {
  const now = Date.now();
  const result: SweepResult = { checked: 0, killed: [], marked: [], failedDispatches: [], skipped: [] };
  for (const row of listLedger(db, "running", 1_000_000)) {
    result.checked++;
    const live = row.pid > 0 && readThreadStartTicks(row.pid) === row.start_ticks;
    const dispatch = row.dispatch_id ? dispatchById(db, row.dispatch_id) : null;
    const dispatchTerminal =
      dispatch === null || dispatch.state === "completed" || dispatch.state === "failed" || dispatch.state === "stopped";
    const heartbeat = dispatch?.heartbeat_at ?? 0;
    const stale = !dispatch || dispatchTerminal || heartbeat === 0 || heartbeat < now - config.workerTimeoutMs;
    if (!live) {
      markLedgerExited(db, row.id, "exited", threadCount());
      if (dispatch && (dispatch.state === "pending" || dispatch.state === "running")) {
        db.query("update dispatch set state = 'failed', ended_at = ? where id = ?").run(now, dispatch.id);
        db.query("update ticket set status = 'failed', updated_at = ? where id = ?").run(now, dispatch.ticket_id);
        db.query("update worker set state = 'exited', last_heartbeat_at = ? where ticket_id = ?").run(now, row.ticket_id);
        result.failedDispatches.push(dispatch.id);
      }
      result.marked.push(row.id);
      continue;
    }
    if (stale) {
      await killOwned(db, row.id);
      if (dispatch && (dispatch.state === "pending" || dispatch.state === "running" || dispatch.state === "stopped")) {
        db.query("update dispatch set state = 'failed', ended_at = ? where id = ?").run(now, dispatch.id);
        db.query("update ticket set status = 'failed', updated_at = ? where id = ?").run(now, dispatch.ticket_id);
        db.query("update worker set state = 'exited', last_heartbeat_at = ? where ticket_id = ?").run(now, row.ticket_id);
        result.failedDispatches.push(dispatch.id);
      }
      result.killed.push(row.id);
    } else {
      result.skipped.push(row.id);
    }
  }
  const staleDispatches = db
    .query("select * from dispatch where state = 'running' and (heartbeat_at is null or heartbeat_at < ?)")
    .all(now - config.workerTimeoutMs) as Array<{ id: string; ticket_id: string }>;
  for (const dispatch of staleDispatches) {
    const live = db
      .query("select count(*) as n from process_ledger where dispatch_id = ? and state = 'running'")
      .get(dispatch.id) as { n: number };
    if (live.n > 0) continue;
    db.query("update dispatch set state = 'failed', ended_at = ? where id = ?").run(now, dispatch.id);
    db.query("update ticket set status = 'failed', updated_at = ? where id = ?").run(now, dispatch.ticket_id);
    db.query("update worker set state = 'exited', last_heartbeat_at = ? where ticket_id = ?").run(now, dispatch.ticket_id);
    result.failedDispatches.push(dispatch.id);
  }
  return result;
}

export function workerCommand(config: HarnessConfig, prompt: string): string[] {
  const extension = new URL("../extension.ts", import.meta.url).pathname;
  return [
    "omp",
    "-p",
    "--mode",
    "json",
    "--model",
    config.workerModel,
    "--max-time",
    "1200",
    "--auto-approve",
    "--no-session",
    "--tools",
    "read,grep,glob,harness_report",
    "-e",
    extension,
    "--",
    prompt,
  ];
}
