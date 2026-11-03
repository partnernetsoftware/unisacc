import {
  acceptTrigger,
  ask,
  resolveGate,
  nextPendingDispatch,
  dispatchById,
  enqueueReply,
  ignoreTrigger,
  listDispatches,
  listGates,
  listOutbox,
  listTickets,
  listTriggers,
  listWorkers,
  openDb,
  outboxByTicket,
  recordRouterDecision,
  reportTicket,
  statusSummary,
  threadByKey,
  ticketById,
  triggerById,
  defaultDbPath,
} from "./db";
import { assertWorkerSlot, GUARDS } from "./guards";
import { claimDispatch } from "./dispatch";
import { isError, parseClaim, parseGate, parseOutbox } from "./protocol";
import { loadConfig } from "./config";
import { removeEndpoint, writeEndpoint } from "./endpoint";
import { reclaimOrphans } from "./process";
import type { ReportStatus } from "./types";

const REPORT_STATUSES: readonly ReportStatus[] = ["progress", "ready", "blocked", "failed", "waiting"];

class HttpError extends Error {
  constructor(
    readonly status: number,
    message: string,
  ) {
    super(message);
  }
}

function json(value: unknown, status = 200): Response {
  return new Response(JSON.stringify(value), {
    status,
    headers: { "content-type": "application/json; charset=utf-8" },
  });
}

function stringField(body: Record<string, unknown>, key: string): string | undefined {
  const value = body[key];
  return typeof value === "string" ? value : undefined;
}

function stringArray(value: unknown): string[] {
  return Array.isArray(value) ? value.filter((item): item is string => typeof item === "string") : [];
}

function requiredString(body: Record<string, unknown>, key: string): string {
  const value = stringField(body, key);
  if (!value) throw new HttpError(400, `${key} is required`);
  return value;
}

function reportStatus(value: string | undefined): ReportStatus {
  if (!value || !REPORT_STATUSES.includes(value as ReportStatus)) {
    throw new HttpError(400, `status must be one of ${REPORT_STATUSES.join("|")}`);
  }
  return value as ReportStatus;
}

const db = openDb(defaultDbPath());
const config = loadConfig();
const token = crypto.randomUUID();

function authorized(req: Request): boolean {
  return req.headers.get("authorization") === `Bearer ${token}`;
}

async function readBody(req: Request): Promise<Record<string, unknown>> {
  let value: unknown;
  try {
    value = await req.json();
  } catch {
    throw new HttpError(400, "body must be JSON");
  }
  if (typeof value !== "object" || value === null || Array.isArray(value)) {
    throw new HttpError(400, "body must be a JSON object");
  }
  return value as Record<string, unknown>;
}

function readResource(url: URL): unknown {
  const resource = url.searchParams.get("resource") ?? "status";
  const id = url.searchParams.get("id") ?? "";
  switch (resource) {
    case "status":
      return { summary: statusSummary(db), guards: GUARDS };
    case "trigger":
      return triggerById(db, id);
    case "ticket":
      return ticketById(db, id);
    case "thread":
      return threadByKey(db, id);
    case "outbox":
      return outboxByTicket(db, id);
    case "triggers":
      return listTriggers(db, 50);
    case "tickets":
      return listTickets(db, 50);
    case "workers":
      return listWorkers(db, 50);
    case "replies":
      return listOutbox(db, 50);
    case "gates":
      return listGates(db, id || undefined, 50);
    case "dispatch":
      return dispatchById(db, id);
    case "dispatches":
      return listDispatches(db, 50);
    default:
      throw new HttpError(400, `unknown resource: ${resource}`);
  }
}

