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
ends the pass.  -O2 (liveness, the peep table) is not here yet.

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
LABB = 50 * 10 ** 6              # LAB[round * 2e6 + intern id] = line + 1 of the first `name:` line
LIVEB = 60 * 10 ** 6             # LIVE[(round * 4 + z - 2) * 1e6 + block]
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
    g.els("A0", "A.blk", [("MARK", "q_p")])
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
        ("LDX", "q_t", "q_a", 0)).branch({1: "AL.set"}, "A.done", [("CMPI", "q_t", 0)])
    P("AL.set").a(("ALUI", "add", "q_t", "q_l", 1), ("STX", "q_a", 0, "q_t")).goto("A.done")
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
    p.a(("INTERN", "q_id", "q_ws", "q_we"))
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
    g.on("A.done", [10], "A0", [("ADV",), ("ALUI", "add", "q_l", "q_l", 1)])
    g.on("A.done", [EOF], "A.end", [("ALUI", "add", "q_l", "q_l", 1)])
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
    p.a(("LDI", "z", 2)).label("SV.z")
    p.branch({2: "SV.done"}, "SV.init", [("CMPI", "z", 5)])
    p = P("SV.init")
    p.a(("ALUI", "mul", "q_lb", "rnd", 4), ("ALU", "add", "q_lb", "q_lb", "z"), ("ALUI", "sub", "q_lb", "q_lb", 2),
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
    p.a(("ALUI", "mul", "sc_lb", "rnd", 4), ("ALU", "add", "sc_lb", "sc_lb", "sc_z"), ("ALUI", "sub", "sc_lb", "sc_lb", 2),
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


def build():
    E.prn()
    procs()
    if LEVEL >= 2:
        analysis()
        local()
    p = P("START")
    p.a(("LDI", "vsp", 0))
    for w in ("ret", "jump", "jumpz", "call", ".frame", "store64", ".st"):
        p.a(("SBCLR",), [("SBOUT", c) for c in w.encode()], ("SBINTERN", "id_" + w.strip(".")))
    for f in E.gold("opinfo"):
        if len(f) >= 2 and f[1] == "1":
            p.a(("SBCLR",), [("SBOUT", c) for c in f[0].encode()], ("SBINTERN", "t"), ("LDI", "u", 1), ("STX", "t", SIMPLE, "u"))
    p.a(("LDI", "rnd", 0), ("LDI", "hits", 0)).goto("RSTART")
    # RSTART: a round begins; at -O2 the liveness of the whole tape first
    p = P("RSTART")
    if LEVEL >= 2:
        p.call("ANALYZE").a(("LDI", "q_zero", 0), ("JUMP", "q_zero"))
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
    P("DONE").a(("ACCEPT",)).goto("DEAD")
    g.finish()
    states = {n: [m, {str(k): v for k, v in row.items()}] for n, (m, row) in g.st.items()}
    return {"start": "START", "states": states, "seqs": [list(map(list, s)) for s in g.seqs]}


if __name__ == "__main__":
    d = build()
    s = json.dumps(d, separators=(",", ":"))
    open(sys.argv[1], "w").write(s)
    st, ent, live, ns, na = E.sizes(d)
    sys.stderr.write("states %d  entries %d  action seqs %d (%d actions)  json %d B\n" % (st, ent, ns, na, len(s)))
