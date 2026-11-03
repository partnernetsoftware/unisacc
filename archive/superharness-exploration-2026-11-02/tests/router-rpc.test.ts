import { Database } from "bun:sqlite";
import { mkdtempSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { expect, test } from "bun:test";
import { insertTrigger, openDb } from "../src/db";
import { loadConfig, type HarnessConfig } from "../src/config";
import { runRouterPass } from "../src/router";
import { FakeRouterRpc } from "./fake-router-rpc";

async function withDb(fn: (db: Database) => Promise<void>): Promise<void> {
  const dir = mkdtempSync(join(tmpdir(), "superharness-router-rpc-"));
  const db = openDb(join(dir, "state.db"));
  try {
    await fn(db);
  } finally {
    db.close();
    rmSync(dir, { recursive: true, force: true });
  }
}

function testConfig(overrides: Partial<HarnessConfig> = {}): HarnessConfig {
  return loadConfig({
    dryRun: false,
    routerTurnTimeoutMs: 50,
    routerTurnTokenBudget: 50_000,
    routerHourlyTokenBudget: 500_000,
    ...overrides,
  });
}

function count(db: Database, table: string): number {
  return (db.query(`select count(*) as n from ${table}`).get() as { n: number }).n;
}

test("real pass accepts once and is idempotent on the same trigger", async () => {
  await withDb(async (db) => {
    insertTrigger(db, { source: "test", threadKey: "t", text: "fix docs" });
    const rpc = new FakeRouterRpc([{ text: '{"decision":"accept","reason":"work"}', tokensUsed: 10 }]);
    const first = await runRouterPass(db, rpc, "s1", testConfig());
    expect(first.accepted).toBe(1);
    expect(first.failed).toBe(0);
    expect(first.turns[0]?.state).toBe("decided");
    expect(count(db, "ticket")).toBe(1);
    expect(count(db, "dispatch")).toBe(1);
    const dispatch = db.query("select state from dispatch").get() as { state: string };
    expect(dispatch.state).toBe("pending");
    const second = await runRouterPass(db, rpc, "s1", testConfig());
    expect(second.accepted).toBe(0);
    expect(rpc.prompts.length).toBe(1);
    expect(count(db, "ticket")).toBe(1);
    expect(count(db, "router_turn")).toBe(1);
  });
});

test("ignore records the decision without a ticket", async () => {
  await withDb(async (db) => {
    insertTrigger(db, { source: "test", threadKey: "t", text: "noise" });
    const rpc = new FakeRouterRpc([{ text: '{"decision":"ignore","reason":"noise"}' }]);
    const result = await runRouterPass(db, rpc, "s1", testConfig());
    expect(result.ignored).toBe(1);
    const trigger = db.query("select disposition from trigger").get() as { disposition: string };
    expect(trigger.disposition).toBe("ignored");
    expect(count(db, "ticket")).toBe(0);
  });
});

test("timeout marks the turn and stops the pass without a ticket", async () => {
  await withDb(async (db) => {
    insertTrigger(db, { source: "test", threadKey: "t", text: "work" });
    const rpc = new FakeRouterRpc([{ text: '{"decision":"accept"}', delayMs: 100 }]);
    const result = await runRouterPass(db, rpc, "s1", testConfig({ routerTurnTimeoutMs: 10 }));
    expect(result.failed).toBe(1);
    expect(result.stopped).toBe("timeout");
    expect(count(db, "ticket")).toBe(0);
    const turn = db.query("select state, error from router_turn").get() as { state: string; error: string };
    expect(turn.state).toBe("timeout");
    expect(turn.error).toContain("timed out");
  });
});

test("malformed reply fails and leaves no ticket", async () => {
  await withDb(async (db) => {
    insertTrigger(db, { source: "test", threadKey: "t", text: "work" });
    const rpc = new FakeRouterRpc([{ text: "not-json" }]);
    const result = await runRouterPass(db, rpc, "s1", testConfig());
    expect(result.failed).toBe(1);
    expect(result.stopped).toBe("malformed");
    expect(count(db, "ticket")).toBe(0);
    const trigger = db.query("select disposition from trigger").get() as { disposition: string };
    expect(trigger.disposition).toBe("new");
  });
});

test("worker cap defers before creating a turn", async () => {
  await withDb(async (db) => {
    insertTrigger(db, { source: "test", threadKey: "t", text: "work" });
    db.query("insert into worker(id, ticket_id, state) values ('w1', 't1', 'pending')").run();
    db.query("insert into worker(id, ticket_id, state) values ('w2', 't2', 'pending')").run();
    const rpc = new FakeRouterRpc([{ text: '{"decision":"accept"}' }]);
    const result = await runRouterPass(db, rpc, "s1", testConfig());
    expect(result.deferred).toBe(1);
    expect(result.stopped).toBe("worker-slot");
    expect(rpc.prompts.length).toBe(0);
    expect(count(db, "router_turn")).toBe(0);
  });
});

test("per-turn token budget fails and stops new triggers", async () => {
  await withDb(async (db) => {
    insertTrigger(db, { source: "test", threadKey: "a", text: "one" });
    insertTrigger(db, { source: "test", threadKey: "b", text: "two" });
    const rpc = new FakeRouterRpc([{ text: '{"decision":"accept"}', tokensUsed: 50_001 }]);
    const result = await runRouterPass(db, rpc, "s1", testConfig());
    expect(result.failed).toBe(1);
    expect(result.stopped).toBe("turn-token-budget");
    expect(rpc.prompts.length).toBe(1);
    expect(count(db, "ticket")).toBe(0);
    const remaining = db.query("select count(*) as n from trigger where disposition = 'new'").get() as { n: number };
    expect(remaining.n).toBe(2);
  });
});

test("hourly budget stops before a second turn", async () => {
  await withDb(async (db) => {
    insertTrigger(db, { source: "test", threadKey: "t", text: "work" });
    db.query(
      "insert into router_turn(id, session_id, trigger_id, state, started_at, ended_at, tokens, error) values ('old','s0','old', 'decided', ?, ?, 500_000, null)",
    ).run(Date.now(), Date.now());
    const rpc = new FakeRouterRpc([{ text: '{"decision":"accept"}', tokensUsed: 1 }]);
    const result = await runRouterPass(db, rpc, "s1", testConfig({ routerHourlyTokenBudget: 500_000 }));
    expect(result.failed).toBe(0);
    expect(result.stopped).toBe("hourly-token-budget");
    expect(rpc.prompts.length).toBe(0);
    expect(count(db, "ticket")).toBe(0);
  });
});
