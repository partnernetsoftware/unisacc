/**
 * Decision protocol adapter: superharness speaks the omp-workflow decision
 * vocabulary (dispatch / task / gate / outbox) on top of the existing HTTP
 * surface, without inventing a second set of states.
 *
 * Mapping (one vocabulary, one state machine — see INTERFACE.md §2):
 *   dispatch : POST /route {decision:"accept"|"ignore"}  -> ticket + pending dispatch
 *   task     : POST /dispatch/claim  {workerId}          -> the earliest pending dispatch
 *   gate     : POST /gate/ask | /gate/resolve            -> ticket blocked / running
 *   outbox   : POST /reply {channel, body}               -> pending reply
 *
 * Rules kept from the existing harness (do not duplicate):
 *   - one active dispatch per ticket; `done` exactly once (reportTicket enforces)
 *   - worker slots and spawn budget are checked before any accept/claim
 *   - no shell, no pattern kill: the caller owns the precise pid/pgid
 */

export type Decision = "accept" | "ignore";

export interface RouteRequest {
  triggerId: string;
  decision: Decision;
  reason?: string;
  ticketId?: string;
}

export interface ClaimRequest {
  workerId: string;
  /** optional: only claim dispatches for this ticket (steer / rerun) */
  ticketId?: string;
}

export interface ClaimResult {
  dispatchId: string;
  ticketId: string;
  attempt: number;
  workdir?: string;
}

export interface GateRequest {
  dispatchId: string;
  question: string;
  options?: string[];
}

export interface OutboxRequest {
  channel: string;
  body: string;
  ticketId?: string;
}

/** Parse a harness_route tool argument into a RouteRequest, or a reason it is invalid. */
export function parseRoute(value: unknown): RouteRequest | { error: string } {
  if (!value || typeof value !== "object") return { error: "route must be an object" };
  const v = value as Record<string, unknown>;
  const triggerId = v.triggerId;
  const decision = v.decision;
  if (typeof triggerId !== "string" || !triggerId) return { error: "triggerId is required" };
  if (decision !== "accept" && decision !== "ignore") return { error: "decision must be accept|ignore" };
  const out: RouteRequest = { triggerId, decision };
  if (typeof v.reason === "string") out.reason = v.reason;
  if (typeof v.ticketId === "string") out.ticketId = v.ticketId;
  return out;
}

/**
 * Validate a claim request. A worker may claim at most one dispatch at a time;
 * the caller (dispatch manager) enforces that with the lease table, this only
 * checks the shape so a malformed tool call cannot reach SQLite.
 */
export function parseClaim(value: unknown): ClaimRequest | { error: string } {
  if (!value || typeof value !== "object") return { error: "claim must be an object" };
  const v = value as Record<string, unknown>;
  if (typeof v.workerId !== "string" || !v.workerId) return { error: "workerId is required" };
  const out: ClaimRequest = { workerId: v.workerId };
  if (typeof v.ticketId === "string" && v.ticketId) out.ticketId = v.ticketId;
  return out;
}

/** A gate is a question attached to a running dispatch; it must be non-empty. */
export function parseGate(value: unknown): GateRequest | { error: string } {
  if (!value || typeof value !== "object") return { error: "gate must be an object" };
  const v = value as Record<string, unknown>;
  if (typeof v.dispatchId !== "string" || !v.dispatchId) return { error: "dispatchId is required" };
  if (typeof v.question !== "string" || !v.question.trim()) return { error: "question is required" };
  const out: GateRequest = { dispatchId: v.dispatchId, question: v.question };
  if (Array.isArray(v.options)) out.options = v.options.filter((x): x is string => typeof x === "string");
  return out;
}

/** Outbox entries are queued, never sent directly: the sender owns retries. */
export function parseOutbox(value: unknown): OutboxRequest | { error: string } {
  if (!value || typeof value !== "object") return { error: "reply must be an object" };
  const v = value as Record<string, unknown>;
  if (typeof v.channel !== "string" || !v.channel) return { error: "channel is required" };
  if (typeof v.body !== "string" || !v.body) return { error: "body is required" };
  const out: OutboxRequest = { channel: v.channel, body: v.body };
  if (typeof v.ticketId === "string" && v.ticketId) out.ticketId = v.ticketId;
  return out;
}

export function isError(x: unknown): x is { error: string } {
  return !!x && typeof x === "object" && typeof (x as { error?: unknown }).error === "string";
}
