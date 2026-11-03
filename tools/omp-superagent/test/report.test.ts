import { describe, expect, test } from "bun:test";
import { checkFiles, parseReport, pathsConflict, reconcileVerified, type Report } from "../report.ts";
import { deriveVersion, SA_VERSION } from "../version.ts";

const good = {
  t: "report",
  ticket: "T1",
  state: "done",
  summary: "加了 X",
  verified: ["bun test -> 3 pass"],
  inferred: [],
  files: [{ path: "src/a.ts", change: "新增函数" }],
  blocked: "",
  next: [],
  tokens: 10,
  ms: 5,
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
});

describe("reconcileVerified", () => {
  const rep = {
    ...good,
    verified: ["bun test -> 3 pass", "读到 src/a.ts:1"],
    inferred: [],
  } as Report;

  test("全部逐字命中则原样保留", () => {
    const ev = "log: bun test -> 3 pass\ncat: 读到 src/a.ts:1";
    const r = reconcileVerified(rep, ev);
    expect(r.ok).toBe(true);
    expect(r.report?.verified.length).toBe(2);
    expect(r.droppedVerified?.length).toBe(0);
  });

  test("未命中项降级到 inferred", () => {
    const ev = "log: bun test -> 3 pass";
    const r = reconcileVerified(rep, ev);
    expect(r.report?.verified).toEqual(["bun test -> 3 pass"]);
    expect(r.report?.inferred).toEqual(["[未核到] 读到 src/a.ts:1"]);
    expect(r.droppedVerified).toEqual(["读到 src/a.ts:1"]);
  });

  test("done 且全部核不到则判失败", () => {
    expect(reconcileVerified(rep, "无关日志").ok).toBe(false);
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
