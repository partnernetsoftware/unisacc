"""E4: the tape optimiser as a delta for the generic executor (exec/pp/sim.py,
exec/c/run.c).  Input: an -O0 tape (the text E3 writes); output: the -O1 tape
the reference writes (src/opt.c opt_stack / unisa/opt.py).

    python3 exec/opt/gen.py delta.json

-O1 is opt_round() at optlevel 1, up to four rounds, each over the whole tape:
    .frame 8 / store64 [r7+0], rX / M... / load64 rY, [r7+0] / .frame -8
becomes `mov rY, rX` (left out when X == Y) followed by M, when the pop is at
most line i+18, every line of M is `simple` (the opinfo table's answer for its
op word) and does not name r7, no line of M names rY, and M is empty when
X == Y.  Anything else is copied as it stands.  A round that rewrites nothing
ends the pass.

-O2 (gen.py delta.json 2) adds, per round, the per-line facts, blocks and labels,
the liveness of r2..r5 and with it the carry through r3..r5 and ol_local; then
up to four peep rounds: liveness of r0..r5, stfuse, and each line's relation
with its neighbour, the action for which is the peep table's (loaded at START).
This is the optimiser's algorithm compiled into an action table. The complete
SKIPL/COPYL/SIMPLE/NAMES scans live in scans-{byte,result}.tsv, LOCAL in
local-{byte,result}.tsv, STFUSE in stfuse-{byte,result}.tsv, and PPASS/BCLS/REAL
in peep-{byte,result}.tsv. ANALYZE/SOLVE/SCAN/DEADQ live in analysis-{byte,result}.tsv;
tape operand parsers and REREG live in parsers-{byte,result}.tsv; outer pass
control and stack matching live in rounds-{byte,result}.tsv.
Dynamic peep/opinfo data assembly remains here; src/opt.c and unisa/opt.py
stay the behaviour reference.

The opinfo table is read, not copied: its `simple` column is interned into a
set at START.  Everything else is the byte-level control of the rule.
"""
import importlib.util
import json
import os
import sys

_spec = importlib.util.spec_from_file_location(
    "e3gen", os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "parse", "gen.py"))
E = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(E)
g, P = E.g, E.P
from finite_rules import install as install_rules

SIMPLE = 32 * 10 ** 6            # SIMPLE[intern id of an op word] = 1 when opinfo says simple
LEVEL = int(sys.argv[2]) if len(sys.argv) > 2 else 1
# -O2's per-round line facts (ol_prep) and block liveness (bl_split, bl_solve), by line / block index
KK, RMM, WMM, TGG, BLOF, TSS, TEE, BSS = (41 * 10 ** 6, 42 * 10 ** 6, 43 * 10 ** 6, 44 * 10 ** 6,
                                         45 * 10 ** 6, 46 * 10 ** 6, 47 * 10 ** 6, 48 * 10 ** 6)
LABB = 100 * 10 ** 6             # LAB[round * 2e6 + intern id] = line + 1 of the first `name:` line (rounds < 10)
LIVEB = 200 * 10 ** 6            # LIVE[(round * 6 + z) * 1e6 + block]
LSS, LEE, WIDD, FRR, ISLAB = (51 * 10 ** 6, 52 * 10 ** 6, 53 * 10 ** 6, 54 * 10 ** 6, 55 * 10 ** 6)   # per line
ZOKB = 59 * 10 ** 6              # ZOK[z]
K_SIMPLE, K_LABEL, K_RET, K_JUMP, K_JUMPZ, K_CALL, K_FRAME, K_OTHER = range(8)
MAXJ = 16                        # the pop is line i+2+cnt, cnt <= 16: at most line i+18
WORDMAX = 15                     # ol_opi reads at most 15 bytes of the op word


def procs():
    # Preserve downstream fresh names while the complete scan rules live in TSV.
    bindings = {name: P(state).fresh("b") for name, state in (
        ("word_limit", "SIM.k"), ("word_empty", "SIM.e2"),
        ("word_simple", "SIM.f"), ("register_equal", "NMD.c"))}
    install_rules(g, os.path.dirname(__file__), "scans",
                  bindings=dict(bindings, WORDMAX=WORDMAX, SIMPLE=SIMPLE))


