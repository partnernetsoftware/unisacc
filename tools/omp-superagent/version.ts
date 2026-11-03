// 版本固定:自有源码内容哈希(改动即变);SA_VERSION 环境可覆盖(固定到 git tag)。
// 父旧子新混跑 = 两个不同版本值,证据必须带版本,不同版本结果不许混算。
import { createHash } from "node:crypto";
import { readFileSync } from "node:fs";
import { join } from "node:path";

export const SA_SOURCE_FILES = [
  "version.ts",
  "report.ts",
  "core.ts",
  "extension.ts",
] as const;

export function deriveVersion(dir: string): string {
  const h = createHash("sha256");
  for (const f of SA_SOURCE_FILES) {
    try {
      const b = readFileSync(join(dir, f));
      h.update(f).update("\0").update(b).update("\0");
    } catch {
      h.update(f).update("\0missing\0");
    }
  }
  return `sa-${h.digest("hex").slice(0, 12)}`;
}

const here = new URL(".", import.meta.url).pathname;
export const SA_VERSION = process.env.SA_VERSION ?? deriveVersion(here);
