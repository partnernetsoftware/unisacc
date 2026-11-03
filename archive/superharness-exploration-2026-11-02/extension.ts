import { readEndpoint } from "./src/endpoint";

type Json = Record<string, unknown>;

async function call(path: string, init?: RequestInit): Promise<unknown> {
  const endpoint = readEndpoint();
  if (!endpoint) throw new Error("superharness endpoint not found; start out/superharness/src/server.ts first");
  const response = await fetch(`${endpoint.url}${path}`, {
    ...init,
    headers: {
      "content-type": "application/json",
      authorization: `Bearer ${endpoint.token}`,
      ...(init?.headers ?? {}),
    },
  });
  const text = await response.text();
  let body: unknown = text;
  try {
    body = JSON.parse(text);
  } catch {
    // Keep the raw text in the error/result.
  }
  if (!response.ok) throw new Error(`superharness ${String(response.status)}: ${text}`);
  return body;
}

function textResult(value: unknown): { content: Array<{ type: "text"; text: string }> } {
  return { content: [{ type: "text", text: JSON.stringify(value, null, 2) }] };
}

function jsonBody(value: Json): RequestInit {
  return { method: "POST", body: JSON.stringify(value) };
}

interface ToolLike {
  name: string;
  label: string;
  description: string;
  parameters: unknown;
  approval?: "read" | "write" | "exec";
  execute: (toolCallId: string, params: Json, signal: AbortSignal | undefined, onUpdate: unknown, ctx: unknown) => Promise<unknown>;
}

/**
 * Register the four tools a Router/Worker uses to talk to the harness.
 * The endpoint file is re-read on every call, so a harness restart with a new
 * port/token does not strand a long-lived omp session.
 */
export default function superharnessExtension(pi: { registerTool(tool: ToolLike): void }): void {
  pi.registerTool({
    name: "harness_read",
    label: "Harness Read",
    description: "Read harness state: status, a trigger/ticket/thread/outbox, or recent lists.",
    parameters: {
      type: "object",
      properties: {
        resource: { type: "string", description: "status|trigger|ticket|dispatch|thread|outbox|triggers|tickets|dispatches|workers|replies|gates" },
        id: { type: "string", description: "id/key for the single-resource reads" },
      },
      required: ["resource"],
    },
    approval: "read",
    async execute(_toolCallId, params) {
      const resource = String(params.resource ?? "");
      const id = params.id === undefined ? "" : String(params.id);
      const query = new URLSearchParams({ resource });
      if (id) query.set("id", id);
      return textResult(await call(`/read?${query.toString()}`));
    },
  });

  pi.registerTool({
    name: "harness_route",
    label: "Harness Route",
    description: "Accept or ignore a trigger. Accept creates a ticket and a pending dispatch.",
    parameters: {
      type: "object",
      properties: {
        trigger_id: { type: "string" },
        decision: { type: "string", description: "accept|ignore" },
        ticket_id: { type: "string" },
      },
      required: ["trigger_id", "decision"],
    },
    approval: "write",
    async execute(_toolCallId, params) {
      return textResult(
        await call(
          "/route",
          jsonBody({
            triggerId: String(params.trigger_id ?? ""),
            decision: String(params.decision ?? ""),
            ticketId: params.ticket_id === undefined ? undefined : String(params.ticket_id),
          }),
        ),
      );
    },
  });

  pi.registerTool({
    name: "harness_report",
    label: "Harness Report",
    description: "Report progress, ready, blocked, failed, or waiting for a ticket. Done reports are exactly-once per dispatch.",
    parameters: {
      type: "object",
      properties: {
        ticket_id: { type: "string" },
        dispatch_id: { type: "string" },
        status: { type: "string", description: "progress|ready|blocked|failed|waiting" },
        summary: { type: "string" },
        outcome: { type: "string", description: "succeeded|failed" },
        files_modified: { type: "string", description: "comma-separated paths" },
        tokens_used: { type: "number" },
        reply: { type: "string" },
        channel: { type: "string" },
        address: { type: "string" },
        question: { type: "string" },
        options: { type: "string", description: "comma-separated question options" },
      },
      required: ["ticket_id", "status", "summary"],
    },
    approval: "write",
    async execute(_toolCallId, params) {
      const files = String(params.files_modified ?? "")
        .split(",")
        .map((value) => value.trim())
        .filter(Boolean);
      return textResult(
        await call(
          "/report",
          jsonBody({
            ticketId: String(params.ticket_id ?? ""),
            dispatchId: params.dispatch_id === undefined ? undefined : String(params.dispatch_id),
            status: String(params.status ?? ""),
            summary: String(params.summary ?? ""),
            outcome: params.outcome === undefined ? undefined : String(params.outcome),
            filesModified: files,
            tokensUsed: params.tokens_used === undefined ? 0 : Number(params.tokens_used),
            reply: params.reply === undefined ? undefined : String(params.reply),
            channel: params.channel === undefined ? undefined : String(params.channel),
            address: params.address === undefined ? undefined : String(params.address),
            question: params.question === undefined ? undefined : String(params.question),
            options: String(params.options ?? "")
              .split(",")
              .map((value) => value.trim())
              .filter(Boolean),
          }),
        ),
      );
    },
  });

  pi.registerTool({
    name: "harness_reply",
    label: "Harness Reply",
    description: "Queue a channel reply through the harness outbox.",
    parameters: {
      type: "object",
      properties: {
        ticket_id: { type: "string" },
        channel: { type: "string" },
        address: { type: "string" },
        body: { type: "string" },
      },
      required: ["channel", "body"],
    },
    approval: "write",
    async execute(_toolCallId, params) {
      return textResult(
        await call(
          "/reply",
          jsonBody({
            ticketId: params.ticket_id === undefined ? undefined : String(params.ticket_id),
            channel: String(params.channel ?? ""),
            address: params.address === undefined ? undefined : String(params.address),
            body: String(params.body ?? ""),
          }),
        ),
      );
    },
  });
}
