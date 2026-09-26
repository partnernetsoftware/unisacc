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
This is the optimiser's algorithm compiled into an action table: the rules are
maintained here, in the generator, not removed; the old src/opt.c and
unisa/opt.py stay the behaviour reference.

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
    # SKIPL / COPYL: past the end of the line (its \n included)
    g.on("SKIPL", [10], "RET", [("ADV",)])
    g.on("SKIPL", [EOF], "RET", [])
    g.els("SKIPL", "SKIPL", [("ADV",)])
    g.on("COPYL", [10], "RET", [("COPY",), ("ADV",)])
    g.on("COPYL", [EOF], "RET", [])
    g.els("COPYL", "COPYL", [("COPY",), ("ADV",)])
    # SIMPLE: ok := the line (from its start) is `  WORD...` with WORD simple in opinfo (ol_simple)
    g.els("SIMPLE", "SIM.a", [("LDI", "ok", 0)])
    lit("SIM.a", "  ", "SIM.w", "RET")
    g.els("SIM.w", "SIM.l", [("MARK", "ws"), ("LDI", "wk", 0)])
    g.on("SIM.l", [32] + NL, "SIM.e", [])
    g.els("SIM.l", "SIM.k", [])
    p = P("SIM.k")
    p.branch({1: "SIM.e"}, "SIM.adv", [("CMPI", "wk", WORDMAX)])
    g.els("SIM.adv", "SIM.l", [("ADV",), ("ALUI", "add", "wk", "wk", 1)])
    g.els("SIM.e", "SIM.e2", [("MARK", "we")])
    p = P("SIM.e2")
    p.branch({1: "RET"}, "SIM.f", [("CMPI", "wk", 0)])
    p = P("SIM.f")
    p.a(("INTERN", "wid", "ws", "we"), ("LDX", "s", "wid", SIMPLE)).branch({1: "SIM.y"}, "RET", [("CMPI", "s", 1)])
    P("SIM.y").a(("LDI", "ok", 1)).ret()
    # NAMES: found := the line names register W[nr] (`r` after a non-letter, then digits) -- ol_names
    g.els("NAMES", "NM0", [("LDI", "found", 0)])
    g.on("NM0", NL, "RET", [])
    g.on("NM0", [114], "NMR", [("ADV",)])
    g.on("NM0", LETTER, "NM1", [("ADV",)])
    g.els("NM0", "NM0", [("ADV",)])
    g.on("NM1", NL, "RET", [])
    g.on("NM1", LETTER, "NM1", [("ADV",)])
    g.els("NM1", "NM0", [])                   # the byte after a letter, looked at with a non-letter before it
    g.on("NMR", DIGIT, "NMD", [("LDI", "rn", 0)])
    g.els("NMR", "NM1", [])                   # `r` was a letter: the next byte follows a letter
    g.on("NMD", DIGIT, "NMD", [("BYTE", "bt"), ("ALUI", "sub", "bt", "bt", 48),
                               ("ALUI", "mul", "rn", "rn", 10), ("ALU", "add", "rn", "rn", "bt"), ("ADV",)])
    g.els("NMD", "NMD.c", [])
    p = P("NMD.c")
    p.branch({1: "NMD.y"}, "NM0", [("CMP", "rn", "nr")])     # after digits: a non-letter before the next byte
    P("NMD.y").a(("LDI", "found", 1)).goto("NM0")


