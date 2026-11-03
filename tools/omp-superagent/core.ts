// 核心:角色模板 + 子 omp 调用(spawn/预算/落盘)+ dispatch 记录。
// 与 omp-workflow 的 core.ts 同构,但不 import(两仓独立演化)。
import { spawn } from "node:child_process";
import { mkdirSync, writeFileSync } from "node:fs";
import { join } from "node:path";
import { checkFiles, parseReport, reconcileVerified, type Report } from "./report.ts";
import { SA_VERSION } from "./version.ts";

export type Role = "implementer" | "reviewer" | "scout";

export interface DispatchOpts {
  ticket: string;
  task: string;
  role: Role;
  model?: string;
  tools?: string;
  paths?: string[];
  maxTime?: number; // 秒
  budget?: { maxSteps?: number; maxChars?: number };
  cwd?: string;
  signal?: AbortSignal;
}

export interface DispatchResult {
  id: string;
  ticket: string;
  role: Role;
  model: string;
  state: "done" | "blocked" | "failed" | "invalid" | "timeout" | "error";
  report?: Report;
  err?: string;
  record?: string;
  ms: number;
  tokens: number;
  version: string;
  verifiedDropped?: string[];
}

const DEFAULT_TOOLS: Record<Role, string> = {
  scout: "read,grep,glob",
  reviewer: "read,grep,glob,bash",
  implementer: "read,grep,glob,bash,edit,write",
};

const ROLE_BRIEF: Record<Role, string> = {
  scout: "只读检索:定位事实与落点。禁止改文件,禁止跑会写盘的命令。",
  reviewer: "评审给定补丁/改动:必须实际执行验证命令;只读,不得改文件。",
  implementer: "按任务书实现:只允许改 paths 内文件;每次改动后跑验收命令;不接受未验证的完成。",
};

export const REPORT_PROTOCOL =
  "【report 协议】本轮唯一产出是一个 JSON 对象,不得有任何其它文本/思考块。\n" +
  '{"t":"report","ticket":"<dispatch 给的 ticket>","state":"done|blocked|failed","summary":"一句",' +
  '"verified":["实际跑过的命令/读过的 path:line 及关键输出(逐字)"],"inferred":["未验证推断"],' +
  '"files":[{"path":"..","change":".."}],"blocked":"阻塞原因(无则空)","next":["可执行下一步"],' +
  '"tokens":0,"ms":0}\n' +
  "state=done 时 verified 不得为空;非 implementer 角色 files 必须为空;JSON 之外任何文字都会作废本次 dispatch。\n" +
  "先做最小可行的一步并用命令证明,再扩展;预算用尽前必须给出 report。";

export function buildPrompt(o: DispatchOpts): string {
  const lines = [
    `ROLE: ${o.role} — ${ROLE_BRIEF[o.role]}`,
    `TICKET: ${o.ticket}`,
    `TASK: ${o.task}`,
  ];
  if (o.role === "implementer") lines.push(`ALLOWED_PATHS: ${(o.paths ?? []).join(", ") || "(未给,禁止写)"}`);
  lines.push(REPORT_PROTOCOL);
  return lines.join("\n\n");
}

// 从子 omp 的流式输出里抽全部文本。
export function collectText(chunks: string[]): string {
  return chunks.join("\n");
}

// 证据 = 去掉 report JSON 段之后的剩余文本。report 自身不能作为自己 verified 的证据。
export function stripReportJson(text: string): string {
  const a = text.indexOf("{");
  const b = text.lastIndexOf("}");
  if (a < 0 || b < a) return text;
  return (text.slice(0, a) + "\n" + text.slice(b + 1)).trim();
}

function nowId(): string {
  const t = new Date().toISOString().replace(/[-:TZ.]/g, "").slice(0, 14);
  const r = Math.floor(Math.random() * 1e6).toString(36).padStart(4, "0");
  return `${t}-${r}`;
}

