/** Minimal Super-agent harness domain types. */

export type TriggerDisposition = "new" | "accepted" | "ignored" | "stopped";

export type TicketStatus = "open" | "running" | "ready" | "blocked" | "failed" | "ended";

export type WorkerState = "pending" | "starting" | "running" | "stalled" | "stopped" | "exited";

export type ReportStatus = "progress" | "ready" | "blocked" | "failed" | "waiting";

export interface TriggerInput {
  source: string;
  threadKey: string;
  text: string;
  actor?: string;
  dedupeKey?: string;
  at?: number;
}

export interface TriggerRow {
  id: string;
  source: string;
  thread_key: string;
  actor: string | null;
  text: string;
  at: number;
  disposition: TriggerDisposition;
  dedupe_key: string | null;
}

export interface ThreadRow {
  key: string;
  source: string;
  label: string | null;
  last_trigger_at: number | null;
  summary: string | null;
}

export interface TicketRow {
  id: string;
  thread_key: string;
  trigger_id: string;
  status: TicketStatus;
  worker_pid: number | null;
  worker_pgid: number | null;
  workdir: string | null;
  created_at: number;
  updated_at: number;
  summary: string | null;
}

export interface WorkerRow {
  id: string;
  ticket_id: string;
  pid: number | null;
  pgid: number | null;
  session_dir: string | null;
  workdir: string | null;
  started_at: number | null;
  last_heartbeat_at: number | null;
  state: WorkerState;
}

export interface OutboxRow {
  id: number;
  ticket_id: string | null;
  channel: string;
  address: string | null;
  body: string;
  status: "pending" | "sent" | "dropped";
  kind: string | null;
  attempts: number;
  last_error: string | null;
  created_at: number;
  sent_at: number | null;
}

export type DispatchState = "pending" | "running" | "completed" | "failed" | "stopped";

export interface DispatchRow {
  id: string;
  ticket_id: string;
  attempt: number;
  state: DispatchState;
  pid: number | null;
  pgid: number | null;
  started_at: number | null;
  ended_at: number | null;
  heartbeat_at: number | null;
  report_status: ReportStatus | null;
  report_summary: string | null;
  tokens_used: number;
}

export type GateState = "open" | "answered";

export interface GateRow {
  id: string;
  ticket_id: string;
  dispatch_id: string;
  question: string;
  options: string;
  answer: string | null;
  state: GateState;
  created_at: number;
  resolved_at: number | null;
}

export interface LedgerRow {
  id: string;
  ticket_id: string;
  dispatch_id: string | null;
  pid: number;
  pgid: number;
  start_ticks: string;
  command: string;
  state: "running" | "exited" | "killed" | "dry-run";
  started_at: number;
  exited_at: number | null;
  threads_before?: number | null;
  threads_after?: number | null;
}

export type RouterTurnState = "queued" | "running" | "decided" | "timeout" | "failed";

export interface RouterTurnRow {
  id: string;
  session_id: string;
  trigger_id: string;
  state: RouterTurnState;
  started_at: number;
  ended_at: number | null;
  tokens: number;
  error: string | null;
}

export interface ReportOptions {
  dispatchId?: string;
  outcome?: "succeeded" | "failed";
  filesModified?: string[];
  tokensUsed?: number;
  question?: { text: string; options?: string[] };
}