def analysis():
    """ANALYZE: from x's start -- pass A (lines: kind, read/write masks, block, target token;
    labels), pass B (targets), then the liveness of r2..r5 (bl_solve by rescanning, as
    unisa/opt.py _solve).  Leaves nl, nb, ZOK[z], LIVE."""
    # pass A
    p = P("ANALYZE")
    p.a(("LDI", "q_l", 0), ("LDI", "q_b", 0), ("LDI", "q_cut", 1), ("ALUI", "mul", "q_lab", "rnd", 2 * 10 ** 6),
        ("ALUI", "add", "q_lab", "q_lab", LABB)).goto("A0")
    g.on("A0", [EOF], "A.end", [])
    g.els("A0", "A.blk", [("MARK", "q_p"), ("STX", "q_l", LSS, "q_p"), ("LDI", "q_t", 0), ("STX", "q_l", ISLAB, "q_t"),
                          ("LDI", "q_t", -1), ("STX", "q_l", WIDD, "q_t"), ("STX", "q_l", FRR, "q_t")])
    # the block: a non-space line starts one, and so does the line after jump/jumpz/ret
    p = P("A.blk")
    p.call("A.first").branch({1: "A.new"}, "A.old", [("CMPI", "q_cut", 1)])
    P("A.new").a(("STX", "q_b", BSS, "q_l"), ("ALUI", "add", "q_b", "q_b", 1), ("LDI", "q_cut", 0)).goto("A.old")
    P("A.old").a(("ALUI", "sub", "q_t", "q_b", 1), ("STX", "q_l", BLOF, "q_t"),
                 ("LDI", "q_rm", 0), ("LDI", "q_wm", 0), ("JUMP", "q_p")).branch({1: "A.sp"}, "A.lab", [("CMPI", "q_sp", 1)])
    # A.first: q_sp := the line starts with a space; a non-space line cuts
    g.on("A.first", [32], "RET", [("LDI", "q_sp", 1)])
    g.els("A.first", "RET", [("LDI", "q_sp", 0), ("LDI", "q_cut", 1)])
    # a line not starting with a space: LABEL; `name:` (len > 1, not starting with '.') registers name
    P("A.lab").a(("LDI", "q_k", K_LABEL), ("STX", "q_l", KK, "q_k"), ("STX", "q_l", RMM, "q_rm"), ("STX", "q_l", WMM, "q_wm"),
                 ("LDI", "q_lc", 0), ("LDI", "q_len", 0)).goto("AL.s")
    g.on("AL.s", [46], "AL.skip", [])          # '.': not a label
    g.els("AL.s", "AL.l", [])
    g.on("AL.l", NL, "AL.e", [("MARK", "q_e")])
    g.els("AL.l", "AL.l", [("BYTE", "q_lc"), ("ALUI", "add", "q_len", "q_len", 1), ("ADV",)])
    p = P("AL.e")
    p.branch({1: "AL.e2"}, "A.done", [("CMPI", "q_lc", 58)])
    p = P("AL.e2")
    p.branch({2: "AL.reg"}, "A.done", [("CMPI", "q_len", 1)])
    p = P("AL.reg")
    p.a(("ALUI", "sub", "q_e", "q_e", 1), ("INTERN", "q_id", "q_p", "q_e"), ("ALU", "add", "q_a", "q_lab", "q_id"),
        ("LDX", "q_t", "q_a", 0)).branch({1: "AL.set"}, "AL.mk", [("CMPI", "q_t", 0)])
    P("AL.set").a(("ALUI", "add", "q_t", "q_l", 1), ("STX", "q_a", 0, "q_t")).goto("AL.mk")
    P("AL.mk").a(("LDI", "q_t", 1), ("STX", "q_l", ISLAB, "q_t"),             # the label's name: [TS, TE)
                 ("STX", "q_l", TSS, "q_p"), ("STX", "q_l", TEE, "q_e")).goto("A.done")
    g.on("AL.skip", NL, "A.done", [])
    g.els("AL.skip", "AL.skip", [("ADV",)])
    # a space line: the word from p+2
    g.on("A.sp", [32], "A.sp2", [("ADV",)])
    g.els("A.sp", "A.oth", [])
    g.on("A.sp2", NL, "A.oth", [])                         # ` ` alone: no word
    g.on("A.sp2", [32], "A.w", [("ADV",), ("LDI", "q_s2", 1)])
    g.els("A.sp2", "A.w", [("ADV",), ("LDI", "q_s2", 0)])  # the second byte, whatever it is (ol_simple wants a space)
    g.els("A.w", "A.wl", [("MARK", "q_ws"), ("LDI", "q_wn", 0)])
    g.on("A.wl", [32] + NL, "A.we", [("MARK", "q_we")])
    g.els("A.wl", "A.wl", [("ADV",), ("ALUI", "add", "q_wn", "q_wn", 1)])
    # the operands: every rN after a non-letter; the first one (unless a `[` came before it); again
    p = P("A.we")
    p.a(("LDI", "q_m", 0), ("LDI", "q_f", -1), ("LDI", "q_first", 1), ("LDI", "q_br", 0), ("LDI", "q_again", 0),
        ("COPYW", "q_ls", "q_ws")).goto("OP0")
    g.on("OP0", NL, "A.cls", [("MARK", "q_e")])
    g.on("OP0", [32], "OP0", [("ADV",), ("MARK", "q_ls")])
    g.on("OP0", [91], "OP0", [("ADV",), ("LDI", "q_br", 1)])
    g.on("OP0", [114], "OPR", [("ADV",)])
    g.on("OP0", LETTER, "OP1", [("ADV",)])
    g.els("OP0", "OP0", [("ADV",)])
    g.on("OP1", NL, "A.cls", [("MARK", "q_e")])
    g.on("OP1", LETTER, "OP1", [("ADV",)])
    g.els("OP1", "OP0", [])
    g.on("OPR", DIGIT, "OPD", [("LDI", "q_rn", 0)])
    g.els("OPR", "OP1", [])
    g.on("OPD", DIGIT, "OPD", [("BYTE", "q_bt"), ("ALUI", "sub", "q_bt", "q_bt", 48),
                               ("ALUI", "mul", "q_rn", "q_rn", 10), ("ALU", "add", "q_rn", "q_rn", "q_bt"), ("ADV",)])
    g.els("OPD", "OPD.r", [])
    p = P("OPD.r")
    p.branch({0: "OPD.m"}, "OPD.f", [("CMPI", "q_rn", 16)])
    P("OPD.m").a(("LDI", "q_t", 1), ("ALU", "shl", "q_t", "q_t", "q_rn"), ("ALU", "or", "q_m", "q_m", "q_t")).goto("OPD.f")
    p = P("OPD.f")
    p.branch({1: "OPD.1"}, "OPD.2", [("CMPI", "q_first", 1)])
    p = P("OPD.1")
    p.a(("LDI", "q_first", 0)).branch({1: "OP0"}, "OPD.1f", [("CMPI", "q_br", 1)])
    P("OPD.1f").a(("COPYW", "q_f", "q_rn")).goto("OP0")
    p = P("OPD.2")
    p.branch({1: "OPD.ag"}, "OP0", [("CMP", "q_rn", "q_f")])      # q_f = -1 never equals a register
    P("OPD.ag").a(("LDI", "q_again", 1)).goto("OP0")
    # A.cls: the kind from the word
    p = P("A.cls")
    p.a(("INTERN", "q_id", "q_ws", "q_we"), ("STX", "q_l", WIDD, "q_id"), ("STX", "q_l", FRR, "q_f"),
        ("STX", "q_l", TSS, "q_ls"), ("STX", "q_l", TEE, "q_e"))
    for w, k in (("ret", K_RET), ("jump", K_JUMP), ("jumpz", K_JUMPZ), ("call", K_CALL), (".frame", K_FRAME)):
        hit, nx = "A.c_" + w, "A.cn_" + w
        p.branch({1: hit}, nx, [("CMP", "q_id", "id_" + w.strip("."))])
        q = P(hit)
        q.a(("LDI", "q_k", k))
        if w in ("jump", "jumpz", "ret"):
            q.a(("LDI", "q_cut", 1))
        if w == "jumpz":
            q.a(("COPYW", "q_rm", "q_m"))
        if w in ("jump", "jumpz", "call"):
            q.a(("STX", "q_l", TSS, "q_ls"), ("STX", "q_l", TEE, "q_e"))
        q.goto("A.store")
        p = P(nx)
    # not one of those: simple (opinfo) or other; the word was at most 15 bytes for ol_opi
    p.branch({2: "A.oth"}, "A.cs0", [("CMPI", "q_wn", WORDMAX)])
    P("A.cs0").branch({1: "A.cs"}, "A.oth", [("CMPI", "q_s2", 1)])
    p = P("A.cs")
    p.a(("LDX", "q_t", "q_id", SIMPLE)).branch({1: "A.simple"}, "A.oth", [("CMPI", "q_t", 1)])
    p = P("A.simple")
    p.a(("LDI", "q_k", K_SIMPLE)).branch({1: "A.st"}, "A.sm1", [("CMP", "q_id", "id_store64")])
    P("A.sm1").branch({1: "A.st"}, "A.sm2", [("CMP", "q_id", "id_st")])
    P("A.st").a(("LDI", "q_f", -1)).goto("A.sm2")
    p = P("A.sm2")
    p.branch({0: "A.nof"}, "A.hasf", [("CMPI", "q_f", 0)])          # f < 0
    P("A.nof").a(("COPYW", "q_rm", "q_m")).goto("A.store")
    p = P("A.hasf")
    p.a(("LDI", "q_t", 1), ("ALU", "shl", "q_wm", "q_t", "q_f"), ("LDI", "q_u", -1), ("ALU", "xor", "q_u", "q_u", "q_wm"),
        ("ALU", "and", "q_rm", "q_m", "q_u")).branch({1: "A.ag"}, "A.store", [("CMPI", "q_again", 1)])
    P("A.ag").a(("ALU", "or", "q_rm", "q_rm", "q_wm")).goto("A.store")
    P("A.oth").a(("LDI", "q_k", K_OTHER), ("LDI", "q_rm", 255), ("LDI", "q_wm", 0)).goto("A.oth2")
    g.on("A.oth2", NL, "A.store", [])
    g.els("A.oth2", "A.oth2", [("ADV",)])
    P("A.store").a(("STX", "q_l", KK, "q_k"), ("STX", "q_l", RMM, "q_rm"), ("STX", "q_l", WMM, "q_wm")).goto("A.done")
    # A.done: at the line's end (\n or EOF)
    g.on("A.done", [10], "A0", [("MARK", "q_t"), ("STX", "q_l", LEE, "q_t"), ("ADV",), ("ALUI", "add", "q_l", "q_l", 1)])
    g.on("A.done", [EOF], "A.end", [("MARK", "q_t"), ("STX", "q_l", LEE, "q_t"), ("ALUI", "add", "q_l", "q_l", 1)])
    g.els("A.done", "A.done", [("ADV",)])
    # the LABEL kind for non-space lines is stored at A.done's entry through A.lab -> A.done: store it
    # pass B: targets
    p = P("A.end")
    p.a(("COPYW", "nl", "q_l"), ("COPYW", "nb", "q_b"), ("LDI", "q_l", 0)).label("B.l")
    p.branch({0: "B.k"}, "SOLVE", [("CMP", "q_l", "nl")])
    p = P("B.k")
    p.a(("LDX", "q_k", "q_l", KK), ("LDI", "q_t", -1), ("STX", "q_l", TGG, "q_t"))
    p.branch({1: "B.t"}, "B.k2", [("CMPI", "q_k", K_JUMP)])
    P("B.k2").branch({1: "B.t"}, "B.k3", [("CMPI", "q_k", K_JUMPZ)])
    P("B.k3").branch({1: "B.t"}, "B.n", [("CMPI", "q_k", K_CALL)])
    p = P("B.t")
    p.a(("LDX", "q_ts", "q_l", TSS), ("LDX", "q_te", "q_l", TEE), ("INTERN", "q_id", "q_ts", "q_te"),
        ("ALU", "add", "q_a", "q_lab", "q_id"), ("LDX", "q_t", "q_a", 0)).branch({1: "B.n"}, "B.set", [("CMPI", "q_t", 0)])
    P("B.set").a(("ALUI", "sub", "q_t", "q_t", 1), ("LDX", "q_t", "q_t", BLOF), ("STX", "q_l", TGG, "q_t")).goto("B.n")
    P("B.n").a(("ALUI", "add", "q_l", "q_l", 1)).goto("B.l")
    # SOLVE: z = 2..5; rounds <= 64; blocks from last to first; ZOK[z] := converged
    p = P("SOLVE")
    p.a(("COPYW", "z", "zlo")).label("SV.z")
    p.branch({2: "SV.done"}, "SV.init", [("CMPI", "z", 5)])
    p = P("SV.init")
    p.a(("ALUI", "mul", "q_lb", "rnd", 6), ("ALU", "add", "q_lb", "q_lb", "z"),
        ("ALUI", "mul", "q_lb", "q_lb", 10 ** 6), ("ALUI", "add", "q_lb", "q_lb", LIVEB),
        ("LDI", "q_ch", 1), ("LDI", "q_rr", 0)).label("SV.r")
    p.branch({1: "SV.go"}, "SV.fin", [("CMPI", "q_ch", 1)])
    p = P("SV.go")
    p.branch({0: "SV.go2"}, "SV.fin", [("CMPI", "q_rr", 64)])
    p = P("SV.go2")
    p.a(("LDI", "q_ch", 0), ("ALUI", "add", "q_rr", "q_rr", 1), ("ALUI", "sub", "q_bb", "nb", 1)).label("SV.b")
    p.branch({0: "SV.r"}, "SV.bb", [("CMPI", "q_bb", 0)])        # q_bb < 0: the round is over
    p = P("SV.bb")
    p.a(("ALU", "add", "q_a", "q_lb", "q_bb"), ("LDX", "q_t", "q_a", 0)).branch({1: "SV.scan"}, "SV.nx", [("CMPI", "q_t", 0)])
    p = P("SV.scan")
    p.a(("LDX", "q_s", "q_bb", BSS), ("LDX", "q_t", "q_s", KK)).branch({1: "SV.s1"}, "SV.s0", [("CMPI", "q_t", K_LABEL)])
    P("SV.s1").a(("ALUI", "add", "q_s", "q_s", 1)).goto("SV.s0")
    p = P("SV.s0")
    p.a(("COPYW", "sc_l", "q_s"), ("COPYW", "sc_z", "z")).vpush("q_bb", "q_lb").call("SCAN").vpop("q_bb", "q_lb")
    p.branch({1: "SV.set"}, "SV.nx", [("CMPI", "sc_v", 1)])
    P("SV.set").a(("ALU", "add", "q_a", "q_lb", "q_bb"), ("LDI", "q_t", 1), ("STX", "q_a", 0, "q_t"), ("LDI", "q_ch", 1)).goto("SV.nx")
    P("SV.nx").a(("ALUI", "sub", "q_bb", "q_bb", 1)).goto("SV.b")
    p = P("SV.fin")          # converged iff the last round changed nothing
    p.a(("LDI", "q_t", 1)).branch({1: "SV.bad"}, "SV.ok", [("CMPI", "q_ch", 1)])
    P("SV.bad").a(("LDI", "q_t", 0)).goto("SV.ok")
    P("SV.ok").a(("STX", "z", ZOKB, "q_t"), ("ALUI", "add", "z", "z", 1)).goto("SV.z")
    P("SV.done").ret()
    # SCAN(sc_z, sc_l) -> sc_v: from line sc_l, is sc_z read before written (ol_scan)
    p = P("SCAN")
    p.a(("ALUI", "mul", "sc_lb", "rnd", 6), ("ALU", "add", "sc_lb", "sc_lb", "sc_z"),
        ("ALUI", "mul", "sc_lb", "sc_lb", 10 ** 6), ("ALUI", "add", "sc_lb", "sc_lb", LIVEB),
        ("LDI", "sc_bit", 1), ("ALU", "shl", "sc_bit", "sc_bit", "sc_z")).label("SC.l")
    p.branch({0: "SC.k"}, "SC.live", [("CMP", "sc_l", "nl")])
    p = P("SC.k")
    p.a(("LDX", "sc_k", "sc_l", KK))
    cases = {K_LABEL: "SC.lab", K_RET: "SC.ret", K_JUMP: "SC.jmp", K_JUMPZ: "SC.jz", K_CALL: "SC.call", K_FRAME: "SC.nx"}
    p.branch(cases, "SC.gen", [("RLD", "sc_k")])
    P("SC.lab").a(("LDX", "sc_t", "sc_l", BLOF), ("ALU", "add", "sc_t", "sc_t", "sc_lb"), ("LDX", "sc_v", "sc_t", 0)).ret()
    p = P("SC.ret")
    p.branch({(0, 1): "SC.live"}, "SC.dead", [("CMPI", "sc_z", 1)])
    p = P("SC.jmp")
    p.a(("LDX", "sc_t", "sc_l", TGG)).branch({0: "SC.live"}, "SC.jt", [("CMPI", "sc_t", 0)])
    P("SC.jt").a(("ALU", "add", "sc_t", "sc_t", "sc_lb"), ("LDX", "sc_v", "sc_t", 0)).ret()
    p = P("SC.jz")
    p.a(("LDX", "sc_t", "sc_l", RMM), ("ALU", "and", "sc_t", "sc_t", "sc_bit")).branch({1: "SC.call"}, "SC.live",
                                                                                     [("CMPI", "sc_t", 0)])
    # careful: CMPI t 0 gives 1 when t == 0 (not read); read -> live
    p = P("SC.call")
    p.a(("LDX", "sc_t", "sc_l", TGG)).branch({0: "SC.live"}, "SC.ct", [("CMPI", "sc_t", 0)])
    p = P("SC.ct")
    p.a(("ALU", "add", "sc_t", "sc_t", "sc_lb"), ("LDX", "sc_t", "sc_t", 0)).branch({1: "SC.live"}, "SC.nx", [("CMPI", "sc_t", 1)])
    p = P("SC.gen")
    p.a(("LDX", "sc_t", "sc_l", RMM), ("ALU", "and", "sc_t", "sc_t", "sc_bit")).branch({1: "SC.g2"}, "SC.live", [("CMPI", "sc_t", 0)])
    p = P("SC.g2")
    p.a(("LDX", "sc_t", "sc_l", WMM), ("ALU", "and", "sc_t", "sc_t", "sc_bit")).branch({1: "SC.nx"}, "SC.dead", [("CMPI", "sc_t", 0)])
    P("SC.nx").a(("ALUI", "add", "sc_l", "sc_l", 1)).goto("SC.l")
    P("SC.live").a(("LDI", "sc_v", 1)).ret()
    P("SC.dead").a(("LDI", "sc_v", 0)).ret()
    # DEAD(dz, dfrom) -> dv: ZOK[dz] and SCAN(dz, dfrom) == 0
    p = P("DEADQ")
    p.a(("LDI", "dv", 0), ("LDX", "q_t", "dz", ZOKB)).branch({1: "DQ.s"}, "RET", [("CMPI", "q_t", 1)])
    p = P("DQ.s")
    p.a(("COPYW", "sc_z", "dz"), ("COPYW", "sc_l", "dfrom")).call("SCAN").branch({1: "DQ.y"}, "RET", [("CMPI", "sc_v", 0)])
    # CMPI sc_v 0: 1 when sc_v == 0
    P("DQ.y").a(("LDI", "dv", 1)).ret()