export function recordPath(id: string, cwd: string): string {
  return join(cwd, "out", "superagent", `${id}.json`);
}

function record(d: DispatchResult, cwd: string): void {
  const dir = join(cwd, "out", "superagent");
  mkdirSync(dir, { recursive: true });
  const p = recordPath(d.id, cwd);
  writeFileSync(p, JSON.stringify(d, null, 2));
  d.record = p;
}

// 起一个子 omp 执行 ticket;返回机器校验后的结果。
export async function runDispatch(o: DispatchOpts): Promise<DispatchResult> {
  const cwd = o.cwd ?? process.cwd();
  const model = o.model ?? process.env.SA_MODEL ?? "";
  const tools = o.tools ?? DEFAULT_TOOLS[o.role];
  const maxTime = o.maxTime ?? 300;
  const id = nowId();
  const started = Date.now();
  const base: DispatchResult = {
    id,
    ticket: o.ticket,
    role: o.role,
    model,
    state: "error",
    ms: 0,
    tokens: 0,
    version: SA_VERSION,
  };

  if (o.role === "implementer" && (!o.paths || o.paths.length === 0)) {
    return { ...base, err: "implementer 必须给出允许写入的 paths" };
  }

  const args = [
    "--mode",
    "rpc",
    "--no-session",
    "--tools",
    tools,
    "--max-time",
    String(maxTime),
    "-e",
    join(new URL(".", import.meta.url).pathname, "extension.ts"),
  ];
  if (model) args.push("--model", model);

  const chunks: string[] = [];
  const child = spawn(process.env.SA_OMP_BIN ?? "omp", args, {
    cwd,
    stdio: ["pipe", "pipe", "pipe"],
    detached: true,
  });
  let timedOut = false;
  const timer = setTimeout(() => {
    timedOut = true;
    try {
      process.kill(-child.pid!, "SIGKILL");
    } catch {
      child.kill("SIGKILL");
    }
  }, maxTime * 1000 + 5000);

  child.stdout?.on("data", (b: Buffer) => chunks.push(b.toString()));
  child.stderr?.on("data", (b: Buffer) => chunks.push(b.toString()));
  child.stdin?.on("error", () => {});
  child.stdin?.write(JSON.stringify({ type: "prompt", text: buildPrompt(o) }) + "\n");
  child.stdin?.end();

  const code: number = await new Promise((res) => child.on("close", (c) => res(c ?? -1)));
  clearTimeout(timer);
  const ms = Date.now() - started;
  const text = collectText(chunks);
  // 证据只能是 report JSON 之外的内容:report 自身不算证据,否则任何字符串都能自证。
  const evidence = stripReportJson(text);

  if (timedOut) {
    const r: DispatchResult = { ...base, state: "timeout", ms, err: `超过 ${maxTime}s 被终止` };
    record(r, cwd);
    return r;
  }

  const parsed = parseReport(text);
  if (!parsed.ok || !parsed.report) {
    const r: DispatchResult = { ...base, state: "invalid", ms, err: parsed.err ?? "report 校验失败" };
    record(r, cwd);
    return r;
  }

  const rec = reconcileVerified(parsed.report, evidence);
  if (!rec.ok || !rec.report) {
    const r: DispatchResult = {
      ...base,
      state: "invalid",
      ms,
      err: rec.err,
      report: rec.report,
      verifiedDropped: rec.droppedVerified,
    };
    record(r, cwd);
    return r;
  }
  const files = checkFiles(o.role, o.paths, rec.report);
  if (!files.ok) {
    const r: DispatchResult = { ...base, state: "invalid", ms, err: files.err, report: rec.report };
    record(r, cwd);
    return r;
  }

  const r: DispatchResult = {
    ...base,
    state: rec.report.state,
    ms,
    tokens: rec.report.tokens,
    report: rec.report,
    verifiedDropped: rec.droppedVerified,
    err: code !== 0 ? `子进程退出码 ${code}` : undefined,
  };
  record(r, cwd);
  return r;
}
