import type { Database } from "bun:sqlite";
import {
  acceptTrigger,
  createRouterTurn,
  ignoreTrigger,
  markRouterTurn,
  newTriggers,
  recordRouterDecision,
  routerDecisionByTrigger,
  routerTokensSince,
  routerTurnByTrigger,
} from "./db";
import { assertWorkerSlot } from "./guards";
import { loadConfig, type HarnessConfig } from "./config";
import { RouterTimeoutError, type RouterRpc } from "./router-rpc";
import type { TriggerRow } from "./types";

export interface RouterDecision {
  triggerId: string;
  decision: "accept" | "ignore";
  reason: string;
}

export interface RouterDryRunResult {
  mode: "dry-run";
  accepted: number;
  ignored: number;
  deferred: number;
  decisions: RouterDecision[];
}

export interface RouterTurnResult {
  triggerId: string;
  state: "decided" | "timeout" | "failed";
  decision?: "accept" | "ignore";
  tokens: number;
  error?: string;
}

export interface RouterPassResult {
  mode: "real";
  accepted: number;
  ignored: number;
  deferred: number;
  failed: number;
  tokensUsed: number;
  stopped: null | "timeout" | "malformed" | "turn-token-budget" | "hourly-token-budget" | "worker-slot" | "error";
  turns: RouterTurnResult[];
}

export function buildRouterPrompt(trigger: TriggerRow): string {
  return [
    "You are the superharness Router. You do not execute work.",
    "Read the new trigger, then call harness_route with trigger_id and decision accept|ignore.",
    "Accept only when the trigger asks for work; ignore noise. Do not call harness_report; workers do that.",
    'If harness_route is unavailable, reply exactly {"decision":"accept|ignore","reason":"..."}.',
    "",
    `trigger_id: ${trigger.id}`,
    `thread: ${trigger.thread_key}`,
    `source: ${trigger.source}`,
    `actor: ${trigger.actor ?? "-"}`,
    `text: ${trigger.text}`,
  ].join("\n");
}

function decideDryRun(trigger: TriggerRow): { decision: "accept" | "ignore"; reason: string } {
  if (/(^|\s)(ignore|noop|skip|忽略|不需要)(\s|$)/i.test(trigger.text)) {
    return { decision: "ignore", reason: "dry-run keyword" };
  }
  return { decision: "accept", reason: "dry-run default accept" };
}

/**
 * D2 dry-run router: no omp process. It persists each decision, then uses the
 * same accept/ignore transitions a real Router will call through harness_route.
 * If the worker cap is full, the remaining triggers stay new and are reported
 * as deferred.
 */
export function runRouterDryRun(db: Database): RouterDryRunResult {
  const result: RouterDryRunResult = { mode: "dry-run", accepted: 0, ignored: 0, deferred: 0, decisions: [] };
  for (const trigger of newTriggers(db, 20)) {
    const decision = decideDryRun(trigger);
    if (decision.decision === "accept") {
      try {
        assertWorkerSlot(db);
      } catch {
        result.deferred++;
        continue;
      }
      acceptTrigger(db, trigger.id);
      result.accepted++;
    } else {
      ignoreTrigger(db, trigger.id);
      result.ignored++;
    }
    recordRouterDecision(db, trigger.id, decision.decision, decision.reason, "dry-run");
    result.decisions.push({ triggerId: trigger.id, decision: decision.decision, reason: decision.reason });
  }
  return result;
}

function withTimeout<T>(promise: Promise<T>, timeoutMs: number): Promise<T> {
  let timer: ReturnType<typeof setTimeout> | undefined;
  const timeout = new Promise<never>((_resolve, reject) => {
    timer = setTimeout(() => reject(new RouterTimeoutError(timeoutMs)), timeoutMs);
  });
  return Promise.race([promise, timeout]).finally(() => {
    clearTimeout(timer);
  });
}

function parseDecision(replyText: string): { decision: "accept" | "ignore"; reason: string } {
  const trimmed = replyText.trim();
  const fenced = trimmed.match(/```(?:json)?\s*([\s\S]*?)```/i);
  const candidate = fenced?.[1]?.trim() ?? trimmed;
  const start = candidate.indexOf("{");
  const end = candidate.lastIndexOf("}");
  let value: unknown;
  try {
    if (start < 0 || end <= start) throw new Error("no object");
    value = JSON.parse(candidate.slice(start, end + 1));
  } catch {
    throw new Error(`router reply is not JSON: ${trimmed.slice(0, 300)}`);
  }
  if (!value || typeof value !== "object") throw new Error("router reply is not an object");
  const record = value as Record<string, unknown>;
  const decision = record.decision;
  if (decision !== "accept" && decision !== "ignore") throw new Error("router decision must be accept|ignore");
  return { decision, reason: typeof record.reason === "string" ? record.reason : "router reply" };
}

