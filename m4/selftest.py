"""m4 自检：网到底在不在决策。对每张接线网做两种“说谎”，在语料上重跑：

  shift：每个键的首个头换成下一个类。若输出字节变了或被拒收 → 该网决定了输出。
  trap ：若该网有 trap 类，则每个键都答 trap。若语料被拒收 → 该网能决定拒收。

每张网实测得到的决策种类由 EXPECT 钉死（2026-10-19 基线）。任何网的决策种类变化
（新增或消失）都使闸失败，逼着把 EXPECT 与实际同步改，不让“在决策”口径悄悄漂移。
"""
import glob
import os
import sys
import tempfile

from . import flows
from .__main__ import CORPUS, _ref_bytes
from .contract import wired
from .nets import build_nets

# 基线（实测）：lex、parse 的 shift 直接决定字节；isel、enc 只决定拒收（trap）；
# type/scope/shape/ic/reloc 在语料上 shift 与 trap 都不改变结果 —— 这是待补的缺口。
EXPECT = {
    "lex": {"shift"},
    "parse": {"shift"},
    "type": set(),
    "scope": set(),
    "shape": set(),
    "ic": set(),
    "isel": {"trap"},
    "enc": {"trap"},
    "reloc": set(),
}


class _Shift:
    """每个键的首个头换成下一个类（包一层，IntNet 有 __slots__）。"""

    def __init__(self, net):
        self._net = net
        self.exact = True
        self.heads = net.heads

    def predict(self, key):
        out = dict(self._net.predict(key))
        head, classes = self.heads[0]
        out[head] = classes[(classes.index(out[head]) + 1) % len(classes)]
        return out


class _Trap:
    """每个键都答 trap（仅当该网有 trap 类时使用）。"""

    def __init__(self, net):
        self.exact = True
        self.heads = net.heads

    def predict(self, key):
        return {h: ("trap" if "trap" in cl else cl[0]) for h, cl in self.heads}


def _programs():
    out = []
    for p in sorted(glob.glob(os.path.join(CORPUS, "*.ujs"))):
        with open(p) as f:
            src = f.read()
        out.append((p, src))
    return out


def _decided(nets, mutant_cls, net_name, programs, refs, td):
    """返回 True 若该变异改变了任一程序（字节不同或被拒收）。"""
    mut = dict(nets)
    mut[net_name] = mutant_cls(nets[net_name])
    out = os.path.join(td, "m.wasm")
    for p, src in programs:
        try:
            flows.run(src, mut, out)
        except Exception:
            return True
        with open(out, "rb") as f:
            if f.read() != refs[p]:
                return True
    return False


def measure():
    nets = build_nets(verify=False)
    programs = _programs()
    refs = {}
    with tempfile.TemporaryDirectory() as td:
        for p, src in programs:
            r = _ref_bytes(src, os.path.join(td, "r.wasm"))
            if r is None:
                raise RuntimeError("ref not direct-covered: %s" % p)
            refs[p] = r
        got = {}
        for n in wired():
            kinds = set()
            if _decided(nets, _Shift, n, programs, refs, td):
                kinds.add("shift")
            has_trap = any("trap" in cl for _, cl in nets[n].heads)
            if has_trap and _decided(nets, _Trap, n, programs, refs, td):
                kinds.add("trap")
            got[n] = kinds
    return got


def run(verbose=True):
    got = measure()
    bad = []
    for n in wired():
        ok = got[n] == EXPECT[n]
        if not ok:
            bad.append(n)
        if verbose:
            tag = "as expected" if ok else "CHANGED (expected %s)" % sorted(EXPECT[n])
            print("%-6s decides %-14s %s" % (n, "/".join(sorted(got[n])) or "nothing", tag))
    if verbose:
        print("selftest:", "PASS" if not bad else "FAIL changed: " + ",".join(bad))
    return not bad


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
