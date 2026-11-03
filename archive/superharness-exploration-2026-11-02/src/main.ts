import type { Database } from "bun:sqlite";
import { rmSync } from "node:fs";
import { join } from "node:path";
import {
  acceptTrigger,
  defaultDbPath,
  ignoreTrigger,
  insertTrigger,
  listDispatches,
  listLedger,
  listOutbox,
  listTickets,
  listTriggers,
  listWorkers,
  markLedgerExited,
  markRouterSession,
  openDb,
  recordLedger,
  recordRouterSession,
  consumeSpawnBudget,
  reportTicket,
  spawnBudgetUsed,
  statusSummary,
} from "./db";
import { assertNotNested, assertProcessSlot, assertWorkerSlot, GUARDS } from "./guards";
import { assertThreadBudget, loadConfig, readThreadStartTicks, threadCount } from "./config";
import { dispatchOnce } from "./dispatch";
import { OmpRpcClient } from "./omp-rpc";
import { runRouterDryRun, runRouterPass } from "./router";
import { spawnOwned, sweepOrphans, workerCommand } from "./process";
import type { ReportStatus } from "./types";

function usage(): void {
  console.log(`superharness MVP

  bun run src/main.ts init
  bun run src/main.ts inject --source local --thread t1 --text "fix docs" [--actor owner] [--dedupe key]
  bun run src/main.ts route <trigger-id> --accept [--ticket tk_x] | --ignore
  bun run src/main.ts report <ticket-id> --status <progress|ready|blocked|failed|waiting> --summary "..." [--reply "..." --channel local]
  bun run src/main.ts router --dry-run
  bun run src/main.ts dispatch --once
  bun run src/main.ts watchdog [--once]
  bun run src/main.ts outbox [--limit N]
  bun run src/main.ts worker <ticket-id> --dispatch <dispatch-id> [--prompt "..."]
  bun run src/main.ts status
  bun run src/main.ts doctor
  bun run src/main.ts smoke

Environment:
  SUPERHARNESS_DB              state db path (default out/superharness/state.db)
  SUPERHARNESS_MAX_WORKERS     default ${String(GUARDS.maxWorkers)}
  SUPERHARNESS_MAX_PROCESSES   default ${String(GUARDS.maxProcesses)}
  SUPERHARNESS_MAX_DEPTH       default ${String(GUARDS.maxDepth)}
  SUPERHARNESS_DEPTH           must be 0; >0 refuses to spawn
`);
}

function parseFlags(args: string[]): { positional: string[]; flags: Map<string, string | true> } {
  const positional: string[] = [];
  const flags = new Map<string, string | true>();
  for (let i = 0; i < args.length; i++) {
    const arg = args[i];
    if (!arg.startsWith("--")) {
      positional.push(arg);
      continue;
    }
    const key = arg.slice(2);
    const next = args[i + 1];
    if (next !== undefined && !next.startsWith("--")) {
      flags.set(key, next);
      i++;
    } else {
      flags.set(key, true);
    }
  }
  return { positional, flags };
}

function stringFlag(flags: Map<string, string | true>, name: string): string | undefined {
  const value = flags.get(name);
  return typeof value === "string" ? value : undefined;
}

function boolFlag(flags: Map<string, string | true>, name: string): boolean {
  return flags.get(name) === true;
}

function printJson(value: unknown): void {
  console.log(JSON.stringify(value, null, 2));
}

function withDb(fn: (db: Database) => void): void {
  const db = openDb(defaultDbPath());
  try {
    fn(db);
  } finally {
    db.close();
  }
}

function cmdInit(): void {
  withDb((db) => {
    printJson({ db: defaultDbPath(), summary: statusSummary(db), guards: GUARDS });
  });
}

