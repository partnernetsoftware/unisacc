import { readFileSync, readdirSync } from "node:fs";

export interface HarnessConfig {
  maxWorkers: number;
  maxDepth: number;
  spawnBudgetPerHour: number;
  tokenBudgetPerWorker: number;
  maxThreads: number;
  dryRun: boolean;
  workerModel: string;
  workerTimeoutMs: number;
  routerModel: string;
  routerTurnTimeoutMs: number;
  routerTurnTokenBudget: number;
  routerHourlyTokenBudget: number;
  serverPort: number;
  dbPath: string;
}

function envInt(name: string, fallback: number): number {
  const raw = process.env[name];
  if (raw === undefined || raw.trim() === "") return fallback;
  const value = Number(raw);
  return Number.isFinite(value) && value >= 0 ? Math.floor(value) : fallback;
}

export function loadConfig(overrides: Partial<HarnessConfig> = {}): HarnessConfig {
  return {
    maxWorkers: envInt("SUPERHARNESS_MAX_WORKERS", 2),
    maxDepth: envInt("SUPERHARNESS_MAX_DEPTH", 1),
    spawnBudgetPerHour: envInt("SUPERHARNESS_SPAWN_BUDGET", 20),
    tokenBudgetPerWorker: envInt("SUPERHARNESS_TOKEN_BUDGET", 1_000_000),
    maxThreads: envInt("SUPERHARNESS_MAX_THREADS", 4096),
    dryRun: process.env.SUPERHARNESS_DRY_RUN !== "0",
    workerModel: process.env.SUPERHARNESS_WORKER_MODEL ?? "codebuddy/deepseek-v4.1-flash",
    workerTimeoutMs: envInt("SUPERHARNESS_WORKER_TIMEOUT_MS", 120_000),
    routerModel: process.env.SUPERHARNESS_ROUTER_MODEL ?? "codebuddy/deepseek-v4.1-flash",
    routerTurnTimeoutMs: envInt("SUPERHARNESS_ROUTER_TURN_TIMEOUT_MS", 30_000),
    routerTurnTokenBudget: envInt("SUPERHARNESS_ROUTER_TURN_TOKENS", 50_000),
    routerHourlyTokenBudget: envInt("SUPERHARNESS_ROUTER_HOURLY_TOKENS", 500_000),
    serverPort: envInt("SUPERHARNESS_PORT", 8787),
    dbPath: process.env.SUPERHARNESS_DB ?? "state.db",
    ...overrides,
  };
}

/** Threads are the unit of the nproc guard; /proc is reliable when ps is stripped. */
export function threadCount(): number {
  let dirs: string[];
  try {
    dirs = readdirSync("/proc");
  } catch {
    dirs = [];
  }
  let count = 0;
  for (const dir of dirs) {
    if (!/^\d+$/.test(dir)) continue;
    try {
      count += readdirSync(`/proc/${dir}/task`).length;
    } catch {
      // The process exited between readdir and task read; not an error.
    }
  }
  if (count > 0) return count;
  try {
    const ps = Bun.spawnSync(["ps", "-eLo", "pid="], { stdout: "pipe", stderr: "ignore" });
    return new TextDecoder().decode(ps.stdout).split("\n").filter((line) => line.trim() !== "").length;
  } catch {
    return 0;
  }
}

export function assertThreadBudget(config: HarnessConfig): void {
  const threads = threadCount();
  if (threads + 32 > config.maxThreads) {
    throw new Error(`thread budget exceeded: ${String(threads)}+32 > ${String(config.maxThreads)}`);
  }
}

export function assertNotNested(config: HarnessConfig): void {
  const depth = envInt("SUPERHARNESS_DEPTH", 0);
  if (depth > 0) {
    throw new Error(`superharness refuses to spawn at depth ${String(depth)} (max ${String(config.maxDepth)})`);
  }
}

export function readThreadStartTicks(pid: number): string | null {
  try {
    const stat = readFileSync(`/proc/${String(pid)}/stat`, "utf8");
    const close = stat.lastIndexOf(")");
    if (close < 0) return null;
    const fields = stat.slice(close + 1).trim().split(/\s+/);
    return fields[19] ?? null;
  } catch {
    return null;
  }
}
