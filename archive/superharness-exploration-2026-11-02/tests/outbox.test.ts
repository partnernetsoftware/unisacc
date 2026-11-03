import { Database } from "bun:sqlite";
import { mkdtempSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { expect, test } from "bun:test";
import { acceptTrigger, dispatchByTicket, insertTrigger, openDb, reportTicket } from "../src/db";

function withDb(fn: (db: Database) => void): void {
  const dir = mkdtempSync(join(tmpdir(), "superharness-outbox-"));
  const db = openDb(join(dir, "state.db"));
  try {
    fn(db);
  } finally {
    db.close();
    rmSync(dir, { recursive: true, force: true });
  }
}

function ticketFor(db: Database, text: string) {
  const trigger = insertTrigger(db, { source: "test", threadKey: text, text }).row;
  const ticket = acceptTrigger(db, trigger.id);
  const dispatch = dispatchByTicket(db, ticket.id);
  if (!dispatch) throw new Error("dispatch missing");
  return { ticket, dispatch };
}

test("progress stays in the DB and done writes one outbox row", () => {
  withDb((db) => {
    const { ticket, dispatch } = ticketFor(db, "done");
    reportTicket(db, ticket.id, "progress", "half", undefined, { dispatchId: dispatch.id });
    expect((db.query("select count(*) as n from outbox").get() as { n: number }).n).toBe(0);
    reportTicket(db, ticket.id, "ready", "finished", undefined, { dispatchId: dispatch.id, outcome: "succeeded" });
    const rows = db.query("select kind, body from outbox").all() as Array<{ kind: string; body: string }>;
    expect(rows).toEqual([{ kind: "done", body: "finished" }]);
    expect(() =>
      reportTicket(db, ticket.id, "ready", "again", undefined, { dispatchId: dispatch.id, outcome: "succeeded" }),
    ).toThrow("duplicate done");
  });
});

test("ask writes an ask outbox and opens a gate", () => {
  withDb((db) => {
    const { ticket, dispatch } = ticketFor(db, "ask");
    reportTicket(db, ticket.id, "blocked", "need input", undefined, {
      dispatchId: dispatch.id,
      outcome: "failed",
      question: { text: "which file?", options: ["a", "b"] },
    });
    const rows = db.query("select kind, body from outbox").all() as Array<{ kind: string; body: string }>;
    expect(rows).toEqual([{ kind: "ask", body: "which file?" }]);
    const gate = db.query("select state from gate").get() as { state: string };
    expect(gate.state).toBe("open");
  });
});
