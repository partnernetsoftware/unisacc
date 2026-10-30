"""m4 四流：源 → 流 1…4 → \\0asm 字节。

流 1+2（lex, front）都在 compile_src 里；流 3+4（select, encode）在 emit_wasm 里。
实测：前端也会问 isel（front/compile.py 的 nullish 等），所以阶段边界是“记账上的”，
不是“网的独占”。运行时强制两条：
  1. 命中的网必须是合同里已知的网；
  2. 任何网的 gold 回退计数必须为 0，否则说明没有真正由网驱动。
"""
import os

from ujs.seed.construct.emit_wasm import can_emit_direct
from ujs.seed.construct.front.compile import compile_src
from ujs.seed.construct.oracle import Oracle

from .contract import NETS
from .emit import emit_wasm


def _snap(o):
    return {k: list(v) for k, v in o.stats.items()}


def _delta(a, b):
    out = {}
    for k, (nb, gb) in b.items():
        na, ga = a.get(k, [0, 0])
        if nb - na or gb - ga:
            out[k] = [nb - na, gb - ga]
    return out


def run(src, nets, out_path):
    """源串 → 网驱动编译 → out_path。返回 {wasm, bytes, trace}。"""
    o = Oracle(nets=nets, drive="built")
    s0 = _snap(o)
    fn = compile_src(src, o)
    s1 = _snap(o)
    ok, why = can_emit_direct(fn, o)
    if not ok:
        raise RuntimeError("m4: direct emit not covered: %s" % why)
    emit_wasm(fn, out_path, o)
    s2 = _snap(o)

    trace = {
        "front(lex+parse)": _delta(s0, s1),
        "select+encode": _delta(s1, s2),
    }
    _check_trace(trace)
    with open(out_path, "rb") as f:
        head = f.read(4)
    if head != b"\0asm":
        raise RuntimeError("m4: output is not \\0asm")
    return {"wasm": out_path, "bytes": os.path.getsize(out_path), "trace": trace}


def _check_trace(trace):
    # 命中须是已知网；是否“决策”由 selftest 按变异测量，不在这里判定。
    # 侧信道询问（type/shape/ic）允许被问到，但结果不影响输出。
    allowed = set(NETS)
    for phase, got in trace.items():
        for net, (hits, gold) in got.items():
            if net not in allowed:
                raise AssertionError("m4: phase %s hit unknown net %r" % (phase, net))
            if gold:
                raise AssertionError("m4: net %r fell back to gold %d time(s)" % (net, gold))
