import { Database } from "bun:sqlite";
import { mkdirSync } from "node:fs";
import { dirname, join } from "node:path";
import type {
  DispatchRow,
  GateRow,
  LedgerRow,
  OutboxRow,
  ReportOptions,
  ReportStatus,
  RouterTurnRow,
  RouterTurnState,
  TicketRow,
  TicketStatus,
  TriggerInput,
  TriggerRow,
  WorkerRow,
} from "./types";

const SCHEMA = `
PRAGMA journal_mode=WAL;
PRAGMA foreign_keys=ON;

CREATE TABLE IF NOT EXISTS schema_meta (
  key TEXT PRIMARY KEY,
  value TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS thread (
  key TEXT PRIMARY KEY,
  source TEXT NOT NULL,
  label TEXT,
  last_trigger_at INTEGER,
  summary TEXT
);

CREATE TABLE IF NOT EXISTS trigger (
  id TEXT PRIMARY KEY,
  source TEXT NOT NULL,
  thread_key TEXT NOT NULL,
  actor TEXT,
  text TEXT NOT NULL,
  at INTEGER NOT NULL,
  disposition TEXT NOT NULL DEFAULT 'new',
  dedupe_key TEXT UNIQUE
);
CREATE INDEX IF NOT EXISTS trigger_thread_idx ON trigger(thread_key, at);

CREATE TABLE IF NOT EXISTS ticket (
  id TEXT PRIMARY KEY,
  thread_key TEXT NOT NULL,
  trigger_id TEXT NOT NULL,
  status TEXT NOT NULL,
  worker_pid INTEGER,
  worker_pgid INTEGER,
  workdir TEXT,
  created_at INTEGER NOT NULL,
  updated_at INTEGER NOT NULL,
  summary TEXT
);
CREATE INDEX IF NOT EXISTS ticket_thread_idx ON ticket(thread_key, created_at);

CREATE TABLE IF NOT EXISTS worker (
  id TEXT PRIMARY KEY,
  ticket_id TEXT NOT NULL,
  pid INTEGER,
  pgid INTEGER,
  session_dir TEXT,
  workdir TEXT,
  started_at INTEGER,
  last_heartbeat_at INTEGER,
  state TEXT NOT NULL
);
CREATE INDEX IF NOT EXISTS worker_ticket_idx ON worker(ticket_id);

CREATE TABLE IF NOT EXISTS outbox (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  ticket_id TEXT,
  channel TEXT NOT NULL,
  address TEXT,
  body TEXT NOT NULL,
  status TEXT NOT NULL DEFAULT 'pending',
  kind TEXT,
  attempts INTEGER NOT NULL DEFAULT 0,
  last_error TEXT,
  created_at INTEGER NOT NULL,
  sent_at INTEGER
);
CREATE INDEX IF NOT EXISTS outbox_status_idx ON outbox(status, created_at);

CREATE TABLE IF NOT EXISTS lease (
  name TEXT PRIMARY KEY,
  holder TEXT,
  pid INTEGER,
  acquired_at INTEGER
);

CREATE TABLE IF NOT EXISTS dispatch (
  id TEXT PRIMARY KEY,
  ticket_id TEXT NOT NULL,
  attempt INTEGER NOT NULL,
  state TEXT NOT NULL,
  pid INTEGER,
  pgid INTEGER,
  started_at INTEGER,
  ended_at INTEGER,
  heartbeat_at INTEGER,
  report_status TEXT,
  report_summary TEXT,
  files_modified TEXT,
  tokens_used INTEGER NOT NULL DEFAULT 0
);
CREATE INDEX IF NOT EXISTS dispatch_ticket_idx ON dispatch(ticket_id, attempt);

CREATE TABLE IF NOT EXISTS gate (
  id TEXT PRIMARY KEY,
  ticket_id TEXT NOT NULL,
  dispatch_id TEXT NOT NULL,
  question TEXT NOT NULL,
  options TEXT NOT NULL,
  answer TEXT,
  state TEXT NOT NULL,
  created_at INTEGER NOT NULL,
  resolved_at INTEGER
);
CREATE INDEX IF NOT EXISTS gate_ticket_idx ON gate(ticket_id, created_at);

CREATE TABLE IF NOT EXISTS spawn_budget (
  hour TEXT PRIMARY KEY,
  count INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS router_decision (
  id TEXT PRIMARY KEY,
  trigger_id TEXT NOT NULL,
  decision TEXT NOT NULL,
  reason TEXT,
  mode TEXT NOT NULL,
  at INTEGER NOT NULL
);
CREATE INDEX IF NOT EXISTS router_decision_trigger_idx ON router_decision(trigger_id, at);

CREATE TABLE IF NOT EXISTS router_session (
  id TEXT PRIMARY KEY,
  pid INTEGER,
  pgid INTEGER,
  start_ticks TEXT,
  state TEXT NOT NULL,
  model TEXT NOT NULL,
  started_at INTEGER NOT NULL,
  ended_at INTEGER,
  last_turn_at INTEGER,
  error TEXT
);

CREATE TABLE IF NOT EXISTS router_turn (
  id TEXT PRIMARY KEY,
  session_id TEXT NOT NULL,
  trigger_id TEXT NOT NULL UNIQUE,
  state TEXT NOT NULL,
  started_at INTEGER NOT NULL,
  ended_at INTEGER,
  tokens INTEGER NOT NULL DEFAULT 0,
  error TEXT
);
CREATE INDEX IF NOT EXISTS router_turn_state_idx ON router_turn(state, started_at);

CREATE TABLE IF NOT EXISTS process_ledger (
  id TEXT PRIMARY KEY,
  ticket_id TEXT NOT NULL,
  dispatch_id TEXT,
  pid INTEGER NOT NULL,
  pgid INTEGER NOT NULL,
  start_ticks TEXT NOT NULL,
  command TEXT NOT NULL,
  state TEXT NOT NULL,
  started_at INTEGER NOT NULL,
  exited_at INTEGER,
  threads_before INTEGER,
  threads_after INTEGER
);
CREATE INDEX IF NOT EXISTS process_ledger_state_idx ON process_ledger(state, pid);
`;

