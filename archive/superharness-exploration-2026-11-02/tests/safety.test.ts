import { Database } from "bun:sqlite";
import { mkdtempSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { describe, expect, test } from "bun:test";
import {
  acceptTrigger,
  ask,
  consumeSpawnBudget,
  dispatchByTicket,
  insertTrigger,
  openDb,
  recordLedger,
  reportTicket,
  resolveGate,
} from "../src/db";
import { assertNotNested, assertThreadBudget, loadConfig, threadCount } from "../src/config";
import { assertWorkerSlot } from "../src/guards";
import { killOwned, reclaimOrphans, spawnOwned } from "../src/process";

function withDb(fn: (db: Database) => Promise<void> | void): Promise<void> {
  const dir = mkdtempSync(join(tmpdir(), "superharness-test-"));
  const db = openDb(join(dir, "state.db"));
  return Promise.resolve(fn(db)).finally(() => {
    db.close();
    rmSync(dir, { recursive: true, force: true });
  });
}

function accepted(db: Database, thread = "t1") {
  const trigger = insertTrigger(db, { source: "test", threadKey: thread, text: "do work" });
  return acceptTrigger(db, trigger.row.id);
}

describe("superharness safety", () => {
  test("config defaults cap workers at 2, depth at 1, and dry-run on", () => {
    const config = loadConfig({ dryRun: true });
    expect(config.maxWorkers).toBe(2);
    expect(config.maxDepth).toBe(1);
    expect(config.dryRun).toBe(true);
    expect(config.workerModel).toContain("deepseek-v4.1-flash");
  });

  test("worker slots count pending tickets and reject the third", async () => {
    await withDb((db) => {
      assertWorkerSlot(db);
      accepted(db, "t1");
      assertWorkerSlot(db);
      accepted(db, "t2");
      expect(() => assertWorkerSlot(db)).toThrow(/worker limit/);
    });
  });

  test("nested harness depth is refused", () => {
    process.env.SUPERHARNESS_DEPTH = "1";
    try {
      expect(() => assertNotNested(loadConfig())).toThrow(/depth/);
    } finally {
      delete process.env.SUPERHARNESS_DEPTH;
    }
  });

  test("hourly spawn budget is enforced", async () => {
    await withDb((db) => {
      expect(consumeSpawnBudget(db, 2, Date.parse("2026-11-02T12:00:00Z"))).toBe(1);
      expect(consumeSpawnBudget(db, 2, Date.parse("2026-11-02T12:10:00Z"))).toBe(2);
      expect(() => consumeSpawnBudget(db, 2, Date.parse("2026-11-02T12:20:00Z"))).toThrow(/spawn budget/);
      expect(consumeSpawnBudget(db, 2, Date.parse("2026-11-02T13:00:00Z"))).toBe(1);
    });
  });

  test("thread budget uses /proc and a limit", () => {
    const count = threadCount();
    expect(count).toBeGreaterThan(0);
    expect(() => assertThreadBudget(loadConfig({ maxThreads: count + 100 }))).not.toThrow();
    expect(() => assertThreadBudget(loadConfig({ maxThreads: 1 }))).toThrow(/thread budget/);
  });

  test("dry-run records intent without spawning or consuming budget", async () => {
    await withDb((db) => {
      const ticket = accepted(db);
      const dispatch = dispatchByTicket(db, ticket.id);
      expect(dispatch).not.toBeNull();
      const result = spawnOwned(
        db,
        { ticketId: ticket.id, dispatchId: dispatch!.id, command: ["true"] },
        loadConfig({ dryRun: true }),
      );
      expect(result.dryRun).toBe(true);
      expect(result.pid).toBeNull();
      const ledger = db.query("select state from process_ledger where id = ?").get(result.ledgerId) as { state: string };
      expect(ledger.state).toBe("dry-run");
      const budget = db.query("select count(*) as n from spawn_budget").get() as { n: number };
      expect(budget.n).toBe(0);
    });
  });

  test("real child is killed by exact PGID and ledger is closed", async () => {
    await withDb(async (db) => {
      const ticket = accepted(db);
      const dispatch = dispatchByTicket(db, ticket.id);
      const result = spawnOwned(
        db,
        { ticketId: ticket.id, dispatchId: dispatch!.id, command: ["sleep", "30"] },
        loadConfig({ dryRun: false, spawnBudgetPerHour: 10, maxThreads: threadCount() + 300 }),
      );
      expect(result.pid).toBeGreaterThan(0);
      const outcome = await killOwned(db, result.ledgerId);
      expect(outcome).toBe("killed");
      const ledger = db.query("select state from process_ledger where id = ?").get(result.ledgerId) as { state: string };
      expect(ledger.state).toBe("killed");
    });
  });

  test("orphan reconciliation marks a dead ledger exited without signalling", async () => {
    await withDb(async (db) => {
      recordLedger(db, {
        id: "pl_dead",
        ticket_id: "tk_dead",
        dispatch_id: null,
        pid: 999_999,
        pgid: 999_999,
        start_ticks: "0",
        command: "true",
        state: "running",
        started_at: Date.now(),
        exited_at: null,
      });
      const results = await reclaimOrphans(db);
      expect(results).toEqual([{ id: "pl_dead", result: "marked-exited" }]);
      const ledger = db.query("select state from process_ledger where id = 'pl_dead'").get() as { state: string };
      expect(ledger.state).toBe("exited");
    });
  });

  test("done report is exactly-once per dispatch", async () => {
    await withDb((db) => {
      const ticket = accepted(db);
      const dispatch = dispatchByTicket(db, ticket.id)!;
      reportTicket(db, ticket.id, "ready", "done", undefined, { dispatchId: dispatch.id });
      expect(() => reportTicket(db, ticket.id, "ready", "done again", undefined, { dispatchId: dispatch.id })).toThrow(
        /duplicate done/,
      );
    });
  });

  test("ask creates an open gate and resolve reopens the ticket", async () => {
    await withDb((db) => {
      const ticket = accepted(db);
      const dispatch = dispatchByTicket(db, ticket.id)!;
      const gate = ask(db, ticket.id, dispatch.id, "ship?", ["yes", "no"]);
      expect(gate.state).toBe("open");
      const resolved = resolveGate(db, gate.id, "yes");
      expect(resolved.state).toBe("answered");
      const row = db.query("select status from ticket where id = ?").get(ticket.id) as { status: string };
      expect(row.status).toBe("running");
    });
  });

  test("token budget stops the dispatch", async () => {
    await withDb((db) => {
      const ticket = accepted(db);
      const dispatch = dispatchByTicket(db, ticket.id)!;
      expect(() =>
        reportTicket(db, ticket.id, "progress", "working", undefined, {
          dispatchId: dispatch.id,
          tokensUsed: 10,
          tokenBudget: 5,
        }),
      ).toThrow(/token budget/);
      const row = db.query("select state from dispatch where id = ?").get(dispatch.id) as { state: string };
      expect(row.state).toBe("failed");
    });
  });
});