def local():
    """LOCAL (ol_local): at line li (position ls) `imm r2, N / sub64 rD, r6, r2 / .ld rD, [rD+0], W`
    (or load64) with r2 dead after it -> `.ld rD, [r6-N], W`; lok := 1 and the output written"""
    p = P("LOCAL")
    p.a(("LDI", "lok", 0), ("ALUI", "add", "q_t", "li", 2)).branch({0: "LC.a"}, "RET", [("CMP", "q_t", "nl")])
    g.els("LC.a", "LC.a0", [])
    lit("LC.a0", "  imm r2, ", "LC.n", "RET")
    g.on("LC.n", DIGIT, "LC.nd", [("MARK", "q_n0")])
    g.els("LC.n", "RET", [])
    g.on("LC.nd", DIGIT, "LC.nd", [("ADV",)])
    g.on("LC.nd", [10], "LC.b", [("MARK", "q_n1"), ("ADV",)])
    g.els("LC.nd", "RET", [])
    lit("LC.b", "  sub64 r", "LC.d", "RET")
    g.on("LC.d", [48, 49, 51, 52, 53], "LC.q", [("BYTE", "q_d"), ("ADV",)])
    g.els("LC.d", "RET", [])
    # `, r6, r2`: the C checks only ',' . 'r' '6' . . 'r' '2' (dots: any byte), then the line ends
    pat = [44, None, 114, 54, None, None, 114, 50, 10]
    for k, c in enumerate(pat):
        cur = "LC.q" if k == 0 else "LC.q%d" % k
        nx = "LC.c" if k == len(pat) - 1 else "LC.q%d" % (k + 1)
        if c is None:
            g.on(cur, [10, EOF], "RET", [])
            g.els(cur, nx, [("ADV",)])
        else:
            g.on(cur, [c], nx, [("ADV",)])
            g.els(cur, "RET", [])
    g.els("LC.c", "LC.c0", [("MARK", "q_c0")])
    lit("LC.c0", "  .ld r", "LC.cd1", "LC.c2")
    g.els("LC.cd1", "LC.cd1b", [("BYTE", "q_t")])
    p = P("LC.cd1b")
    p.branch({1: "LC.ld1"}, "LC.c2", [("CMP", "q_t", "q_d")])
    g.els("LC.ld1", "LC.r", [("ADV",), ("LDI", "q_ld", 1)])
    P("LC.c2").a(("JUMP", "q_c0")).goto("LC.c3")
    lit("LC.c3", "  load64 r", "LC.cd2", "RET")
    g.els("LC.cd2", "LC.cd2b", [("BYTE", "q_t")])
    p = P("LC.cd2b")
    p.branch({1: "LC.ld2"}, "RET", [("CMP", "q_t", "q_d")])
    g.els("LC.ld2", "LC.r", [("ADV",), ("LDI", "q_ld", 2)])
    # `, [rD+0]`: ',' . '[' 'r' D '+' '0' ']'
    g.on("LC.r", [44], "LC.r1", [("ADV",)])
    g.els("LC.r", "RET", [])
    g.on("LC.r1", [10, EOF], "RET", [])
    g.els("LC.r1", "LC.r2", [("ADV",)])
    lit("LC.r2", "[r", "LC.r3", "RET")
    g.els("LC.r3", "LC.r3b", [("BYTE", "q_t")])
    p = P("LC.r3b")
    p.branch({1: "LC.r4"}, "RET", [("CMP", "q_t", "q_d")])
    g.els("LC.r4", "LC.r5", [("ADV",)])
    lit("LC.r5", "+0]", "LC.tail", "RET")
    p = P("LC.tail")
    p.branch({1: "LC.t1"}, "LC.t2", [("CMPI", "q_ld", 1)])
    g.on("LC.t2", NL, "LC.dead", [("MARK", "q_after")])      # load64: the line ends here
    g.els("LC.t2", "RET", [])
    g.on("LC.t1", [44], "LC.t1r", [("MARK", "q_r0")])        # .ld: `, W` follows
    g.els("LC.t1", "RET", [])
    g.on("LC.t1r", NL, "LC.dead", [("MARK", "q_r1"), ("MARK", "q_after")])
    g.els("LC.t1r", "LC.t1r", [("ADV",)])
    p = P("LC.dead")
    p.a(("LDI", "dz", 2), ("ALUI", "add", "dfrom", "li", 3)).call("DEADQ").branch({1: "LC.emit"}, "RET", [("CMPI", "dv", 1)])
    p = P("LC.emit")
    p.a(("LDI", "lok", 1)).branch({1: "LC.e1"}, "LC.e2", [("CMPI", "q_ld", 1)])
    P("LC.e1").o("  .ld r").a(("OUTW", "q_d")).o(", [r6-").a(("SPAN2", "q_n0", "q_n1")).o("]").a(("SPAN2", "q_r0", "q_r1")).o("\n").goto("LC.fin")
    P("LC.e2").o("  load64 r").a(("OUTW", "q_d")).o(", [r6-").a(("SPAN2", "q_n0", "q_n1")).o("]\n").goto("LC.fin")
    g.on("LC.fin", [10], "RET", [("ADV",)])     # past line i+2's newline
    g.els("LC.fin", "RET", [])


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
    """STFUSE(pi): imm r2, N / sub64 rA, r6, r2 / M / .st|store64 [rA+0], rV -> M, one frame-relative
    store (pk_stfuse).  sk := the next line or 0; the output is written on success"""
    p = P("STFUSE")
    p.a(("LDI", "sk", 0), ("ALUI", "add", "q_t", "pi", 3)).branch({0: "SF.a"}, "RET", [("CMP", "q_t", "nl")])
    p = P("SF.a")
    p.a(("LDX", "q_t", "pi", LSS), ("JUMP", "q_t")).goto("SF.a0")
    lit("SF.a0", "  imm r2, ", "SF.n", "RET")
    g.on("SF.n", DIGIT, "SF.nd", [("MARK", "sf_n0")])
    g.els("SF.n", "RET", [])
    g.on("SF.nd", DIGIT, "SF.nd", [("ADV",)])
    g.on("SF.nd", [10], "SF.b", [("MARK", "sf_n1"), ("ADV",)])
    g.els("SF.nd", "RET", [])
    lit("SF.b", "  sub64 r", "SF.d", "RET")
    g.on("SF.d", [48, 49, 51, 52, 53], "SF.q", [("BYTE", "sf_a"), ("ADV",)])
    g.els("SF.d", "RET", [])
    pat = [44, None, 114, 54, None, None, 114, 50, 10]
    for k, c in enumerate(pat):
        cur = "SF.q" if k == 0 else "SF.q%d" % k
        nx = "SF.j" if k == len(pat) - 1 else "SF.q%d" % (k + 1)
        if c is None:
            g.on(cur, NL, "RET", [])
            g.els(cur, nx, [("ADV",)])
        else:
            g.on(cur, [c], nx, [("ADV",)])
            g.els(cur, "RET", [])
    p = P("SF.j")
    p.a(("ALUI", "sub", "sf_ad", "sf_a", 48), ("ALUI", "add", "sf_j", "pi", 2)).label("SF.l")
    p.a(("ALUI", "add", "q_t", "pi", 11)).branch({0: "SF.l2"}, "RET", [("CMP", "sf_j", "q_t")])
    P("SF.l2").branch({0: "SF.line"}, "RET", [("CMP", "sf_j", "nl")])
    p = P("SF.line")
    p.a(("LDX", "q_t", "sf_j", LSS), ("JUMP", "q_t"), ("MARK", "sf_l0")).goto("SF.p1")
    lit("SF.p1", "  .st [r", "SF.st1", "SF.p2x")
    P("SF.p2x").a(("JUMP", "sf_l0")).goto("SF.p2")
    lit("SF.p2", "  store64 [r", "SF.st2", "SF.other")
    P("SF.st1").a(("LDI", "sf_st", 1)).goto("SF.sa")
    P("SF.st2").a(("LDI", "sf_st", 2)).goto("SF.sa")
    g.els("SF.sa", "SF.sa2", [("BYTE", "q_t")])
    p = P("SF.sa2")
    p.branch({1: "SF.sb"}, "RET", [("CMP", "q_t", "sf_a")])
    g.els("SF.sb", "SF.sc", [("ADV",)])
    lit("SF.sc", "+0], r", "SF.sv", "RET")
    g.on("SF.sv", DIGIT, "SF.svd", [("LDI", "sf_v", 0)])
    g.els("SF.sv", "RET", [])
    g.on("SF.svd", DIGIT, "SF.svd", [("BYTE", "q_bt"), ("ALUI", "sub", "q_bt", "q_bt", 48),
                                     ("ALUI", "mul", "sf_v", "sf_v", 10), ("ALU", "add", "sf_v", "sf_v", "q_bt"), ("ADV",)])
    g.els("SF.svd", "SF.sve", [("MARK", "sf_q")])
    p = P("SF.sve")
    p.branch({1: "RET"}, "SF.v2", [("CMP", "sf_v", "sf_ad")])
    P("SF.v2").branch({1: "RET"}, "SF.v3", [("CMPI", "sf_v", 2)])
    p = P("SF.v3")
    p.branch({1: "SF.t1"}, "SF.t2", [("CMPI", "sf_st", 1)])
    g.on("SF.t2", NL, "SF.dead", [])
    g.els("SF.t2", "RET", [])
    g.on("SF.t1", [44], "SF.t1r", [])
    g.els("SF.t1", "RET", [])
    g.on("SF.t1r", NL, "SF.dead", [("MARK", "sf_q1")])
    g.els("SF.t1r", "SF.t1r", [("ADV",)])
    p = P("SF.dead")
    p.a(("COPYW", "dz", "sf_ad"), ("ALUI", "add", "dfrom", "sf_j", 1)).call("DEADQ").branch({1: "SF.dead2"}, "RET", [("CMPI", "dv", 1)])
    p = P("SF.dead2")
    p.a(("LDI", "dz", 2), ("ALUI", "add", "dfrom", "sf_j", 1)).call("DEADQ").branch({1: "SF.emit"}, "RET", [("CMPI", "dv", 1)])
    p = P("SF.emit")
    p.a(("ALUI", "add", "q_t", "pi", 2), ("LDX", "q_t", "q_t", LSS), ("SPAN2", "q_t", "sf_l0"))
    p.branch({1: "SF.e1"}, "SF.e2", [("CMPI", "sf_st", 1)])
    P("SF.e1").o("  .st [r6-").a(("SPAN2", "sf_n0", "sf_n1")).o("], r").num("sf_v").a(("SPAN2", "sf_q", "sf_q1")).o("\n").goto("SF.ok")
    P("SF.e2").o("  store64 [r6-").a(("SPAN2", "sf_n0", "sf_n1")).o("], r").num("sf_v").o("\n").goto("SF.ok")
    P("SF.ok").a(("ALUI", "add", "sk", "sf_j", 1)).ret()
    # not a store: M's line must be simple and name neither rA nor r2
    p = P("SF.other")
    p.a(("LDX", "q_t", "sf_j", KK)).branch({1: "SF.o2"}, "RET", [("CMPI", "q_t", K_SIMPLE)])
    p = P("SF.o2")
    p.a(("JUMP", "sf_l0"), ("COPYW", "nr", "sf_ad")).call("NAMES").branch({1: "RET"}, "SF.o3", [("CMPI", "found", 1)])
    p = P("SF.o3")
    p.a(("JUMP", "sf_l0"), ("LDI", "nr", 2)).call("NAMES").branch({1: "RET"}, "SF.o4", [("CMPI", "found", 1)])
    P("SF.o4").a(("ALUI", "add", "sf_j", "sf_j", 1)).goto("SF.l")


