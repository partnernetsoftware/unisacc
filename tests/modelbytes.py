#!/usr/bin/env python3
"""Keep the measured artifact's functional byte ledger in PRD reproducible.
Snapshot check is offline; --artifact also verifies its exact size and digest.
No size ceiling: this is attribution, not a reason to remove functionality.
"""
import argparse
import collections
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BEGIN = "<!-- model-bytes:begin -->"
END = "<!-- model-bytes:end -->"
FUNCTIONS = {
    "e2": "预处理、目标预定义宏与位置模式",
    "e1": "词法与 token/位置输出",
    "e3": "解析、类型/作用域、tape、错误与警告",
    "e4": "O1/O2 优化",
    "lower": "ABI、调用、目标指令 lowering 与数据布局",
    "elf": "目标指令编码及 ELF/Mach-O/PE 镜像写出",
    "tokenpp": "公开 token 路线的预处理",
    "tokenlex": "公开 token 路线的词法输出",
    "units": "多文件分帧与文件级 static 隔离",
}


def render(evidence):
    a = evidence["artifact_ledger"]
    total = a["total_bytes"]
    parts = a["package_parts"]
    assert sum(parts.values()) == a["package_bytes"]
    assert a["package_offset"] + a["package_bytes"] + a["footer_bytes"] == total
    assert sum(m["bytes"] for m in a["models"]) == parts["model bodies"]
    assert sum(a["network_record_bytes"].values()) == parts["model bodies"]
    assert len(a["models"]) == a["unique_models"]
    assert len({m["sha256"] for m in a["models"]}) == len(a["models"])
    assert sum(m["refs"] for m in a["models"]) == a["stage_rows"]
    assert sum(m["bytes"] * m["refs"] for m in a["models"]) == a["logical_model_bytes"]
    def row(name, n):
        return f"| {name} | {n:,} | {n / total * 100:.2f}% |"
    lines = [f"快照 SHA-256：`{a['sha256']}`；总计 **{total:,} B**。", "",
             "| 物理内容 | 字节 | 占整个 .com |", "|---|---:|---:|"]
    for name, n in [(f"{a['unique_models']} 个共享网络体", parts["model bodies"]),
                    ("平台驱动、APE 启动/加载与对齐（混合账）", a["package_offset"]),
                    (f"{sum(r['key'].startswith(chr(0) + 'hdr/') for r in a['resources'])} 份 C 头文件/库实现源码", parts["C header bodies"]),
                    ("两 ISA 通用推理执行核资源", parts["kernel bodies"]),
                    ("目录、记录头与资源键", a["package_bytes"] - parts["model bodies"] - parts["C header bodies"] - parts["kernel bodies"]),
                    ("尾部", a["footer_bytes"])]:
        lines.append(row(name, n))
    grouped = collections.defaultdict(lambda: [0, 0, 0])
    for m in a["models"]:
        assert len(m["stages"]) == 1 and m["stages"][0] in FUNCTIONS
        v = grouped[m["stages"][0]]
        v[0] += 1; v[1] += m["bytes"]; v[2] += m["refs"]
    assert set(grouped) == set(FUNCTIONS)
    lines += ["", "| 模型阶段 / 具体功能 | 物理模型数 | 模型体 B | 占 .com | 阶段行引用数 |",
              "|---|---:|---:|---:|---:|"]
    for stage, meaning in FUNCTIONS.items():
        n, size, refs = grouped[stage]
        lines.append(f"| `{stage}`：{meaning} | {n} | {size:,} | {size / total * 100:.2f}% | {refs} |")
    lines += ["", "下表是网络体的内部拆分，与上表重叠，不能再相加：", "",
              "| 网络记录 / 含义 | 字节 | 占整个 .com |", "|---|---:|---:|"]
    meanings = {"H": "阈值/选择网络参数记录", "Q": "动作序列声明（包含编译模板动作）",
                "S": "字节字符串声明", "N": "网络头记录"}
    assert set(a["network_record_bytes"]) == set(meanings)
    for tag, meaning in meanings.items():
        lines.append(row(f"`{tag}`：{meaning}", a["network_record_bytes"][tag]))
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true")
    parser.add_argument("--artifact", type=Path)
    args = parser.parse_args()
    evidence = json.loads((ROOT / "research/s17-final-evidence.json").read_text())
    text = render(evidence)
    if args.artifact:
        raw = args.artifact.read_bytes()
        a = evidence["artifact_ledger"]
        assert len(raw) == a["total_bytes"] and hashlib.sha256(raw).hexdigest() == a["sha256"], "artifact differs from measured snapshot"
    prd = ROOT / "prd.md"
    old = prd.read_text()
    assert old.count(BEGIN) == old.count(END) == 1, "missing/duplicate ledger marker"
    start = old.index(BEGIN) + len(BEGIN)
    end = old.index(END, start)
    new = old[:start] + "\n" + text + old[end:]
    if args.write:
        prd.write_text(new)
    else:
        assert old == new, "PRD model byte ledger is stale; run tests/modelbytes.py --write"
    print("model byte ledger: physical sums, sharing and functional attribution match")


if __name__ == "__main__":
    main()