function cmdInject(args: string[]): void {
  const { flags } = parseFlags(args);
  const source = stringFlag(flags, "source") ?? "local";
  const threadKey = stringFlag(flags, "thread") ?? `${source}/default`;
  const text = stringFlag(flags, "text");
  if (!text) throw new Error("inject requires --text");
  withDb((db) => {
    const result = insertTrigger(db, {
      source,
      threadKey,
      text,
      actor: stringFlag(flags, "actor"),
      dedupeKey: stringFlag(flags, "dedupe"),
    });
    printJson(result);
  });
}

function cmdRoute(args: string[]): void {
  const { positional, flags } = parseFlags(args);
  const triggerId = positional[0] ?? stringFlag(flags, "trigger");
  if (!triggerId) throw new Error("route requires <trigger-id>");
  withDb((db) => {
    if (boolFlag(flags, "ignore")) {
      ignoreTrigger(db, triggerId);
      printJson({ trigger: triggerId, disposition: "ignored" });
      return;
    }
    if (!boolFlag(flags, "accept")) throw new Error("route requires --accept or --ignore");
    assertWorkerSlot(db);
    const ticket = acceptTrigger(db, triggerId, stringFlag(flags, "ticket"));
    printJson(ticket);
  });
}

function cmdReport(args: string[]): void {
  const { positional, flags } = parseFlags(args);
  const ticketId = positional[0] ?? stringFlag(flags, "ticket");
  const status = stringFlag(flags, "status") as ReportStatus | undefined;
  const summary = stringFlag(flags, "summary") ?? "";
  if (!ticketId || !status) {
    throw new Error("report requires <ticket-id> --status <progress|ready|blocked|failed|waiting>");
  }
  withDb((db) => {
    const reply = stringFlag(flags, "reply");
    const ticket = reportTicket(
      db,
      ticketId,
      status,
      summary,
      reply ? { channel: stringFlag(flags, "channel") ?? "local", address: stringFlag(flags, "address"), body: reply } : undefined,
    );
    printJson(ticket);
  });
}

function cmdDoctor(): void {
  withDb((db) => {
    const config = loadConfig();
    const threads = threadCount();
    const used = spawnBudgetUsed(db);
    const rows = listLedger(db, undefined, 1_000);
    const dispatchCounts = db
      .query("select state, count(*) as n from dispatch group by state")
      .all() as Array<{ state: string; n: number }>;
    const live: unknown[] = [];
    const orphans: unknown[] = [];
    const notRunning: unknown[] = [];
    for (const row of rows) {
      if (row.state !== "running") {
        notRunning.push(row);
        continue;
      }
      const ticks = row.pid > 0 ? readThreadStartTicks(row.pid) : null;
      if (ticks !== null && ticks === row.start_ticks) live.push(row);
      else orphans.push(row);
    }
    printJson({
      dryRun: config.dryRun,
      threads,
      maxThreads: config.maxThreads,
      budget: { used, limit: config.spawnBudgetPerHour, remaining: Math.max(0, config.spawnBudgetPerHour - used) },
      ledger: rows.length,
      dispatches: Object.fromEntries(dispatchCounts.map((row) => [row.state, row.n])),
      live,
      orphans,
      notRunning,
    });
  });
}

