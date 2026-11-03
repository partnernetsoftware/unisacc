// 核心:角色模板 + 子 omp 调用(spawn/预算/落盘)+ dispatch 记录。
// 与 omp-workflow 的 core.ts 同构,但不 import(两仓独立演化)。
import { spawn } from "node:child_process";
import { mkdirSync, writeFileSync } from "node:fs";
import { join } from "node:path";
import { randomUUID } from "node:crypto";
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
  trace?: { stdout: string; stderr: string };
  verifiedDropped?: string[];
}

const DEFAULT_TOOLS: Record<Role, string> = {
  scout: "read,grep,glob",
  reviewer: "read,grep,glob",
  implementer: "",
};

const ROLE_BRIEF: Record<Role, string> = {
  scout: "只读检索:定位事实与落点。禁止改文件,禁止跑会写盘的命令。",
  reviewer: "评审给定补丁/改动:只读,不得改文件。",
  implementer: "写入能力当前不可安全约束,禁止执行。",
};

export const REPORT_PROTOCOL =
  "【report 协议】本轮唯一产出是一个 JSON 对象,不得有任何其它文本/思考块。\n" +
  '{"t":"report","ticket":"<dispatch 给的 ticket>","state":"done|blocked|failed","summary":"一句",' +
  '"verified":["实际跑过的命令/读过的 path:line 及关键输出(逐字)"],"inferred":["未验证推断"],' +
  '"files":[{"path":"..","change":".."}],"blocked":"阻塞原因(无则空)","next":["可执行下一步"],' +
  '"tokens":0,"ms":0}\n' +
  "state=done 时 verified 不得为空;非 implementer 角色 files 必须为空;JSON 之外任何文字都会作废本次 dispatch。\n" +
  "verified 每项必须是本次成功工具返回文本中逐字出现的非空片段；不得引用助手消息或 stderr。";

export function buildPrompt(o: DispatchOpts): string {
  const lines = [
    `ROLE: ${o.role} — ${ROLE_BRIEF[o.role]}`,
    `TICKET: ${o.ticket}`,
    `TASK: ${o.task}`,
  ];
  lines.push("推理应基于工具返回的逐字证据，区分已验证事实和推断；禁止用 bash 的 echo/printf 等伪造思考或工具证据。当前子进程未提供结构化 think 工具，此规范仅靠提示约束，不能强制执行。");
  if (o.role === "implementer") lines.push(`ALLOWED_PATHS: ${(o.paths ?? []).join(", ") || "(未给,禁止写)"}`);
  lines.push(REPORT_PROTOCOL);
  return lines.join("\n\n");
}

export function recordPath(id: string, cwd: string): string {
  return join(cwd, "out", "superagent", `${id}.json`);
}

function record(d: DispatchResult, cwd: string): void {
  const dir = join(cwd, "out", "superagent");
  mkdirSync(dir, { recursive: true });
  const p = recordPath(d.id, cwd);
  d.record = p;
  writeFileSync(p, JSON.stringify(d, null, 2));
}