async function handle(req: Request): Promise<Response> {
  const url = new URL(req.url);
  if (req.method === "GET" && url.pathname === "/health") {
    return json({ ok: true, summary: statusSummary(db) });
  }
  if (!authorized(req)) throw new HttpError(401, "unauthorized");
  if (req.method === "GET" && url.pathname === "/read") {
    return json({ ok: true, data: readResource(url) });
  }
  if (req.method === "POST" && url.pathname === "/route") {
    const body = await readBody(req);
    const triggerId = requiredString(body, "triggerId");
    const decision = requiredString(body, "decision");
    if (decision === "ignore") {
      ignoreTrigger(db, triggerId);
      recordRouterDecision(db, triggerId, decision, stringField(body, "reason") ?? "harness_route", "real");
      return json({ ok: true, decision, triggerId });
    }
    if (decision !== "accept") throw new HttpError(400, "decision must be accept|ignore");
    assertWorkerSlot(db);
    const ticket = acceptTrigger(db, triggerId, stringField(body, "ticketId"));
    recordRouterDecision(db, triggerId, decision, stringField(body, "reason") ?? "harness_route", "real");
    return json({ ok: true, decision, ticket });
  }
  if (req.method === "POST" && url.pathname === "/report") {
    const body = await readBody(req);
    const ticketId = requiredString(body, "ticketId");
    const status = reportStatus(stringField(body, "status"));
    const summary = stringField(body, "summary") ?? "";
    const replyBody = stringField(body, "reply");
    const reply = replyBody
      ? { channel: stringField(body, "channel") ?? "local", address: stringField(body, "address"), body: replyBody }
      : undefined;
    const outcomeRaw = stringField(body, "outcome");
    if (outcomeRaw !== undefined && outcomeRaw !== "succeeded" && outcomeRaw !== "failed") {
      throw new HttpError(400, "outcome must be succeeded|failed");
    }
    const question = stringField(body, "question");
    const ticket = reportTicket(db, ticketId, status, summary, reply, {
      dispatchId: stringField(body, "dispatchId"),
      outcome: outcomeRaw as "succeeded" | "failed" | undefined,
      filesModified: stringArray(body.filesModified),
      tokensUsed: typeof body.tokensUsed === "number" ? body.tokensUsed : 0,
      question: question ? { text: question, options: stringArray(body.options) } : undefined,
      tokenBudget: config.tokenBudgetPerWorker,
    });
    return json({ ok: true, ticket });
  }
  if (req.method === "POST" && url.pathname === "/dispatch/claim") {
    const body = await readBody(req);
    const claim = parseClaim(body);
    if (isError(claim)) throw new HttpError(400, claim.error);
    const next = nextPendingDispatch(db);
    if (!next) return json({ ok: true, dispatch: null });
    if (claim.ticketId && next.ticket.id !== claim.ticketId) return json({ ok: true, dispatch: null });
    assertWorkerSlot(db);
    const lease = claimDispatch(db, next.dispatch.id, claim.workerId, next.ticket.workdir ?? null);
    return json({ ok: true, dispatch: lease });
  }
  if (req.method === "POST" && url.pathname === "/gate/ask") {
    const body = await readBody(req);
    const gateReq = parseGate(body);
    if (isError(gateReq)) throw new HttpError(400, gateReq.error);
    const ticketId = requiredString(body, "ticketId");
    const gate = ask(db, ticketId, gateReq.dispatchId, gateReq.question, gateReq.options ?? []);
    return json({ ok: true, gate });
  }
  if (req.method === "POST" && url.pathname === "/gate/resolve") {
    const body = await readBody(req);
    const gateId = requiredString(body, "gateId");
    const answer = requiredString(body, "answer");
    const gate = resolveGate(db, gateId, answer);
    return json({ ok: true, gate });
  }
  if (req.method === "POST" && url.pathname === "/outbox/enqueue") {
    const body = await readBody(req);
    const outReq = parseOutbox(body);
    if (isError(outReq)) throw new HttpError(400, outReq.error);
    const reply = enqueueReply(db, outReq.channel, outReq.body, outReq.ticketId ?? null);
    return json({ ok: true, reply });
  }
  if (req.method === "POST" && url.pathname === "/reply") {
    const body = await readBody(req);
    enqueueReply(db, {
      ticketId: stringField(body, "ticketId"),
      channel: stringField(body, "channel") ?? "local",
      address: stringField(body, "address"),
      body: requiredString(body, "body"),
    });
    return json({ ok: true });
  }
  throw new HttpError(404, `no route: ${req.method} ${url.pathname}`);
}

const hostname = process.env.SUPERHARNESS_HOST ?? "127.0.0.1";
await reclaimOrphans(db);
const server = Bun.serve({
  hostname,
  port: config.serverPort,
  async fetch(req) {
    try {
      return await handle(req);
    } catch (error) {
      if (error instanceof HttpError) return json({ ok: false, error: error.message }, error.status);
      return json({ ok: false, error: error instanceof Error ? error.message : String(error) }, 500);
    }
  },
});
writeEndpoint({ url: `http://${hostname}:${String(server.port)}`, token });

const shutdown = (): void => {
  removeEndpoint();
  server.stop(true);
  db.close();
  process.exit(0);
};
process.on("SIGINT", shutdown);
process.on("SIGTERM", shutdown);

console.log(`superharness listening on http://${hostname}:${String(server.port)} (db ${defaultDbPath()})`);