def analysis():
    # Ordered metadata retains all existing state names; analysis control is in TSV.
    bindings = {}
    for line in open(os.path.join(os.path.dirname(__file__), "analysis-names.tsv")):
        if not line.startswith("#"):
            name, prefix, kind = line.rstrip("\n").split("\t")
            bindings[name] = P(prefix).fresh(kind)
    bindings.update({name: globals()[name] for name in (
        "SIMPLE", "KK", "RMM", "WMM", "TGG", "BLOF", "TSS", "TEE", "BSS",
        "LABB", "LIVEB", "LSS", "LEE", "WIDD", "FRR", "ISLAB", "ZOKB", "WORDMAX",
        "K_SIMPLE", "K_LABEL", "K_RET", "K_JUMP", "K_JUMPZ", "K_CALL", "K_FRAME", "K_OTHER")})
    bindings["VS"] = E.VS
    install_rules(g, os.path.dirname(__file__), "analysis", bindings=bindings,
                  classes={name: [value] for name, value in bindings.items() if name.startswith("K_")})


def local():
    # Allocate the original branch/call names; all LOCAL control is declared in TSV.
    bindings = {name: P(state).fresh(kind) for name, state, kind in (
        ("bounds", "LOCAL", "b"), ("load32_dest", "LC", "b"),
        ("load64_dest", "LC", "b"), ("base_register", "LC", "b"),
        ("load_tail", "LC", "b"), ("dead_return", "LC", "r"),
        ("dead_result", "LC", "b"), ("load_emit", "LC", "b"))}
    install_rules(g, os.path.dirname(__file__), "local", bindings=bindings)


# ---- -O2's second half: the peep table's rounds (unisa/opt.py _Peep, src/opt.c peep_round) ----
PFIELDS = {}
for _ln in open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "weights", "gold", "peep.tsv")):
    _f = _ln.rstrip("\n").split("\t")
    if _ln.startswith("#field") or _ln.startswith("#head"):
        PFIELDS[_f[1]] = _f[2:]
PA, PB, PR, PY = PFIELDS["a"], PFIELDS["b"], PFIELDS["rel"], PFIELDS["y"]
PEEPB, ACLSB, BCLSB = 300 * 10 ** 6, 301 * 10 ** 6, 302 * 10 ** 6   # PEEP[(a*|b| + b)*|rel| + rel] = y index + 1


def parsers():
    # Preserve branch names and the shared PRN continuation; matching lives in TSV.
    specs = (("PL_b1", "PL", "b"), ("PI_b2", "PI", "b"),
             ("RR_b3", "RR", "b"), ("RR_b4", "RR", "b"),
             ("RR_b5", "RR", "b"), ("RR_r6", "RR", "r"), ("RR_b7", "RR", "b"))
    bindings = {name: P(prefix).fresh(kind) for name, prefix, kind in specs}
    install_rules(g, os.path.dirname(__file__), "parsers", bindings=bindings)


def stfuse():
    # Keep the original fresh names and shared DEADQ/NAMES/PRN continuations.
    specs = (("bounds", "STFUSE", "b"), ("window", "SF", "b"),
             ("end", "SF", "b"), ("base", "SF", "b"),
             ("value_alias", "SF", "b"), ("value_r2", "SF", "b"),
             ("store_tail", "SF", "b"), ("dead_a_return", "SF", "r"),
             ("dead_a", "SF", "b"), ("dead_r2_return", "SF", "r"),
             ("dead_r2", "SF", "b"), ("store_emit", "SF", "b"),
             ("print_st_return", "SF", "r"), ("print_store_return", "SF", "r"),
             ("simple", "SF", "b"), ("names_a_return", "SF", "r"),
             ("names_a", "SF", "b"), ("names_r2_return", "SF", "r"),
             ("names_r2", "SF", "b"))
    bindings = {name: P(state).fresh(kind) for name, state, kind in specs}
    install_rules(g, os.path.dirname(__file__), "stfuse",
                  bindings=dict(bindings, LSS=LSS, KK=KK, K_SIMPLE=K_SIMPLE))