export function openDb(path: string): Database {
  mkdirSync(dirname(path), { recursive: true });
  const db = new Database(path);
  db.exec(SCHEMA);
  const columns = db.query("pragma table_info(outbox)").all() as Array<{ name: string }>;
  if (!columns.some((column) => column.name === "kind")) db.exec("alter table outbox add column kind TEXT");
  db.exec("CREATE UNIQUE INDEX IF NOT EXISTS outbox_terminal_idx ON outbox(ticket_id, kind) WHERE kind IS NOT NULL");
  db.query("insert or replace into schema_meta(key, value) values ('schema', '1')").run();
  return db;
}

export function defaultDbPath(): string {
  return process.env.SUPERHARNESS_DB ?? join(import.meta.dir, "..", "state.db");
}

function shortId(prefix: string): string {
  return `${prefix}_${crypto.randomUUID().slice(0, 8)}`;
}

export function insertTrigger(db: Database, input: TriggerInput): { row: TriggerRow; duplicate: boolean } {
  const at = input.at ?? Date.now();
  if (input.dedupeKey) {
    const existing = db.query("select * from trigger where dedupe_key = ?").get(input.dedupeKey) as TriggerRow | null;
    if (existing) return { row: existing, duplicate: true };
  }
  const id = shortId("tr");
  db.transaction(() => {
    db.query(
      "insert into thread(key, source, label, last_trigger_at) values (?, ?, ?, ?) on conflict(key) do update set last_trigger_at = excluded.last_trigger_at",
    ).run(input.threadKey, input.source, input.threadKey, at);
    db.query(
      "insert into trigger(id, source, thread_key, actor, text, at, disposition, dedupe_key) values (?, ?, ?, ?, ?, ?, 'new', ?)",
    ).run(id, input.source, input.threadKey, input.actor ?? null, input.text, at, input.dedupeKey ?? null);
  })();
  const row = db.query("select * from trigger where id = ?").get(id) as TriggerRow;
  return { row, duplicate: false };
}

export function triggerById(db: Database, id: string): TriggerRow | null {
  return db.query("select * from trigger where id = ?").get(id) as TriggerRow | null;
}

export function ignoreTrigger(db: Database, triggerId: string): void {
  const trigger = triggerById(db, triggerId);
  if (!trigger) throw new Error(`trigger not found: ${triggerId}`);
  if (trigger.disposition !== "new") throw new Error(`trigger ${triggerId} already ${trigger.disposition}`);
  db.query("update trigger set disposition = 'ignored' where id = ?").run(triggerId);
}

