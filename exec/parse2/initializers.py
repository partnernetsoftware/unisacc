"""Aggregate initializer delta procedures, shared by all storage classes.

The cursor is a scalar-slot index. Each brace level describes a subaggregate;
closing it advances to its end, while designators resolve in that level.
Member counts/offsets come from the parsed type layout, widths from ELSZ/STOREV.
This is the reference initaggr/slotat algorithm compiled into generic actions,
not a new executor primitive or a claim of complete C initializer semantics.
"""


import json
from pathlib import Path
from finite_rules import install as install_rules


def install(E, P, SBB, LOC, DIM, SSZ, SMN, SMEM, MOF, MSZ, MPT, MBS, MAR, SFLAT, MFLAT, MEMBER_STRIDE):
    bindings = dict(SBB=SBB, LOC=LOC, SSZ=SSZ, SMN=SMN, SMEM=SMEM, MOF=MOF, MPT=MPT, MBS=MBS,
                    MAR=MAR, SFLAT=SFLAT, MFLAT=MFLAT, MEMBER_STRIDE=MEMBER_STRIDE)
    bindings.update(PTR=E.PTR, BASE=E.BASE, ARR=E.ARR, DIM1=DIM + 1, DIM2=DIM + 2)
    root = Path(__file__).parent
    def rows(name):
        return [line.split("\t") for line in (root / ("initializers-" + name + ".tsv")).read_text().splitlines()[1:]]
    groups = {name: tuple(slots.split(",")) for name, slots in rows("context")}
    ctx = groups["context"]
    saved = ctx + groups["root"] + groups["extra"]
    sequences = dict(context_root=[("LDI", v, 0) for v in ctx if v != "ic_slots"],
                     context_save=[("COPYW", "ip_" + v[3:], v) for v in ctx],
                     context_reset=[("LDI", v, 0) for v in ctx])
    p = P("initializers.bindings")
    for name, method, slots in (("push_context", "vpush", ctx), ("pop_context", "vpop", ctx),
                                ("push_saved", "vpush", saved), ("pop_saved", "vpop", saved)):
        p.acts = []
        sequences[name] = getattr(p, method)(*slots).acts
    for prefix, kind, key in rows("fresh"):
        p.cur = prefix
        bindings[key] = p.fresh(kind)
    sequences.update((name, E.O(json.loads(text))) for name, text in rows("text"))
    sequences.update((name, E.rej(message)) for name, message in rows("reject"))
    classes = {name: [E.TK_ID if token == "identifier" else E.TK[token]] for name, token in rows("tokens")}
    install_rules(E.g, root, "initializers", bindings=bindings, sequences=sequences, classes=classes, section="main")
