"""m4 流×网合同。依据：research/m4-flow-net-contract-draft.md §2–§4，以及实测（见 README）。

10 张网全部构造并穷举验收（acc）；流接线其中 9 张。
irsel：表核保留、运行不接线（§4①，草案默认）。ujs 里没有任何 ask("irsel")，实测证实。
ic：实测被前端 compile 询问（front/compile.py 的 ic 取值），故接入 front 流，不再是可选。
"""

# 10 张网，与 ujs.seed.construct.gold.STAGES 同名同序。
NETS = ("lex", "parse", "type", "scope", "shape", "irsel", "ic", "isel", "enc", "reloc")

# 4 流：名字 → 绑定的网。流 1+2 在 ujs 前端里是同一次 compile，运行时按阶段记账。
FLOWS = (
    ("lex", ("lex",)),
    ("front", ("parse", "type", "scope", "shape", "ic")),
    ("select", ("isel",)),
    ("encode", ("enc", "reloc")),
)

RESERVED = ("irsel",)


def wired():
    """流接线的网（9 张）。"""
    return tuple(n for _, ns in FLOWS for n in ns)


def check_contract():
    """合同自检：每张网要么接线，要么在 RESERVED 里说明。"""
    w = set(wired())
    for n in NETS:
        if n not in w and n not in RESERVED:
            raise AssertionError("m4 contract: net %r unwired and unexplained" % n)
    for n in w:
        if n not in NETS:
            raise AssertionError("m4 contract: flow names unknown net %r" % n)
    return True
