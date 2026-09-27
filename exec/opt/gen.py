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
in peep-{byte,result}.tsv. ANALYZE/SOLVE/SCAN/DEADQ live in analysis-{byte,result}.tsv.
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
g, P, EOF = E.g, E.P, 256
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
LETTER = sorted(set(range(97, 123)) | set(range(65, 91)) | {95})     # isal
DIGIT = list(range(48, 58))
NL = [10, EOF]
MAXJ = 16                        # the pop is line i+2+cnt, cnt <= 16: at most line i+18
WORDMAX = 15                     # ol_opi reads at most 15 bytes of the op word


def lit(st, text, ok, fail):
    """match text byte by byte from state st: ok after the whole, fail at the first mismatch"""
    b = text.encode()
    for k, c in enumerate(b):
        cur = st if k == 0 else "%s.%d" % (st, k)
        nx = ok if k == len(b) - 1 else "%s.%d" % (st, k + 1)
        g.on(cur, [c], nx, [("ADV",)])
        g.els(cur, fail, [])


def regnum(st, dst, ok, fail):
    """one or more digits -> W[dst]; state ok at the first byte after them"""
    g.on(st, DIGIT, st + ".a", [("LDI", dst, 0)])
    g.els(st, fail, [])
    g.on(st + ".a", DIGIT, st + ".a", [("BYTE", "bt"), ("ALUI", "sub", "bt", "bt", 48),
                                       ("ALUI", "mul", dst, dst, 10), ("ALU", "add", dst, dst, "bt"), ("ADV",)])
    g.els(st + ".a", ok, [])


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


def rtok(st, dst, terms, fail):
    """`r` then digits -> W[dst]; then one of terms {bytes: state} (not consumed), else fail"""
    g.on(st, [114], st + ".r", [("ADV",)])
    g.els(st, fail, [])
    g.on(st + ".r", DIGIT, st + ".d", [("LDI", dst, 0)])
    g.els(st + ".r", fail, [])
    g.on(st + ".d", DIGIT, st + ".d", [("BYTE", "q_bt"), ("ALUI", "sub", "q_bt", "q_bt", 48),
                                       ("ALUI", "mul", dst, dst, 10), ("ALU", "add", dst, dst, "q_bt"), ("ADV",)])
    for keys, nx in terms:
        g.on(st + ".d", keys, nx, [])
    g.els(st + ".d", fail, [])


def anyb(st, nx, fail):
    """one byte that is not the end of the line"""
    g.on(st, NL, fail, [])
    g.els(st, nx, [("ADV",)])


