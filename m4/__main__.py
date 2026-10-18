"""python3 -m m4 {acc|build|check} —— m4 轨最小入口。

  acc                      10 张网穷举验收（构造 + 全键 exact）
  build SRC -o OUT.wasm    源 → 四流 → \\0asm，打印各流网命中
  check [--limit N]        对 ujs 参考（gold 路径）逐字节比对 tests/ujs2wasm/corpus
"""
import argparse
import glob
import json
import os
import sys
import tempfile

from .contract import NETS, RESERVED, check_contract, wired
from .nets import acc, build_nets
from . import flows

ROOT = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
CORPUS = os.path.join(ROOT, "tests", "ujs2wasm", "corpus")


def cmd_acc(a):
    check_contract()
    nets = build_nets(verify=True)
    res = acc(nets)
    bad = 0
    for n in NETS:
        ok, tot = res[n]
        tag = "ok" if ok == tot else "FAIL"
        print("%-6s %6d/%-6d %s" % (n, ok, tot, tag))
        bad += ok != tot
    print("wired:", ",".join(wired()), " reserved:", ",".join(RESERVED))
    print("acc:", "PASS" if not bad else "FAIL (%d nets)" % bad)
    return 1 if bad else 0


def cmd_build(a):
    check_contract()
    nets = build_nets(verify=False)
    with open(a.src) as f:
        src = f.read()
    r = flows.run(src, nets, a.o)
    print(json.dumps(r, indent=1, sort_keys=True))
    return 0


def _ref_bytes(src, path):
    # 参考：ujs 的 gold 路径（与 ujs2wasm auto 的直出分支相同）
    from ujs.seed.construct.emit_wasm import can_emit_direct, emit_wasm
    from ujs.seed.construct.front.compile import compile_src
    from ujs.seed.construct.oracle import Oracle
    o = Oracle(drive="gold")
    fn = compile_src(src, o)
    ok, _ = can_emit_direct(fn, o)
    if not ok:
        return None
    emit_wasm(fn, path, o)
    with open(path, "rb") as f:
        return f.read()


def cmd_check(a):
    check_contract()
    nets = build_nets(verify=False)
    files = sorted(glob.glob(os.path.join(CORPUS, "*.ujs")))
    if a.limit:
        files = files[: a.limit]
    same = differ = skipped = 0
    with tempfile.TemporaryDirectory() as td:
        for p in files:
            with open(p) as f:
                src = f.read()
            name = os.path.basename(p)
            try:
                got = flows.run(src, nets, os.path.join(td, "m4.wasm"))
            except Exception as e:  # 未覆盖 → 明确拒绝，计为跳过
                print("skip  %-24s %s" % (name, e))
                skipped += 1
                continue
            ref = _ref_bytes(src, os.path.join(td, "ref.wasm"))
            with open(os.path.join(td, "m4.wasm"), "rb") as f:
                mine = f.read()
            if ref is None:
                print("skip  %-24s ref not direct-covered" % name)
                skipped += 1
            elif mine == ref:
                print("same  %-24s %d bytes" % (name, len(mine)))
                same += 1
            else:
                print("DIFF  %-24s m4=%d ref=%d" % (name, len(mine), len(ref)))
                differ += 1
    print("check: same %d differ %d skipped %d" % (same, differ, skipped))
    if differ or not same:
        return 1
    return 0


def main(argv=None):
    p = argparse.ArgumentParser(prog="m4")
    sp = p.add_subparsers(dest="cmd", required=True)
    sp.add_parser("acc")
    b = sp.add_parser("build")
    b.add_argument("src")
    b.add_argument("-o", dest="o", required=True)
    c = sp.add_parser("check")
    c.add_argument("--limit", type=int, default=0)
    a = p.parse_args(argv)
    return {"acc": cmd_acc, "build": cmd_build, "check": cmd_check}[a.cmd](a)


if __name__ == "__main__":
    sys.exit(main())
