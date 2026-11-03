// 核心:角色模板 + 子 omp 调用(spawn/预算/落盘)+ dispatch 记录。
// 与 omp-workflow 的 core.ts 同构,但不 import(两仓独立演化)。
import { spawn } from "node:child_process";
import { mkdirSync, writeFileSync } from "node:fs";
import { join } from "node:path";
import { fileURLToPath } from "node:url";
import { randomUUID } from "node:crypto";
import { checkFiles, parseReport, reconcileVerified, type EvidenceReceipt, type VerifiedEvidence, type Report } from "./report.ts";
import { SA_VERSION } from "./version.ts";

export type Role = "implementer" | "reviewer" | "scout";

export interface TaskContract {
  goal: string;
  context: string[];
  accept: string[];
  nonGoals: string[];
}

export interface DispatchOpts {
  ticket: string;
  task: TaskContract;
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
  tokens: number | null;
  version: string;
  trace?: { stdout: string; stderr: string };
  verifiedDropped?: VerifiedEvidence[];
}

const DEFAULT_TOOLS: Record<Role, string> = {
  scout: "read,grep,glob",
  reviewer: "read,grep,glob,bash",
  implementer: "read,grep,glob,bash,edit,write",
};

const ROLE_BRIEF: Record<Role, string> = {
  scout: "只读检索:定位事实与落点。禁止改文件,禁止跑会写盘的命令。",
  reviewer: "评审给定补丁/改动:只读,不得改文件。",
  implementer: "仅在授权路径内实现任务。",
};

export const REPORT_PROTOCOL =
  "【report 协议】本轮唯一产出是一个 JSON 对象,不得有任何其它文本/思考块。\n" +
  '{"t":"report","ticket":"<dispatch 给的 ticket>","state":"done|blocked|failed","summary":"一句",' +
  '"verified":[{"toolCallId":"工具调用 ID","kind":"output|command|path","value":"逐字证据"}],"inferred":["未验证推断"],' +
  '"files":[{"path":"..","change":".."}],"blocked":"阻塞原因(无则空)","next":["可执行下一步"]}\n' +
  "state=done 时 verified 不得为空;非 implementer 角色 files 必须为空;JSON 之外任何文字都会作废本次 dispatch。\n" +
  "verified 必须关联成功工具调用：output 为工具输出逐字片段，command 为 bash 命令，path 为 read 路径。";