def parsers():
    # PSTORE: `  store64 [SLOT], rX` -> st_x (-1), st_ms/st_me
    g.els("PSTORE", "PS.a", [("LDI", "st_x", -1)])
    lit("PS.a", "  store64 [", "PS.m", "RET")
    g.els("PS.m", "PS.s", [("MARK", "st_ms")])
    g.on("PS.s", [93], "PS.c", [("MARK", "st_me"), ("ADV",)])
    g.on("PS.s", NL, "RET", [])
    g.els("PS.s", "PS.s", [("ADV",)])
    lit("PS.c", ", ", "PS.r", "RET")
    rtok("PS.r", "q_v", [(NL, "PS.ok")], "RET")
    P("PS.ok").a(("COPYW", "st_x", "q_v")).ret()
    # PLOAD: `  load64 rY, [SLOT]` -> ld_y (-1), ld_ms/ld_me
    g.els("PLOAD", "PL.a", [("LDI", "ld_y", -1)])
    lit("PL.a", "  load64 ", "PL.r", "RET")
    rtok("PL.r", "q_v", [([44], "PL.c")], "RET")
    lit("PL.c", ", [", "PL.m", "RET")
    g.els("PL.m", "PL.s", [("MARK", "ld_ms"), ("LDI", "q_lc", 0)])
    g.on("PL.s", NL, "PL.e", [("MARK", "ld_me")])
    g.els("PL.s", "PL.s", [("BYTE", "q_lc"), ("ADV",)])
    p = P("PL.e")
    p.branch({1: "PL.ok"}, "RET", [("CMPI", "q_lc", 93)])
    P("PL.ok").a(("ALUI", "sub", "ld_me", "ld_me", 1), ("COPYW", "ld_y", "q_v")).ret()
    # PIMM: `  imm rK, DIGITS` (1..18 digits) -> im_k (-1), im_v (64-bit), im_n0/im_n1
    g.els("PIMM", "PI.a", [("LDI", "im_k", -1)])
    lit("PI.a", "  imm ", "PI.r", "RET")
    rtok("PI.r", "q_v", [([44], "PI.c")], "RET")
    lit("PI.c", ", ", "PI.n", "RET")
    g.on("PI.n", DIGIT, "PI.d", [("MARK", "im_n0"), ("LDI", "im_v", 0), ("LDI", "q_nd", 0)])
    g.els("PI.n", "RET", [])
    g.on("PI.d", DIGIT, "PI.d", [("BYTE", "q_bt"), ("ALUI", "sub", "q_bt", "q_bt", 48), ("A64I", "mul", "im_v", "im_v", 10),
                                 ("A64", "add", "im_v", "im_v", "q_bt"), ("ALUI", "add", "q_nd", "q_nd", 1), ("ADV",)])
    g.on("PI.d", NL, "PI.e", [("MARK", "im_n1")])
    g.els("PI.d", "RET", [])
    p = P("PI.e")
    p.branch({2: "RET"}, "PI.ok", [("CMPI", "q_nd", 18)])
    P("PI.ok").a(("COPYW", "im_k", "q_v")).ret()
    # PMOV: `  mov rD,?rS` -> mv_d (-1), mv_s
    g.els("PMOV", "PM.a", [("LDI", "mv_d", -1)])
    lit("PM.a", "  mov ", "PM.r", "RET")
    rtok("PM.r", "q_v", [([44], "PM.c")], "RET")
    g.els("PM.c", "PM.c1", [("ADV",)])
    anyb("PM.c1", "PM.s", "RET")
    rtok("PM.s", "q_w", [(NL, "PM.ok")], "RET")
    P("PM.ok").a(("COPYW", "mv_d", "q_v"), ("COPYW", "mv_s", "q_w")).ret()
    # PTHREE: ` ?WORD... rD,?rS,?rT` (after the first space from index 2) -> th_ok, th_d/th_s/th_t
    g.els("PTHREE", "PT.a", [("LDI", "th_ok", 0)])
    g.on("PT.a", [32], "PT.b", [("ADV",)])
    g.els("PT.a", "RET", [])
    anyb("PT.b", "PT.w", "RET")
    g.on("PT.w", [32], "PT.r1", [("ADV",)])
    g.on("PT.w", NL, "RET", [])
    g.els("PT.w", "PT.w", [("ADV",)])
    rtok("PT.r1", "th_d", [([44], "PT.c1")], "RET")
    g.els("PT.c1", "PT.c1b", [("ADV",)])
    anyb("PT.c1b", "PT.r2", "RET")
    rtok("PT.r2", "th_s", [([44], "PT.c2")], "RET")
    g.els("PT.c2", "PT.c2b", [("ADV",)])
    anyb("PT.c2b", "PT.r3", "RET")
    rtok("PT.r3", "th_t", [(NL, "PT.ok")], "RET")
    P("PT.ok").a(("LDI", "th_ok", 1)).ret()
    # REREG(rr_a -> rr_b, rr_all): the line at the cursor written out with register rr_a as rr_b --
    # every register token, or only the first one (rr_all 0)
    g.els("REREG", "RR0", [("LDI", "rr_done", 0), ("LDI", "q_pa", 0)])
    g.on("RR0", [10], "RET", [])
    g.on("RR0", [EOF], "RET", [])
    g.on("RR0", [114], "RR.r", [("MARK", "rr_p")])
    g.on("RR0", LETTER, "RR0", [("COPY",), ("ADV",), ("LDI", "q_pa", 1)])
    g.els("RR0", "RR0", [("COPY",), ("ADV",), ("LDI", "q_pa", 0)])
    p = P("RR.r")       # an `r`: a register token when not after a letter, not at the start, a digit next
    p.branch({1: "RR.lit"}, "RR.r1", [("CMPI", "q_pa", 1)])
    P("RR.r1").branch({1: "RR.lit"}, "RR.r2", [("CMPI", "rr_done", 1)])
    p = P("RR.r2")
    p.a(("ADV",)).goto("RR.r3")
    g.on("RR.r3", DIGIT, "RR.d", [("LDI", "q_rn", 0)])
    g.els("RR.r3", "RR.back", [])
    P("RR.back").a(("JUMP", "rr_p")).goto("RR.lit")
    g.els("RR.lit", "RR0", [("COPY",), ("ADV",), ("LDI", "q_pa", 1)])
    g.on("RR.d", DIGIT, "RR.d", [("BYTE", "q_bt"), ("ALUI", "sub", "q_bt", "q_bt", 48),
                                 ("ALUI", "mul", "q_rn", "q_rn", 10), ("ALU", "add", "q_rn", "q_rn", "q_bt"), ("ADV",)])
    g.els("RR.d", "RR.e", [("MARK", "rr_q")])
    p = P("RR.e")
    p.branch({1: "RR.sub"}, "RR.keep", [("CMP", "q_rn", "rr_a")])
    P("RR.sub").o("r").num("rr_b").goto("RR.f")
    P("RR.keep").a(("SPAN2", "rr_p", "rr_q")).goto("RR.f")
    p = P("RR.f")
    p.a(("LDI", "q_pa", 0)).branch({1: "RR0"}, "RR.one", [("CMPI", "rr_all", 1)])
    P("RR.one").a(("LDI", "rr_done", 1)).goto("RR0")


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
    # Pure schema assembly: the answer indices follow the current peep head order.
    dispatch = bindings['PP_b85']
    for index, name in enumerate(PY):
        if name not in ("-", "keep"):
            g.on(dispatch, [index], "PX." + name, [], "r")
    g.els(dispatch, "PP.copy", [], "r")


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
    p.a(("LDI", "vsp", 0))
    for w in ("ret", "jump", "jumpz", "call", ".frame", "store64", ".st", "mov"):
        p.a(("SBCLR",), [("SBOUT", c) for c in w.encode()], ("SBINTERN", "id_" + w.strip(".")))
    if LEVEL >= 2:
        peep_start(p)
    for f in E.gold("opinfo"):
        if len(f) >= 2 and f[1] == "1":
            p.a(("SBCLR",), [("SBOUT", c) for c in f[0].encode()], ("SBINTERN", "t"), ("LDI", "u", 1), ("STX", "t", SIMPLE, "u"))
    p.a(("LDI", "rnd", 0), ("LDI", "hits", 0)).goto("RSTART")
    # RSTART: a round begins; at -O2 the liveness of the whole tape first
    p = P("RSTART")
    if LEVEL >= 2:
        p.a(("LDI", "zlo", 2)).call("ANALYZE").a(("LDI", "q_zero", 0), ("JUMP", "q_zero"))
    p.a(("LDI", "li", 0)).goto("L0")

    # L0: the start of a line
    g.on("L0", [EOF], "ROUND", [])
    g.els("L0", "L0.m", [("MARK", "ls")])
    lit("L0.m", "  .frame 8\n", "ST", "COPY1")
    lit("ST", "  store64 [r7+0], r", "ST.x", "COPY1")
    regnum("ST.x", "X", "ST.nl", "COPY1")
    g.on("ST.nl", [10], "MID", [("ADV",), ("MARK", "mid"), ("LDI", "cnt", 0)])
    g.els("ST.nl", "COPY1", [])
    # MID: line i+2+cnt.  The pop (and `.frame -8` after it) ends M; any other line must be
    # simple and must not name r7
    p = P("MID")
    p.branch({2: "COPY1"}, "MID.t", [("CMPI", "cnt", MAXJ)])
    g.els("MID.t", "MID.p", [("MARK", "lp")])
    lit("MID.p", "  load64 r", "MID.y", "MID.np")
    regnum("MID.y", "Y", "MID.yc", "MID.np")
    lit("MID.yc", ", [r7+0]\n", "MID.f", "MID.np")
    lit("MID.f", "  .frame -8\n", "OK1", "COPY1")
    p = P("MID.np")
    p.a(("JUMP", "lp")).call("SIMPLE").branch({1: "MID.s"}, "COPY1", [("CMPI", "ok", 1)])
    p = P("MID.s")
    p.a(("JUMP", "lp"), ("LDI", "nr", 7)).call("NAMES").branch({1: "COPY1"}, "MID.n", [("CMPI", "found", 1)])
    p = P("MID.n")
    p.a(("JUMP", "lp")).call("SKIPL").a(("ALUI", "add", "cnt", "cnt", 1)).goto("MID")
    # OK1: past `.frame -8\n`.  M = [mid, lp): no line of it names rY; empty when X == Y
    p = P("OK1")
    p.a(("MARK", "after"), ("JUMP", "mid"), ("COPYW", "nr", "Y")).label("OK.l")
    p.a(("MARK", "cur")).branch({1: "OK.d"}, "OK.n", [("CMP", "cur", "lp")])
    p = P("OK.n")
    p.call("NAMES").branch({1: "ZTRY"}, "OK.s", [("CMPI", "found", 1)])
    p = P("OK.s")
    p.a(("JUMP", "cur")).call("SKIPL").goto("OK.l")
    p = P("OK.d")
    p.branch({1: "OK.xy"}, "OK.emit", [("CMP", "X", "Y")])
    P("OK.xy").branch({1: "OK.emit"}, "ZTRY", [("CMP", "mid", "lp")])       # X == Y: only with M empty
    p = P("OK.emit")
    p.branch({1: "OK.m"}, "OK.mv", [("CMP", "X", "Y")])
    p = P("OK.mv")
    p.o("  mov r").num("Y").o(", r").num("X").o("\n").goto("OK.m")
    p = P("OK.m")
    p.a(("SPAN2", "mid", "lp"), ("JUMP", "after"), ("ALUI", "add", "hits", "hits", 1),
        ("ALU", "add", "li", "li", "cnt"), ("ALUI", "add", "li", "li", 4)).goto("L0")
    # ZTRY (-O2): carry X in r3..r5 -- one not X or Y, not named in M, dead at line j+2 (the pop + 2)
    p = P("ZTRY")
    if LEVEL < 2:
        p.goto("COPY1")
    else:
        p.a(("LDI", "zz", 3)).label("ZT.l")
        p.branch({2: "COPY1"}, "ZT.a", [("CMPI", "zz", 5)])
        P("ZT.a").branch({1: "ZT.nx"}, "ZT.b", [("CMP", "zz", "X")])
        P("ZT.b").branch({1: "ZT.nx"}, "ZT.c", [("CMP", "zz", "Y")])
        p = P("ZT.c")          # does M name zz
        p.a(("JUMP", "mid"), ("COPYW", "nr", "zz")).label("ZT.m")
        p.a(("MARK", "cur")).branch({1: "ZT.d"}, "ZT.mn", [("CMP", "cur", "lp")])
        p = P("ZT.mn")
        p.call("NAMES").branch({1: "ZT.nx"}, "ZT.ms", [("CMPI", "found", 1)])
        p = P("ZT.ms")
        p.a(("JUMP", "cur")).call("SKIPL").goto("ZT.m")
        p = P("ZT.d")
        p.a(("COPYW", "dz", "zz"), ("ALU", "add", "dfrom", "li", "cnt"), ("ALUI", "add", "dfrom", "dfrom", 4)).call("DEADQ")
        p.branch({1: "ZT.emit"}, "ZT.nx", [("CMPI", "dv", 1)])
        P("ZT.nx").a(("ALUI", "add", "zz", "zz", 1)).goto("ZT.l")
        p = P("ZT.emit")
        p.o("  mov r").num("zz").o(", r").num("X").o("\n").a(("SPAN2", "mid", "lp"))
        p.o("  mov r").num("Y").o(", r").num("zz").o("\n")
        p.a(("JUMP", "after"), ("ALUI", "add", "hits", "hits", 1), ("ALU", "add", "li", "li", "cnt"), ("ALUI", "add", "li", "li", 4)).goto("L0")
    # COPY1: not a rewrite -- the line at ls as it stands
    p = P("COPY1")
    if LEVEL >= 2:
        p.a(("JUMP", "ls")).call("LOCAL").branch({1: "C1.loc"}, "C1.cp", [("CMPI", "lok", 1)])
        P("C1.loc").a(("ALUI", "add", "li", "li", 3), ("ALUI", "add", "hits", "hits", 1)).goto("L0")
        p = P("C1.cp")
    p.a(("JUMP", "ls")).call("COPYL").a(("ALUI", "add", "li", "li", 1)).goto("L0")
    # ROUND: at most four rounds; a round with no rewrite ends the pass
    p = P("ROUND")
    p.branch({1: "DONE"}, "RND.n", [("CMPI", "hits", 0)])
    p = P("RND.n")
    p.a(("ALUI", "add", "rnd", "rnd", 1)).branch({1: "DONE"}, "RND.s", [("CMPI", "rnd", 4)])
    P("RND.s").a(("SWAP",), ("LDI", "hits", 0)).goto("RSTART")
    if LEVEL < 2:
        P("DONE").a(("ACCEPT",)).goto("DEAD")
    else:
        # the peep table's rounds: at most four, a round with no rewrite ends them
        P("DONE").a(("SWAP",), ("LDI", "prnd", 0)).goto("PR.go")
        p = P("PR.go")
        p.a(("ALUI", "add", "rnd", "prnd", 4), ("LDI", "zlo", 0), ("LDI", "hits", 0)).call("ANALYZE").call("PPASS")
        p.branch({1: "PR.end"}, "PR.n", [("CMPI", "hits", 0)])
        p = P("PR.n")
        p.a(("ALUI", "add", "prnd", "prnd", 1)).branch({1: "PR.end"}, "PR.s", [("CMPI", "prnd", 4)])
        P("PR.s").a(("SWAP",)).goto("PR.go")
        P("PR.end").a(("ACCEPT",)).goto("DEAD")
    g.finish()
    states = {n: [m, {str(k): v for k, v in row.items()}] for n, (m, row) in g.st.items()}
    return {"start": "START", "states": states, "seqs": [list(map(list, s)) for s in g.seqs]}


if __name__ == "__main__":
    d = build()
    s = json.dumps(d, separators=(",", ":"))
    open(sys.argv[1], "w").write(s)
    st, ent, live, ns, na = E.sizes(d)
    sys.stderr.write("states %d  entries %d  action seqs %d (%d actions)  json %d B\n" % (st, ent, ns, na, len(s)))
