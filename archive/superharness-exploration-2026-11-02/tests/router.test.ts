import { Database } from "bun:sqlite";
import { mkdtempSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { expect, test } from "bun:test";
import { insertTrigger, openDb } from "../src/db";
import { runRouterDryRun } from "../src/router";

function withDb(fn: (db: Database) => void): void {
  const dir = mkdtempSync(join(tmpdir(), "superharness-router-"));
  const db = openDb(join(dir, "state.db"));
  try {
    fn(db);
  } finally {
    db.close();
    rmSync(dir, { recursive: true, force: true });
  }
}

test("dry-run router persists decisions and defers beyond the worker cap", () => {
  withDb((db) => {
    insertTrigger(db, { source: "test", threadKey: "t1", text: "fix docs" });
    insertTrigger(db, { source: "test", threadKey: "t2", text: "ignore this noise" });
    insertTrigger(db, { source: "test", threadKey: "t3", text: "fix tests" });
    const result = runRouterDryRun(db);
    expect(result.accepted).toBe(2);
    expect(result.ignored).toBe(1);
    expect(result.deferred).toBe(0);
    const tickets = db.query("select count(*) as n from ticket").get() as { n: number };
    const dispatches = db.query("select count(*) as n from dispatch").get() as { n: number };
    const decisions = db.query("select count(*) as n from router_decision").get() as { n: number };
    expect(tickets.n).toBe(2);
    expect(dispatches.n).toBe(2);
    expect(decisions.n).toBe(3);
  });
});

test("dry-run router leaves the third trigger deferred at the cap", () => {
  withDb((db) => {
    insertTrigger(db, { source: "test", threadKey: "a", text: "one" });
    insertTrigger(db, { source: "test", threadKey: "b", text: "two" });
    insertTrigger(db, { source: "test", threadKey: "c", text: "three" });
    const result = runRouterDryRun(db);
    expect(result.accepted).toBe(2);
    expect(result.deferred).toBe(1);
    const remaining = db.query("select count(*) as n from trigger where disposition = 'new'").get() as { n: number };
    expect(remaining.n).toBe(1);
  });
});
