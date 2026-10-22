"""m4 自检：网到底在不在决策。对每张接线网做两种“说谎”，在语料上重跑：

  shift：每个键的首个头换成下一个类。若输出字节变了或被拒收 → 该网决定了输出。
  trap ：若该网有 trap 类，则每个键都答 trap。若语料被拒收 → 该网能决定拒收。

每张网实测得到的决策种类由 EXPECT 钉死（2026-10-19 基线）。任何网的决策种类变化
（新增或消失）都使闸失败，逼着把 EXPECT 与实际同步改，不让“在决策”口径悄悄漂移。
"""
import glob
import os
import subprocess
import sys
import tempfile

from . import flows
from .__main__ import CORPUS, PROBES, ROOT, _ref_bytes
from .contract import ISEL_CHOICE, NETS
from .nets import build_nets

# 基线（实测）。全部 10 张网都测，漂移即失败。
# 决策类：lex、parse 的 shift 改字节或拒收；isel、enc 的 shift/trap 拒收。
# 未决策类：type、shape、ic（侧信道，答案被丢弃）、scope（分支不可达）、irsel、reloc（不在路径上）。
EXPECT = {
    "lex": {"shift"},
    "parse": {"shift"},
    "type": set(),
    "scope": set(),
    "shape": set(),
    "ic": set(),
    "irsel": set(),
    "isel": {"shift", "trap"},
    "enc": {"shift", "trap"},
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
    for p in sorted(glob.glob(os.path.join(CORPUS, "*.ujs"))) + sorted(glob.glob(os.path.join(PROBES, "*.ujs"))):
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
        for n in NETS:
            kinds = set()
            if _decided(nets, _Shift, n, programs, refs, td):
                kinds.add("shift")
            has_trap = any("trap" in cl for _, cl in nets[n].heads)
            if has_trap and _decided(nets, _Trap, n, programs, refs, td):
                kinds.add("trap")
            got[n] = kinds
    return got


class _Force:
    """把某一个 isel 键改答成指定形态，其余键仍走原网。"""

    def __init__(self, net, key, answer):
        self._net = net
        self._key = key
        self._answer = answer
        self.exact = True
        self.heads = net.heads

    def predict(self, key):
        out = dict(self._net.predict(key))
        if tuple(key) == self._key:
            out[self.heads[0][0]] = self._answer
        return out


def _choose_proof(nets):
    """eq/ne：两个合法形态都出 wasm，字节不同；gold 形态与参考逐字节相同。

    探针是 corpus/ternary.ujs（只有 eq，没有会被 shift 拒收的别的算术 op）。
    两条都在 Node 里求值得 100，说明备选不是拒收、也不是坏模块。
    """
    src_path = os.path.join(CORPUS, "ternary.ujs")
    with open(src_path) as f:
        src = f.read()
    run_mjs = os.path.join(ROOT, "tests", "ujs2wasm", "run_wasm.mjs")
    with tempfile.TemporaryDirectory() as td:
        gold_path = os.path.join(td, "gold.wasm")
        alt_path = os.path.join(td, "alt.wasm")
        ref = _ref_bytes(src, os.path.join(td, "ref.wasm"))
        flows.run(src, nets, gold_path)
        with open(gold_path, "rb") as f:
            gold = f.read()
        if gold != ref:
            raise AssertionError("m4 choose: gold path drifted from ujs reference")
        alt_nets = dict(nets)
        alt_nets["isel"] = _Force(nets["isel"], ("eq",), ISEL_CHOICE["eq"][1])
        flows.run(src, alt_nets, alt_path)
        with open(alt_path, "rb") as f:
            alt = f.read()
        if alt == gold:
            raise AssertionError("m4 choose: f64_cmp emitted the same bytes as i64_cmp")
        for label, path in (("i64_cmp", gold_path), ("f64_cmp", alt_path)):
            got = subprocess.check_output(
                ["node", run_mjs, path], text=True).strip()
            if got != "100":
                raise AssertionError("m4 choose: %s ran as %r, want 100" % (label, got))
    return True


def run(verbose=True):
    choose_ok = True
    try:
        _choose_proof(build_nets(verify=False))
        if verbose:
            print("isel   choose eq i64_cmp|f64_cmp  bytes differ, both run 100")
    except Exception as e:
        choose_ok = False
        if verbose:
            print("isel   choose FAIL:", e)
    got = measure()
    bad = []
    for n in NETS:
        ok = got[n] == EXPECT[n]
        if not ok:
            bad.append(n)
        if verbose:
            tag = "as expected" if ok else "CHANGED (expected %s)" % sorted(EXPECT[n])
            print("%-6s decides %-14s %s" % (n, "/".join(sorted(got[n])) or "nothing", tag))
    if verbose:
        print("selftest:", "PASS" if not bad else "FAIL changed: " + ",".join(bad))
    return choose_ok and not bad


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