export function acceptTrigger(db: Database, triggerId: string, ticketId?: string): TicketRow {
  return db.transaction(() => {
    const trigger = triggerById(db, triggerId);
    if (!trigger) throw new Error(`trigger not found: ${triggerId}`);
    if (trigger.disposition !== "new") throw new Error(`trigger ${triggerId} already ${trigger.disposition}`);
    const id = ticketId ?? shortId("tk");
    const now = Date.now();
    const workdir = join(import.meta.dir, "..", "work", id);
    db.query(
      "insert into ticket(id, thread_key, trigger_id, status, workdir, created_at, updated_at) values (?, ?, ?, 'open', ?, ?, ?)",
    ).run(id, trigger.thread_key, triggerId, workdir, now, now);
    db.query("update trigger set disposition = 'accepted' where id = ?").run(triggerId);
    db.query(
      "insert into worker(id, ticket_id, session_dir, workdir, state) values (?, ?, ?, ?, 'pending')",
    ).run(id, id, join(import.meta.dir, "..", "sessions", id), workdir);
    db.query(
      "insert into dispatch(id, ticket_id, attempt, state, started_at, heartbeat_at, tokens_used) values (?, ?, 1, 'pending', ?, ?, 0)",
    ).run(shortId("dp"), id, now, now);
    return db.query("select * from ticket where id = ?").get(id) as TicketRow;
  })();
}

export function ticketById(db: Database, id: string): TicketRow | null {
  return db.query("select * from ticket where id = ?").get(id) as TicketRow | null;
}

export function dispatchById(db: Database, id: string): DispatchRow | null {
  return db.query("select * from dispatch where id = ?").get(id) as DispatchRow | null;
}

export function dispatchByTicket(db: Database, ticketId: string): DispatchRow | null {
  return db.query("select * from dispatch where ticket_id = ? order by attempt desc limit 1").get(ticketId) as DispatchRow | null;
}

export function nextPendingDispatch(
  db: Database,
): { dispatch: DispatchRow; ticket: TicketRow } | null {
  const dispatch = db
    .query(
      "select * from dispatch where state = 'pending' order by started_at asc, rowid asc limit 1",
    )
    .get() as DispatchRow | null;
  if (!dispatch) return null;
  const ticket = ticketById(db, dispatch.ticket_id);
  if (!ticket || ticket.status === "ready" || ticket.status === "failed" || ticket.status === "ended") return null;
  return { dispatch, ticket };
}