def peepround():
    """PPASS: one peep round over x (after ANALYZE with z 0..5): per line, the relation of it
    and its neighbour, the peep table's action for (acls, bcls, rel), and the rewrite"""
    p = P("PPASS")
    p.a(("LDI", "pi", 0)).goto("PP0")
    p = P("PP0")
    p.branch({0: "PP.line"}, "RET", [("CMP", "pi", "nl")])
    p = P("PP.line")
    p.a(("LDX", "q_t", "pi", LSS), ("JUMP", "q_t"), ("LDX", "q_k", "pi", KK)).branch({1: "PP.copy"}, "PP.sf", [("CMPI", "q_k", K_LABEL)])
    p = P("PP.copy")          # the line as it stands
    p.a(("LDX", "q_t", "pi", LSS), ("JUMP", "q_t")).call("COPYL").a(("ALUI", "add", "pi", "pi", 1)).goto("PP0")
    p = P("PP.sf")
    p.call("STFUSE").branch({1: "PP.a"}, "PP.sfok", [("CMPI", "sk", 0)])
    P("PP.sfok").a(("COPYW", "pi", "sk"), ("ALUI", "add", "hits", "hits", 1)).goto("PP0")
    # a = acls(A); b = bcls(i+1); rel = none
    p = P("PP.a")
    p.a(("LDX", "q_w", "pi", WIDD), ("LDI", "pa_", PA.index("other")), ("LDI", "rel", PR.index("none")),
        ("LDI", "pt", -1), ("LDI", "px", -1), ("LDI", "py", -1)).branch({0: "PP.b"}, "PP.a1", [("CMPI", "q_w", 0)])
    p = P("PP.a1")
    p.a(("LDX", "q_t", "q_w", ACLSB)).branch({1: "PP.b"}, "PP.a2", [("CMPI", "q_t", 0)])
    P("PP.a2").a(("ALUI", "sub", "pa_", "q_t", 1)).goto("PP.b")
    p = P("PP.b")
    p.a(("ALUI", "add", "bl_", "pi", 1)).call("BCLS").a(("COPYW", "pb_", "bc"))
    p.a(("LDX", "q_w", "pi", WIDD)).branch({1: "PP.jmp"}, "PP.j2", [("CMP", "q_w", "id_jump")])
    P("PP.j2").branch({1: "PP.jmp"}, "PP.st", [("CMP", "q_w", "id_jumpz")])
    # jump/jumpz: to the very next label (to_next) or to a line that is itself a jump (to_jump)
    p = P("PP.jmp")
    p.a(("LDX", "q_ts", "pi", TSS), ("LDX", "q_te", "pi", TEE), ("INTERN", "pname", "q_ts", "q_te"),
        ("LDI", "q_u", 0), ("ALUI", "add", "q_kk", "pi", 1)).label("PP.jl")
    p.branch({0: "PP.jl2"}, "PP.jd", [("CMP", "q_kk", "nl")])
    p = P("PP.jl2")
    p.a(("LDX", "q_t", "q_kk", ISLAB)).branch({1: "PP.jl3"}, "PP.jd", [("CMPI", "q_t", 1)])
    p = P("PP.jl3")
    p.a(("LDX", "q_ts", "q_kk", TSS), ("LDX", "q_te", "q_kk", TEE), ("INTERN", "q_id", "q_ts", "q_te"))
    p.branch({1: "PP.jl4"}, "PP.jl5", [("CMP", "q_id", "pname")])
    P("PP.jl4").a(("LDI", "q_u", 1)).goto("PP.jl5")
    P("PP.jl5").a(("ALUI", "add", "q_kk", "q_kk", 1)).goto("PP.jl")
    p = P("PP.jd")
    p.branch({1: "PP.tn"}, "PP.tj", [("CMPI", "q_u", 1)])
    p = P("PP.tn")
    p.a(("ALUI", "add", "rl_", "pi", 1)).call("REAL").a(("COPYW", "bl_", "rl_")).call("BCLS")
    p.a(("COPYW", "pb_", "bc"), ("LDI", "rel", PR.index("to_next"))).goto("PP.act")
    p = P("PP.tj")
    p.a(("ALU", "add", "q_a", "q_lab", "pname"), ("LDX", "q_t", "q_a", 0)).branch({1: "PP.act"}, "PP.tj1", [("CMPI", "q_t", 0)])
    p = P("PP.tj1")
    p.a(("COPYW", "rl_", "q_t")).call("REAL").a(("COPYW", "pt", "rl_")).branch({0: "PP.tj2"}, "PP.act", [("CMP", "pt", "nl")])
    p = P("PP.tj2")
    p.a(("LDX", "q_t", "pt", WIDD)).branch({1: "PP.tj3"}, "PP.act", [("CMP", "q_t", "id_jump")])
    p = P("PP.tj3")
    p.a(("LDX", "q_ts", "pt", TSS), ("LDX", "q_te", "pt", TEE), ("INTERN", "q_id", "q_ts", "q_te"))
    p.branch({1: "PP.act"}, "PP.tj4", [("CMP", "q_id", "pname")])
    p = P("PP.tj4")
    p.a(("COPYW", "bl_", "pt")).call("BCLS").a(("COPYW", "pb_", "bc"), ("LDI", "rel", PR.index("to_jump"))).goto("PP.act")
    # a store and the next line: the same slot
    p = P("PP.st")
    p.a(("ALUI", "add", "q_t", "pi", 1)).branch({0: "PP.st1"}, "PP.h4", [("CMP", "q_t", "nl")])
    p = P("PP.st1")
    p.a(("LDX", "q_t", "pi", LSS), ("JUMP", "q_t")).call("PSTORE").branch({0: "PP.im"}, "PP.st2", [("CMPI", "st_x", 0)])
    p = P("PP.st2")
    p.a(("COPYW", "px", "st_x"), ("INTERN", "q_m", "st_ms", "st_me"), ("COPYW", "q_ms", "st_ms"), ("COPYW", "q_me", "st_me"),
        ("ALUI", "add", "q_t", "pi", 1), ("LDX", "q_t", "q_t", LSS), ("JUMP", "q_t"), ("MARK", "q_b0")).call("PLOAD")
    p.branch({0: "PP.st3"}, "PP.st4", [("CMPI", "ld_y", 0)])
    p = P("PP.st3")
    p.a(("JUMP", "q_b0")).call("PSTORE").a(("COPYW", "ld_y", "st_x"), ("COPYW", "ld_ms", "st_ms"), ("COPYW", "ld_me", "st_me")).goto("PP.st4")
    p = P("PP.st4")
    p.branch({0: "PP.h4"}, "PP.st5", [("CMPI", "ld_y", 0)])
    p = P("PP.st5")
    p.a(("INTERN", "q_m2", "ld_ms", "ld_me")).branch({1: "PP.st6"}, "PP.h4", [("CMP", "q_m", "q_m2")])
    p = P("PP.st6")          # a slot starting `r7` is not a slot
    p.a(("JUMP", "q_ms")).goto("PP.r7")
    lit("PP.r7", "r7", "PP.h4", "PP.st7")
    p = P("PP.st7")
    p.a(("COPYW", "py", "ld_y")).branch({1: "PP.ssr"}, "PP.ss", [("CMP", "px", "ld_y")])
    P("PP.ssr").a(("LDI", "rel", PR.index("same_slot_same_reg"))).goto("PP.h4")
    P("PP.ss").a(("LDI", "rel", PR.index("same_slot"))).goto("PP.h4")
    # an imm and the next line (only when the line was not a store)
    p = P("PP.im")
    p.a(("LDX", "q_t", "pi", LSS), ("JUMP", "q_t")).call("PIMM").branch({0: "PP.h4"}, "PP.im1", [("CMPI", "im_k", 0)])
    p = P("PP.im1")
    p.a(("COPYW", "px", "im_k"), ("LDI", "th_ok", 0)).branch({2: "PP.imv"}, "PP.im2", [("CMPI", "px", 5)])
    p = P("PP.im2")
    p.a(("ALUI", "add", "q_t", "pi", 1), ("LDX", "q_t", "q_t", WIDD)).branch({1: "PP.imv"}, "PP.im3", [("CMP", "q_t", "id_mov")])
    p = P("PP.im3")
    p.a(("ALUI", "add", "q_t", "pi", 1), ("LDX", "q_t", "q_t", LSS), ("JUMP", "q_t")).call("PTHREE").goto("PP.imv")
    p = P("PP.imv")
    p.branch({1: "PP.i3"}, "PP.imm", [("CMPI", "th_ok", 1)])
    p = P("PP.i3")
    p.branch({1: "PP.i3b"}, "PP.h4", [("CMP", "th_t", "px")])
    P("PP.i3b").branch({1: "PP.h4"}, "PP.i3c", [("CMP", "th_s", "px")])
    p = P("PP.i3c")
    p.a(("COPYW", "dz", "px"), ("ALUI", "add", "dfrom", "pi", 2)).call("DEADQ").branch({1: "PP.i3d"}, "PP.h4", [("CMPI", "dv", 1)])
    p = P("PP.i3d")
    p.a(("LDI", "q_z", 0)).branch({1: "PP.c0"}, "PP.i3e", [("C64", "im_v", "q_z")])
    P("PP.c0").a(("LDI", "rel", PR.index("const0"))).goto("PP.h4")
    p = P("PP.i3e")
    p.a(("LDI", "q_z", 1)).branch({1: "PP.c1"}, "PP.i3f", [("C64", "im_v", "q_z")])
    P("PP.c1").a(("LDI", "rel", PR.index("const1"))).goto("PP.h4")
    p = P("PP.i3f")          # v & (v - 1) == 0
    p.a(("A64I", "sub", "q_z", "im_v", 1), ("A64", "and", "q_z", "q_z", "im_v"), ("LDI", "q_zz", 0))
    p.branch({1: "PP.p2"}, "PP.h4", [("C64", "q_z", "q_zz")])
    P("PP.p2").a(("LDI", "rel", PR.index("pow2"))).goto("PP.h4")
    p = P("PP.imm")          # not three registers: `mov rD, rX` with X dead after it
    p.branch({2: "PP.h4"}, "PP.imm1", [("CMPI", "px", 5)])
    p = P("PP.imm1")
    p.a(("ALUI", "add", "q_t", "pi", 1), ("LDX", "q_t", "q_t", LSS), ("JUMP", "q_t")).call("PMOV")
    p.branch({0: "PP.h4"}, "PP.imm2", [("CMPI", "mv_d", 0)])
    P("PP.imm2").branch({1: "PP.imm3"}, "PP.h4", [("CMP", "mv_s", "px")])
    P("PP.imm3").branch({1: "PP.h4"}, "PP.imm4", [("CMP", "mv_d", "px")])
    p = P("PP.imm4")
    p.a(("COPYW", "dz", "px"), ("ALUI", "add", "dfrom", "pi", 2)).call("DEADQ").branch({1: "PP.cd"}, "PP.h4", [("CMPI", "dv", 1)])
    P("PP.cd").a(("LDI", "rel", PR.index("copy_dead"))).goto("PP.h4")
    # [H4]: copy_into, dest_to_mov -- when rel is still none and there is a next line
    p = P("PP.h4")
    p.branch({1: "PP.h4a"}, "PP.act", [("CMPI", "rel", PR.index("none"))])
    p = P("PP.h4a")
    p.a(("ALUI", "add", "q_t", "pi", 1)).branch({0: "PP.h4b"}, "PP.ad", [("CMP", "q_t", "nl")])
    p = P("PP.h4b")
    p.a(("LDX", "q_t", "pi", LSS), ("JUMP", "q_t")).call("PMOV").branch({0: "PP.dm"}, "PP.ci", [("CMPI", "mv_d", 0)])
    p = P("PP.ci")          # mov rY, rX; the next line simple, reads rY, does not write it; rY dead after
    p.branch({2: "PP.dm"}, "PP.ci1", [("CMPI", "mv_d", 5)])
    P("PP.ci1").branch({1: "PP.dm"}, "PP.ci2", [("CMP", "mv_d", "mv_s")])
    p = P("PP.ci2")
    p.a(("ALUI", "add", "q_t", "pi", 1), ("LDX", "q_k", "q_t", KK)).branch({1: "PP.ci3"}, "PP.dm", [("CMPI", "q_k", K_SIMPLE)])
    p = P("PP.ci3")
    p.a(("ALUI", "add", "q_t", "pi", 1), ("LDX", "q_r", "q_t", RMM), ("LDX", "q_w", "q_t", WMM), ("LDI", "q_b", 1),
        ("ALU", "shl", "q_b", "q_b", "mv_d"), ("ALU", "and", "q_r", "q_r", "q_b"), ("ALU", "and", "q_w", "q_w", "q_b"))
    p.branch({1: "PP.dm"}, "PP.ci4", [("CMPI", "q_r", 0)])
    P("PP.ci4").branch({1: "PP.ci5"}, "PP.dm", [("CMPI", "q_w", 0)])
    p = P("PP.ci5")
    p.a(("COPYW", "dz", "mv_d"), ("ALUI", "add", "dfrom", "pi", 2)).call("DEADQ").branch({1: "PP.ci6"}, "PP.dm", [("CMPI", "dv", 1)])
    P("PP.ci6").a(("LDI", "rel", PR.index("copy_into")), ("COPYW", "py", "mv_d"), ("COPYW", "px", "mv_s")).goto("PP.act")
    p = P("PP.dm")          # a simple non-store line writing rD, then `mov rE, rD` with D dead after
    p.a(("LDX", "q_k", "pi", KK)).branch({1: "PP.dm1"}, "PP.ad", [("CMPI", "q_k", K_SIMPLE)])
    p = P("PP.dm1")
    p.a(("LDX", "q_w", "pi", WIDD)).branch({1: "PP.ad"}, "PP.dm2", [("CMP", "q_w", "id_store64")])
    P("PP.dm2").branch({1: "PP.ad"}, "PP.dm3", [("CMP", "q_w", "id_st")])
    p = P("PP.dm3")
    p.a(("LDX", "q_db", "pi", FRR)).branch({0: "PP.ad"}, "PP.dm4", [("CMPI", "q_db", 0)])
    P("PP.dm4").branch({2: "PP.ad"}, "PP.dm5", [("CMPI", "q_db", 5)])
    p = P("PP.dm5")
    p.a(("LDX", "q_w", "pi", WMM), ("LDI", "q_b", 1), ("ALU", "shl", "q_b", "q_b", "q_db"), ("ALU", "and", "q_w", "q_w", "q_b"))
    p.branch({1: "PP.ad"}, "PP.dm6", [("CMPI", "q_w", 0)])
    p = P("PP.dm6")
    p.a(("ALUI", "add", "q_t", "pi", 1), ("LDX", "q_t", "q_t", LSS), ("JUMP", "q_t")).call("PMOV")
    p.branch({0: "PP.ad"}, "PP.dm7", [("CMPI", "mv_d", 0)])
    P("PP.dm7").branch({1: "PP.dm8"}, "PP.ad", [("CMP", "mv_s", "q_db")])
    P("PP.dm8").branch({1: "PP.ad"}, "PP.dm9", [("CMP", "mv_d", "q_db")])
    p = P("PP.dm9")
    p.a(("COPYW", "dz", "q_db"), ("ALUI", "add", "dfrom", "pi", 2)).call("DEADQ").branch({1: "PP.dm10"}, "PP.ad", [("CMPI", "dv", 1)])
    P("PP.dm10").a(("LDI", "rel", PR.index("dest_to_mov")), ("COPYW", "px", "q_db"), ("COPYW", "py", "mv_d")).goto("PP.act")
    # a_dead: a simple non-store line whose destination rD is dead right after it
    p = P("PP.ad")
    p.branch({1: "PP.ad0"}, "PP.act", [("CMPI", "rel", PR.index("none"))])
    p = P("PP.ad0")
    p.a(("LDX", "q_k", "pi", KK)).branch({1: "PP.ad1"}, "PP.act", [("CMPI", "q_k", K_SIMPLE)])
    p = P("PP.ad1")
    p.a(("LDX", "q_w", "pi", WIDD)).branch({1: "PP.act"}, "PP.ad2", [("CMP", "q_w", "id_store64")])
    P("PP.ad2").branch({1: "PP.act"}, "PP.ad3", [("CMP", "q_w", "id_st")])
    p = P("PP.ad3")
    p.a(("LDX", "q_db", "pi", FRR)).branch({0: "PP.act"}, "PP.ad4", [("CMPI", "q_db", 0)])
    P("PP.ad4").branch({2: "PP.act"}, "PP.ad5", [("CMPI", "q_db", 5)])
    p = P("PP.ad5")
    p.a(("LDX", "q_w", "pi", WMM), ("LDI", "q_b", 1), ("ALU", "shl", "q_b", "q_b", "q_db"), ("ALU", "and", "q_w", "q_w", "q_b"))
    p.branch({1: "PP.act"}, "PP.ad6", [("CMPI", "q_w", 0)])
    p = P("PP.ad6")
    p.a(("COPYW", "dz", "q_db"), ("ALUI", "add", "dfrom", "pi", 1)).call("DEADQ").branch({1: "PP.ad7"}, "PP.act", [("CMPI", "dv", 1)])
    P("PP.ad7").a(("LDI", "rel", PR.index("a_dead"))).goto("PP.act")
    # the table's answer and the rewrite
    p = P("PP.act")
    p.branch({1: "PP.copy"}, "PP.ask", [("CMPI", "rel", PR.index("none"))])
    p = P("PP.ask")
    p.a(("ALUI", "mul", "q_t", "pa_", len(PB)), ("ALU", "add", "q_t", "q_t", "pb_"), ("ALUI", "mul", "q_t", "q_t", len(PR)),
        ("ALU", "add", "q_t", "q_t", "rel"), ("LDX", "act", "q_t", PEEPB), ("ALUI", "sub", "act", "act", 1))
    acts = {PY.index(y): "PX." + y for y in PY if y not in ("-", "keep")}
    p.branch(acts, "PP.copy", [("RLD", "act")])
    A = ("LDX", "q_t", "pi", LSS)
    P("PX.load_to_mov").a(A, ("JUMP", "q_t")).call("COPYL").o("  mov r").num("py").o(", r").num("px").o("\n").goto("PX.i2")
    P("PX.drop_b").a(A, ("JUMP", "q_t")).call("COPYL").goto("PX.i2")
    P("PX.drop_a").goto("PX.i1")
    P("PX.retarget").a(("LDX", "q_ts", "pi", LSS), ("LDX", "q_te", "pi", TSS), ("SPAN2", "q_ts", "q_te"),
                       ("LDX", "q_ts", "pt", TSS), ("LDX", "q_te", "pt", TEE), ("SPAN2", "q_ts", "q_te")).o("\n").goto("PX.i1")
    P("PX.to_mov").o("  mov r").num("th_d").o(", r").num("th_s").o("\n").goto("PX.i2")
    p = P("PX.to_shl")      # imm rX, log2(v) / shl64 rD, rS, rX
    p.a(("COPYW", "q_z", "im_v"), ("LDI", "q_lg", -1), ("LDI", "q_zz", 0)).label("PX.lg")
    p.branch({1: "PX.lgd"}, "PX.lg1", [("C64", "q_z", "q_zz")])
    P("PX.lg1").a(("A64I", "shr", "q_z", "q_z", 1), ("ALUI", "add", "q_lg", "q_lg", 1)).goto("PX.lg")
    P("PX.lgd").o("  imm r").num("px").o(", ").num("q_lg").o("\n  shl64 r").num("th_d").o(", r").num("th_s").o(", r").num("px").o("\n").goto("PX.i2")
    P("PX.retarget_dest").a(A, ("JUMP", "q_t"), ("COPYW", "rr_a", "px"), ("COPYW", "rr_b", "py"), ("LDI", "rr_all", 0)).call("REREG").o("\n").goto("PX.i2")
    P("PX.fold_copy").a(("ALUI", "add", "q_t", "pi", 1), ("LDX", "q_t", "q_t", LSS), ("JUMP", "q_t"),
                        ("COPYW", "rr_a", "py"), ("COPYW", "rr_b", "px"), ("LDI", "rr_all", 1)).call("REREG").o("\n").goto("PX.i2")
    P("PX.fold_imm").o("  imm r").num("mv_d").o(", ").a(("LDI", "nx", 1), ("COPYW", "nv", "im_v")).call("NUMOUT").o("\n").goto("PX.i2")
    P("PX.i1").a(("ALUI", "add", "pi", "pi", 1), ("ALUI", "add", "hits", "hits", 1)).goto("PP0")
    P("PX.i2").a(("ALUI", "add", "pi", "pi", 2), ("ALUI", "add", "hits", "hits", 1)).goto("PP0")
    # BCLS(bl_) -> bc: `none` past the end; `other` for a label line or an unknown word
    p = P("BCLS")
    p.a(("LDI", "bc", PB.index("none"))).branch({0: "BC.1"}, "RET", [("CMP", "bl_", "nl")])
    p = P("BC.1")
    p.a(("LDI", "bc", PB.index("other")), ("LDX", "q_w", "bl_", WIDD)).branch({0: "RET"}, "BC.2", [("CMPI", "q_w", 0)])
    p = P("BC.2")
    p.a(("LDX", "q_t", "q_w", BCLSB)).branch({1: "RET"}, "BC.3", [("CMPI", "q_t", 0)])
    P("BC.3").a(("ALUI", "sub", "bc", "q_t", 1)).ret()
    # REAL(rl_): past label lines
    p = P("REAL")
    p.label("RL.l")
    p.branch({0: "RL.1"}, "RET", [("CMP", "rl_", "nl")])
    p = P("RL.1")
    p.a(("LDX", "q_t", "rl_", ISLAB)).branch({1: "RL.2"}, "RET", [("CMPI", "q_t", 1)])
    P("RL.2").a(("ALUI", "add", "rl_", "rl_", 1)).goto("RL.l")


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