// 起一个子 omp 执行 ticket;返回机器校验后的结果。
export async function runDispatch(o: DispatchOpts): Promise<DispatchResult> {
  const cwd = o.cwd ?? process.cwd();
  const model = o.model ?? process.env.SA_MODEL ?? "";
  const tools = o.tools ?? DEFAULT_TOOLS[o.role];
  const maxTime = o.maxTime ?? 300;
  const id = randomUUID();
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

  if (o.role === "implementer") {
    return { ...base, err: "当前无法强制限制写入路径，拒绝 implementer" };
  }
  if (o.role !== "scout" && o.role !== "reviewer") return { ...base, err: "未知角色" };
  const allowedTools: Record<string, true> = { read: true, grep: true, glob: true };
  if (tools.split(",").some((tool) => !Object.hasOwn(allowedTools, tool.trim()))) {
    return { ...base, err: "只读角色仅允许 read,grep,glob" };
  }
  if (o.budget) return { ...base, err: "当前无法强制执行步骤或字符预算" };
  if (o.signal?.aborted) return { ...base, err: "dispatch 已取消" };

  const args = ["--mode", "rpc", "--no-session", "--no-extensions", "--tools", tools, "--max-time", String(maxTime)];
  if (model) args.push("--model", model);

  const child = spawn(process.env.SA_OMP_BIN ?? "omp", args, {
    cwd,
    stdio: ["pipe", "pipe", "pipe"],
    detached: true,
  });
  let timedOut = false;
  let aborted = false;
  let failure = "";
  let stdout = "";
  let stderr = "";
  let text = "";
  const evidence: string[] = [];
  let buffer = "";
  let promptCompleted = false;
  let gotText = false;
  const kill = () => {
    if (child.pid) {
      try { process.kill(-child.pid, "SIGKILL"); return; } catch { /* already exited */ }
    }
    child.kill("SIGKILL");
  };
  const timer = setTimeout(() => { timedOut = true; kill(); }, maxTime * 1000 + 5000);
  const abort = () => { aborted = true; kill(); };
  o.signal?.addEventListener("abort", abort, { once: true });
  if (o.signal?.aborted) abort();
  const send = (frame: object) => child.stdin?.write(JSON.stringify(frame) + "\n");
  child.on("error", (e) => { failure = e.message; });
  child.stdin?.on("error", (e) => { if (!gotText) failure ||= e.message; });
  child.stdout?.setEncoding("utf8");
  child.stdout?.on("data", (chunk: string) => {
    stdout += chunk;
    buffer += chunk;
    let newline: number;
    while ((newline = buffer.indexOf("\n")) >= 0) {
      const line = buffer.slice(0, newline);
      buffer = buffer.slice(newline + 1);
      if (!line.trim()) continue;
      let frame: Record<string, unknown>;
      try {
        const parsed: unknown = JSON.parse(line);
        if (!parsed || typeof parsed !== "object" || Array.isArray(parsed)) throw new Error("帧不是对象");
        frame = parsed as Record<string, unknown>;
      } catch {
        failure ||= "RPC 输出不是合法 JSONL";
        kill();
        continue;
      }
      if (frame.type === "tool_execution_end" && !promptCompleted && frame.isError !== true &&
          typeof frame.toolName === "string" && Object.hasOwn(allowedTools, frame.toolName) &&
          typeof frame.result === "object" && frame.result !== null && "content" in frame.result) {
        const content = frame.result.content;
        if (Array.isArray(content)) for (const item of content) {
          if (item && typeof item === "object" && item.type === "text" && typeof item.text === "string")
            evidence.push(item.text);
        }
      } else if (frame.type === "response" && frame.id === "prompt" && frame.success === false) {
        failure ||= `RPC prompt 失败: ${JSON.stringify(frame.error)}`;
        kill();
      } else if (frame.type === "prompt_result" && frame.id === "prompt") {
        if (frame.status !== "completed") {
          failure ||= `RPC prompt 未完成: ${String(frame.status)} ${JSON.stringify(frame.error ?? "")}`;
          kill();
        } else if (!promptCompleted) {
          promptCompleted = true;
          send({ id: "last", type: "get_last_assistant_text" });
        }
      } else if (frame.type === "response" && frame.id === "last" && promptCompleted && !gotText) {
        if (frame.success === false || !frame.data || typeof frame.data !== "object" ||
            !("text" in frame.data) || typeof frame.data.text !== "string") {
          failure ||= "RPC 未返回最终报告";
        } else {
          text = frame.data.text;
          gotText = true;
        }
        child.stdin?.end();
      }
    }
  });
  child.stderr?.setEncoding("utf8");
  child.stderr?.on("data", (chunk: string) => { stderr += chunk; });
  const { promise: closed, resolve: close } = Promise.withResolvers<{ code: number | null; signal: NodeJS.Signals | null }>();
  child.on("close", (code, signal) => close({ code, signal }));
  send({ id: "prompt", type: "prompt", message: buildPrompt(o) });
  const outcome = await closed;
  clearTimeout(timer);
  o.signal?.removeEventListener("abort", abort);
  const ms = Date.now() - started;
  const finish = (r: DispatchResult): DispatchResult => {
    r.trace = { stdout, stderr };
    try { record(r, cwd); } catch (e) {
      return { ...r, state: "error", err: `记录失败: ${String(e)}` };
    }
    return r;
  };
  if (timedOut) return finish({ ...base, state: "timeout", ms, err: `超过 ${maxTime}s 被终止` });
  if (aborted) return finish({ ...base, ms, err: "dispatch 已取消" });
  if (failure || outcome.code !== 0 || outcome.signal || !gotText || buffer.trim()) {
    return finish({ ...base, ms, err: failure || `子进程异常退出: code=${outcome.code} signal=${outcome.signal}` });
  }
  const parsed = parseReport(text);
  if (!parsed.ok || !parsed.report)
    return finish({ ...base, state: "invalid", ms, err: parsed.err ?? "report 校验失败" });
  if (parsed.report.ticket !== o.ticket)
    return finish({ ...base, state: "invalid", ms, err: "report ticket 与 dispatch 不一致" });
  const rec = reconcileVerified(parsed.report, evidence);
  if (!rec.ok || !rec.report)
    return finish({ ...base, state: "invalid", ms, err: rec.err, report: rec.report, verifiedDropped: rec.droppedVerified });
  const files = checkFiles(o.role, o.paths, rec.report, cwd);
  if (!files.ok) return finish({ ...base, state: "invalid", ms, err: files.err, report: rec.report });
  return finish({ ...base, state: rec.report.state, ms, tokens: rec.report.tokens,
    report: rec.report, verifiedDropped: rec.droppedVerified });
}