export function reportTicket(
  db: Database,
  ticketId: string,
  status: ReportStatus,
  summary: string,
  reply?: { channel: string; address?: string; body: string },
  options: ReportOptions & { tokenBudget?: number } = {},
): TicketRow {
  if (options.dispatchId && options.tokenBudget !== undefined) {
    const existing = dispatchById(db, options.dispatchId);
    if (existing && existing.tokens_used + (options.tokensUsed ?? 0) > options.tokenBudget) {
      const now = Date.now();
      db.transaction(() => {
        db.query("update dispatch set state = 'failed', ended_at = ?, tokens_used = ? where id = ?").run(
          now,
          existing.tokens_used + (options.tokensUsed ?? 0),
          existing.id,
        );
        db.query("update ticket set status = 'failed', summary = ?, updated_at = ? where id = ?").run(
          `token budget exceeded: ${String(existing.tokens_used + (options.tokensUsed ?? 0))}/${String(options.tokenBudget)}`,
          now,
          ticketId,
        );
        db.query("update worker set state = 'exited', last_heartbeat_at = ? where ticket_id = ?").run(now, ticketId);
      })();
      throw new Error(`token budget exceeded: ${String(existing.tokens_used + (options.tokensUsed ?? 0))}/${String(options.tokenBudget)}`);
    }
  }
  return db.transaction(() => {
    const ticket = ticketById(db, ticketId);
    if (!ticket) throw new Error(`ticket not found: ${ticketId}`);
    const now = Date.now();
    let dispatch: DispatchRow | null = null;
    let filesModified = JSON.stringify(options.filesModified ?? []);
    let tokensUsed = options.tokensUsed ?? 0;
    if (options.dispatchId) {
      dispatch = dispatchById(db, options.dispatchId);
      if (!dispatch || dispatch.ticket_id !== ticketId) {
        throw new Error(`dispatch not found for ticket: ${options.dispatchId}`);
      }
      if ((status === "ready" || status === "failed") && (dispatch.state === "completed" || dispatch.state === "failed")) {
        throw new Error(`duplicate done for dispatch ${dispatch.id}`);
      }
      if (status === "ready" && options.outcome === "failed") throw new Error("ready report cannot have outcome=failed");
      if (status === "failed" && options.outcome === "succeeded") throw new Error("failed report cannot have outcome=succeeded");
      const total = dispatch.tokens_used + tokensUsed;
      if (options.tokenBudget !== undefined && total > options.tokenBudget) {
        db.query("update dispatch set state = 'failed', ended_at = ?, tokens_used = ? where id = ?").run(now, total, dispatch.id);
        db.query("update ticket set status = 'failed', summary = ?, updated_at = ? where id = ?").run(
          `token budget exceeded: ${String(total)}/${String(options.tokenBudget)}`,
          now,
          ticketId,
        );
        throw new Error(`token budget exceeded: ${String(total)}/${String(options.tokenBudget)}`);
      }
      const endedAt = status === "ready" || status === "failed" || status === "blocked" ? now : null;
      db.query(
        "update dispatch set state = ?, heartbeat_at = ?, ended_at = coalesce(?, ended_at), report_status = ?, report_summary = ?, files_modified = ?, tokens_used = ? where id = ?",
      ).run(
        status === "ready" ? "completed" : status === "failed" ? "failed" : "running",
        now,
        endedAt,
        status,
        summary,
        filesModified,
        total,
        dispatch.id,
      );
    }
    let mapped: TicketStatus =
      status === "ready" ? "ready" : status === "blocked" ? "blocked" : status === "failed" ? "failed" : status === "progress" ? "running" : "open";
    if (options.question) mapped = "blocked";
    db.query("update ticket set status = ?, summary = ?, updated_at = ? where id = ?").run(mapped, summary, now, ticketId);
    const workerState = status === "ready" || status === "blocked" || status === "failed" ? "exited" : status === "progress" ? "running" : "stopped";
    db.query("update worker set state = ?, last_heartbeat_at = ? where ticket_id = ?").run(workerState, now, ticketId);
    if (options.question) {
      const dispatchId = dispatch?.id ?? dispatchByTicket(db, ticketId)?.id;
      if (!dispatchId) throw new Error("ask requires a dispatch");
      db.query(
        "insert into gate(id, ticket_id, dispatch_id, question, options, state, created_at) values (?, ?, ?, ?, ?, 'open', ?)",
      ).run(shortId("gt"), ticketId, dispatchId, options.question.text, JSON.stringify(options.question.options ?? []), now);
    }
    const kind = options.question ? "ask" : status === "ready" ? "done" : status === "blocked" ? "blocked" : null;
    if (kind) {
      const body = reply?.body ?? options.question?.text ?? summary;
      db.query(
        "insert or ignore into outbox(ticket_id, channel, address, body, status, kind, created_at) values (?, ?, ?, ?, 'pending', ?, ?)",
      ).run(ticketId, reply?.channel ?? "local", reply?.address ?? null, body, kind, now);
    }
    return db.query("select * from ticket where id = ?").get(ticketId) as TicketRow;
  })();
}

export function heartbeat(db: Database, dispatchId: string, tokensUsed = 0): DispatchRow {
  return db.transaction(() => {
    const dispatch = dispatchById(db, dispatchId);
    if (!dispatch) throw new Error(`dispatch not found: ${dispatchId}`);
    const now = Date.now();
    db.query("update dispatch set heartbeat_at = ?, state = case when state = 'pending' then 'running' else state end, tokens_used = tokens_used + ? where id = ?").run(
      now,
      tokensUsed,
      dispatchId,
    );
    db.query("update worker set last_heartbeat_at = ? where ticket_id = ?").run(now, dispatch.ticket_id);
    return dispatchById(db, dispatchId) as DispatchRow;
  })();
}

export function ask(
  db: Database,
  ticketId: string,
  dispatchId: string,
  question: string,
  options: string[] = [],
): GateRow {
  return db.transaction(() => {
    const dispatch = dispatchById(db, dispatchId);
    if (!dispatch || dispatch.ticket_id !== ticketId) throw new Error(`dispatch not found for ticket: ${dispatchId}`);
    const now = Date.now();
    const id = shortId("gt");
    db.query(
      "insert into gate(id, ticket_id, dispatch_id, question, options, state, created_at) values (?, ?, ?, ?, ?, 'open', ?)",
    ).run(id, ticketId, dispatchId, question, JSON.stringify(options), now);
    db.query("update ticket set status = 'blocked', updated_at = ? where id = ?").run(now, ticketId);
    return db.query("select * from gate where id = ?").get(id) as GateRow;
  })();
}

