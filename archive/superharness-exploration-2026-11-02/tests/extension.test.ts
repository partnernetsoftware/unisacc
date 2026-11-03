import { mkdtempSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { expect, test } from "bun:test";
import extension from "../extension";
import { dispatchByTicket, insertTrigger, openDb } from "../src/db";
import { readEndpoint } from "../src/endpoint";

type Tool = {
  name: string;
  execute: (id: string, params: Record<string, unknown>, signal?: AbortSignal, update?: unknown, ctx?: unknown) => Promise<unknown>;
};

function resultText(result: unknown): string {
  const value = result as { content?: Array<{ text?: string }> };
  return value.content?.[0]?.text ?? "";
}

test("omp extension tool bridge talks to the harness server", async () => {
  const dir = mkdtempSync(join(tmpdir(), "superharness-ext-"));
  const dbPath = join(dir, "state.db");
  const endpointFile = join(dir, "endpoint.env");
  const env = {
    ...process.env,
    SUPERHARNESS_DB: dbPath,
    SUPERHARNESS_ENDPOINT: endpointFile,
    SUPERHARNESS_PORT: "0",
    SUPERHARNESS_HOST: "127.0.0.1",
    SUPERHARNESS_DRY_RUN: "1",
  };
  const server = Bun.spawn(["bun", "src/server.ts"], {
    cwd: join(import.meta.dir, ".."),
    env,
    stdout: "pipe",
    stderr: "pipe",
  });
  const previousEndpoint = process.env.SUPERHARNESS_ENDPOINT;
  process.env.SUPERHARNESS_ENDPOINT = endpointFile;
  try {
    // The server prints its listening line after the endpoint file is written.
    const reader = server.stdout.getReader();
    const decoder = new TextDecoder();
    let log = "";
    while (!log.includes("listening")) {
      const { value, done } = await reader.read();
      if (done) break;
      log += decoder.decode(value);
    }
    reader.releaseLock();
    const endpoint = readEndpoint();
    if (!endpoint) throw new Error(`server did not write endpoint: ${log}`);
    expect(endpoint).not.toBeNull();

    const tools = new Map<string, Tool>();
    extension({ registerTool: (tool: Tool) => tools.set(tool.name, tool) });
    expect([...tools.keys()].sort()).toEqual(["harness_read", "harness_reply", "harness_report", "harness_route"]);

    const db = openDb(dbPath);
    try {
      const trigger = insertTrigger(db, { source: "test", threadKey: "ext/thread", text: "bridge" });
      const route = tools.get("harness_route")!;
      const routed = await route.execute("1", { trigger_id: trigger.row.id, decision: "accept" });
      expect(resultText(routed)).toContain('"ok": true');

      const ticket = db.query("select * from ticket limit 1").get() as { id: string };
      const dispatch = dispatchByTicket(db, ticket.id);
      expect(dispatch).not.toBeNull();

      const report = await tools.get("harness_report")!.execute("2", {
        ticket_id: ticket.id,
        dispatch_id: dispatch!.id,
        status: "ready",
        summary: "bridge done",
        files_modified: "a.ts,b.ts",
        tokens_used: 12,
      });
      expect(resultText(report)).toContain('"status": "ready"');

      const read = await tools.get("harness_read")!.execute("3", { resource: "ticket", id: ticket.id });
      expect(resultText(read)).toContain('"id": "');

      const reply = await tools.get("harness_reply")!.execute("4", { ticket_id: ticket.id, channel: "local", body: "done" });
      expect(resultText(reply)).toContain('"ok": true');

      const rows = db.query("select kind, body from outbox where ticket_id = ? order by id").all(ticket.id) as Array<{
        kind: string | null;
        body: string;
      }>;
      expect(rows.length).toBe(2);
      expect(rows[0]?.kind).toBe("done");
      expect(rows[1]?.body).toBe("done");
    } finally {
      db.close();
    }
  } finally {
    if (previousEndpoint === undefined) delete process.env.SUPERHARNESS_ENDPOINT;
    else process.env.SUPERHARNESS_ENDPOINT = previousEndpoint;
    server.kill("SIGTERM");
    await server.exited;
    rmSync(dir, { recursive: true, force: true });
  }
});
