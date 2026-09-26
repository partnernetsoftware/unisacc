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


def build():
    E.prn()
    procs()
    p = P("START")
    for f in E.gold("opinfo"):
        if len(f) >= 2 and f[1] == "1":
            p.a(("SBCLR",), [("SBOUT", c) for c in f[0].encode()], ("SBINTERN", "t"), ("LDI", "u", 1), ("STX", "t", SIMPLE, "u"))
    p.a(("LDI", "rnd", 0), ("LDI", "hits", 0)).goto("L0")

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
    p.call("NAMES").branch({1: "COPY1"}, "OK.s", [("CMPI", "found", 1)])
    p = P("OK.s")
    p.a(("JUMP", "cur")).call("SKIPL").goto("OK.l")
    p = P("OK.d")
    p.branch({1: "OK.xy"}, "OK.emit", [("CMP", "X", "Y")])
    P("OK.xy").branch({1: "OK.emit"}, "COPY1", [("CMP", "mid", "lp")])      # X == Y: only with M empty
    p = P("OK.emit")
    p.branch({1: "OK.m"}, "OK.mv", [("CMP", "X", "Y")])
    p = P("OK.mv")
    p.o("  mov r").num("Y").o(", r").num("X").o("\n").goto("OK.m")
    p = P("OK.m")
    p.a(("SPAN2", "mid", "lp"), ("JUMP", "after"), ("ALUI", "add", "hits", "hits", 1)).goto("L0")
    # COPY1: not a rewrite -- the line at ls as it stands
    p = P("COPY1")
    p.a(("JUMP", "ls")).call("COPYL").goto("L0")
    # ROUND: at most four rounds; a round with no rewrite ends the pass
    p = P("ROUND")
    p.branch({1: "DONE"}, "RND.n", [("CMPI", "hits", 0)])
    p = P("RND.n")
    p.a(("ALUI", "add", "rnd", "rnd", 1)).branch({1: "DONE"}, "RND.s", [("CMPI", "rnd", 4)])
    P("RND.s").a(("SWAP",), ("LDI", "hits", 0)).goto("L0")
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
