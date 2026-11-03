// 工具调用前检查写入目标；bash 是协作约束，不是安全沙箱。
import type { ExtensionAPI } from "@oh-my-pi/pi-coding-agent";
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
