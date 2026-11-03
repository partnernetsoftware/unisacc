// omp-super2 UI:只做格式化,不依赖终端库;TUI 消费行数组,测试直接断言文本。
// 设计原则:运行中给位置与耗时,结束后给结论与证据;成功不额外抢注意力。
import type { DispatchResult } from "../omp-superagent/core.ts";

export type UiState = "running" | DispatchResult["state"] | "cancelled";

export type UiColor =
  | "accent"
  | "success"
  | "error"
  | "warning"
  | "muted"
  | "dim"
  | "toolTitle"
  | "text";

export interface Style {
  fg(color: UiColor, text: string): string;
  bold(text: string): string;
}

export interface TicketView {
  ticket: string;
  role: string;
  state: UiState;
  started: number;
  ended?: number;
  id?: string;
  record?: string;
  summary?: string;
  err?: string;
  verified?: number;
  files?: number;
  next?: string[];
  acceptance?: string;
}

export interface DispatchDetails {
  ok?: boolean;
  acceptance?: string;
  id?: string;
  ticket?: string;
  role?: string;
  state?: string;
  err?: string;
  report?: {
    summary?: string;
    verified?: unknown[];
    files?: unknown[];
    next?: string[];
  };
  record?: string;
  ms?: number;
  verifiedDropped?: unknown[];
  startedAt?: number;
  uiVersion?: string;
  version?: string;
  tokens?: number | null;
}

const ICONS: Record<UiState, string> = {
  running: "▶",
  done: "✓",
  blocked: "⏸",
  failed: "✗",
  invalid: "!",
  timeout: "⌛",
  error: "✗",
  cancelled: "⊘",
};

const COLORS: Record<UiState, UiColor> = {
  running: "accent",
  done: "success",
  blocked: "warning",
  failed: "error",
  invalid: "error",
  timeout: "error",
  error: "error",
  cancelled: "muted",
};

export function iconFor(state: UiState): string {
  return ICONS[state] ?? "?";
}

export function colorFor(state: UiState): UiColor {
  return COLORS[state] ?? "muted";
}

export function formatDuration(ms: number): string {
  const total = Math.max(0, Math.floor(ms / 1000));
  const h = Math.floor(total / 3600);
  const m = Math.floor((total % 3600) / 60);
  const s = total % 60;
  const pad = (n: number) => String(n).padStart(2, "0");
  return h > 0 ? `${h}:${pad(m)}:${pad(s)}` : `${pad(m)}:${pad(s)}`;
}

function clip(text: string | undefined, max: number): string {
  if (!text) return "";
  const one = text.replace(/\s+/g, " ").trim();
  return one.length <= max ? one : `${one.slice(0, Math.max(0, max - 1))}…`;
}

export function viewFromResult(r: DispatchResult): TicketView {
  return {
    ticket: r.ticket,
    role: r.role,
    state: r.state,
    started: Date.now() - r.ms,
    ended: Date.now(),
    id: r.id,
    record: r.record,
    summary: r.report?.summary,
    err: r.err,
    verified: r.report?.verified.length ?? 0,
    files: r.report?.files.length ?? 0,
    next: r.report?.next,
    acceptance: r.acceptance ? (r.acceptance.ok ? "宿主检查通过" : r.acceptance.err) : undefined,
  };
}

export function viewFromDetails(d: DispatchDetails | undefined): TicketView | undefined {
  if (!d?.ticket) return undefined;
  const state = (d.state ?? "running") as UiState;
  const ended = state === "running" ? undefined : Date.now();
  return {
    ticket: d.ticket,
    role: d.role ?? "implementer",
    state,
    started: d.startedAt ?? Date.now() - (d.ms ?? 0),
    ended,
    id: d.id,
    record: d.record,
    summary: d.report?.summary,
    err: d.err,
    verified: d.report?.verified?.length ?? 0,
    files: d.report?.files?.length ?? 0,
    next: d.report?.next,
    acceptance: d.acceptance,
  };
}

export function statusText(views: TicketView[], now: number, theme: Style): string | undefined {
  if (views.length === 0) return undefined;
  const running = views.filter((v) => v.state === "running");
  const done = views.filter((v) => v.state === "done").length;
  const lead = running[0];
  let out = theme.fg("accent", "sa2");
  if (lead) {
    out += theme.fg("muted", `  ${running.length} run`);
    out += `  ${theme.fg("accent", `${iconFor("running")} ${clip(lead.ticket, 18)} ${formatDuration(now - lead.started)}`)}`;
  } else {
    out += theme.fg("muted", `  ${iconFor("done")} ${done} done`);
  }
  return out;
}