def peepround():
    # Fresh names are assembler metadata; all fixed PPASS/BCLS/REAL rules are in TSV.
    bindings = {}
    for line in open(os.path.join(os.path.dirname(__file__), "peep-names.tsv")):
        if not line.startswith("#"):
            name, prefix, kind = line.rstrip("\n").split("\t")
            bindings[name] = P(prefix).fresh(kind)
    bindings.update({name: globals()[name] for name in (
        "LSS", "WIDD", "TSS", "TEE", "ISLAB", "KK", "RMM", "WMM", "FRR",
        "ACLSB", "BCLSB", "PEEPB", "K_LABEL", "K_SIMPLE")})
    for prefix, values in (("PA", PA), ("PB", PB), ("PR", PR)):
        bindings.update((prefix + "_" + name, index) for index, name in enumerate(values))
    bindings.update(PB_size=len(PB), PR_size=len(PR))
    install_rules(g, os.path.dirname(__file__), "peep", bindings=bindings)
    # Materialize the current head order; target policy and fallback are declarations
    # (setup-template.tsv: one answer row per peep head, then the fallback default).
    from finite_rules import install_template
    from pathlib import Path
    root = Path(__file__).parent
    targets = dict(line.split('\t') for line in
                   (root/'answer-targets.tsv').read_text().splitlines()
                   if line and not line.startswith('#'))
    Y = [dict(i=index, target=targets.get(name, targets['*']).format(name=name)) for index, name in enumerate(PY)]
    install_template(g, root, 'setup', dict(Y=Y), None, bindings={'dispatch': bindings['PP_b85']}, section='answer')


def peep_start(p):
    """START's part for -O2: the peep table and opinfo's classes, as memory"""
    for f in E.gold("peep"):
        if len(f) == 4 and f[0] in PA and f[1] in PB and f[2] in PR and f[3] in PY:
            k = (PA.index(f[0]) * len(PB) + PB.index(f[1])) * len(PR) + PR.index(f[2])
            p.a(("LDI", "q_t", k), ("LDI", "q_u", PY.index(f[3]) + 1), ("STX", "q_t", PEEPB, "q_u"))
    for f in E.gold("opinfo"):
        if len(f) == 4:
            p.a(("SBCLR",), [("SBOUT", c) for c in f[0].encode()], ("SBINTERN", "q_t"))
            if f[2] in PA:
                p.a(("LDI", "q_u", PA.index(f[2]) + 1), ("STX", "q_t", ACLSB, "q_u"))
            if f[3] in PB:
                p.a(("LDI", "q_u", PB.index(f[3]) + 1), ("STX", "q_t", BCLSB, "q_u"))


def build():
    E.prn()
    procs()
    if LEVEL >= 2:
        E.numout()
        analysis()
        local()
        parsers()
        stfuse()
        peepround()
    p = P("START")
    for w in ("ret", "jump", "jumpz", "call", ".frame", "store64", ".st", "mov"):
        p.a(("SBCLR",), [("SBOUT", c) for c in w.encode()], ("SBINTERN", "id_" + w.strip(".")))
    if LEVEL >= 2:
        peep_start(p)
    for f in E.gold("opinfo"):
        if len(f) >= 2 and f[1] == "1":
            p.a(("SBCLR",), [("SBOUT", c) for c in f[0].encode()], ("SBINTERN", "t"), ("LDI", "u", 1), ("STX", "t", SIMPLE, "u"))
    install_rules(g, os.path.dirname(__file__), "setup",
                  sequences={"data": p.acts}, section="start")
    level = "2" if LEVEL >= 2 else "1"
    bindings = {"MAXJ": MAXJ}
    for line in open(os.path.join(os.path.dirname(__file__), "rounds-names.tsv")):
        if not line.startswith("#"):
            selected, name, prefix, kind = line.rstrip("\n").split("\t")
            if selected == level:
                bindings[name] = P(prefix).fresh(kind)
    for section in ("common", level):
        install_rules(g, os.path.dirname(__file__), "rounds", bindings=bindings, section=section)
    g.finish()
    states = {n: [m, {str(k): v for k, v in row.items()}] for n, (m, row) in g.st.items()}
    return {"start": "START", "states": states, "seqs": [list(map(list, s)) for s in g.seqs]}


if __name__ == "__main__":
    d = build()
    s = json.dumps(d, separators=(",", ":"))
    open(sys.argv[1], "w").write(s)
    st, ent, live, ns, na = E.sizes(d)
    sys.stderr.write("states %d  entries %d  action seqs %d (%d actions)  json %d B\n" % (st, ent, ns, na, len(s)))
