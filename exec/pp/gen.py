"""E2 feasibility, MINIMUM slice: the preprocessor as one finite delta.

    python3 exec/pp/gen.py [out.json]      -> writes the table, prints sizes

Machine and primitives: exec/pp/sim.py (generic, option A); design:
research/e2-pp-delta.md.  The delta runs the reference's passes in order,
each pass reading x and writing o, SWAP between passes:

  P0 shebang + splice      src/front_pp.c splice(), fe_load's shebang blank
  P1 decomment             decomment()            (reject: unterminated comment)
  P3 directives            preprocess(): #ifdef #ifndef #else #endif #define
                           #undef #include, unknown directives and dead lines
                           blanked; a live #include splices the file in and
                           re-runs P0, P1 on the whole buffer (as incdo does)
  P4 expansion rounds      expandsrc()/emitrange()/emitbody(), OBJECT-LIKE
                           macros only, <= 8 rounds, segment per byte

NOT covered (the delta rejects with a `not covered: ...` code, never guesses):
#if/#elif expressions, _Pragma; function-like calls with zero parameters,
variadic, # or ##, an argument containing `(`, or a function-like name in a body.  Also not
modelled: autoinc() (the on-demand header prepend -- compare against the
reference built without it, see compare.py), #pragma push_macro/pop_macro
(rejected as not covered when live and spelled exactly; the reference's
prefix match `push_macroX` is not reproduced), -D/-U/-I/-include, the
file:line:col rendering of diagnostics (the reject kind is compared, not the
text).

Derived, not typed in: the directive vocabulary from DIRV
(kernel/unisa_model.inc), and what each (directive, defined) pair does from
weights/gold/pp.tsv (the shipped pp table).  Transcribed from
src/front_pp.c: byte classes, the predefined macros of predef() for the
default -E target (lnx/x86_64), the include search order.
"""
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
EOF = 256

_inc = open(os.path.join(ROOT, "kernel", "unisa_model.inc"), encoding="latin-1").read()
DIRV = tuple(re.search(r'char \*DIRV = "((?:[^"\\]|\\.)*)";', _inc).group(1).split("\\0")[:-1])


def load_pp_table():
    head, T = None, {}
    for ln in open(os.path.join(ROOT, "weights", "gold", "pp.tsv"), encoding="utf-8"):
        f = ln.rstrip("\n").split("\t")
        if f[0] == "#head":
            head = f[3:]
            continue
        if ln.startswith("#") or len(f) < 3 or f[0] == "dir":
            continue
        T[(f[0], int(f[1]))] = f[2]
    return head, T


PPHEAD, PPT = load_pp_table()          # head: take skip pop macro -> a = 0..3
assert PPHEAD == ["take", "skip", "pop", "macro"], PPHEAD

AL = set(range(97, 123)) | set(range(65, 91)) | {95}
DI = set(range(48, 58))
ID = AL | DI
WS = {32, 9}
PREDEF = ["__linux__", "__unix__", "__ELF__", "__x86_64__", "__LP64__", "__UNISA__"]
SEGINF = 1000000000
# W regions (addresses; plain named slots are strings)
DIRB, NEWB, MACB, TAKEB, SEENB = 10 ** 7, 2 * 10 ** 7, 5 * 10 ** 7, 6 * 10 ** 7, 61 * 10 ** 6
SPLB, IRLN, IRNL = 11 * 10 ** 7, 12 * 10 ** 7, 121 * 10 ** 6
F_NAME, F_BODY, F_FN, F_FROM, F_TO, F_PREV, F_ACT, F_UP = 0, 1, 2, 3, 4, 5, 6, 7
F_NP, F_P0, MAXP = 8, 9, 8          # function-like: parameter count, parameter ids
FSZ = F_P0 + MAXP
ARGB, ARGE = 62 * 10 ** 6, 63 * 10 ** 6   # argument blobs; the argument frame's entry
# F_FN: 0 object-like, 1 function-like (covered), 2 function-like not covered
# (zero parameters, variadic, more than MAXP, malformed)


class G:
    def __init__(self):
        self.st = {}
        self.seqs, self.seqix = [], {}
        self.labels = set()
        self.unreach = 0

    def seq(self, acts):
        acts = tuple(tuple(a) for a in acts)
        if acts not in self.seqix:
            self.seqix[acts] = len(self.seqs)
            self.seqs.append(acts)
        return self.seqix[acts]

    def on(self, name, keys, nxt, acts=(), mode="b"):
        if name not in self.st:
            self.st[name] = [mode, {}]
        assert self.st[name][0] == mode, name
        row = self.st[name][1]
        for k in keys:
            if k not in row:
                row[k] = (nxt, self.seq(acts))

    def els(self, name, nxt, acts=(), mode="b"):
        self.on(name, range(257), nxt, acts, mode)

    def r(self, name, cases):          # a state that reads r
        for keys, (nxt, acts) in cases.items():
            self.on(name, keys if isinstance(keys, tuple) else (keys,), nxt, acts, "r")

    def call(self, sub, ret):
        self.labels.add(ret)
        return sub, [("PUSH", ret)]

    def finish(self):
        self.st["RET"] = ["t", {g: (g, self.seq([("POP",)])) for g in sorted(self.labels)}]
        self.st["RET"][1]["BOT"] = ("DEAD", self.seq([("REJECT", "unreachable")]))
        for name, (mode, row) in self.st.items():
            if mode in "br":
                for k in range(257):
                    if k not in row:
                        row[k] = ("DEAD", self.seq([("REJECT", "unreachable")]))
                        self.unreach += 1
        self.els("DEAD", "DEAD", [("REJECT", "unreachable")])


PUSHM = [("STX", "me", F_UP, "CUR"), ("COPYW", "CUR", "me"), ("LDI", "one", 1),
         ("STX", "me", F_ACT, "one"), ("ALUI", "add", "DEP", "DEP", 1),
         ("LDX", "BB", "me", F_BODY), ("INPUSH", "BB")]


