import { describe, expect, test, beforeAll } from "bun:test";
import { mkdtempSync, existsSync, readFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { buildPrompt, recordPath, runDispatch } from "../core.ts";

const fake = join(import.meta.dir, "fake-omp.sh");

function cwd(): string {
  return mkdtempSync(join(tmpdir(), "sa-test-"));
}

beforeAll(() => {
  process.env.SA_OMP_BIN = fake;
});

describe("buildPrompt", () => {
  test("implementer 必须列出允许路径", () => {
    const p = buildPrompt({ ticket: "T1", task: "加函数", role: "implementer", paths: ["src/"] });
    expect(p).toContain("ALLOWED_PATHS: src/");
    expect(p).toContain("TICKET: T1");
    expect(p).toContain("【report 协议】");
  });

  test("scout 不给路径,提示未给则禁止写", () => {
    const p = buildPrompt({ ticket: "T2", task: "查调用点", role: "scout" });
    expect(p).not.toContain("ALLOWED_PATHS");
    expect(p).toContain("ROLE: scout");
  });
});

describe("runDispatch 终态", () => {
  test("done:verified 逐字可核,record 落盘", async () => {
    process.env.FAKE_MODE = "done";
    const dir = cwd();
    const r = await runDispatch({ ticket: "T1", task: "x", role: "scout", cwd: dir });
    expect(r.state).toBe("done");
    expect(r.report?.verified).toEqual(["VERIFIED-TOKEN"]);
    expect(r.tokens).toBe(7);
    expect(existsSync(recordPath(r.id, dir))).toBe(true);
    const saved = JSON.parse(readFileSync(recordPath(r.id, dir), "utf8"));
    expect(saved.version).toMatch(/^sa-[0-9a-f]{12}$/);
    expect(saved.ticket).toBe("T1");
  });

  test("blocked 冒泡原因", async () => {
    process.env.FAKE_MODE = "blocked";
    const r = await runDispatch({ ticket: "T2", task: "x", role: "scout", cwd: cwd() });
    expect(r.state).toBe("blocked");
    expect(r.report?.blocked).toBe("需要凭证");
  });

  test("JSON 外有文字 → invalid", async () => {
    process.env.FAKE_MODE = "noise";
    const r = await runDispatch({ ticket: "T3", task: "x", role: "scout", cwd: cwd() });
    expect(r.state).toBe("invalid");
    expect(r.err).toContain("之外有文字");
  });

  test("verified 核不到且 done → invalid,并记录降级", async () => {
    process.env.FAKE_MODE = "unverified";
    const r = await runDispatch({ ticket: "T4", task: "x", role: "scout", cwd: cwd() });
    expect(r.state).toBe("invalid");
    expect(r.verifiedDropped).toEqual(["NEVER-IN-OUTPUT"]);
  });

  test("scout 声明改动文件 → invalid", async () => {
    process.env.FAKE_MODE = "files";
    const r = await runDispatch({ ticket: "T5", task: "x", role: "scout", cwd: cwd() });
    expect(r.state).toBe("invalid");
    expect(r.err).toContain("不许声明改动文件");
  });

  test("implementer 提交越界文件 → invalid", async () => {
    process.env.FAKE_MODE = "files";
    const r = await runDispatch({
      ticket: "T6",
      task: "x",
      role: "implementer",
      paths: ["tools/"],
      cwd: cwd(),
    });
    expect(r.state).toBe("invalid");
    expect(r.err).toContain("越界写入");
  });

  test("implementer 前缀命中 → done", async () => {
    process.env.FAKE_MODE = "files";
    const r = await runDispatch({
      ticket: "T7",
      task: "x",
      role: "implementer",
      paths: ["src"],
      cwd: cwd(),
    });
    expect(r.state).toBe("done");
    expect(r.report?.files[0].path).toBe("src/x.ts");
  });

  test("implementer 无 paths 直接拒,不起进程", async () => {
    const r = await runDispatch({ ticket: "T8", task: "x", role: "implementer", cwd: cwd() });
    expect(r.state).toBe("error");
    expect(r.err).toContain("必须给出允许写入的 paths");
  });

  test("进程超时被终止并记 timeout", async () => {
    process.env.FAKE_MODE = "hang";
    const r = await runDispatch({ ticket: "T9", task: "x", role: "scout", cwd: cwd(), maxTime: 1 });
    expect(r.state).toBe("timeout");
  }, 20000);

  test("退出码非 0 但 report 合法 → 仍按 report 判,err 记退出码", async () => {
    process.env.FAKE_MODE = "other";
    const r = await runDispatch({ ticket: "T10", task: "x", role: "scout", cwd: cwd() });
    expect(r.state).toBe("invalid");
  });
});
