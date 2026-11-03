// UI 层版本固定:自身源码 + 复用的 v1 核心一起哈希;SA2_VERSION 可覆盖。
import { createHash } from "node:crypto";
import { readFileSync } from "node:fs";
import { join } from "node:path";

const FILES = [
  "extension.ts",
  "ui.ts",
  "../omp-superagent/core.ts",
  "../omp-superagent/report.ts",
  "../omp-superagent/acceptance.ts",
  "../omp-superagent/worker.ts",
  "../omp-superagent/version.ts",
] as const;

function deriveVersion(dir: string): string {
  const h = createHash("sha256");
  for (const file of FILES) {
    try {
      h.update(file).update("\0").update(readFileSync(join(dir, file))).update("\0");
    } catch {
      h.update(file).update("\0missing\0");
    }
  }
  return `sa2-${h.digest("hex").slice(0, 12)}`;
}

const here = new URL(".", import.meta.url).pathname;
export const SA2_VERSION = process.env.SA2_VERSION ?? deriveVersion(here);
