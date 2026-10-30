"""m4 网表：构造 10 张 IntNet（复用 ujs 的网注册表 gold.STAGES，不登记进 unisa 默认 18）。

acc = 构造 + 全键枚举 exact：每张网对每个 key 的整数预测 == gold 标签。
"""
from ujs.seed.construct.gold import STAGES as GOLD_NETS   # ujs gold calls its net registry STAGES; .py identifiers are not renamed (docs/glossary.md §4)
from unisa.intnet import build

from .contract import NETS


def build_nets(verify=True):
    return {n: build(n, verify=verify, stages=GOLD_NETS) for n in NETS}


def acc(nets):
    """穷举验收。返回 {net: (exact_keys, total_keys)}。"""
    out = {}
    for n in NETS:
        st = GOLD_NETS[n]
        net = nets[n]
        ok = tot = 0
        for kv in st.keys():
            tot += 1
            if net.predict(kv) == st.label(*kv):
                ok += 1
        out[n] = (ok, tot)
    return out
