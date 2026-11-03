// report 契约:schema + 机器校验。
// 硬性规则:done 且 verified 空 → INVALID;非 implementer 写 files → INVALID;verified 逐字核对不上 → 降级 inferred。
import { lstatSync, realpathSync } from "node:fs";
import { isAbsolute, relative, resolve, sep, dirname, basename } from "node:path";

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
export function reconcileVerified(report: Report, evidence: string | string[]): CheckResult {
  const kept: string[] = [];
  const dropped: string[] = [];
  for (const v of report.verified) {
    if (v.trim().length > 0 && (Array.isArray(evidence) ? evidence.some((item) => item.includes(v)) : evidence.includes(v))) kept.push(v);
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

// 将已有祖先解析为真实路径，避免 ..、绝对路径及符号链接绕过边界。
function canonical(path: string, cwd: string): string {
  if (!path || path.includes("\0")) throw new Error("路径为空或含 NUL");
  let current = resolve(cwd, path);
  const suffix: string[] = [];
  for (;;) {
    try {
      return resolve(realpathSync(current), ...suffix);
    } catch (e: unknown) {
      if (!(e instanceof Error) || !("code" in e) || e.code !== "ENOENT") throw e;
      // 悬空链接不可按普通的尚不存在路径处理。
      try {
        lstatSync(current);
        throw new Error("路径包含悬空符号链接");
      } catch (statError: unknown) {
        if (!(statError instanceof Error) || !("code" in statError) || statError.code !== "ENOENT") throw statError;
      }
      const parent = dirname(current);
      if (parent === current) throw e;
      suffix.unshift(basename(current));
      current = parent;
    }
  }
}

function contains(parent: string, child: string): boolean {
  const rel = relative(parent, child);
  return rel === "" || (!isAbsolute(rel) && rel !== ".." && !rel.startsWith(`..${sep}`));
}

export function checkFiles(role: string, paths: string[] | undefined, report: Report, cwd = process.cwd()): CheckResult {
  if (role !== "implementer") {
    if (report.files.length > 0) return { ok: false, err: `${role} 角色不许声明改动文件` };
    return { ok: true, report };
  }
  if (!paths || paths.length === 0) return { ok: false, err: "implementer 必须给出允许写入的 paths" };
  try {
    const root = canonical(".", cwd);
    const allowed = paths.map((p) => canonical(p, cwd));
    if (allowed.some((p) => !contains(root, p))) return { ok: false, err: "允许路径超出工作目录" };
    for (const f of report.files) {
      const target = canonical(f.path, cwd);
      if (!contains(root, target) || !allowed.some((p) => contains(p, target)))
        return { ok: false, err: `越界写入: ${f.path}` };
    }
  } catch (e) {
    return { ok: false, err: `路径无法安全解析: ${String(e)}` };
  }
  return { ok: true, report };
}

// 解析失败时保守判冲突，不能将未知路径当作互不重叠。
export function pathsConflict(a: string[], b: string[], cwd = process.cwd()): string | undefined {
  try {
    for (const x of a) for (const y of b) {
      const cx = canonical(x, cwd);
      const cy = canonical(y, cwd);
      if (contains(cx, cy) || contains(cy, cx)) return `${x} ↔ ${y}`;
    }
  } catch {
    return "路径无法安全解析";
  }
  return undefined;
}