def ea(dst, e):              # W[dst] := address of macro entry W[e]
    return [("ALUI", "mul", dst, e, FSZ), ("ALUI", "add", dst, dst, MACB)]


def sbconst(s):
    return [("SBCLR",)] + [("SBOUT", c) for c in s.encode()]


def build():
    g = G()
    NC = lambda what: [("REJECT", "not covered: " + what)]   # noqa: E731

    # ---- init: constant ids (built byte by byte, then interned) ---------
    init = []
    for k, w in enumerate(DIRV):
        init += sbconst(w) + [("SBINTERN", "t"), ("ALUI", "add", "a", "t", DIRB),
                              ("LDI", "v", k + 1), ("STX", "a", 0, "v")]
    init += sbconst("pragma") + [("SBINTERN", "t"), ("ALUI", "add", "a", "t", DIRB),
                                 ("LDI", "v", 100), ("STX", "a", 0, "v")]
    init += sbconst("_Pragma") + [("SBINTERN", "ID_PRAGMAOP")]
    init += sbconst("push_macro") + [("SBINTERN", "ID_PUSHM")]
    init += sbconst("pop_macro") + [("SBINTERN", "ID_POPM")]
    for w, nm in (("0", "ID_0"), ("1", "ID_1"), ("defined", "ID_DEFD")):
        init += sbconst(w) + [("SBINTERN", nm)]
    init += [("LDI", "RUN", 0), ("LDI", "FP", 0)]
    g.els("START", "P0S", init)

    # ---- P0: shebang, then splice -----------------------------------------
    g.on("P0S", [35], "P0S1", [("MARK", "A"), ("ADV",)])
    g.els("P0S", "P0")
    g.on("P0S1", [33], "P0SB", [("JUMP", "A")])
    g.els("P0S1", "P0", [("JUMP", "A")])
    g.on("P0SB", [10, EOF], "P0")
    g.els("P0SB", "P0SB", [("OUT", 32), ("ADV",)])
    rec = [("OLEN", "t"), ("ALUI", "add", "a", "NSPL", SPLB), ("STX", "a", 0, "t"),
           ("ALUI", "add", "NSPL", "NSPL", 1)]
    g.on("P0", [92], "P0B", [("MARK", "A"), ("ADV",)])
    g.on("P0", [EOF], "P1", [("SWAP",)])
    g.els("P0", "P0", [("COPY",), ("ADV",)])
    g.on("P0B", [10], "P0", [("ADV",)] + rec)
    g.on("P0B", [13], "P0BR", [("ADV",)])
    g.els("P0B", "P0", [("JUMP", "A"), ("COPY",), ("ADV",)])
    g.on("P0BR", [10], "P0", [("ADV",)] + rec)
    g.els("P0BR", "P0", [("JUMP", "A"), ("COPY",), ("ADV",)])

    # ---- P1: decomment ------------------------------------------------------
    g.on("P1", [34], "P1Q34", [("COPY",), ("ADV",)])
    g.on("P1", [39], "P1Q39", [("COPY",), ("ADV",)])
    g.on("P1", [47], "P1SL", [("MARK", "A"), ("ADV",)])
    g.on("P1", [EOF], "P3START", [("SWAP",)])
    g.els("P1", "P1", [("COPY",), ("ADV",)])
    for q in (34, 39):
        g.on("P1Q%d" % q, [92], "P1Q%dE" % q, [("COPY",), ("ADV",)])
        g.on("P1Q%d" % q, [q], "P1", [("COPY",), ("ADV",)])
        g.on("P1Q%d" % q, [EOF], "P1")
        g.els("P1Q%d" % q, "P1Q%d" % q, [("COPY",), ("ADV",)])
        g.on("P1Q%dE" % q, [EOF], "P1")
        g.els("P1Q%dE" % q, "P1Q%d" % q, [("COPY",), ("ADV",)])
    g.on("P1SL", [47], "P1LC")
    g.on("P1SL", [42], "P1BC", [("ADV",), ("LDI", "NL", 0)])
    g.els("P1SL", "P1", [("JUMP", "A"), ("COPY",), ("ADV",)])
    g.on("P1LC", [10, EOF], "P1", [("OUT", 32)])
    g.els("P1LC", "P1LC", [("ADV",)])
    unterminated = [("REJECT", "unterminated comment")]
    g.on("P1BC", [10], "P1BC", [("ALUI", "add", "NL", "NL", 1), ("ADV",)])
    g.on("P1BC", [42], "P1BS", [("ADV",)])
    g.on("P1BC", [EOF], "DEAD", unterminated)
    g.els("P1BC", "P1BC", [("ADV",)])
    g.on("P1BS", [47], "P1NL", [("ADV",), ("OUT", 32), ("CMPI", "NL", 0)])
    g.on("P1BS", [EOF], "DEAD", unterminated)
    g.els("P1BS", "P1BC")
    g.r("P1NL", {2: ("P1NL", [("OUT", 10), ("ALUI", "sub", "NL", "NL", 1), ("CMPI", "NL", 0)]),
                 1: ("P1", [])})

    # ---- MFIND: M := entry for id NID live in segment SEGQ (-1: now) -------
    g.els("MFIND", "MF1", [("ALUI", "add", "mfa", "NID", NEWB), ("LDX", "mft", "mfa", 0),
                            ("ALUI", "sub", "M", "mft", 1), ("CMPI", "SEGQ", 0)])
    g.r("MF1", {0: ("MFN", [("CMPI", "M", 0)]), (1, 2): ("MFW", [("CMPI", "M", 0)])})
    g.r("MFN", {0: ("RET", []), (1, 2): ("MFN2", ea("mfe", "M") + [("LDX", "mft", "mfe", F_TO),
                                                                   ("CMPI", "mft", SEGINF)])})
    g.r("MFN2", {0: ("RET", [("LDI", "M", -1)]), (1, 2): ("RET", [])})
    g.r("MFW", {0: ("RET", []), (1, 2): ("MFW2", ea("mfe", "M") + [("LDX", "mft", "mfe", F_FROM),
                                                                   ("CMP", "mft", "SEGQ")])})
    prev = ("MFW", [("LDX", "M", "mfe", F_PREV), ("CMPI", "M", 0)])
    g.r("MFW2", {2: prev, (0, 1): ("MFW3", [("LDX", "mft", "mfe", F_TO), ("CMP", "SEGQ", "mft")])})
    g.r("MFW3", {0: ("RET", []), (1, 2): prev})

    # ---- MDEF: a new entry for NID (mdef), address in EA ---------------------
    g.els("MDEF", "MD1", [("ALUI", "add", "mda", "NID", NEWB), ("LDX", "mdt", "mda", 0),
                           ("ALUI", "sub", "OLD", "mdt", 1), ("CMPI", "OLD", 0)])
    g.r("MD1", {0: ("MD2", []), (1, 2): ("MD1B", ea("mde", "OLD") + [("LDX", "mdt", "mde", F_TO),
                                                                     ("CMPI", "mdt", SEGINF)])})
    g.r("MD1B", {0: ("MD2", []), (1, 2): ("MD2", [("STX", "mde", F_TO, "CURSEG")])})
    g.els("MD2", "RET", ea("EA", "NMAC") + [
        ("STX", "EA", F_FROM, "CURSEG"), ("LDI", "mdt", SEGINF), ("STX", "EA", F_TO, "mdt"),
        ("STX", "EA", F_PREV, "OLD"), ("STX", "EA", F_NAME, "NID"), ("LDI", "mdt", 0),
        ("STX", "EA", F_BODY, "mdt"), ("STX", "EA", F_FN, "mdt"),
        ("ALUI", "add", "mda", "NID", NEWB), ("ALUI", "add", "mdt", "NMAC", 1),
        ("STX", "mda", 0, "mdt"), ("ALUI", "add", "NMAC", "NMAC", 1)])

    # ---- P3: directives -------------------------------------------------------
    g.els("P3START", "P3S2", [("RLD", "RUN")])
    # predef(): six object-like macros with body "1" (tgt lnx/x86_64)
    chain = "P3PD0"
    g.r("P3S2", {0: (chain, [("LDI", "RUN", 1), ("LDI", "CURSEG", 0), ("SETOT", "CURSEG")]),
                 1: ("P3L0", [("LDI", "Z", 0), ("SPAN2", "Z", "RESUME"), ("JUMP", "RESUME")])})
    for k, nm in enumerate(PREDEF):
        nxt = "P3PD%d" % (k + 1) if k + 1 < len(PREDEF) else "P3L0"
        sub, pu = g.call("MDEF", "P3PDR%d" % k)
        g.els("P3PD%d" % k, sub, sbconst(nm) + [("SBINTERN", "NID")] + pu)
        g.els("P3PDR%d" % k, nxt, sbconst("1") + [("SBSAVE", "t"), ("STX", "EA", F_BODY, "t")])

    # line start: live := every open level taking
    g.els("P3L0", "LIVEL", [("MARK", "LS"), ("LDI", "LIVE", 1), ("LDI", "K", 0),
                            ("CMP", "K", "NDEPTH")])
    g.r("LIVEL", {0: ("LIVE2", [("ALUI", "add", "a", "K", TAKEB), ("LDX", "t", "a", 0), ("RLD", "t")]),
                  (1, 2): ("P3WS", [])})
    g.r("LIVE2", {0: ("P3WS", [("LDI", "LIVE", 0)]),
                  tuple(range(1, 257)): ("LIVEL", [("ALUI", "add", "K", "K", 1), ("CMP", "K", "NDEPTH")])})
    g.on("P3WS", WS, "P3WS", [("ADV",)])
    g.on("P3WS", [35], "DIR", [("ADV",)])
    g.els("P3WS", "P3LINE", [("JUMP", "LS"), ("RLD", "LIVE")])
    g.r("P3LINE", {1: ("P3COPY", []), 0: ("P3BLANK", [])})
    g.on("P3COPY", [10], "P3L0", [("COPYT",), ("ADV",), ("ALUI", "add", "LINES", "LINES", 1)])
    g.on("P3COPY", [EOF], "P4", [("SWAP",), ("LDI", "ESEG", 0), ("SETOT", "ESEG")])
    g.els("P3COPY", "P3COPY", [("COPYT",), ("ADV",)])
    g.on("P3BLANK", [10], "P3L0", [("COPYT",), ("ADV",), ("ALUI", "add", "LINES", "LINES", 1)])
    g.on("P3BLANK", [EOF], "P4", [("SWAP",), ("LDI", "ESEG", 0), ("SETOT", "ESEG")])
    g.els("P3BLANK", "P3BLANK", [("OUT", 32), ("ADV",)])
    blank = ("P3BLANK", [("JUMP", "LS")])

    g.on("DIR", WS, "DIR", [("ADV",)])
    g.els("DIR", "DW", [("MARK", "WS")])
    g.on("DW", AL, "DW", [("ADV",)])
    g.els("DW", "DWS", [("MARK", "WE"), ("INTERN", "DID", "WS", "WE")])
    g.on("DWS", WS, "DWS", [("ADV",)])
    g.els("DWS", "DN", [("MARK", "NS")])
    g.on("DN", ID, "DN", [("ADV",)])
    g.els("DN", "DEOL", [("MARK", "NE"), ("INTERN", "NID", "NS", "NE")])
    g.on("DEOL", [10, EOF], "DSW", [("MARK", "LE"), ("ALUI", "add", "a", "DID", DIRB),
                                    ("LDX", "DC", "a", 0), ("RLD", "DC")])
    g.els("DEOL", "DEOL", [("ADV",)])
    # DC: 0 unknown, k+1 = DIRV[k], 100 = pragma (push/pop_macro not covered: blanked)
    cases = {0: blank, 100: ("PRAG", [("RLD", "LIVE")])}
    # #pragma push_macro / pop_macro (live) are outside the slice: reject, do
    # not guess; any other #pragma is blanked, as pushpop() leaves it
    g.r("PRAG", {0: blank, 1: ("PRAG1", [("JUMP", "WE")])})
    g.on("PRAG1", WS, "PRAG1", [("ADV",)])
    g.els("PRAG1", "PRAG2", [("MARK", "PA")])
    g.on("PRAG2", AL, "PRAG2", [("ADV",)])
    g.els("PRAG2", "PRAG3", [("MARK", "PB"), ("INTERN", "t", "PA", "PB"), ("CMP", "t", "ID_PUSHM")])
    g.r("PRAG3", {1: ("DEAD", NC("#pragma push_macro")),
                  (0, 2): ("PRAG4", [("CMP", "t", "ID_POPM")])})
    g.r("PRAG4", {1: ("DEAD", NC("#pragma pop_macro")), (0, 2): blank})
    for k, w in enumerate(DIRV):
        cases[k + 1] = ("D_%s" % w, [])
    g.r("DSW", cases)
    newseg = [("ALUI", "add", "CURSEG", "CURSEG", 1), ("SETOT", "CURSEG")]

    def act(d, a, name):
        """the code after `a = inf(S_PP, key)` in preprocess(), for (d, a)."""
        w = DIRV[d]
        if w == "if":
            d = 0                                   # a pushed level, as #ifdef
        if d == 7 and a == 3:                       # include (when live)
            g.r(name, {1: ("INC0", [("JUMP", "WE")]), 0: blank})
            return
        if d < 3:                                   # push a level
            t = [("COPYW", "t", "LIVE")] if a == 0 else [("LDI", "t", 0)]
            g.els(name, blank[0], t + [("ALUI", "add", "a", "NDEPTH", TAKEB), ("STX", "a", 0, "t"),
                                        ("ALUI", "add", "a", "NDEPTH", SEENB), ("STX", "a", 0, "t"),
                                        ("ALUI", "add", "NDEPTH", "NDEPTH", 1), ("JUMP", "LS")])
            return
        if d in (3, 4):                             # elif / else
            n2 = name + "b"
            g.els(name, n2, [("CMPI", "NDEPTH", 0)])
            base = [("ALUI", "sub", "k", "NDEPTH", 1), ("ALUI", "add", "ta", "k", TAKEB),
                    ("ALUI", "add", "sa", "k", SEENB), ("LDI", "z", 0), ("STX", "ta", 0, "z")]
            if a == 0:
                g.r(n2, {2: (name + "c", base + [("LDX", "s", "sa", 0), ("RLD", "s")]), (0, 1): blank})
                g.r(name + "c", {0: ("P3BLANK", [("LDI", "one", 1), ("STX", "ta", 0, "one"),
                                                 ("STX", "sa", 0, "one"), ("JUMP", "LS")]),
                                 tuple(range(1, 257)): blank})
            else:
                g.r(n2, {2: ("P3BLANK", base + [("JUMP", "LS")]), (0, 1): blank})
            return
        if a == 2:
            g.els(name, name + "b", [("CMPI", "NDEPTH", 0)])
            g.r(name + "b", {2: ("P3BLANK", [("ALUI", "sub", "NDEPTH", "NDEPTH", 1), ("JUMP", "LS")]),
                             (0, 1): blank})
            return
        if a == 3 and w == "undef":
            sub, pu = g.call("MFIND", name + "m")
            g.r(name, {1: (sub, [("LDI", "SEGQ", -1)] + pu), 0: blank})
            g.els(name + "m", name + "n", [("CMPI", "M", 0)])
            g.r(name + "n", {0: blank, (1, 2): ("P3BLANK", newseg + ea("t2", "M") +
                                                [("STX", "t2", F_TO, "CURSEG"), ("JUMP", "LS")])})
            return
        if a == 3 and w == "define":
            g.r(name, {1: ("DEF0", newseg + [("JUMP", "NE")]), 0: blank})
            return
        g.els(name, blank[0], blank[1])             # nothing else happens

    # the table decides: a = PP[(directive, flag)]
    for d, w in enumerate(DIRV):
        st = "D_%s" % w
        if w in ("if", "elif"):
            # smallest #if/#elif: `0`, `1`, `defined X`, `defined ( X )`
            # alone on the line; a dead #if is not evaluated (as the
            # reference); anything else: not covered
            nc = ("DEAD", NC("#%s expression" % w))
            ev = st + "_e"
            if w == "if":
                g.els(st, st + "_l", [("RLD", "LIVE")])
                g.r(st + "_l", {0: (st + "_a0", [("RLD", "LIVE")]),
                                tuple(range(1, 257)): (ev, [("CMP", "NID", "ID_0")])})
            else:
                g.els(st, ev, [("CMP", "NID", "ID_0")])
            g.r(ev, {1: (st + "_z0", [("JUMP", "NE")]), (0, 2): (ev + "1", [("CMP", "NID", "ID_1")])})
            g.r(ev + "1", {1: (st + "_z1", [("JUMP", "NE")]), (0, 2): (ev + "2", [("CMP", "NID", "ID_DEFD")])})
            g.r(ev + "2", {1: (st + "_d", [("JUMP", "NE"), ("LDI", "par", 0)]), (0, 2): nc})
            for v in (0, 1):                        # constant: rest of line blank
                z = st + "_z%d" % v
                g.on(z, WS, z, [("ADV",)])
                g.on(z, [10, EOF], st + "_a%d" % v, [("RLD", "LIVE")])
                g.els(z, *nc)
            g.on(st + "_d", WS, st + "_d", [("ADV",)])
            g.on(st + "_d", [40], st + "_dp", [("ADV",), ("LDI", "par", 1)])
            g.on(st + "_d", AL, st + "_di", [("MARK", "NS")])
            g.els(st + "_d", *nc)
            g.on(st + "_dp", WS, st + "_dp", [("ADV",)])
            g.on(st + "_dp", AL, st + "_di", [("MARK", "NS")])
            g.els(st + "_dp", *nc)
            g.on(st + "_di", ID, st + "_di", [("ADV",)])
            g.els(st + "_di", st + "_dw", [("MARK", "NE"), ("INTERN", "NID", "NS", "NE"), ("RLD", "par")],
                  )
            g.els(st + "_dw", st + "_dw0", [])
            g.on(st + "_dw0", WS, st + "_dw0", [("ADV",)])
            g.on(st + "_dw0", [10, EOF], st + "_dq", [("CMPI", "par", 0)])
            g.on(st + "_dw0", [41], st + "_dr", [("ADV",), ("CMPI", "par", 1)])
            g.els(st + "_dw0", *nc)
            g.on(st + "_dr", WS, st + "_dr", [("ADV",)])
            g.on(st + "_dr", [10, EOF], st + "_dq", [])
            g.els(st + "_dr", *nc)
            sub, pu = g.call("MFIND", st + "_m")
            g.r(st + "_dq", {1: (sub, [("LDI", "SEGQ", -1)] + pu), (0, 2): nc})
            g.els(st + "_m", st + "_f", [("CMPI", "M", 0)])
            flag = {0: 0, 1: 1, 2: 1}
            g.r(st + "_f", {k: (st + "_a%d" % flag[k], [("RLD", "LIVE")]) for k in (0, 1, 2)})
        elif w in ("ifdef", "ifndef"):
            sub, pu = g.call("MFIND", st + "_m")
            g.els(st, sub, [("LDI", "SEGQ", -1)] + pu)
            g.els(st + "_m", st + "_f", [("CMPI", "M", 0)])
            flag = {0: 0, 1: 1, 2: 1}
            g.r(st + "_f", {k: (st + "_a%d" % flag[k], [("RLD", "LIVE")]) for k in (0, 1, 2)})
        elif w == "else":
            g.els(st, st + "_n", [("CMPI", "NDEPTH", 0)])
            g.r(st + "_n", {2: (st + "_s", [("ALUI", "sub", "k", "NDEPTH", 1),
                                            ("ALUI", "add", "sa", "k", SEENB), ("LDX", "s", "sa", 0),
                                            ("RLD", "s")]),
                            (0, 1): (st + "_a0", [("RLD", "LIVE")])})
            g.r(st + "_s", {0: (st + "_a1", [("RLD", "LIVE")]),
                            tuple(range(1, 257)): (st + "_a0", [("RLD", "LIVE")])})
        else:
            g.els(st, st + "_a1", [("RLD", "LIVE")])
        for fl in (0, 1):
            a = PPHEAD.index(PPT[(w, fl)])
            act(d, a, st + "_a%d" % fl)

    # #define: name at NS..NE (NID); function-like only when `(` touches it
    g.on("DEF0", [40], "DEFFN", [])
    g.els("DEF0", "DB_WS", [])
    # function-like: recorded (so #ifdef and redefinition see it); its
    # parameters and body are not parsed in this slice -- invoking it rejects
    g.els("DEFFN", "MDEF", [("PUSH", "DEFFNR")])
    g.labels.add("DEFFNR")
    fn2 = ("P3BLANK", [("LDI", "t", 2), ("STX", "EA", F_FN, "t"), ("JUMP", "LS")])
    g.els("DEFFNR", "DP0", [("ADV",), ("LDI", "NP", 0)])
    g.on("DP0", WS, "DP0", [("ADV",)])
    g.on("DP0", AL, "DPID", [("MARK", "PS_")])
    g.els("DP0", *fn2)
    g.on("DPID", ID, "DPID", [("ADV",)])
    g.els("DPID", "DPIDS", [("MARK", "PE_"), ("INTERN", "pid", "PS_", "PE_"), ("CMPI", "NP", MAXP)])
    g.r("DPIDS", {0: ("DP1", [("ALUI", "add", "pa", "EA", F_P0), ("ALU", "add", "pa", "pa", "NP"),
                              ("STX", "pa", 0, "pid"), ("ALUI", "add", "NP", "NP", 1)]),
                  (1, 2): fn2})
    g.on("DP1", WS, "DP1", [("ADV",)])
    g.on("DP1", [44], "DP0", [("ADV",)])
    g.on("DP1", [41], "DBF", [("ADV",), ("STX", "EA", F_NP, "NP")])
    g.els("DP1", *fn2)
    g.on("DBF", WS, "DBF", [("ADV",)])
    g.els("DBF", "P3BLANK", [("MARK", "VS"), ("BLOBSAVE", "BODY", "VS", "LE"),
                             ("STX", "EA", F_BODY, "BODY"), ("LDI", "one", 1),
                             ("STX", "EA", F_FN, "one"), ("JUMP", "LS")])
    g.on("DB_WS", WS, "DB_WS", [("ADV",)])
    sub, pu = g.call("MDEF", "DB_R")
    g.els("DB_WS", sub, [("MARK", "VS"), ("BLOBSAVE", "BODY", "VS", "LE")] + pu)
    g.els("DB_R", "P3BLANK", [("STX", "EA", F_BODY, "BODY"), ("JUMP", "LS")])

    # #include: incdo()
    g.on("INC0", WS, "INC0", [("ADV",)])
    g.on("INC0", [34], "IN34", [("ADV",), ("MARK", "NM"), ("LDI", "IQ", 34)])
    g.on("INC0", [60], "IN62", [("ADV",), ("MARK", "NM"), ("LDI", "IQ", 60)])
    g.els("INC0", *blank)
    for q in (34, 62):
        g.on("IN%d" % q, [q, 10, EOF], "INC1", [("MARK", "NME"), ("CMPI", "NINCL", 200)])
        g.els("IN%d" % q, "IN%d" % q, [("ADV",)])
    g.r("INC1", {2: blank, (0, 1): ("INC2", [("JUMP", "NM")])})
    trydisk = sbconst("include/") + [("SBSPAN", "NM", "NME"), ("SBFIND", "HB"), ("CMPI", "HB", 0)]
    g.on("INC2", [47], "INC4", [("SBCLR",), ("SBSPAN", "NM", "NME"), ("SBFIND", "HB"), ("CMPI", "HB", 0)])
    g.els("INC2", "INC2Q", [("RLD", "IQ")])
    # "x.h": the source file's directory first (dir = srcpath up to its last '/')
    g.r("INC2Q", {34: ("SRCD", [("LDI", "DL", 0), ("LDI", "SRCB", 1), ("INPUSH", "SRCB")]),
                  60: ("INC5", trydisk)})
    g.on("SRCD", [47], "SRCD", [("ADV",), ("MARK", "DL")])
    g.on("SRCD", [EOF], "INC4", [("LDI", "Z", 0), ("SBCLR",), ("SBSPAN", "Z", "DL"), ("INPOP",),
                                 ("SBSPAN", "NM", "NME"), ("SBFIND", "HB"), ("CMPI", "HB", 0)])
    g.els("SRCD", "SRCD", [("ADV",)])
    g.r("INC4", {1: ("INC5", trydisk), (0, 2): ("INCOK", [])})
    g.r("INC5", {1: ("INC6", sbconst("\0hdr/") + [("SBSPAN", "NM", "NME"), ("SBFIND", "HB"),
                                                  ("CMPI", "HB", 0)]),
                 (0, 2): ("INCOK", [])})
    g.r("INC6", {1: ("DEAD", [("REJECT", "no such file for #include")]), (0, 2): ("INCOK", [])})
    # found: o holds the buffer up to LS; write the file, a newline, the rest
    # of x from LE, and run P0, P1 and P3 again (P3 resumes at LS)
    g.els("INCOK", "INCH", [("OLEN", "RESUME"), ("ALUI", "add", "a", "NIREG", IRLN),
                            ("ALUI", "add", "t", "LINES", 1), ("STX", "a", 0, "t"),
                            ("LDI", "HNL", 1), ("INPUSH", "HB")])
    g.on("INCH", [10], "INCH", [("COPYT",), ("ADV",), ("ALUI", "add", "HNL", "HNL", 1)])
    g.on("INCH", [EOF], "P0", [("INPOP",), ("OUT", 10), ("XLEN", "XE"), ("SPAN2", "LE", "XE"),
                               ("ALUI", "add", "a", "NIREG", IRNL), ("STX", "a", 0, "HNL"),
                               ("ALUI", "add", "NIREG", "NIREG", 1),
                               ("ALUI", "add", "NINCL", "NINCL", 1), ("SWAP",)])
    g.els("INCH", "INCH", [("COPYT",), ("ADV",)])

    # ---- P4: expansion rounds (object-like macros) ---------------------------
    # a round is emitrange(0, nsrc, 0); up to 8 rounds while one changed
    g.els("P4", "ERL", [("LDI", "CHANGED", 0)])
    g.on("ERL", [EOF], "P4END", [("CMPI", "CHANGED", 0)])
    g.on("ERL", [34], "ERS34", [("COPYT",), ("ADV",)])
    g.on("ERL", [39], "ERS39", [("COPYT",), ("ADV",)])
    g.on("ERL", AL, "ERID", [("MARK", "IS"), ("XATTR", "ESEG"), ("SETOT", "ESEG")])
    g.els("ERL", "ERL", [("COPYT",), ("ADV",)])
    for q in (34, 39):
        g.on("ERS%d" % q, [92], "ERS%dE" % q, [("COPYT",), ("ADV",)])
        g.on("ERS%d" % q, [q], "ERL", [("COPYT",), ("ADV",)])
        g.on("ERS%d" % q, [EOF], "ERL")
        g.els("ERS%d" % q, "ERS%d" % q, [("COPYT",), ("ADV",)])
        g.on("ERS%dE" % q, [EOF], "ERL")
        g.els("ERS%dE" % q, "ERS%d" % q, [("COPYT",), ("ADV",)])
    # identend(): letters, digits, and \uXXXX / \UXXXXXXXX
    HEX = DI | set(range(97, 103)) | set(range(65, 71))
    idend = [("MARK", "IE"), ("INTERN", "NID", "IS", "IE"), ("CMP", "NID", "ID_PRAGMAOP")]
    g.on("ERID", ID, "ERID", [("ADV",)])
    g.on("ERID", [92], "UCN", [("MARK", "U"), ("ADV",)])
    g.els("ERID", "ERPR", idend)
    g.on("UCN", [117], "UCN4_0", [("ADV",)])
    g.on("UCN", [85], "UCN8_0", [("ADV",)])
    g.els("UCN", "ERIDX", [("JUMP", "U")])
    for n in (4, 8):
        for k in range(n):
            g.on("UCN%d_%d" % (n, k), HEX, "UCN%d_%d" % (n, k + 1) if k + 1 < n else "ERID", [("ADV",)])
            g.els("UCN%d_%d" % (n, k), "ERIDX", [("JUMP", "U")])
    g.els("ERIDX", "ERPR", idend)
    sub, pu = g.call("MFIND", "ERM")
    g.r("ERPR", {1: ("DEAD", NC("_Pragma")),
                 (0, 2): (sub, [("COPYW", "SEGQ", "ESEG")] + pu)})
    g.els("ERM", "ERM2", [("CMPI", "M", 0)])
    g.r("ERM2", {0: ("ERL", [("SPANT", "IS")]),
                 (1, 2): ("ERM3", ea("me", "M") + [("LDX", "fn", "me", F_FN), ("RLD", "fn")])})
    # function-like: only an invocation (name, blanks/newlines, `(`) is out of scope
    start = [("OUT", 32), ("LDI", "DEP", 0), ("LDI", "SEP", 0), ("LDI", "CUR", 0)]
    g.r("ERM3", {2: ("ERFN", []), 1: ("ERFC", [("LDI", "NLC", 0)]),
                 0: ("EB", [("LDI", "NLC", 0), ("LDI", "FNE", 0)] + start + [("LDI", "O0", -1)] + PUSHM)})
    # function-like call: name, blanks/newlines (counted, re-emitted after
    # the expansion), `(`, arguments split at commas, `)`
    g.on("ERFC", [32, 9], "ERFC", [("ADV",)])
    g.on("ERFC", [10], "ERFC", [("ADV",), ("ALUI", "add", "NLC", "NLC", 1)])
    g.on("ERFC", [40], "ARG", [("ADV",), ("LDI", "NA", 0), ("MARK", "AS")])
    g.els("ERFC", "ERL", [("JUMP", "IE"), ("SPANT", "IS")])
    save = [("MARK", "AE"), ("BLOBSAVE", "ab", "AS", "AE"), ("ALUI", "add", "a", "NA", ARGB),
            ("STX", "a", 0, "ab"), ("ALUI", "add", "NA", "NA", 1), ("ADV",)]
    g.on("ARG", [40], "DEAD", NC("nested call args"))
    g.on("ARG", [44], "ARGC", save + [("CMPI", "NA", MAXP)])
    g.r("ARGC", {0: ("ARG", [("MARK", "AS")]), (1, 2): ("DEAD", NC("argument count"))})
    g.on("ARG", [41], "ARGN", save + [("LDX", "np", "me", F_NP), ("CMP", "NA", "np")])
    # a function-like expansion that emits nothing is padded by one space only
    g.r("ARGN", {1: ("EB", [("COPYW", "FNE", "me")] + start + [("OLEN", "O0")] + PUSHM),
                 (0, 2): ("DEAD", NC("argument count"))})
    g.on("ARG", [10], "ARG", [("ADV",), ("ALUI", "add", "NLC", "NLC", 1)])
    g.on("ARG", [EOF], "DEAD", NC("unterminated macro call"))
    g.on("ARG", [34], "ARQ34", [("ADV",)])
    g.on("ARG", [39], "ARQ39", [("ADV",)])
    g.els("ARG", "ARG", [("ADV",)])
    for q in (34, 39):
        g.on("ARQ%d" % q, [92], "ARQ%dE" % q, [("ADV",)])
        g.on("ARQ%d" % q, [q], "ARG", [("ADV",)])
        g.on("ARQ%d" % q, [10, EOF], "DEAD", NC("unterminated literal in a call"))
        g.els("ARQ%d" % q, "ARQ%d" % q, [("ADV",)])
        g.on("ARQ%dE" % q, [EOF], "DEAD", NC("unterminated literal in a call"))
        g.els("ARQ%dE" % q, "ARQ%d" % q, [("ADV",)])
    g.on("ERFN", [32, 9, 10], "ERFN", [("ADV",)])
    g.on("ERFN", [40], "DEAD", NC("function-like macro invocation"))
    g.els("ERFN", "ERL", [("JUMP", "IE"), ("SPANT", "IS")])
    # token-level rescan of an object-like body (reference: tokens joined by
    # one space, the whole expansion padded by one space each side).  The
    # hide set of an object-like body token is the set of active macros:
    # F_ACT marks them, F_UP chains them (CUR is the innermost).
    g.on("EB", [EOF], "EBX", [("INPOP",), ("STX", "CUR", F_ACT, "Z0"), ("LDX", "CUR", "CUR", F_UP),
                              ("ALUI", "sub", "DEP", "DEP", 1), ("CMPI", "DEP", 0)])
    g.r("EBX", {1: ("EBX2", [("OLEN", "t"), ("CMP", "t", "O0")]), (0, 2): ("EB", [])})
    fin = [("LDI", "CHANGED", 1), ("LDI", "FNE", 0), ("CMPI", "NLC", 0)]
    g.r("EBX2", {1: ("EBNL", fin), (0, 2): ("EBNL", [("OUT", 32)] + fin)})
    g.r("EBNL", {2: ("EBNL", [("OUT", 10), ("ALUI", "sub", "NLC", "NLC", 1), ("CMPI", "NLC", 0)]),
                 (0, 1): ("ERL", [])})
    g.on("EB", [32, 9, 10], "EB", [("ADV",)])
    g.on("EB", [35], "DEAD", NC("# or ## in an object-like body"))
    sep = [("RLD", "SEP")]
    g.on("EB", AL, "EBSP", [("MARK", "BIS")] + sep)
    # the separating space of an identifier is pending (PS) until it is known
    # not to expand: an expanded name's first body token brings its own space
    g.r("EBSP", {1: ("EBID", [("LDI", "PS", 1)]), 0: ("EBID", [("LDI", "PS", 0)])})
    g.on("EB", ID - AL, "EBN0", sep)
    g.on("EB", [46], "EBDT", [("MARK", "BIS"), ("ADV",)])
    g.on("EBDT", DI, "EBN0", [("JUMP", "BIS")] + sep)
    g.els("EBDT", "EBP0", [("JUMP", "BIS")] + sep)
    g.on("EB", [34, 39], "EBQ0", sep)
    g.els("EB", "EBP0", sep)
    for pre, nxt in (("EBN0", "EBN"), ("EBQ0", "EBQ"), ("EBP0", "EBP")):
        g.r(pre, {1: (nxt, [("OUT", 32)]), 0: (nxt, [("LDI", "SEP", 1)])})
    # pp-number
    g.on("EBN", [101, 69, 112, 80], "EBNE", [("COPYT",), ("ADV",)])
    g.on("EBN", ID | {46}, "EBN", [("COPYT",), ("ADV",)])
    g.els("EBN", "EB")
    g.on("EBNE", [43, 45], "EBN", [("COPYT",), ("ADV",)])
    g.els("EBNE", "EBN")
    # character constant / string literal
    g.on("EBQ", [34], "EBS34", [("COPYT",), ("ADV",)])
    g.on("EBQ", [39], "EBS39", [("COPYT",), ("ADV",)])
    for q in (34, 39):
        g.on("EBS%d" % q, [92], "EBS%dE" % q, [("COPYT",), ("ADV",)])
        g.on("EBS%d" % q, [q], "EB", [("COPYT",), ("ADV",)])
        g.on("EBS%d" % q, [EOF], "EB")
        g.els("EBS%d" % q, "EBS%d" % q, [("COPYT",), ("ADV",)])
        g.on("EBS%dE" % q, [EOF], "EB")
        g.els("EBS%dE" % q, "EBS%d" % q, [("COPYT",), ("ADV",)])
    # punctuators, longest match
    P = ["...", "<<=", ">>=", "->", "++", "--", "<<", ">>", "<=", ">=", "==", "!=", "&&", "||",
         "*=", "/=", "%=", "+=", "-=", "&=", "^=", "|=", "<:", ":>", "<%", "%>"]
    pre = {}
    for t in P:
        for k in range(1, len(t)):
            pre.setdefault(t[:k], set()).add(t[k])
    def pn(s):
        return "EBP_" + "_".join(str(ord(c)) for c in s)
    for c in range(256):
        if chr(c) in pre:
            g.on("EBP", [c], pn(chr(c)), [("COPYT",), ("ADV",)])
    g.els("EBP", "EB", [("COPYT",), ("ADV",)])
    for s_, nx in pre.items():
        for c in nx:
            t = s_ + c
            if t in pre:
                g.on(pn(s_), [ord(c)], pn(t), [("COPYT",), ("ADV",)])
            elif t in P:
                g.on(pn(s_), [ord(c)], "EB", [("COPYT",), ("ADV",)])
        g.els(pn(s_), "EB") if s_ != ".." else g.els(pn(s_), "DEAD", NC("`..` in a body"))
    # identifier in a body: expand when an inactive object-like macro
    g.on("EBID", ID, "EBID", [("ADV",)])
    g.on("EBID", [92], "DEAD", NC("UCN in a body"))
    sub2, pu2 = g.call("MFIND", "EBM")
    g.els("EBID", "EBPR", [("MARK", "BIE"), ("INTERN", "NID", "BIS", "BIE"), ("CMP", "NID", "ID_PRAGMAOP")])
    g.r("EBPR", {1: ("DEAD", NC("_Pragma")), (0, 2): ("EBPB", [("CMP", "CUR", "FNE")])})
    # directly in a function-like body: a parameter name pushes its argument
    # (the argument frame is the entry ARGE, so the hide-set rule is unchanged)
    g.r("EBPB", {1: ("EBPL", [("LDI", "PK", 0), ("LDX", "FNP", "FNE", F_NP), ("CMP", "PK", "FNP")]),
                 (0, 2): (sub2, pu2)})
    g.r("EBPL", {0: ("EBPC", [("ALUI", "add", "pa", "FNE", F_P0), ("ALU", "add", "pa", "pa", "PK"),
                              ("LDX", "pv", "pa", 0), ("CMP", "pv", "NID")]),
                 (1, 2): (sub2, pu2)})
    g.r("EBPC", {1: ("EB", [("ALUI", "add", "pa", "PK", ARGB), ("LDX", "ab", "pa", 0),
                            ("LDI", "me", ARGE), ("STX", "me", F_BODY, "ab")] + PUSHM),
                 (0, 2): ("EBPL", [("ALUI", "add", "PK", "PK", 1), ("CMP", "PK", "FNP")])})
    g.els("EBM", "EBM2", [("CMPI", "M", 0)])
    g.r("EBM2", {0: ("EBNX", [("RLD", "PS")]),
                 (1, 2): ("EBM3", ea("me", "M") + [("LDX", "act", "me", F_ACT), ("RLD", "act")])})
    g.r("EBNX", {1: ("EB", [("OUT", 32), ("SPANT", "BIS")]), 0: ("EB", [("LDI", "SEP", 1), ("SPANT", "BIS")])})
    g.r("EBM3", {1: ("EBNX", [("RLD", "PS")]),
                 0: ("EBM4", [("LDX", "fn", "me", F_FN), ("RLD", "fn")])})
    g.r("EBM4", {(1, 2): ("DEAD", NC("function-like macro name in a body")),
                 0: ("EB", PUSHM)})
    # end of a round: none changed -> x is the result; else again, at most 8
    g.r("P4END", {1: ("ACC", [("OCLR",), ("LDI", "Z", 0), ("XLEN", "XE"), ("SPAN2", "Z", "XE")]),
                  (0, 2): ("ACC", [])})  # the token rescan is complete: one round
    g.r("P4N", {1: ("ACC", []), (0, 2): ("P4", [("SWAP",)])})
    g.els("ACC", "ACC", [("ACCEPT",)])
    g.finish()
    return g


