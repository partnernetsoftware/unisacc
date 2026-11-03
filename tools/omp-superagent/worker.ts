// 工具调用前检查写入目标；bash 是协作约束，不是安全沙箱。
import type { ExtensionAPI } from "@oh-my-pi/pi-coding-agent";
import { readFile } from "node:fs/promises";
import { resolve } from "node:path";
import { checkFiles, type Report } from "./report.ts";


type Policy = { role: string; paths: string[]; cwd: string };

export function checkWriteCall(policy: Policy, toolName: string, input: Record<string, unknown>): string | undefined {
  if (toolName !== "write" && toolName !== "edit") return;
  if (policy.role !== "implementer") return "只读角色不允许写入工具";
  const targets: string[] = [];
  if (toolName === "write") {
    if (typeof input.path !== "string") return "无法解析写入路径";
    targets.push(input.path);
  } else {
    // 自由格式编辑可能包含多个文件、移动和 URI；不猜测其解析结果。
    // 仅接受结构化单路径模式，并检查每个重命名目标。
    if (typeof input.path !== "string" || "input" in input) return "无法安全解析编辑路径，请使用 write";
    targets.push(input.path);
    if ("edits" in input) {
      if (!Array.isArray(input.edits)) return "无法解析编辑操作";
      for (const edit of input.edits) {
        if (!edit || typeof edit !== "object" || Array.isArray(edit)) return "无法解析编辑操作";
        if ("rename" in edit) {
          if (typeof edit.rename !== "string") return "无法解析重命名目标";
          targets.push(edit.rename);
        }
      }
    } else if (typeof input.old_string !== "string" || typeof input.new_string !== "string") {
      return "无法解析编辑操作";
    }
  }
  if (!targets.length || targets.some((p) => !p || /^[a-z][a-z0-9+.-]*:/i.test(p))) return "无法解析本地写入路径";
  const report: Report = { t: "report", ticket: "policy", state: "blocked", summary: "policy", verified: [], inferred: [], files: targets.map((path) => ({ path, change: "" })), blocked: "policy", next: [] };
  const result = checkFiles(policy.role, policy.paths, report, policy.cwd);
  return result.ok ? undefined : result.err;
}

export default function (pi: ExtensionAPI) {
  let policy: Policy | undefined;
  try {
    const parsed = JSON.parse(process.env.SA_WORKER_POLICY ?? "null");
    if (parsed && typeof parsed.role === "string" && typeof parsed.cwd === "string" && Array.isArray(parsed.paths) && parsed.paths.every((p: unknown) => typeof p === "string")) policy = parsed;
  } catch { /* 缺失策略时拒绝写入 */ }
  const z = pi.zod;
  pi.registerTool({
    name: "think",
    label: "think",
    description: "记录基于文件逐字证据的推理；未验证的假设单独列出。ref 使用 path:line，quote 必须是该行非空逐字片段。",
    parameters: z.object({
      claim: z.string().trim().min(1),
      steps: z.array(z.object({
        claim: z.string().trim().min(1),
        ref: z.string().trim().min(1),
        quote: z.string().min(1),
      })).max(8),
      assumptions: z.array(z.string().trim().min(1)).max(8),
      confidence: z.number().min(0).max(1),
    }),
    async execute(_callId, params) {
      const { claim, steps, assumptions, confidence } = params;
      if (steps.length === 0 && assumptions.length === 0)
        return { content: [{ type: "text" as const, text: "须提供可核证据或未验证假设" }], isError: true };
      const cwd = policy?.cwd ?? process.cwd();
      for (const step of steps) {
        const match = /^(.*):([1-9]\d*)$/.exec(step.ref);
        if (!match || !match[1] || !step.quote.trim()) {
          return { content: [{ type: "text" as const, text: `无效引用或空引文: ${step.ref}` }], isError: true };
        }
        const line = Number(match[2]);
        if (!Number.isSafeInteger(line)) {
          return { content: [{ type: "text" as const, text: `无效行号: ${step.ref}` }], isError: true };
        }
        try {
          const text = await readFile(resolve(cwd, match[1]), "utf8");
          if (!text.split(/\r?\n/)[line - 1]?.includes(step.quote)) {
            return { content: [{ type: "text" as const, text: `引用无法逐字核对: ${step.ref}` }], isError: true };
          }
        } catch {
          return { content: [{ type: "text" as const, text: `无法读取引用: ${step.ref}` }], isError: true };
        }
      }
      return { content: [{ type: "text" as const, text: JSON.stringify({ claim, steps, assumptions, confidence }) }] };
    },
  });
  pi.on("tool_call", async (event) => {
    if (event.toolName !== "write" && event.toolName !== "edit") return;
    const reason = policy ? checkWriteCall(policy, event.toolName, event.input) : "缺少写入授权策略";
    if (reason) return { block: true, reason };
  });
  pi.on("session_start", async (_event, ctx) => {
    if (policy && process.env.SA_WORKER_NONCE) ctx.ui.notify(`sa-worker-ready:${process.env.SA_WORKER_NONCE}`);
  });
  pi.on("tool_result", async (event) => ({
    content: [...event.content, { type: "text", text: `\n[工具收据 ID: ${event.toolCallId}]` }],
  }));
}
