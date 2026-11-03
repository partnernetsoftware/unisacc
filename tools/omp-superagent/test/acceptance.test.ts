import { describe, expect, test } from "bun:test";
import { mkdtempSync, mkdirSync, rmSync, symlinkSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { checkAcceptance } from "../acceptance.ts";

describe("checkAcceptance", () => {
  test("检查宿主文件存在及完整文本", () => {
    const root = mkdtempSync(join(tmpdir(), "acceptance-"));
    try {
      mkdirSync(join(root, "src"));
      writeFileSync(join(root, "src", "a.txt"), "你好\n");
      expect(checkAcceptance(root, [{ kind: "exists", path: "src/a.txt" }, { kind: "text", path: "src/a.txt", text: "你好\n" }])).toEqual({ ok: true });
      expect(checkAcceptance(root, [{ kind: "text", path: "src/a.txt", text: "你好" }])).toMatchObject({ ok: false, err: expect.stringContaining("不匹配") });
      expect(checkAcceptance(root, [{ kind: "exists", path: "missing" }])).toMatchObject({ ok: false, err: expect.stringContaining("读取失败") });
      expect(checkAcceptance(root, [{ kind: "exists", path: "src" }])).toMatchObject({ ok: false, err: expect.stringContaining("普通文件") });
    } finally { rmSync(root, { recursive: true, force: true }); }
  });

  test("拒绝越界和绝对路径", () => {
    const root = mkdtempSync(join(tmpdir(), "acceptance-"));
    try {
      for (const path of ["../outside", join(root, "file"), "."]) {
        expect(checkAcceptance(root, [{ kind: "exists", path }])).toMatchObject({ ok: false, err: expect.stringContaining("工作目录内") });
      }
    } finally { rmSync(root, { recursive: true, force: true }); }
  });

  test("拒绝文件和中间目录符号链接", () => {
    const root = mkdtempSync(join(tmpdir(), "acceptance-"));
    const outside = mkdtempSync(join(tmpdir(), "acceptance-outside-"));
    try {
      writeFileSync(join(outside, "file"), "ok");
      symlinkSync(join(outside, "file"), join(root, "link"));
      symlinkSync(outside, join(root, "dir"));
      for (const path of ["link", "dir/file"]) {
        expect(checkAcceptance(root, [{ kind: "exists", path }])).toMatchObject({ ok: false, err: expect.stringContaining("符号链接") });
      }
    } finally {
      rmSync(root, { recursive: true, force: true });
      rmSync(outside, { recursive: true, force: true });
    }
  });

  test("拒绝无效规则和非 UTF-8 文本", () => {
    const root = mkdtempSync(join(tmpdir(), "acceptance-"));
    try {
      for (const checks of [[], null, [{}], [{ kind: "text", path: "a" }], [{ kind: "exists", path: "a\0b" }]]) {
        expect(checkAcceptance(root, checks).ok).toBe(false);
      }
      writeFileSync(join(root, "binary"), Buffer.from([0xff]));
      expect(checkAcceptance(root, [{ kind: "text", path: "binary", text: "�" }]).ok).toBe(false);
    } finally { rmSync(root, { recursive: true, force: true }); }
  });
});
