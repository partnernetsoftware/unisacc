import { describe, expect, test } from "bun:test";
import { mkdtempSync, mkdirSync, symlinkSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { checkFiles, parseReport, pathsConflict, reconcileVerified, type Report } from "../report.ts";
import { deriveVersion, SA_VERSION } from "../version.ts";

const good = {
  t: "report",
  ticket: "T1",
  state: "done",
  summary: "加了 X",
  verified: [{ toolCallId: "1", kind: "output", value: "3 pass" }],
  inferred: [],
  files: [{ path: "src/a.ts", change: "新增函数" }],
  blocked: "",
  next: [],
};

describe("parseReport", () => {
  test("合法 report 通过", () => {
    const r = parseReport(JSON.stringify(good));
    expect(r.ok).toBe(true);
    expect(r.report?.ticket).toBe("T1");
  });

  test("JSON 外有文字即失败", () => {
    const r = parseReport("好的,结果如下 " + JSON.stringify(good));
    expect(r.ok).toBe(false);
    expect(r.err).toContain("之外有文字");
  });

  test("done 但 verified 空必须失败", () => {
    expect(parseReport(JSON.stringify({ ...good, verified: [] })).ok).toBe(false);
  });

  test("blocked 无原因必须失败", () => {
    expect(parseReport(JSON.stringify({ ...good, state: "blocked", blocked: "" })).ok).toBe(false);
  });

  test("state 非法必须失败", () => {
    expect(parseReport(JSON.stringify({ ...good, state: "ok" })).ok).toBe(false);
  });

  test("t 必须是 report", () => {
    expect(parseReport(JSON.stringify({ ...good, t: "done" })).ok).toBe(false);
  });

  test("files 元素形状错误必须失败", () => {
    expect(parseReport(JSON.stringify({ ...good, files: [{ path: "a" }] })).ok).toBe(false);
  });

  test("拒绝旧字符串证据和无效证据类型", () => {
    expect(parseReport(JSON.stringify({ ...good, verified: ["bun test"] })).ok).toBe(false);
    expect(parseReport(JSON.stringify({ ...good, verified: [{ toolCallId: "1", kind: "test", value: "ok" }] })).ok).toBe(false);
    expect(parseReport(JSON.stringify({ ...good, verified: [{ toolCallId: "", kind: "output", value: "ok" }] })).ok).toBe(false);
  });

  test("报告中的自报 token 不进入宿主报告", () => {
    const result = parseReport(JSON.stringify({ ...good, tokens: -100, ms: -1 }));
    expect(result.ok).toBe(true);
    expect(result.report).not.toHaveProperty("tokens");
    expect(result.report).not.toHaveProperty("ms");
  });
});

describe("reconcileVerified", () => {
  const output = { toolCallId: "1", kind: "output" as const, value: "3 pass" };
  const path = { toolCallId: "1", kind: "path" as const, value: "src/a.ts" };
  const command = { toolCallId: "2", kind: "command" as const, value: "bun test" };
  const rep = { ...good, verified: [output, path, command], inferred: [] } as Report;
  const receipts = [
    { toolCallId: "1", toolName: "read", args: { path: "src/a.ts" }, output: "bun test -> 3 pass" },
    { toolCallId: "2", toolName: "bash", args: { command: "bun test" }, output: "3 pass" },
  ];

  test("按调用类型与编号匹配", () => {
    const r = reconcileVerified(rep, receipts);
    expect(r.ok).toBe(true);
    expect(r.report?.verified).toEqual(rep.verified);
  });

  test("不能以读取到的文本冒充执行命令", () => {
    const forged = { ...command, toolCallId: "1" };
    const r = reconcileVerified({ ...rep, verified: [output, forged] }, receipts);
    expect(r.report?.verified).toEqual([output]);
    expect(r.droppedVerified).toEqual([forged]);
    expect(r.report?.inferred).toEqual([`[未核到] ${JSON.stringify(forged)}`]);
  });

  test("不能跨调用匹配或拼接结果", () => {
    expect(reconcileVerified({ ...rep, verified: [{ ...output, toolCallId: "3" }] }, receipts).ok).toBe(false);
    expect(reconcileVerified({ ...rep, verified: [{ ...output, value: "pass3" }] }, receipts).ok).toBe(false);
    expect(reconcileVerified({ ...rep, verified: [{ ...output, value: " " }] }, receipts).ok).toBe(false);
  });

  test("路径和命令必须精确匹配参数", () => {
    expect(reconcileVerified({ ...rep, verified: [{ ...path, value: "src" }] }, receipts).ok).toBe(false);
    expect(reconcileVerified({ ...rep, verified: [{ ...command, value: "bun" }] }, receipts).ok).toBe(false);
  });
});

describe("checkFiles", () => {
  const rep = { ...good, files: [{ path: "src/a.ts", change: "x" }] } as Report;

  test("scout 声明改动文件被拒", () => {
    expect(checkFiles("scout", undefined, rep).ok).toBe(false);
  });

  test("implementer 无 paths 被拒", () => {
    expect(checkFiles("implementer", [], rep).ok).toBe(false);
  });

  test("implementer 越界写被拒", () => {
    expect(checkFiles("implementer", ["tools/"], rep).ok).toBe(false);
  });

  test("implementer 前缀命中通过", () => {
    expect(checkFiles("implementer", ["src"], rep).ok).toBe(true);
  });
});

describe("pathsConflict", () => {
  test("同一前缀冲突", () => {
    expect(pathsConflict(["src/"], ["src/a.ts"])).toBeDefined();
  });

  test("不相干路径不冲突", () => {
    expect(pathsConflict(["src/"], ["tools/"])).toBeUndefined();
  });

  test("前缀互为父子冲突", () => {
    expect(pathsConflict(["out/a"], ["out/a/b"])).toBeDefined();
  });

  test("规范化父目录和符号链接后判断冲突", () => {
    const dir = mkdtempSync(join(tmpdir(), "sa-path-"));
    mkdirSync(join(dir, "src"));
    symlinkSync("src", join(dir, "alias"));
    expect(pathsConflict(["src/a/.."], ["alias/file"], dir)).toBeDefined();
    expect(pathsConflict(["src/../other"], ["src/file"], dir)).toBeUndefined();
  });

  test("拒绝路径穿越和指向外部的符号链接", () => {
    const dir = mkdtempSync(join(tmpdir(), "sa-path-"));
    mkdirSync(join(dir, "src"));
    symlinkSync(tmpdir(), join(dir, "src", "outside"));
    const report = { ...good, files: [{ path: "src/../other/file", change: "x" }] } as Report;
    expect(checkFiles("implementer", ["src"], report, dir).ok).toBe(false);
    report.files[0].path = "src/outside/file";
    expect(checkFiles("implementer", ["src"], report, dir).ok).toBe(false);
  });
});

describe("version", () => {
  test("版本形如 sa-<12 hex>", () => {
    expect(SA_VERSION).toMatch(/^sa-[0-9a-f]{12}$/);
  });

  test("不存在的目录也稳定(缺文件计入哈希)", () => {
    expect(deriveVersion("/nonexistent-xyz")).toMatch(/^sa-[0-9a-f]{12}$/);
    expect(deriveVersion("/nonexistent-xyz")).toBe(deriveVersion("/nonexistent-xyz"));
  });
});
