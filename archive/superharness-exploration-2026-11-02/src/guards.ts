import type { Database } from "bun:sqlite";

function envInt(name: string, fallback: number): number {
  const raw = process.env[name];
  if (raw === undefined || raw.trim() === "") return fallback;
  const value = Number(raw);
  return Number.isFinite(value) && value >= 0 ? Math.floor(value) : fallback;
}

/**
 * Hard limits are deliberately small. A harness that spawns agents must be
 * unable to repeat the .cx recursion incident: cap workers, cap total
 * processes, and refuse to run at depth > 0.
 */
export const GUARDS = {
  maxWorkers: envInt("SUPERHARNESS_MAX_WORKERS", 2),
  maxProcesses: envInt("SUPERHARNESS_MAX_PROCESSES", 6),
  maxDepth: envInt("SUPERHARNESS_MAX_DEPTH", 1),
  maxRuntimeMs: envInt("SUPERHARNESS_MAX_RUNTIME_MS", 20 * 60_000),
};

export function assertNotNested(): void {
  const depth = envInt("SUPERHARNESS_DEPTH", 0);
  if (depth > 0) {
    throw new Error(`superharness refuses to spawn: SUPERHARNESS_DEPTH=${String(depth)} (max ${String(GUARDS.maxDepth)})`);
  }
}

function count(db: Database, sql: string): number {
  const row = db.query(sql).get() as { n: number } | null;
  return row?.n ?? 0;
}

export function activeWorkerCount(db: Database): number {
  return count(db, "select count(*) as n from worker where state in ('starting','running','stalled')");
}

export function pendingWorkerCount(db: Database): number {
  return count(db, "select count(*) as n from worker where state = 'pending'");
}

/** Total live ledger rows plus the requested new process must fit maxProcesses. */
export function assertProcessSlot(db: Database, extra = 1): void {
  const live = count(db, "select count(*) as n from process_ledger where state = 'running'");
  if (live + extra > GUARDS.maxProcesses) {
    throw new Error(`process budget reached: ${String(live)}+${String(extra)}/${String(GUARDS.maxProcesses)}`);
  }
}

export function assertWorkerSlot(db: Database): void {
  const total = count(db, "select count(*) as n from worker where state in ('pending','starting','running','stalled')");
  if (total >= GUARDS.maxWorkers) {
    throw new Error(`worker limit reached: ${String(total)}/${String(GUARDS.maxWorkers)}`);
  }
  if (total >= GUARDS.maxProcesses) {
    throw new Error(`process budget reached: ${String(total)}/${String(GUARDS.maxProcesses)}`);
  }
}