export function buildPrompt(o: DispatchOpts): string {
  const lines = [
    `ROLE: ${o.role} — ${ROLE_BRIEF[o.role]}`,
    `TICKET: ${o.ticket}`,
    `TASK: ${JSON.stringify(o.task)}`,
  ];
  lines.push("推理应基于工具返回的逐字证据，区分已验证事实和推断；禁止用 bash 的 echo/printf 等伪造思考或工具证据。当前子进程未提供结构化 think 工具，此规范仅靠提示约束，不能强制执行。");
  if (o.role === "implementer") lines.push(`ALLOWED_PATHS: ${(o.paths ?? []).join(", ") || "(未给,禁止写)"}`);
  lines.push("bash 在宿主直接执行，不是沙箱；必须自行遵守角色只读及授权路径约束，报告检查无法阻止 bash 越界写入。");
  if (o.role === "implementer") lines.push("edit 仅允许结构化单路径 replace/patch 模式；自由格式多文件补丁会被拒绝，请对单个文件使用 write。");
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
  writeFileSync(p, JSON.stringify({ ...d, record: p }, null, 2));
  d.record = p;
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
    tokens: null,
    version: SA_VERSION,
  };

  const task = o.task;
  if (!task || typeof task.goal !== "string" || !task.goal.trim() ||
      ![task.context, task.accept, task.nonGoals].every((items) =>
        Array.isArray(items) && items.length > 0 && items.every((item) => typeof item === "string" && item.trim()))) {
    return { ...base, err: "任务合同必须包含目标、上下文、验收条件和非目标" };
  }
  if (o.role !== "scout" && o.role !== "reviewer" && o.role !== "implementer") return { ...base, err: "未知角色" };
  const permitted = o.role === "scout" ? ["read", "grep", "glob"] :
    o.role === "reviewer" ? ["read", "grep", "glob", "bash"] :
    ["read", "grep", "glob", "bash", "edit", "write"];
  const selected = tools.split(",").map((tool) => tool.trim());
  if (selected.some((tool) => !permitted.includes(tool))) return { ...base, err: "角色不允许指定工具" };
  const allowedTools = Object.fromEntries(selected.map((tool) => [tool, true]));
  if (o.role === "implementer") {
    const check = checkFiles(o.role, o.paths, { t: "report", ticket: o.ticket, state: "blocked", summary: "预检", verified: [], inferred: [], files: [], blocked: "预检", next: [] }, cwd);
    if (!check.ok) return { ...base, err: check.err };
  }
  if (o.budget) return { ...base, err: "当前无法强制执行步骤或字符预算" };
  if (o.signal?.aborted) return { ...base, err: "dispatch 已取消" };

  const args = ["--mode", "rpc", "--no-session", "--no-extensions", "-e", fileURLToPath(new URL("./worker.ts", import.meta.url)), "--tools", tools, "--max-time", String(maxTime)];
  const nonce = randomUUID();
  if (model) args.push("--model", model);

  const child = spawn(process.env.SA_OMP_BIN ?? "omp", args, {
    cwd,
    stdio: ["pipe", "pipe", "pipe"],
    env: { ...process.env, SA_WORKER_POLICY: JSON.stringify({ role: o.role, paths: o.paths ?? [], cwd }), SA_WORKER_NONCE: nonce },
    detached: true,
  });
  let timedOut = false;
  let aborted = false;
  let failure = "";
  let stdout = "";
  let stderr = "";
  let text = "";
  const evidence: EvidenceReceipt[] = [];
  const calls = new Map<string, { toolName: string; args: unknown }>();
  const seenCalls = new Set<string>();
  let buffer = "";
  let promptCompleted = false;
  let gotText = false;
  let workerReady = false;
  const kill = () => {
    if (child.pid) {
      try { process.kill(-child.pid, "SIGKILL"); return; } catch { /* already exited */ }
    }
    child.kill("SIGKILL");
  };
  const startupTimer = setTimeout(() => { failure ||= "worker 授权握手超时"; kill(); }, 15000);
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
      if (frame.type === "extension_ui_request" && frame.method === "notify" &&
          frame.message === `sa-worker-ready:${nonce}` && !workerReady) {
        workerReady = true;
        clearTimeout(startupTimer);
        send({ id: "prompt", type: "prompt", message: buildPrompt(o) });
        continue;
      }
      if (frame.type === "tool_execution_start" && !promptCompleted &&
          typeof frame.toolCallId === "string" && typeof frame.toolName === "string") {
        if (seenCalls.has(frame.toolCallId)) {
          failure ||= "RPC 工具调用 ID 重复";
          kill();
        } else {
          seenCalls.add(frame.toolCallId);
          if (Object.hasOwn(allowedTools, frame.toolName))
            calls.set(frame.toolCallId, { toolName: frame.toolName, args: frame.args });
        }
      } else if (frame.type === "tool_execution_end" && !promptCompleted &&
          typeof frame.toolCallId === "string" && typeof frame.toolName === "string") {
        const call = calls.get(frame.toolCallId);
        calls.delete(frame.toolCallId);
        if (call && call.toolName === frame.toolName && frame.isError === false &&
            typeof frame.result === "object" && frame.result !== null && "content" in frame.result) {
          const content = frame.result.content;
          if (Array.isArray(content)) for (const item of content) {
            if (item && typeof item === "object" && item.type === "text" && typeof item.text === "string")
              evidence.push({ toolCallId: frame.toolCallId, toolName: call.toolName, args: call.args, output: item.text === `\n[工具收据 ID: ${frame.toolCallId}]` ? "" : item.text });
          }
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
  // 只有扩展确认策略已安装后才发送任务。
  const outcome = await closed;
  clearTimeout(startupTimer);
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
  if (!workerReady) return finish({ ...base, ms, err: failure || "worker 授权握手未完成" });
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
  return finish({ ...base, state: rec.report.state, ms,
    report: rec.report, verifiedDropped: rec.droppedVerified });
}
