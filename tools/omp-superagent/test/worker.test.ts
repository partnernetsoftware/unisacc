import { describe, expect, test } from "bun:test";
import { mkdtempSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import type { ExtensionAPI } from "@oh-my-pi/pi-coding-agent";
import worker from "../worker.ts";

function schema(): unknown {
  return new Proxy((): unknown => schema(), { get: () => (): unknown => schema() });
}

type Result = { content: { type: string; text: string }[]; isError?: boolean };
type Tool = { name: string; execute: (id: string, params: object) => Promise<Result> };
type Hook = (event: unknown, ctx: unknown) => Promise<unknown>;

describe("worker think", () => {
  test("独立注册并逐行核对证据，假设保持分离，同时保留原有钩子", async () => {
    const cwd = mkdtempSync(join(tmpdir(), "sa-think-"));
    writeFileSync(join(cwd, "proof.txt"), "第一行\n第二行证据\n");
    const oldPolicy = process.env.SA_WORKER_POLICY;
    const oldNonce = process.env.SA_WORKER_NONCE;
    process.env.SA_WORKER_POLICY = JSON.stringify({ role: "scout", paths: [], cwd });
    process.env.SA_WORKER_NONCE = "nonce";
    try {
      const tools = new Map<string, Tool>();
      const hooks = new Map<string, Hook>();
      worker({ zod: schema(), registerTool(tool: Tool) { tools.set(tool.name, tool); }, on(name: string, hook: Hook) { hooks.set(name, hook); } } as unknown as ExtensionAPI);
      const think = tools.get("think")!;
      const params = { claim: "结论", steps: [{ claim: "证据", ref: "proof.txt:2", quote: "行证据" }], assumptions: ["未经验证"], confidence: 0.8 };
      const result = await think.execute("1", params);
      expect(result.isError).not.toBe(true);
      expect(JSON.parse(result.content[0].text)).toEqual(params);
      expect((await think.execute("pure", { ...params, steps: [], assumptions: ["可能"] })).isError).not.toBe(true);
      expect((await think.execute("empty", { ...params, steps: [], assumptions: [] })).isError).toBe(true);
      for (const step of [
        { ...params.steps[0], quote: "第一行" },
        { ...params.steps[0], ref: "proof.txt:9" },
        { ...params.steps[0], ref: "proof.txt" },
        { ...params.steps[0], ref: "missing:1" },
        { ...params.steps[0], quote: " " },
      ]) expect((await think.execute("2", { ...params, steps: [step] })).isError).toBe(true);
      const notices: string[] = [];
      await hooks.get("session_start")!({}, { ui: { notify: (value: string) => notices.push(value) } });
      expect(notices).toEqual(["sa-worker-ready:nonce"]);
      expect((await hooks.get("tool_call")!({ toolName: "write", input: { path: "x" } }, {}) as { block: boolean }).block).toBe(true);
      expect((await hooks.get("tool_result")!({ content: [], toolCallId: "abc" }, {}) as { content: { text: string }[] }).content[0].text).toContain("abc");
    } finally {
      if (oldPolicy === undefined) delete process.env.SA_WORKER_POLICY; else process.env.SA_WORKER_POLICY = oldPolicy;
      if (oldNonce === undefined) delete process.env.SA_WORKER_NONCE; else process.env.SA_WORKER_NONCE = oldNonce;
    }
  });
});
