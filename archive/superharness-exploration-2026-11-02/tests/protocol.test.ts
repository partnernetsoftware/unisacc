import { Database } from "bun:sqlite";
import { mkdtempSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { describe, expect, test } from "bun:test";
import { acceptTrigger, dispatchByTicket, insertTrigger, openDb, reportTicket } from "../src/db";
import { claimDispatch } from "../src/dispatch";
import { isError, parseClaim, parseGate, parseOutbox, parseRoute } from "../src/protocol";

function withDb(fn: (db: Database) => void): void {
  const dir = mkdtempSync(join(tmpdir(), "superharness-proto-"));
  const db = openDb(join(dir, "state.db"));
  try {
    fn(db);
  } finally {
    db.close();
    rmSync(dir, { recursive: true, force: true });
  }
}

function accepted(db: Database, thread = "t1") {
  const trigger = insertTrigger(db, { source: "test", threadKey: thread, text: "do work" });
  return acceptTrigger(db, trigger.row.id);
}

describe("decision protocol shapes", () => {
  test("route rejects a missing triggerId or an unknown decision", () => {
    expect(isError(parseRoute({ decision: "accept" }))).toBe(true);
    expect(isError(parseRoute({ triggerId: "tr1", decision: "maybe" }))).toBe(true);
    const ok = parseRoute({ triggerId: "tr1", decision: "accept", reason: "r" });
    expect(isError(ok)).toBe(false);
    expect((ok as { decision: string }).decision).toBe("accept");
  });

  test("claim needs a worker id and keeps an optional ticket filter", () => {
    expect(isError(parseClaim({}))).toBe(true);
    expect(isError(parseClaim({ workerId: "" }))).toBe(true);
    const ok = parseClaim({ workerId: "w1", ticketId: "tk1" });
    expect(isError(ok)).toBe(false);
    expect((ok as { ticketId: string }).ticketId).toBe("tk1");
  });

  test("gate requires a dispatch id and a non-empty question", () => {
    expect(isError(parseGate({ dispatchId: "d1", question: "   " }))).toBe(true);
    const ok = parseGate({ dispatchId: "d1", question: "which file?", options: ["a", 3, "b"] });
    expect(isError(ok)).toBe(false);
    expect((ok as { options: string[] }).options).toEqual(["a", "b"]);
  });

  test("outbox requires channel and body and never sends directly", () => {
    expect(isError(parseOutbox({ channel: "1:cc" }))).toBe(true);
    expect(isError(parseOutbox({ body: "hi" }))).toBe(true);
    const ok = parseOutbox({ channel: "1:cc", body: "hi", ticketId: "tk1" });
    expect(isError(ok)).toBe(false);
  });
});

describe("dispatch claim", () => {
  test("claims the pending dispatch and moves ticket to running", () => {
    withDb((db) => {
      const ticket = accepted(db);
      const pending = dispatchByTicket(db, ticket.id);
      expect(pending?.state).toBe("pending");
      const lease = claimDispatch(db, pending!.id, "worker-1", "/tmp/w1");
      expect(lease.ticketId).toBe(ticket.id);
      expect(lease.attempt).toBe(1);
      const after = dispatchByTicket(db, ticket.id);
      expect(after?.state).toBe("running");
      const worker = db.query("select state, workdir from worker where id = ?").get("worker-1") as {
        state: string;
        workdir: string;
      };
      expect(worker.state).toBe("running");
      expect(worker.workdir).toBe("/tmp/w1");
    });
  });

  test("a second claim on the same ticket is refused (one active dispatch)", () => {
    withDb((db) => {
      const ticket = accepted(db);
      const first = dispatchByTicket(db, ticket.id)!;
      claimDispatch(db, first.id, "worker-1");
      expect(() => claimDispatch(db, first.id, "worker-2")).toThrow(/not pending/);
    });
  });

  test("done stays exactly once: a second ready report is refused", () => {
    withDb((db) => {
      const ticket = accepted(db);
      const d = dispatchByTicket(db, ticket.id)!;
      claimDispatch(db, d.id, "worker-1");
      reportTicket(db, ticket.id, "ready", "sum", undefined, { dispatchId: d.id, outcome: "succeeded" });
      expect(() =>
        reportTicket(db, ticket.id, "ready", "sum again", undefined, { dispatchId: d.id, outcome: "succeeded" }),
      ).toThrow(/duplicate/i);
    });
  });
});

describe("extension tool bridge", () => {
  test("each verb posts to its decision-protocol path with a bearer token", async () => {
    const calls: { path: string; body: unknown; auth: string }[] = [];
    const fakeFetch = (async (url: string | URL | Request, init?: RequestInit) => {
      calls.push({
        path: String(url).replace("http://h", ""),
        body: JSON.parse(String(init?.body ?? "{}")) as unknown,
        auth: String((init?.headers as Record<string, string>).authorization),
      });
      return new Response('{"ok":true}', { status: 200 });
    }) as unknown as typeof fetch;
    const ctx = { url: "http://h", token: "tok", fetch: fakeFetch };
    const { harnessRoute, harnessClaim, harnessGate, harnessReply, PROTOCOL_TOOLS } = await import(
      "../src/extension-protocol"
    );
    await harnessRoute(ctx, { triggerId: "tr1", decision: "accept" });
    await harnessClaim(ctx, { workerId: "w1" });
    await harnessGate(ctx, { ticketId: "tk1", dispatchId: "d1", question: "?" });
    await harnessReply(ctx, { channel: "1:cc", body: "hi" });
    expect(calls.map((c) => c.path)).toEqual(["/route", "/dispatch/claim", "/gate/ask", "/outbox/enqueue"]);
    expect(calls.every((c) => c.auth === "Bearer tok")).toBe(true);
    expect(PROTOCOL_TOOLS.map((t) => t.name)).toEqual([
      "harness_route",
      "harness_claim",
      "harness_gate",
      "harness_reply",
    ]);
  });

  test("a missing token is refused before any request", async () => {
    const { harnessRoute } = await import("../src/extension-protocol");
    await expect(harnessRoute({ url: "http://h" }, { triggerId: "tr1", decision: "accept" })).rejects.toThrow(
      /SUPERHARNESS_TOKEN/,
    );
  });
});