export function resolveGate(db: Database, gateId: string, answer: string): GateRow {
  return db.transaction(() => {
    const gate = db.query("select * from gate where id = ?").get(gateId) as GateRow | null;
    if (!gate) throw new Error(`gate not found: ${gateId}`);
    if (gate.state !== "open") throw new Error(`gate already ${gate.state}`);
    const now = Date.now();
    db.query("update gate set state = 'answered', answer = ?, resolved_at = ? where id = ?").run(answer, now, gateId);
    db.query("update ticket set status = 'running', updated_at = ? where id = ?").run(now, gate.ticket_id);
    return db.query("select * from gate where id = ?").get(gateId) as GateRow;
  })();
}

export function listDispatches(db: Database, limit = 50): DispatchRow[] {
  return db.query("select * from dispatch order by started_at desc limit ?").all(limit) as DispatchRow[];
}

export function listGates(db: Database, ticketId?: string, limit = 20): GateRow[] {
  if (ticketId) {
    return db.query("select * from gate where ticket_id = ? order by created_at desc limit ?").all(ticketId, limit) as GateRow[];
  }
  return db.query("select * from gate order by created_at desc limit ?").all(limit) as GateRow[];
}

export function consumeSpawnBudget(db: Database, limit: number, now = Date.now()): number {
  const hour = new Date(now).toISOString().slice(0, 13);
  return db.transaction(() => {
    const row = db.query("select count from spawn_budget where hour = ?").get(hour) as { count: number } | null;
    const next = (row?.count ?? 0) + 1;
    if (next > limit) throw new Error(`spawn budget exceeded: ${String(next)}/${String(limit)} for ${hour}`);
    db.query("insert into spawn_budget(hour, count) values (?, ?) on conflict(hour) do update set count = excluded.count").run(hour, next);
    return next;
  })();
}

export function spawnBudgetUsed(db: Database, now = Date.now()): number {
  const hour = new Date(now).toISOString().slice(0, 13);
  const row = db.query("select count from spawn_budget where hour = ?").get(hour) as { count: number } | null;
  return row?.count ?? 0;
}

export function recordLedger(db: Database, row: LedgerRow): void {
  db.query(
    "insert into process_ledger(id, ticket_id, dispatch_id, pid, pgid, start_ticks, command, state, started_at, exited_at, threads_before, threads_after) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
  ).run(
    row.id,
    row.ticket_id,
    row.dispatch_id,
    row.pid,
    row.pgid,
    row.start_ticks,
    row.command,
    row.state,
    row.started_at,
    row.exited_at,
    row.threads_before ?? null,
    row.threads_after ?? null,
  );
}

export function ledgerById(db: Database, id: string): LedgerRow | null {
  return db.query("select * from process_ledger where id = ?").get(id) as LedgerRow | null;
}

export function listLedger(db: Database, state?: string, limit = 100): LedgerRow[] {
  if (state) {
    return db.query("select * from process_ledger where state = ? order by started_at desc limit ?").all(state, limit) as LedgerRow[];
  }
  return db.query("select * from process_ledger order by started_at desc limit ?").all(limit) as LedgerRow[];
}

export function markLedgerExited(db: Database, id: string, state: "exited" | "killed", threadsAfter?: number): void {
  db.query("update process_ledger set state = ?, exited_at = ?, threads_after = coalesce(?, threads_after) where id = ?").run(
    state,
    Date.now(),
    threadsAfter ?? null,
    id,
  );
}

export function enqueueReply(
  db: Database,
  input: { ticketId?: string; channel: string; address?: string; body: string; kind?: string },
): void {
  db.query(
    "insert or ignore into outbox(ticket_id, channel, address, body, status, kind, created_at) values (?, ?, ?, ?, 'pending', ?, ?)",
  ).run(input.ticketId ?? null, input.channel, input.address ?? null, input.body, input.kind ?? null, Date.now());
}

export function threadByKey(db: Database, key: string): Record<string, unknown> | null {
  return db.query("select * from thread where key = ?").get(key) as Record<string, unknown> | null;
}

export function outboxByTicket(db: Database, ticketId: string): OutboxRow[] {
  return db.query("select * from outbox where ticket_id = ? order by created_at asc").all(ticketId) as OutboxRow[];
}

export function newTriggers(db: Database, limit = 20): TriggerRow[] {
  return db.query("select * from trigger where disposition = 'new' order by at asc limit ?").all(limit) as TriggerRow[];
}

