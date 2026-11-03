import { closeSync, constants, fstatSync, lstatSync, openSync, readFileSync } from "node:fs";
import { isAbsolute, relative, resolve, sep } from "node:path";
import { TextDecoder } from "node:util";

export type AcceptanceCheck =
  | { kind: "exists"; path: string }
  | { kind: "text"; path: string; text: string };

export type AcceptanceResult = { ok: true } | { ok: false; err: string };

// 验收仅反映检查时的文件状态，不能保证之后不变。
// 逐段 lstat 和末项 O_NOFOLLOW 拒绝符号链接，但并发替换中间目录仍有 TOCTOU 风险；
// 调用方必须确保验收期间目录树不被不可信进程修改。
export function checkAcceptance(cwd: string, checks: unknown): AcceptanceResult {
  if (!Array.isArray(checks) || checks.length === 0) return { ok: false, err: "验收条件必须是非空数组" };
  if (typeof cwd !== "string" || !cwd) return { ok: false, err: "工作目录无效" };
  const root = resolve(cwd);
  for (let index = 0; index < checks.length; index++) {
    const check = checks[index];
    if (!check || typeof check !== "object" || Array.isArray(check) ||
        (check.kind !== "exists" && check.kind !== "text") ||
        typeof check.path !== "string" || !check.path || check.path.includes("\0") ||
        (check.kind === "text" && typeof check.text !== "string")) {
      return { ok: false, err: `验收条件 ${index + 1} 格式无效` };
    }
    const path = check.path as string;
    const target = resolve(root, path);
    const rel = relative(root, target);
    if (isAbsolute(path) || !rel || rel === ".." || rel.startsWith(`..${sep}`) || isAbsolute(rel)) {
      return { ok: false, err: `验收路径不在工作目录内: ${path}` };
    }
    let fd: number | undefined;
    try {
      // 检查每个组成部分；不能防止检查后中间目录被并发替换。
      let current = root;
      for (const part of rel.split(sep)) {
        current = resolve(current, part);
        if (lstatSync(current).isSymbolicLink()) return { ok: false, err: `验收路径包含符号链接: ${path}` };
      }
      fd = openSync(target, constants.O_RDONLY | constants.O_NOFOLLOW | constants.O_NONBLOCK);
      if (!fstatSync(fd).isFile()) return { ok: false, err: `验收路径不是普通文件: ${path}` };
      if (check.kind === "text") {
        const text = new TextDecoder("utf-8", { fatal: true }).decode(readFileSync(fd));
        if (text !== check.text) return { ok: false, err: `验收文件文本不匹配: ${path}` };
      }
    } catch (error) {
      const detail = error instanceof Error ? error.message : String(error);
      return { ok: false, err: `验收文件读取失败: ${path}: ${detail}` };
    } finally {
      if (fd !== undefined) closeSync(fd);
    }
  }
  return { ok: true };
}
