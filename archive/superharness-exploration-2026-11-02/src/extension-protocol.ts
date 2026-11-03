/**
 * omp extension tool bridge for the decision protocol.
 *
 * Loaded with `-e out/superharness/extension.ts` (or this file directly in a
 * Worker session). It exposes the four verbs of the protocol as tools that talk
 * to the harness HTTP server with a bearer token from SUPERHARNESS_TOKEN and
 * SUPERHARNESS_URL. Nothing here spawns: the harness owns processes.
 *
 *   harness_route(triggerId, decision)            accept | ignore
 *   harness_claim(workerId, ticketId?)            take one pending dispatch
 *   harness_gate(ticketId, dispatchId, question)  ask; ticket becomes blocked
 *   harness_reply(channel, body)                  queue an outbox message
 *
 * The same vocabulary as the decision protocol (see src/protocol.ts); there is
 * no second set of states.
 */

export interface ProtocolToolContext {
  url?: string;
  token?: string;
  fetch?: typeof fetch;
}

function env(ctx: ProtocolToolContext): { url: string; token: string } {
  const url = ctx.url ?? (typeof process !== "undefined" ? process.env.SUPERHARNESS_URL : undefined);
  const token = ctx.token ?? (typeof process !== "undefined" ? process.env.SUPERHARNESS_TOKEN : undefined);
  if (!url || !token) throw new Error("harness: SUPERHARNESS_URL and SUPERHARNESS_TOKEN are required");
  return { url, token };
}

async function post(ctx: ProtocolToolContext, path: string, body: unknown): Promise<unknown> {
  const { url, token } = env(ctx);
  const doFetch = ctx.fetch ?? fetch;
  const res = await doFetch(`${url}${path}`, {
    method: "POST",
    headers: { "content-type": "application/json", authorization: `Bearer ${token}` },
    body: JSON.stringify(body),
  });
  const text = await res.text();
  if (!res.ok) throw new Error(`harness ${path} ${String(res.status)}: ${text}`);
  return JSON.parse(text) as unknown;
}

export async function harnessRoute(
  ctx: ProtocolToolContext,
  args: { triggerId: string; decision: "accept" | "ignore"; reason?: string },
): Promise<unknown> {
  return post(ctx, "/route", args);
}

export async function harnessClaim(
  ctx: ProtocolToolContext,
  args: { workerId: string; ticketId?: string },
): Promise<unknown> {
  return post(ctx, "/dispatch/claim", args);
}

export async function harnessGate(
  ctx: ProtocolToolContext,
  args: { ticketId: string; dispatchId: string; question: string; options?: string[] },
): Promise<unknown> {
  return post(ctx, "/gate/ask", args);
}

export async function harnessReply(
  ctx: ProtocolToolContext,
  args: { channel: string; body: string; ticketId?: string },
): Promise<unknown> {
  return post(ctx, "/outbox/enqueue", args);
}

/** Shape the four verbs for omp's registerTool (names + JSON schemas kept in one place). */
export const PROTOCOL_TOOLS = [
  {
    name: "harness_route",
    description: "Router decision for one trigger: accept (opens a ticket) or ignore.",
    parameters: {
      type: "object",
      required: ["triggerId", "decision"],
      properties: {
        triggerId: { type: "string" },
        decision: { type: "string", enum: ["accept", "ignore"] },
        reason: { type: "string" },
      },
    },
  },
  {
    name: "harness_claim",
    description: "Claim the earliest pending dispatch for this worker (one active per ticket).",
    parameters: {
      type: "object",
      required: ["workerId"],
      properties: { workerId: { type: "string" }, ticketId: { type: "string" } },
    },
  },
  {
    name: "harness_gate",
    description: "Ask the human a blocking question; the ticket becomes blocked until answered.",
    parameters: {
      type: "object",
      required: ["ticketId", "dispatchId", "question"],
      properties: {
        ticketId: { type: "string" },
        dispatchId: { type: "string" },
        question: { type: "string" },
        options: { type: "array", items: { type: "string" } },
      },
    },
  },
  {
    name: "harness_reply",
    description: "Queue a reply in the outbox; the harness sends it and owns retries.",
    parameters: {
      type: "object",
      required: ["channel", "body"],
      properties: { channel: { type: "string" }, body: { type: "string" }, ticketId: { type: "string" } },
    },
  },
] as const;
