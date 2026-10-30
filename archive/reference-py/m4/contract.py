"""m4 流×网合同。依据：research/m4-flow-net-contract-draft.md §2–§4，以及实测（见 README）。

10 张网全部构造并穷举验收（acc）。但并非每张网都在决策，合同按实测分三类：

  FLOWS    ：接线且真决定输出或拒收（4 张：lex、parse、isel、enc）。
  RESERVED ：不在决策路径上，保留（irsel、reloc、scope）。理由见下。
  OBSERVED ：前端只做侧信道观测，答案被丢弃（type、shape、ic）。

这是对草案“4 流 × 10 网”的实测修订：只有 4 张网真的决定输出，其余 6 张不是。
口径是否改由政委定；本文件只如实记录。
"""

# 10 张网，与 ujs.seed.construct.gold.STAGES（即 ujs 的网注册表）同名同序。
NETS = ("lex", "parse", "type", "scope", "shape", "irsel", "ic", "isel", "enc", "reloc")

# 4 流：名字 → 决策网。流 1+2 在 ujs 前端里是同一次 compile，运行时按阶段记账。
FLOWS = (
    ("lex", ("lex",)),
    ("front", ("parse",)),
    ("select", ("isel",)),
    ("encode", ("enc",)),
)

# irsel：ujs 中无 ask("irsel")。
# reloc：直出路径（emit_wasm）从不询问，只有 lower_wasm 询问。
# scope：front.bind 只检查 act == "illegal"，而调用点的 ctx 恒为 mod/block，该分支不可达。
RESERVED = ("irsel", "reloc", "scope")

# type、shape、ic：compile.note_binop 以“侧信道”发出这些询问（注释写明为 web 热图），
# 结果被丢弃，不影响任何输出。
OBSERVED = ("type", "shape", "ic")

# 同一 op 的多个合法 isel 形态。第一项是 gold 形态（语料逐字节路径）；
# 其余项是网可以改选的另一实现，emit 必须写出不同的 WAT，不得只做合法/拒收。
# f64_cmp 是 WASM_FORMS 里紧挨 i64_cmp 的下一类，shift 变异会落到它。
ISEL_CHOICE = {
    "eq": ("i64_cmp", "f64_cmp"),
    "ne": ("i64_cmp", "f64_cmp"),
}


def wired():
    """真决定输出或拒收的网（4 张）。"""
    return tuple(n for _, ns in FLOWS for n in ns)


def check_contract():
    """每张网必须恰好归入一类；类之间不重叠，名字都在 NETS 里。"""
    buckets = [wired(), RESERVED, OBSERVED]
    seen = [n for b in buckets for n in b]
    if sorted(seen) != sorted(NETS) or len(seen) != len(set(seen)):
        raise AssertionError("m4 contract: nets not partitioned exactly: %r" % seen)
    for op, forms in ISEL_CHOICE.items():
        if len(forms) < 2 or len(set(forms)) != len(forms):
            raise AssertionError("m4 contract: %s needs two distinct forms" % op)
        if forms[0] == forms[1]:
            raise AssertionError("m4 contract: choice collapsed")
    return True
