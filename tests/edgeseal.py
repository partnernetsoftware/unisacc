#!/usr/bin/env python3
"""边级封存原件：按探针冻结「流段 F 向网 N 的规范化问答面」并回放判等。

叶 = tests/seals/edge-<流>-<网>/（manifest.tsv + asks.tsv）。冻结主体是排序去重的
(key, answer, refcount) 映射——参考路线（compile_file / optimise / lower 分相位）
对固定探针的边使用面；全序列已实测双跑确定，refcount 因此入判等。对生产端的
约束是组合推理：边原件（这条边消费了哪些键）× stages/stages-back（问过比对）×
oracle-cache（DENSE≡weights）× net-*-seal（weights≡gold）。设计依据与边界见
tests/SEAL-MODULES.md「边级」节。

  edgeseal.py --produce LEAF ROOT    # 从 manifest 的探针现录（不信任何缓存），写 asks.tsv
  edgeseal.py --gate    LEAF ROOT    # 回放判等 + 自校验 + 钉扎 + 三类负例必红
"""
from __future__ import annotations

import hashlib
import sys
from pathlib import Path

PHASES = ("front", "e4", "lower")
# 相位 -> 该相位允许出现的边流名（front 细分到网归属由 FLOW_OF_NET 决定）
FLOW_OF_NET = {
    "pp": "e2", "lex": "e1",
    "parse": "e3", "type": "e3", "scope": "e3", "irsel": "e3", "tyinfo": "e3",
    "pfconv": "e3", "binsel": "e3", "prec": "e3",
    "opinfo": "e4", "peep": "e4",
    "abi": "lower", "regmap": "lower", "reloc": "lower", "enc": "lower",
}


def esc(s: str) -> str:
    out = []
    for ch in s:
        if ch == "\\":
            out.append("\\\\")
        elif 0x20 <= ord(ch) <= 0x7E:
            out.append(ch)
        else:
            out.append("\\x%02x" % ord(ch))
    return "".join(out)


def serialise(key, answer) -> tuple[str, str]:
    k = esc("\x1f".join(key))
    if isinstance(answer, dict):
        a = esc("\x1f".join("%s=%s" % (h, answer[h]) for h in sorted(answer)))
    else:
        a = esc(str(answer))
    return k, a


def record(probe: str, target: str) -> list[tuple[str, str, tuple, object]]:
    """跑参考全程路线，逐问记录（相位, 网, key, answer）。"""
    sys.path.insert(0, ".")
    from unisa.__main__ import _built
    from unisa.oracle import Oracle
    from unisa.driver import compile_file
    from unisa.lower import lower
    from unisa import opt as _opt

    nets = _built()
    o = Oracle(nets, drive="built")
    phase = ["front"]
    rec: list[tuple[str, str, tuple, object]] = []
    orig = o.ask

    def ask(stage, key, _orig=orig):
        out = _orig(stage, key)
        rec.append((phase[0], stage, tuple(key), out))
        return out

    o.ask = ask
    _opt._ORACLE[:] = [o]
    _opt._INFO.clear()
    t = compile_file(probe, o, target)
    phase[0] = "e4"
    _opt.optimise(t.to_text(), 2, o)
    phase[0] = "lower"
    lower(t, target, o, drive="built")
    return rec


def edge_rows(rec, flow: str, net: str) -> list[str]:
    """规范化：该边的 (key, answer) 去重排序，refcount 为参考端问次。"""
    counts: dict[tuple[str, str], int] = {}
    for ph, stage, key, ans in rec:
        if stage != net:
            continue
        got_flow = FLOW_OF_NET[stage] if ph == "front" else ph
        if got_flow != flow:
            continue
        counts[serialise(key, ans)] = counts.get(serialise(key, ans), 0) + 1
    return ["%s\t%s\t%d" % (k, a, n) for (k, a), n in sorted(counts.items())]


def read_manifest(leaf: Path) -> dict[str, str]:
    m: dict[str, str] = {}
    for line in (leaf / "manifest.tsv").read_text().splitlines():
        if not line or line.startswith("#"):
            continue
        k, v = line.split("\t", 1)
        m[k] = v
    for want in ("flow", "net", "probe", "probe_sha256", "target", "table_sha256", "n_keys", "asks_sha256"):
        if want not in m:
            raise SystemExit("edgeseal: manifest missing field %s" % want)
    return m


def sha(p: Path) -> str:
    return hashlib.sha256(p.read_bytes()).hexdigest()


def body_sha(rows: list[str]) -> str:
    return hashlib.sha256(("\n".join(rows) + "\n").encode()).hexdigest()


