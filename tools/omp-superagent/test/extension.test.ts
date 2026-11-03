import { describe, expect, test } from "bun:test";
import { mkdtempSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import type { ExtensionAPI } from "@oh-my-pi/pi-coding-agent";
import extension from "../extension.ts";

// 只实现注册期间使用的 schema 链，执行时直接传入已验证的参数。
function schema(): unknown {
  return new Proxy((): unknown => schema(), { get: () => (): unknown => schema() });
}

describe("dispatch ticket 生命周期", () => {
  test("未接纳的参数错误允许修正后重试，同票并发仍被拒绝", async () => {
    type Tool = { name: string; execute: (id: string, params: object) => Promise<{ isError?: boolean; details?: { state: string } }> };
    const tools = new Map<string, Tool>();
    // 测试替身仅模拟该扩展使用的宿主方法。
    const host = { zod: schema(), setLabel() {}, registerTool(tool: Tool) { tools.set(tool.name, tool); }, registerCommand() {} };
    extension(host as unknown as ExtensionAPI);
    const dispatch = tools.get("dispatch")!;
    const oldBin = process.env.SA_OMP_BIN;
    const oldMode = process.env.FAKE_MODE;
    process.env.SA_OMP_BIN = join(import.meta.dir, "fake-omp.sh");
    process.env.FAKE_MODE = "done";
    const params = { ticket: "T1", task: { goal: "定位示例", context: ["example.txt"], accept: ["NOT_APPLICABLE"], nonGoals: ["不修改文件"] }, role: "scout", cwd: mkdtempSync(join(tmpdir(), "sa-ext-")) };
    try {
      const rejected = await dispatch.execute("1", { ...params, tools: "read,not-a-tool" });
      expect(rejected.details?.state).toBe("error");
      const running = dispatch.execute("2", params);
      const duplicate = await dispatch.execute("3", params);
      expect(duplicate.isError).toBe(true);
      const result = await running;
      expect(result.details?.state).toBe("done");
      expect((await dispatch.execute("4", params)).isError).toBe(true);
    } finally {
      if (oldBin === undefined) delete process.env.SA_OMP_BIN; else process.env.SA_OMP_BIN = oldBin;
      if (oldMode === undefined) delete process.env.FAKE_MODE; else process.env.FAKE_MODE = oldMode;
    }
  });
});
