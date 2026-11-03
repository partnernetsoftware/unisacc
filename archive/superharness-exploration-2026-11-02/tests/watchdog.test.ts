import { Database } from "bun:sqlite";
import { mkdtempSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { expect, test } from "bun:test";
import { acceptTrigger, dispatchByTicket, insertTrigger, openDb, recordLedger } from "../src/db";
import { readThreadStartTicks } from "../src/config";
import { killOwned, sweepOrphans } from "../src/process";

async function withDb(fn: (db: Database) => Promise<void>): Promise<void> {
  const dir = mkdtempSync(join(tmpdir(), "superharness-watchdog-"));
  const db = openDb(join(dir, "state.db"));
  try {
    await fn(db);
  } finally {
    db.close();
    rmSync(dir, { recursive: true, force: true });
  }
}

function ledger(id: string, pid: number, dispatchId: string | null) {
  return {
    id,
    ticket_id: dispatchId ? "ticket" : "-probe-",
    dispatch_id: dispatchId,
    pid,
    pgid: pid,
    start_ticks: readThreadStartTicks(pid) ?? "0",
    command: "sleep 60",
    state: "running" as const,
    started_at: Date.now(),
    exited_at: null,
    threads_before: 0,
    threads_after: null,
  };
}

test("a dead ledger PID is marked, never signalled", async () => {
  await withDb(async (db) => {
    recordLedger(db, ledger("pl_dead", 999_999, null));
    const result = await sweepOrphans(db);
    expect(result.marked).toContain("pl_dead");
    const row = db.query("select state from process_ledger where id = 'pl_dead'").get() as { state: string };
    expect(row.state).toBe("exited");
  });
});

test("a live ledger row with no dispatch is killed by exact PGID", async () => {
  await withDb(async (db) => {
    const child = Bun.spawn(["sleep", "60"], { detached: true, stdio: ["ignore", "ignore", "ignore"] });
    try {
      recordLedger(db, ledger("pl_orphan", child.pid, null));
      const result = await sweepOrphans(db);
      expect(result.killed).toContain("pl_orphan");
      expect(readThreadStartTicks(child.pid)).toBeNull();
    } finally {
      try {
        process.kill(-child.pid, "SIGKILL");
      } catch {
        // Already gone.
      }
    }
  });
});

test("a live worker with a fresh heartbeat is left alone", async () => {
  await withDb(async (db) => {
    const trigger = insertTrigger(db, { source: "test", threadKey: "healthy", text: "work" }).row;
    const ticket = acceptTrigger(db, trigger.id);
    const dispatch = dispatchByTicket(db, ticket.id)!;
    db.query("update dispatch set state = 'running', heartbeat_at = ?, started_at = ? where id = ?").run(
      Date.now(),
      Date.now(),
      dispatch.id,
    );
    const child = Bun.spawn(["sleep", "60"], { detached: true, stdio: ["ignore", "ignore", "ignore"] });
    try {
      recordLedger(db, ledger("pl_healthy", child.pid, dispatch.id));
      const result = await sweepOrphans(db);
      expect(result.skipped).toContain("pl_healthy");
      expect(readThreadStartTicks(child.pid)).not.toBeNull();
      await killOwned(db, "pl_healthy");
    } finally {
      try {
        process.kill(-child.pid, "SIGKILL");
      } catch {
        // Already gone.
      }
    }
  });
});

test("a stale running dispatch with no live ledger is failed", async () => {
  await withDb(async (db) => {
    const trigger = insertTrigger(db, { source: "test", threadKey: "stale", text: "work" }).row;
    const ticket = acceptTrigger(db, trigger.id);
    const dispatch = dispatchByTicket(db, ticket.id)!;
    db.query("update dispatch set state = 'running', heartbeat_at = 1, started_at = 1 where id = ?").run(dispatch.id);
    const result = await sweepOrphans(db);
    expect(result.failedDispatches).toContain(dispatch.id);
    const row = db.query("select state from dispatch where id = ?").get(dispatch.id) as { state: string };
    expect(row.state).toBe("failed");
  });
});
