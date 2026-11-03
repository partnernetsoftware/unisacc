// report 契约:schema + 机器校验。
// 硬性规则:done 且 verified 空 → INVALID;非 implementer 写 files → INVALID;verified 逐字核对不上 → 降级 inferred。
export const REPORT_STATES = ["done", "blocked", "failed"] as const;
export type ReportState = (typeof REPORT_STATES)[number];

export interface ReportFile {
  path: string;
  change: string;
}
export interface Report {
  t: "report";
  ticket: string;
  state: ReportState;
  summary: string;
  verified: string[];
  inferred: string[];
  files: ReportFile[];
  blocked: string;
  next: string[];
  tokens: number;
  ms: number;
}

export interface CheckResult {
  ok: boolean;
  err?: string;
  report?: Report;
  droppedVerified?: string[];
}

const MAX_SUMMARY = 400;
const MAX_ITEM = 600;

function s(v: unknown, max = MAX_ITEM): boolean {
  return typeof v === "string" && v.length > 0 && v.length <= max;
}
function sarr(v: unknown, maxItems = 20): v is string[] {
  return Array.isArray(v) && v.every((x) => typeof x === "string" && x.length <= MAX_ITEM) && v.length <= maxItems;
}

export function extractJson(text: string): unknown {
  const a = text.indexOf("{");
  const b = text.lastIndexOf("}");
  if (a < 0 || b < a) throw new Error("没有 JSON 对象");
  const outside = (text.slice(0, a) + text.slice(b + 1)).replace(/```(?:json)?/g, "").trim();
  if (outside) throw new Error("JSON 之外有文字(report 不允许自有文本)");
  return JSON.parse(text.slice(a, b + 1));
}

export function parseReport(text: string): CheckResult {
  let raw: any;
  try {
    raw = extractJson(text);
  } catch (e: any) {
    return { ok: false, err: e.message };
  }
  if (typeof raw !== "object" || raw === null || Array.isArray(raw))
    return { ok: false, err: "不是 JSON 对象" };
  if (raw.t !== "report") return { ok: false, err: "t 必须是 report" };
  if (!s(raw.ticket, 120)) return { ok: false, err: "ticket 缺失或过长" };
  if (!(REPORT_STATES as readonly string[]).includes(raw.state))
    return { ok: false, err: `state 必须是 ${REPORT_STATES.join("|")}` };
  if (!s(raw.summary, MAX_SUMMARY)) return { ok: false, err: "summary 缺失或过长" };
  if (!sarr(raw.verified)) return { ok: false, err: "verified 必须是字符串数组" };
  if (!sarr(raw.inferred)) return { ok: false, err: "inferred 必须是字符串数组" };
  if (!Array.isArray(raw.files)) return { ok: false, err: "files 必须是数组" };
  for (const f of raw.files) {
    if (typeof f !== "object" || f === null || !s(f.path, 400) || typeof f.change !== "string")
      return { ok: false, err: "files 元素须为 {path, change}" };
  }
  if (typeof raw.blocked !== "string") return { ok: false, err: "blocked 必须是字符串" };
  if (!sarr(raw.next)) return { ok: false, err: "next 必须是字符串数组" };
  if (raw.state === "done" && (raw.verified as string[]).length === 0)
    return { ok: false, err: "state=done 但 verified 为空(不许空口完成)" };
  if (raw.state === "blocked" && !s(raw.blocked, MAX_ITEM))
    return { ok: false, err: "state=blocked 必须给出 blocked 原因" };
  return {
    ok: true,
    report: {
      t: "report",
      ticket: raw.ticket,
      state: raw.state,
      summary: raw.summary,
      verified: raw.verified,
      inferred: raw.inferred,
      files: raw.files,
      blocked: raw.blocked,
      next: raw.next,
      tokens: Number.isFinite(raw.tokens) ? Number(raw.tokens) : 0,
      ms: Number.isFinite(raw.ms) ? Number(raw.ms) : 0,
    },
  };
}

// 逐字核对:evidence 是本次子会话里可取到的工具轨迹/输出拼接体。
// 命中 → 保留;未命中 → 从 verified 移到 inferred,并返回被降级项。
export function reconcileVerified(report: Report, evidence: string): CheckResult {
  const kept: string[] = [];
  const dropped: string[] = [];
  for (const v of report.verified) {
    if (v.length > 0 && evidence.includes(v)) kept.push(v);
    else dropped.push(v);
  }
  if (dropped.length === 0) return { ok: true, report, droppedVerified: [] };
  const next: Report = {
    ...report,
    verified: kept,
    inferred: [...report.inferred, ...dropped.map((d) => `[未核到] ${d}`)],
  };
  if (next.state === "done" && kept.length === 0)
    return { ok: false, err: "verified 全部无法逐字核对,且 state=done", report: next, droppedVerified: dropped };
  return { ok: true, report: next, droppedVerified: dropped };
}

// 角色与路径授权:非 implementer 不许声明改动文件;implementer 的每个 path 必须落在允许前缀内。
export function checkFiles(role: string, paths: string[] | undefined, report: Report): CheckResult {
  if (role !== "implementer") {
    if (report.files.length > 0) return { ok: false, err: `${role} 角色不许声明改动文件` };
    return { ok: true, report };
  }
  if (!paths || paths.length === 0) return { ok: false, err: "implementer 必须给出允许写入的 paths" };
  for (const f of report.files) {
    if (!paths.some((p) => f.path === p || f.path.startsWith(p.endsWith("/") ? p : p + "/")))
      return { ok: false, err: `越界写入: ${f.path}` };
  }
  return { ok: true, report };
}

// 并发冲突:两组允许路径前缀是否有重叠(用于同会话多 dispatch 的写冲突域)。
export function pathsConflict(a: string[], b: string[]): string | undefined {
  const norm = (p: string) => (p.endsWith("/") ? p : p + "/");
  for (const x of a) {
    for (const y of b) {
      if (x === y || norm(x).startsWith(norm(y)) || norm(y).startsWith(norm(x))) return `${x} ↔ ${y}`;
    }
  }
  return undefined;
}