/**
 * D3a: one bounded real-router pass over new triggers using an injected RPC.
 * It never spawns a process itself. Every turn is persisted before and after
 * the call, so a crash leaves an auditable row instead of a silent duplicate.
 */
export async function runRouterPass(
  db: Database,
  rpc: RouterRpc,
  sessionId: string,
  config: HarnessConfig = loadConfig(),
): Promise<RouterPassResult> {
  const result: RouterPassResult = {
    mode: "real",
    accepted: 0,
    ignored: 0,
    deferred: 0,
    failed: 0,
    tokensUsed: 0,
    stopped: null,
    turns: [],
  };
  const hourStart = Date.now() - 3_600_000;
  for (const trigger of newTriggers(db, 20)) {
    if (routerTurnByTrigger(db, trigger.id)) continue;
    const usedThisHour = routerTokensSince(db, hourStart);
    if (usedThisHour >= config.routerHourlyTokenBudget) {
      result.stopped = "hourly-token-budget";
      break;
    }
    try {
      assertWorkerSlot(db);
    } catch {
      result.deferred++;
      result.stopped = "worker-slot";
      break;
    }
    const turn = createRouterTurn(db, sessionId, trigger.id);
    try {
      await rpc.newSession();
      const reply = await withTimeout(
        rpc.prompt(buildRouterPrompt(trigger), {
          timeoutMs: config.routerTurnTimeoutMs,
          maxTokens: config.routerTurnTokenBudget,
        }),
        config.routerTurnTimeoutMs,
      );
      const tokens = Number.isFinite(reply.tokensUsed) ? Math.max(0, reply.tokensUsed) : 0;
      result.tokensUsed += tokens;
      if (tokens > config.routerTurnTokenBudget) {
        markRouterTurn(db, turn.id, "failed", tokens, "turn-token-budget");
        result.failed++;
        result.turns.push({ triggerId: trigger.id, state: "failed", tokens, error: "turn-token-budget" });
        result.stopped = "turn-token-budget";
        break;
      }
      if (usedThisHour + tokens > config.routerHourlyTokenBudget) {
        markRouterTurn(db, turn.id, "failed", tokens, "hourly-token-budget");
        result.failed++;
        result.turns.push({ triggerId: trigger.id, state: "failed", tokens, error: "hourly-token-budget" });
        result.stopped = "hourly-token-budget";
        break;
      }
      const existing = routerDecisionByTrigger(db, trigger.id);
      let decision: "accept" | "ignore";
      let reason: string;
      if (existing) {
        if (existing.decision !== "accept" && existing.decision !== "ignore") {
          throw new Error(`invalid persisted router decision: ${existing.decision}`);
        }
        decision = existing.decision;
        reason = existing.reason;
      } else {
        const parsed = parseDecision(reply.text);
        decision = parsed.decision;
        reason = parsed.reason;
      }
      if (!existing) {
        if (decision === "accept") {
          assertWorkerSlot(db);
          acceptTrigger(db, trigger.id);
          result.accepted++;
        } else {
          ignoreTrigger(db, trigger.id);
          result.ignored++;
        }
        recordRouterDecision(db, trigger.id, decision, reason, "real");
      } else if (decision === "accept") {
        result.accepted++;
      } else {
        result.ignored++;
      }
      markRouterTurn(db, turn.id, "decided", tokens);
      result.turns.push({ triggerId: trigger.id, state: "decided", decision, tokens });
    } catch (error) {
      if (error instanceof RouterTimeoutError) {
        markRouterTurn(db, turn.id, "timeout", 0, error.message);
        result.failed++;
        result.turns.push({ triggerId: trigger.id, state: "timeout", tokens: 0, error: error.message });
        result.stopped = "timeout";
        break;
      }
      const message = error instanceof Error ? error.message : String(error);
      markRouterTurn(db, turn.id, "failed", 0, message);
      result.failed++;
      result.turns.push({ triggerId: trigger.id, state: "failed", tokens: 0, error: message });
      result.stopped = message.includes("JSON") || message.includes("decision") ? "malformed" : "error";
      break;
    }
  }
  return result;
}
