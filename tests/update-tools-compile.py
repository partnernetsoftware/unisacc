#!/usr/bin/env python3
"""显式生成审阅候选，不安装、不隐式批准库存。"""
import pathlib
import subprocess
import sys


def batch(i, tier=None):
    suffix = f"\t{tier}" if tier else ""
    return (f"tools-compile-{i:03d}\tIMPLEMENTED\tcontract\ttests/tools-compile.cx\tSOURCE\tlnx/x86_64\t55\t-\ttests/tools-compile.cx\t{{root}};{{out}};{i}\ttools.compile-all\trc0:TOOLS_COMPILE\tproduct{suffix}\n")


def main():
    if len(sys.argv) != 3:
        raise SystemExit("用法: python3 tests/update-tools-compile.py ROOT NEW_DIR")
    root = pathlib.Path(sys.argv[1]).resolve()
    out = pathlib.Path(sys.argv[2]).resolve()
    out.mkdir(mode=0o700)
    manifest = out / "tools-compile-sources.tsv"
    subprocess.run(["sh", str(root / "unisacc.com"), str(root / "tests/tools-compile.cx"), "--", "--register", str(root), str(manifest)], check=True, timeout=55)
    count = len(manifest.read_bytes().splitlines())
    shards = (count + 7) // 8
    lines = (root / "tests/batches.tsv").read_text().splitlines(keepends=True)
    first = next(i for i, line in enumerate(lines) if line.startswith("tools-compile-"))
    tier = None
    if lines[0].rstrip("\n").endswith("\ttier"):
        tiers = {line.rstrip("\n").split("\t")[-1]
                 for line in lines if line.startswith("tools-compile-")}
        if len(tiers) != 1 or not tiers <= {"commit", "daily", "release"}:
            raise SystemExit("工具编译片档位不一致，拒绝隐式重分档")
        tier = tiers.pop()
    kept = [line for line in lines if not line.startswith("tools-compile-")]
    kept[first:first] = [batch(i, tier) for i in range(shards)]
    (out / "batches.tsv").write_text("".join(kept))
    # coverage只描述动态契约；实际数量由source清单和batch结构行给出。
    (out / "coverage.tsv").write_bytes((root / "tests/coverage.tsv").read_bytes())
    print(f"待人工审阅：{out}；{count}源/{shards}片。普通检查不会安装候选。")


if __name__ == "__main__":
    main()