export function recordRouterDecision(db: Database, triggerId: string, decision: string, reason: string, mode: string): void {
  db.query("insert into router_decision(id, trigger_id, decision, reason, mode, at) values (?, ?, ?, ?, ?, ?)").run(
    shortId("rd"),
    triggerId,
    decision,
    reason,
    mode,
    Date.now(),
  );
}

export function listRouterDecisions(db: Database, limit = 50): Array<Record<string, unknown>> {
  return db.query("select * from router_decision order by at desc limit ?").all(limit) as Array<Record<string, unknown>>;
}

export function routerDecisionByTrigger(
  db: Database,
  triggerId: string,
): { decision: string; reason: string; mode: string } | null {
  const row = db
    .query("select decision, reason, mode from router_decision where trigger_id = ? order by at desc limit 1")
    .get(triggerId) as { decision: string; reason: string; mode: string } | null;
  return row ?? null;
}

export function routerTurnByTrigger(db: Database, triggerId: string): RouterTurnRow | null {
  return db.query("select * from router_turn where trigger_id = ?").get(triggerId) as RouterTurnRow | null;
}

export function createRouterTurn(db: Database, sessionId: string, triggerId: string): RouterTurnRow {
  const existing = routerTurnByTrigger(db, triggerId);
  if (existing) return existing;
  const row: RouterTurnRow = {
    id: shortId("rt"),
    session_id: sessionId,
    trigger_id: triggerId,
    state: "running",
    started_at: Date.now(),
    ended_at: null,
    tokens: 0,
    error: null,
  };
  db.query(
    "insert into router_turn(id, session_id, trigger_id, state, started_at, ended_at, tokens, error) values (?, ?, ?, ?, ?, ?, ?, ?)",
  ).run(row.id, row.session_id, row.trigger_id, row.state, row.started_at, null, 0, null);
  return row;
}

export function markRouterTurn(
  db: Database,
  id: string,
  state: RouterTurnState,
  tokens: number,
  error: string | null = null,
): void {
  db.query("update router_turn set state = ?, ended_at = ?, tokens = ?, error = ? where id = ?").run(
    state,
    Date.now(),
    tokens,
    error,
    id,
  );
}

export function recordRouterSession(
  db: Database,
  row: { id: string; pid: number; pgid: number; startTicks: string; model: string },
): void {
  db.query(
    "insert into router_session(id, pid, pgid, start_ticks, state, model, started_at) values (?, ?, ?, ?, 'starting', ?, ?)",
  ).run(row.id, row.pid, row.pgid, row.startTicks, row.model, Date.now());
}

export function markRouterSession(db: Database, id: string, state: string, error: string | null = null): void {
  db.query("update router_session set state = ?, ended_at = ?, error = ? where id = ?").run(state, Date.now(), error, id);
}

export function routerTokensSince(db: Database, since: number): number {
  const row = db.query("select coalesce(sum(tokens), 0) as n from router_turn where started_at >= ?").get(since) as {
    n: number;
  } | null;
  return row?.n ?? 0;
}

export function listTriggers(db: Database, limit = 20): TriggerRow[] {
  return db.query("select * from trigger order by at desc limit ?").all(limit) as TriggerRow[];
}

export function listTickets(db: Database, limit = 20): TicketRow[] {
  return db.query("select * from ticket order by created_at desc limit ?").all(limit) as TicketRow[];
}

export function listWorkers(db: Database, limit = 20): WorkerRow[] {
  return db.query("select * from worker order by rowid desc limit ?").all(limit) as WorkerRow[];
}

export function listOutbox(db: Database, limit = 20): OutboxRow[] {
  return db.query("select * from outbox order by created_at desc limit ?").all(limit) as OutboxRow[];
}

interface CountRow {
  n: number;
}

function count(db: Database, sql: string): number {
  const row = db.query(sql).get() as CountRow | null;
  return row?.n ?? 0;
}

export function statusSummary(db: Database): Record<string, number> {
  return {
    triggers: count(db, "select count(*) as n from trigger"),
    tickets: count(db, "select count(*) as n from ticket"),
    workers: count(db, "select count(*) as n from worker"),
    dispatches: count(db, "select count(*) as n from dispatch"),
    gates: count(db, "select count(*) as n from gate"),
    pendingOutbox: count(db, "select count(*) as n from outbox where status = 'pending'"),
  };
}