async function cmdRouter(args: string[]): Promise<void> {
  const { flags } = parseFlags(args);
  if (!boolFlag(flags, "real")) {
    withDb((db) => {
      printJson(runRouterDryRun(db));
    });
    return;
  }
  if (!boolFlag(flags, "once")) throw new Error("D3b real Router requires --once");
  const config = loadConfig();
  if (config.dryRun) throw new Error("SUPERHARNESS_DRY_RUN=0 is required for --real");
  assertNotNested(config);
  assertThreadBudget(config);
  const db = openDb(defaultDbPath());
  const sessionId = `rtr_${crypto.randomUUID().slice(0, 8)}`;
  const ledgerId = `pl_${crypto.randomUUID().slice(0, 8)}`;
  const threadsBefore = threadCount();
  const startedAt = Date.now();
  const rpc = new OmpRpcClient({
    model: config.routerModel,
    cwd: join(import.meta.dir, ".."),
    env: { CODEBUDDY_LOCAL_KEY: "local", SUPERHARNESS_DEPTH: "1" },
  });
  let ledgerRecorded = false;
  let killNeeded = false;
  try {
    assertProcessSlot(db);
    consumeSpawnBudget(db, config.spawnBudgetPerHour);
    await rpc.start();
    const pid = rpc.pid;
    const pgid = rpc.pgid;
    const startTicks = readThreadStartTicks(pid);
    if (!pid || !pgid || !startTicks) {
      killNeeded = true;
      throw new Error("router process has no stable PID/PGID/starttime");
    }
    ledgerRecorded = true;
    recordLedger(db, {
      id: ledgerId,
      ticket_id: "-router-",
      dispatch_id: null,
      pid,
      pgid,
      start_ticks: startTicks,
      command: `omp --mode rpc --model ${config.routerModel}`,
      state: "running",
      started_at: Date.now(),
      exited_at: null,
      threads_before: threadsBefore,
      threads_after: null,
    });
    recordRouterSession(db, { id: sessionId, pid, pgid, startTicks, model: config.routerModel });
    const result = await runRouterPass(db, rpc, sessionId, config, 1);
    const elapsedMs = Date.now() - startedAt;
    await rpc.stop();
    const threadsAfter = threadCount();
    markLedgerExited(db, ledgerId, "exited", threadsAfter);
    markRouterSession(db, sessionId, result.stopped ? "failed" : "idle", result.stopped);
    const residual = readThreadStartTicks(pid) === startTicks;
    printJson({
      ok: result.failed === 0 && !result.stopped,
      sessionId,
      ledgerId,
      pid,
      elapsedMs,
      tokensUsed: result.tokensUsed,
      threadsBefore,
      threadsAfter,
      residual,
      result,
    });
    if (residual) process.exitCode = 1;
  } catch (error) {
    if (killNeeded || ledgerRecorded) {
      await rpc.stop();
    }
    if (ledgerRecorded) markLedgerExited(db, ledgerId, "killed", threadCount());
    markRouterSession(db, sessionId, "failed", error instanceof Error ? error.message : String(error));
    throw error;
  } finally {
    db.close();
  }
}

function cmdWorker(args: string[]): void {
  const { positional, flags } = parseFlags(args);
  const ticketId = positional[0] ?? stringFlag(flags, "ticket");
  const dispatchId = stringFlag(flags, "dispatch");
  if (!ticketId || !dispatchId) throw new Error("worker requires <ticket-id> --dispatch <dispatch-id>");
  const prompt =
    stringFlag(flags, "prompt") ??
    [
      "You are a superharness worker. Do not modify files.",
      `Call the harness_report tool exactly once with ticket_id=${JSON.stringify(ticketId)}, dispatch_id=${JSON.stringify(dispatchId)}, status="ready", summary="D2 single worker probe", outcome="succeeded".`,
      "Do not call any other tool. Reply DONE.",
    ].join("\n");
  const config = loadConfig();
  const db = openDb(defaultDbPath());
  try {
    const result = spawnOwned(
      db,
      {
        ticketId,
        dispatchId,
        command: workerCommand(config, prompt),
        cwd: join(import.meta.dir, ".."),
        env: { CODEBUDDY_LOCAL_KEY: "local" },
        dryRun: false,
      },
      config,
    );
    printJson({ dryRun: result.dryRun, ledgerId: result.ledgerId, pid: result.pid, pgid: result.pgid });
    if (result.child) {
      void result.child.exited
        .then((code) => {
          markLedgerExited(db, result.ledgerId, "exited", threadCount());
          // If the worker reported, reportTicket already moved the dispatch to
          // its terminal state. A still-running dispatch means the process
          // exited without a report, so record the failure instead of guessing.
          db.query("update dispatch set state = 'failed', ended_at = ? where id = ? and state = 'running'").run(
            Date.now(),
            dispatchId,
          );
          printJson({ exitCode: code, ledgerId: result.ledgerId });
          db.close();
        })
        .catch((error: unknown) => {
          db.close();
          console.error(String(error));
          process.exitCode = 1;
        });
    } else {
      db.close();
    }
  } catch (error) {
    db.close();
    throw error;
  }
}