def sizes(g):
    ent = sum(len(r) for _, r in g.st.values())
    sparse = 0
    for mode, row in g.st.values():
        vals = list(row.values())
        common = max(set(vals), key=vals.count)
        sparse += 1 + 4 + 5 * sum(1 for v in vals if v != common)
    pool = sum(len(s) for s in g.seqs)
    poolb = sum(1 + sum(1 + 4 * (len(a) - 1) for a in s) for s in g.seqs)
    modes = {}
    for mode, _ in g.st.values():
        modes[mode] = modes.get(mode, 0) + 1
    return dict(states=len(g.st), modes=modes, entries=ent, unreachable=g.unreach,
                seqs=len(g.seqs), pool_actions=pool, pool_bytes=poolb,
                sparse_bytes=sparse + poolb, dense_bytes=4 * ent + poolb + len(g.st),
                gamma=len(g.labels) + 1,
                action_kinds=sorted(set(a[0] for s in g.seqs for a in s)))


def main():
    g = build()
    out = sys.argv[1] if len(sys.argv) > 1 else "/tmp/e2delta.json"
    json.dump({"start": "START", "states": {k: [m, {str(kk): list(v) for kk, v in r.items()}]
                                            for k, (m, r) in g.st.items()},
               "seqs": [list(map(list, s)) for s in g.seqs]}, open(out, "w"))
    for k, v in sizes(g).items():
        print("%-14s %s" % (k, v))


if __name__ == "__main__":
    main()
