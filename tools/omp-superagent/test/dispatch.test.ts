import { describe, expect, test, beforeAll } from "bun:test";
import { mkdtempSync, existsSync, readFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { buildPrompt, recordPath, runDispatch } from "../core.ts";

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