async function cmdDispatch(args: string[]): Promise<void> {
  const { flags } = parseFlags(args);
  if (!boolFlag(flags, "once")) throw new Error("D4 dispatch requires --once");
  const config = loadConfig();
  const db = openDb(defaultDbPath());
  try {
    printJson(await dispatchOnce(db, config));
  } finally {
    db.close();
  }
}

async function cmdWatchdog(): Promise<void> {
  const config = loadConfig();
  const db = openDb(defaultDbPath());
  try {
    printJson(await sweepOrphans(db, config));
  } finally {
    db.close();
  }
}

function cmdOutbox(args: string[]): void {
  const { flags } = parseFlags(args);
  const limit = Number(stringFlag(flags, "limit") ?? "20");
  withDb((db) => {
    printJson(listOutbox(db, Number.isFinite(limit) ? limit : 20));
  });
}

function cmdStatus(): void {
  withDb((db) => {
    printJson({
      db: defaultDbPath(),
      summary: statusSummary(db),
      triggers: listTriggers(db, 10),
      tickets: listTickets(db, 10),
      workers: listWorkers(db, 10),
      dispatches: listDispatches(db, 10),
      outbox: listOutbox(db, 10),
    });
  });
}

function cmdSmoke(): void {
  const smokePath = `${import.meta.dir}/../smoke.db`;
  rmSync(smokePath, { force: true });
  const db = openDb(smokePath);
  try {
    assertNotNested();
    const injected = insertTrigger(db, {
      source: "smoke",
      threadKey: "smoke/thread",
      text: "write docs",
      actor: "owner",
      dedupeKey: `smoke-${String(Date.now())}`,
    });
    if (injected.duplicate) throw new Error("smoke trigger unexpectedly duplicate");
    assertWorkerSlot(db);
    const ticket = acceptTrigger(db, injected.row.id);
    const reported = reportTicket(db, ticket.id, "ready", "docs written", {
      channel: "local",
      body: "done: docs written",
    });
    const outbox = listOutbox(db, 5);
    const summary = statusSummary(db);
    const ok =
      reported.status === "ready" &&
      outbox.length === 1 &&
      outbox[0]?.status === "pending" &&
      summary.triggers === 1 &&
      summary.tickets === 1 &&
      summary.workers === 1;
    printJson({
      ok,
      trigger: injected.row.id,
      ticket: ticket.id,
      ticketStatus: reported.status,
      outbox: outbox.length,
      summary,
    });
    if (!ok) process.exit(1);
  } finally {
    db.close();
  }
}

const [command, ...rest] = Bun.argv.slice(2);
switch (command) {
  case "init":
    cmdInit();
    break;
  case "inject":
    cmdInject(rest);
    break;
  case "route":
    cmdRoute(rest);
    break;
  case "report":
    cmdReport(rest);
    break;
  case "router":
    await cmdRouter(rest);
    break;
  case "dispatch":
    await cmdDispatch(rest);
    break;
  case "watchdog":
    await cmdWatchdog();
    break;
  case "outbox":
    cmdOutbox(rest);
    break;
  case "worker":
    cmdWorker(rest);
    break;
  case "status":
    cmdStatus();
    break;
  case "doctor":
    cmdDoctor();
    break;
  case "smoke":
    cmdSmoke();
    break;
  case undefined:
    usage();
    break;
  default:
    console.error(`unknown command: ${command}`);
    usage();
    process.exit(1);
}
