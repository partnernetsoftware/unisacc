// 向 worker 暴露宿主生成的工具调用 ID；父进程仍独立核对 RPC 事件。
import type { ExtensionAPI } from "@oh-my-pi/pi-coding-agent";

export default function (pi: ExtensionAPI) {
  pi.on("tool_result", async (event) => ({
    content: [...event.content, { type: "text", text: `\n[工具收据 ID: ${event.toolCallId}]` }],
  }));
}
