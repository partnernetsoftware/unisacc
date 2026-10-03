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

from facts.load import facts
LEVEL = int(sys.argv[2]) if len(sys.argv) > 2 else 1
globals().update((r['name'], r['value']) for r in facts('opt-gen-constants'))   # memory regions, line kinds, limits
FRESH = {}
for _r in facts('opt-gen-fresh'):
    FRESH.setdefault(_r['group'], []).append((_r['name'], _r['prefix'], _r['kind']))


def procs():
    # Preserve downstream fresh names while the complete scan rules live in TSV.
    bindings = {name: P(prefix).fresh(kind) for name, prefix, kind in FRESH['procs']}
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
    bindings = {name: P(prefix).fresh(kind) for name, prefix, kind in FRESH['local']}
    install_rules(g, os.path.dirname(__file__), "local", bindings=bindings)


# ---- -O2's second half: the peep table's rounds (unisa/opt.py _Peep, src/opt.c peep_round) ----
PFIELDS = {}
for _ln in open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "weights", "gold", "peep.tsv")):
    _f = _ln.rstrip("\n").split("\t")
    if _ln.startswith("#field") or _ln.startswith("#head"):
        PFIELDS[_f[1]] = _f[2:]
PA, PB, PR, PY = PFIELDS["a"], PFIELDS["b"], PFIELDS["rel"], PFIELDS["y"]


def parsers():
    # Preserve branch names and the shared PRN continuation; matching lives in TSV.
    specs = FRESH['parsers']
    bindings = {name: P(prefix).fresh(kind) for name, prefix, kind in specs}
    install_rules(g, os.path.dirname(__file__), "parsers", bindings=bindings)


def stfuse():
    # Keep the original fresh names and shared DEADQ/NAMES/PRN continuations.
    specs = FRESH['stfuse']
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
    for w in facts('opt-gen-startwords'):
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
