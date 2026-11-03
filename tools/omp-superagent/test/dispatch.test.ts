import { describe, expect, test, beforeAll } from "bun:test";
import { mkdtempSync, existsSync, readFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { buildPrompt, recordPath, runDispatch } from "../core.ts";
import type { ExtensionAPI } from "@oh-my-pi/pi-coding-agent";
import worker, { checkWriteCall } from "../worker.ts";
function schema(): unknown {
  return new Proxy((): unknown => schema(), { get: () => (): unknown => schema() });
}

const fake = join(import.meta.dir, "fake-omp.sh");
function cwd(): string { return mkdtempSync(join(tmpdir(), "sa-test-")); }
const task = { goal: "定位示例", context: ["example.txt"], accept: ["NOT_APPLICABLE"], nonGoals: ["不修改文件"] };
beforeAll(() => { process.env.SA_OMP_BIN = fake; });

async function dispatch(mode: string) {
  process.env.FAKE_MODE = mode;
  return runDispatch({ ticket: "T1", task, role: "scout", cwd: cwd() });
}

describe("RPC dispatch", () => {
  test("提示包含任务和报告契约", () => {
    expect(buildPrompt({ ticket: "T1", task, role: "scout" })).toContain("TICKET: T1");
    expect(buildPrompt({ ticket: "T1", task, role: "scout" })).toContain("verified");
  });
  test("结构化合同每项均不可为空", async () => {
    for (const bad of [
      { ...task, goal: " " },
      { ...task, context: [] },
      { ...task, accept: [] },
      { ...task, nonGoals: [""] },
    ]) {
      const r = await runDispatch({ ticket: "T1", task: bad, role: "scout", cwd: cwd() });
      expect(r.state).toBe("error");
      expect(r.record).toBeUndefined();
    }
  });
  test("工具结果可作为证据并保存记录", async () => {
    const r = await dispatch("done");
    expect(r.state).toBe("done");
    expect(r.report?.verified).toEqual([{ toolCallId: "1", kind: "output", value: "VERIFIED-TOKEN" }]);
    expect(r.tokens).toBeNull();
    expect(r.record && existsSync(r.record)).toBe(true);
    expect(JSON.parse(readFileSync(r.record!, "utf8")).ticket).toBe("T1");
    expect(JSON.parse(readFileSync(r.record!, "utf8")).trace.stdout).toContain("tool_execution_end");
    expect(r.record).toBe(recordPath(r.id, r.record!.replace(/\/out\/superagent\/[^/]+$/, "")));
  });
  test("worker 声称完成但宿主文件验收失败时返回失败", async () => {
    process.env.FAKE_MODE = "done";
    const root = cwd();
    const r = await runDispatch({ ticket: "T1", task, role: "scout", cwd: root, checks: [{ kind: "exists", path: "missing" }] });
    expect(r.state).toBe("failed");
    expect(r.acceptance).toMatchObject({ ok: false });
    expect(r.err).toContain("读取失败");
  });
  test("blocked 原因冒泡", async () => {
    const r = await dispatch("blocked");
    expect(r.state).toBe("blocked");
    expect(r.report?.blocked).toBe("需要凭证");
  });
  test("报告外文字无效", async () => { expect((await dispatch("noise")).state).toBe("invalid"); });
  test("未知证据不能自证", async () => {
    const r = await dispatch("unverified");
    expect(r.state).toBe("invalid");
    expect(r.verifiedDropped).toEqual([{ toolCallId: "1", kind: "output", value: "NEVER-IN-OUTPUT" }]);
  });
  test("助手消息不是工具证据", async () => { expect((await dispatch("forged")).state).toBe("invalid"); });
  test("工具收据标记不能作为输出证据", async () => {
    expect((await dispatch("receipt")).state).toBe("invalid");
  });
  test("ticket 不匹配无效", async () => { expect((await dispatch("mismatch")).state).toBe("invalid"); });
  test("只读角色声明文件无效", async () => { expect((await dispatch("files")).state).toBe("invalid"); });
  test("非零退出不可返回 done", async () => { expect((await dispatch("nonzero")).state).toBe("error"); });
  test("畸形 RPC 和提前退出不可返回 done", async () => {
    expect((await dispatch("badjson")).state).toBe("error");
    expect((await dispatch("crash")).state).toBe("error");
  });
  test("角色工具白名单和写路径参数必须校验", async () => {
    expect((await runDispatch({ ticket: "T1", task, role: "implementer", cwd: cwd() })).state).toBe("error");
    expect((await runDispatch({ ticket: "T1", task, role: "scout", tools: "read,bash", cwd: cwd() })).state).toBe("error");
  });
  test("未握手时不发送任务", async () => {
    process.env.FAKE_MODE = "nohandshake";
    const root = cwd();
    const result = await runDispatch({ ticket: "T1", task, role: "implementer", paths: ["src"], cwd: root, maxTime: 1 });
    expect(result.state).not.toBe("done");
    expect(existsSync(join(root, "received-prompt"))).toBe(false);
  }, 20000);
  test("实现者与评审可派发并显式声明 bash 边界", async () => {
    process.env.FAKE_MODE = "done";
    const root = cwd();
    expect((await runDispatch({ ticket: "T1", task, role: "implementer", paths: ["src"], cwd: root })).state).toBe("done");
    expect((await runDispatch({ ticket: "T1", task, role: "reviewer", cwd: root })).state).toBe("done");
    expect(buildPrompt({ ticket: "T1", task, role: "reviewer" })).toContain("不是沙箱");
  });
  test("写入拦截检查所有目标及重命名", () => {
    const root = cwd();
    const policy = { role: "implementer", paths: ["src"], cwd: root };
    expect(checkWriteCall(policy, "write", { path: "src/a" })).toBeUndefined();
    expect(checkWriteCall(policy, "write", { path: "../escape" })).toBeDefined();
    expect(checkWriteCall(policy, "write", { path: "xd://debug" })).toBeDefined();
    expect(checkWriteCall(policy, "edit", { path: "src/a", old_string: "a", new_string: "b" })).toBeUndefined();
    expect(checkWriteCall(policy, "edit", { path: "src/a", edits: [{ op: "update", rename: "src/b" }] })).toBeUndefined();
    expect(checkWriteCall(policy, "edit", { path: "src/a", input: "*** Begin Patch" })).toBeDefined();
    expect(checkWriteCall(policy, "edit", { path: "src/a", edits: [{ op: "update", rename: "other/a", diff: "@@\\n-a\\n+b\\n" }] })).toBeDefined();
    expect(checkWriteCall(policy, "edit", { input: "*** Begin Patch\n[src/a#ABCD]\nREM\n[other/b#ABCD]\nREM\n*** End Patch\n" })).toBeDefined();
    expect(checkWriteCall(policy, "edit", { input: "unparseable" })).toBeDefined();
    expect(checkWriteCall({ ...policy, role: "reviewer" }, "write", { path: "src/a" })).toBeDefined();
  });
  test("worker 在真实工具执行前注册阻断", async () => {
    const handlers: Record<string, Function> = {};
    const previous = process.env.SA_WORKER_POLICY;
    process.env.SA_WORKER_POLICY = JSON.stringify({ role: "implementer", paths: ["src"], cwd: cwd() });
    try {
      // 测试替身仅实现本扩展使用的 on 接口。
      worker({ zod: schema(), registerTool() {}, on: (name: string, fn: Function) => { handlers[name] = fn; } } as unknown as ExtensionAPI);
      expect(await handlers.tool_call({ toolName: "write", input: { path: "outside" } })).toMatchObject({ block: true });
      expect(await handlers.tool_call({ toolName: "write", input: { path: "src/a" } })).toBeUndefined();
      expect(await handlers.tool_call({ toolName: "bash", input: { command: "true" } })).toBeUndefined();
    } finally {
      if (previous === undefined) delete process.env.SA_WORKER_POLICY;
      else process.env.SA_WORKER_POLICY = previous;
    }
  });
  test("不存在的进程返回错误", async () => {
    process.env.SA_OMP_BIN = "/nonexistent/omp";
    try { expect((await dispatch("done")).state).toBe("error"); }
    finally { process.env.SA_OMP_BIN = fake; }
  });
  test("取消会终止进行中的进程", async () => {
    process.env.FAKE_MODE = "hang";
    const controller = new AbortController();
    const running = runDispatch({ ticket: "T1", task, role: "scout", cwd: cwd(), signal: controller.signal });
    controller.abort();
    expect((await running).err).toContain("取消");
  });
  test("超时终止进程", async () => {
    process.env.FAKE_MODE = "hang";
    expect((await runDispatch({ ticket: "T1", task, role: "scout", cwd: cwd(), maxTime: 1 })).state).toBe("timeout");
  }, 20000);
});