def pins_ok(root: Path, m: dict[str, str]) -> int:
    bad = 0
    probe = root / m["probe"]
    if sha(probe) != m["probe_sha256"]:
        print("edgeseal: FAIL probe %s changed since recording (sha mismatch)" % m["probe"])
        bad += 1
    gold = root / ("weights/gold/%s.tsv" % m["net"])
    if sha(gold) != m["table_sha256"]:
        print("edgeseal: FAIL gold table %s evolved; re-record the edge leaf" % gold.name)
        bad += 1
    return bad


def gate(leaf: Path, root: Path) -> int:
    m = read_manifest(leaf)
    if pins_ok(root, m):
        return 1
    frozen = (leaf / "asks.tsv").read_text().splitlines()
    frozen = [l for l in frozen if l and not l.startswith("#")]
    if len(frozen) != int(m["n_keys"]):
        print("edgeseal: FAIL asks.tsv has %d rows, manifest says %s" % (len(frozen), m["n_keys"]))
        return 1
    if body_sha(frozen) != m["asks_sha256"]:
        print("edgeseal: FAIL asks.tsv bytes do not match manifest asks_sha256")
        return 1
    rec = record(str(root / m["probe"]), m["target"])
    live = edge_rows(rec, m["flow"], m["net"])
    if not live:
        print("edgeseal: FAIL probe never crossed edge %s->%s (nothing checked)" % (m["flow"], m["net"]))
        return 1
    if live != frozen:
        print("edgeseal: FAIL edge %s->%s replay differs from frozen asks.tsv" % (m["flow"], m["net"]))
        for line in list(frozen)[:3]:
            print("  frozen  %s" % line)
        for line in live[:3]:
            print("  replay  %s" % line)
        return 1
    # 负例：三类变异各自必红（删一行 / 改答案一字节 / 改键一字节）
    import re
    mut_drop = frozen[:-1]
    def flip(line: str, col: int) -> str:
        parts = line.split("\t")
        parts[col] = ("X" + parts[col][1:]) if parts[col][0] != "X" else ("Y" + parts[col][1:])
        return "\t".join(parts)
    mut_ans = [flip(frozen[0], 1)] + frozen[1:]
    mut_key = [flip(frozen[0], 0)] + frozen[1:]
    for name, mut in (("drop-row", mut_drop), ("flip-answer", mut_ans), ("flip-key", mut_key)):
        if live == mut:
            print("edgeseal: FAIL negative %s was NOT caught" % name)
            return 1
    print("edgeseal: edge %s->%s PASS probe %s: %d keys replayed byte-equal; negatives drop-row/flip-answer/flip-key all caught"
          % (m["flow"], m["net"], m["probe"], len(frozen)))
    return 0


def produce(leaf: Path, root: Path) -> int:
    m = read_manifest(leaf)
    rec = record(str(root / m["probe"]), m["target"])
    rows = edge_rows(rec, m["flow"], m["net"])
    if not rows:
        print("edgeseal: refuse to freeze an empty edge %s->%s (probe never crossed it)" % (m["flow"], m["net"]))
        return 1
    rows2 = edge_rows(record(str(root / m["probe"]), m["target"]), m["flow"], m["net"])
    if rows != rows2:
        print("edgeseal: refuse to freeze: two recordings differ (route not deterministic here)")
        return 1
    (leaf / "asks.tsv").write_text(
        "# frozen edge ask surface %s->%s probe %s -- written by edgeseal.py --produce; do not edit\n"
        "# key\tanswer\trefcount (sorted, deduplicated; refcount is the reference-route ask count)\n"
        % (m["flow"], m["net"], m["probe"]) + "\n".join(rows) + "\n")
    text = (leaf / "manifest.tsv").read_text()
    import re
    text = re.sub(r"(?m)^n_keys\t.*$", "n_keys\t%d" % len(rows), text)
    text = re.sub(r"(?m)^asks_sha256\t.*$", "asks_sha256\t%s" % body_sha(rows), text)
    text = re.sub(r"(?m)^probe_sha256\t.*$", "probe_sha256\t%s" % sha(root / m["probe"]), text)
    text = re.sub(r"(?m)^table_sha256\t.*$", "table_sha256\t%s" % sha(root / ("weights/gold/%s.tsv" % m["net"])), text)
    (leaf / "manifest.tsv").write_text(text)
    print("edgeseal: froze %d keys for edge %s->%s (double-recorded, byte-equal)" % (len(rows), m["flow"], m["net"]))
    return 0


def main() -> int:
    if len(sys.argv) != 4 or sys.argv[1] not in ("--gate", "--produce"):
        print(__doc__)
        return 2
    leaf, root = Path(sys.argv[2]), Path(sys.argv[3]).resolve()
    import os
    os.chdir(root)
    return gate(leaf, root) if sys.argv[1] == "--gate" else produce(leaf, root)


if __name__ == "__main__":
    sys.exit(main())