export function boardLines(views: TicketView[], now: number, theme: Style): string[] {
  if (views.length === 0) return [];
  const running = views.filter((v) => v.state === "running");
  const finished = views.filter((v) => v.state !== "running").slice(-5).reverse();
  const ordered = [...running, ...finished];
  const done = views.filter((v) => v.state === "done").length;
  const lines = [
    `${theme.fg("toolTitle", theme.bold("omp-super2"))}${theme.fg("muted", `  ${running.length} running · ${done} done`)}`,
  ];
  for (const v of ordered.slice(0, 7)) {
    const ticket = clip(v.ticket, 24).padEnd(24);
    const role = clip(v.role, 11).padEnd(11);
    const dur = formatDuration((v.ended ?? now) - v.started).padStart(5);
    const state = v.state.padEnd(8);
    const color = colorFor(v.state);
    lines.push(
      ` ${theme.fg(color, iconFor(v.state))} ${theme.fg("text", ticket)} ` +
        `${theme.fg("dim", role)} ${theme.fg("muted", dur)} ${theme.fg(color, state)}`,
    );
  }
  if (ordered.length > 7) lines.push(theme.fg("dim", `  … +${ordered.length - 7}`));
  return lines;
}

export function detailLines(view: TicketView, now: number, theme: Style): string[] {
  const color = colorFor(view.state);
  const dur = formatDuration((view.ended ?? now) - view.started);
  const lines = [
    `${theme.fg("toolTitle", theme.bold("omp-super2"))}  ${theme.fg(color, `${iconFor(view.state)} ${view.ticket}`)}`,
    `${theme.fg("muted", "state")}    ${theme.fg(color, view.state)}  ${theme.fg("muted", dur)}`,
    `${theme.fg("muted", "role")}     ${view.role}`,
  ];
  if (view.id) lines.push(`${theme.fg("muted", "id")}       ${view.id}`);
  lines.push(`${theme.fg("muted", "evidence")} verified ${view.verified ?? 0} · files ${view.files ?? 0}`);
  if (view.summary) lines.push(`${theme.fg("muted", "summary")}  ${clip(view.summary, 100)}`);
  if (view.err) lines.push(`${theme.fg("error", "error")}    ${clip(view.err, 120)}`);
  if (view.acceptance) lines.push(`${theme.fg("muted", "checks")}   ${clip(view.acceptance, 100)}`);
  if (view.next?.length) lines.push(`${theme.fg("muted", "next")}     ${clip(view.next.join(" · "), 120)}`);
  if (view.record) lines.push(`${theme.fg("dim", view.record)}`);
  return lines;
}

export function notificationText(view: TicketView): string | undefined {
  if (view.state === "running" || view.state === "done") return undefined;
  const dur = formatDuration((view.ended ?? Date.now()) - view.started);
  if (view.state === "blocked") return `⏸ ${view.ticket} 需要人:${clip(view.err || view.summary, 80)}`;
  if (view.state === "timeout") return `⌛ ${view.ticket} 超时 · ${dur}`;
  if (view.state === "cancelled") return `⊘ ${view.ticket} 已取消`;
  return `✗ ${view.ticket} ${view.state} · ${dur}${view.err ? ` · ${clip(view.err, 80)}` : ""}`;
}

export function callLine(args: { ticket?: string; role?: string }, theme: Style): string {
  const ticket = args.ticket ?? "?";
  const role = args.role ?? "implementer";
  return (
    `${theme.fg("toolTitle", theme.bold("dispatch"))} ` +
    `${theme.fg("accent", ticket)}${theme.fg("muted", ` · ${role}`)}`
  );
}

export function resultLines(
  d: DispatchDetails | undefined,
  expanded: boolean,
  theme: Style,
): string[] {
  const view = viewFromDetails(d);
  if (!view) return [theme.fg("muted", "dispatch 无结果")];
  const color = colorFor(view.state);
  const dur = formatDuration((view.ended ?? Date.now()) - view.started);
  const head =
    `${theme.fg(color, iconFor(view.state))} ` +
    `${theme.fg("text", view.ticket)} ${theme.fg("muted", `· ${view.role} · ${dur}`)}`;
  if (view.state === "running") return [head, theme.fg("muted", "运行中…")];
  const lines = [
    head,
    `${theme.fg("muted", "evidence")} verified ${view.verified ?? 0} · files ${view.files ?? 0}`,
  ];
  if (view.err) lines.push(theme.fg("error", clip(view.err, 120)));
  if (expanded) {
    if (view.summary) lines.push(clip(view.summary, 100));
    if (view.acceptance) lines.push(theme.fg("muted", `checks: ${clip(view.acceptance, 100)}`));
    if (view.next?.length) lines.push(theme.fg("muted", `next: ${clip(view.next.join(" · "), 120)}`));
    if (view.record) lines.push(theme.fg("dim", view.record));
  } else {
    lines.push(theme.fg("dim", "展开看 summary / next / record"));
  }
  return lines;
}
