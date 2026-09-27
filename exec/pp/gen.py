"""E2 feasibility, MINIMUM slice: the preprocessor as one finite delta.

    python3 exec/pp/gen.py [out.json] [OS/ARCH]      -> writes the table, prints sizes

Machine and primitives: exec/pp/sim.py (generic, option A); design:
research/e2-pp-delta.md.  The delta runs the reference's passes in order,
each pass reading x and writing o, SWAP between passes:

  P0 shebang + splice      src/front_pp.c splice(), fe_load's shebang blank
  P1 decomment             decomment()            (reject: unterminated comment)
  P3 directives            preprocess(): #ifdef #ifndef #else #endif #define
                           #undef #include, unknown directives and dead lines
                           blanked; a live #include splices the file in and
                           re-runs P0, P1 on the whole buffer (as incdo does)
  P4 token rescan          object/function macros, argument pre-expansion,
                           zero/variadic arguments, # and covered ## forms;
                           replacement frames can continue into their parent
                           balanced _Pragma calls are ignored as in the reference

NOT covered (the delta rejects with a `not covered: ...` code, never guesses):
unterminated _Pragma calls; more than MAXP parameters; general punctuator/literal ## operands
(the identifier/digit and object-like `# ## #` forms are covered);
# / ## bodies on an #if line.
autoinc() (P2, the on-demand header prepend) is modelled: build_autoinc,
its trigger names read from include/*.h (E2_AUTOINC=0 builds without it).
Also not modelled: #pragma push_macro/pop_macro
(rejected as not covered when live and spelled exactly; the reference's
prefix match `push_macroX` is not reproduced), the
file:line:col rendering of diagnostics (the reject kind is compared, not the
text).

Directive vocabulary and decisions come from weights/gold/pp.tsv, including
its schema. Target predefinitions come from predefines.tsv. No old kernel
file supplies generation data. Byte classes, include search order and much
of the control flow below remain handwritten; declaration migration is not
complete merely because runtime transitions are constructed into networks.
"""
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
EOF = 256

sys.path.insert(0, ROOT)
from unisa.tsvgold import load_table
from pathlib import Path
sys.path.insert(0, os.path.join(ROOT, "exec"))
from finite_rules import load as load_rules

_name, _fields, _heads, _rows = load_table(os.path.join(ROOT, "weights/gold/pp.tsv"))
assert _name == "pp" and [n for n, _ in _fields] == ["dir", "defined"]
assert _fields[1][1] == ("0", "1") and [n for n, _, _ in _heads] == ["y"]
DIRV = _fields[0][1]
PPHEAD = list(_heads[0][1])
PPT = {(d, int(b)): labels["y"] for (d, b), labels in _rows.items()}
assert PPHEAD == ["take", "skip", "pop", "macro"], PPHEAD


def load_predefines():
    path = os.path.join(HERE, "predefines.tsv")
    rows = {}
    for line, text in enumerate(open(path, encoding="utf-8"), 1):
        if text.startswith("#") or not text.strip():
            continue
        fields = text.rstrip("\n").split("\t")
        key, names = tuple(fields[:2]), fields[2:]
        if (len(fields) < 3 or key in rows or len(set(names)) != len(names) or
                any(not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", name) for name in names)):
            raise ValueError(f"{path}:{line}: invalid or duplicate predefinition row")
        rows[key] = names
    expected = {("os", x) for x in ("lnx", "osx", "win")} | {
        ("arch", x) for x in ("x86_64", "arm64")} | {("common", "*")}
    if set(rows) != expected:
        raise ValueError(f"{path}: expected OS, architecture and common declarations")
    return rows


PREDEF = load_predefines()


# autoinc (src/front_pp.c autoinc/hdrneeded): the trigger data build_autoinc
# puts in the delta.  Order is the reference's; trigger names are derived from
# include/*.h by hdrneeded's line rule (a line opening `static` with `(` and
# a `{` after it names the identifier before the first `(`).
AUTOINC_ORDER = ("assert.h", "ctype.h", "stdlib.h", "string.h", "wchar.h", "stdio.h")


def autoinc_map():
    m = {}
    for h in AUTOINC_ORDER:
        names = []
        for ln in open(os.path.join(ROOT, "include", h), encoding="latin-1").read().split("\n"):
            if len(ln) > 7 and ln.startswith("static") and "(" in ln and "{" in ln[ln.index("("):]:
                mm = re.search(r"([A-Za-z0-9_]+)\s*$", ln[:ln.index("(")])
                if mm:
                    names.append(mm.group(1))
        m[h] = names
    return m


AUTOINC = os.environ.get("E2_AUTOINC", "1") != "0"   # E2_AUTOINC=0: the delta without it
AIB = 68 * 10 ** 6       # W[AIB + id]: bit 1 called, bit 2 defined (srcuse)


def install_rules(g, stem, bindings=None, sequences=None, classes=None):
    for suffix, mode in (("byte", "b"), ("result", "r")):
        for state, row in load_rules(Path(HERE) / (stem + "-" + suffix + ".tsv"),
                                     sequences or {}, bindings=bindings, classes=classes).items():
            for key, (target, actions) in row.items():
                g.on(state, [key], target, actions, mode)
                g.labels.update(a[1] for a in actions if a[0] == "PUSH")


def build_autoinc(g, locations=False):
    """P2 autoinc, first run only (RUN == 0), between decomment and P3.
    One scan over x: every maximal identifier run followed (spaces, tabs,
    newlines) by `(` is a call; the `(`'s matching `)` followed by `{` is a
    definition; a call to printf sets RTP (991d337: stdio.h for any printf call).  Then, per header in AUTOINC_ORDER, a name of
    autoinc_map() (printf excluded, as hdrneeded does) with status exactly
    `called` pulls it in; the lines are emitted in prepend order (rtprintf's
    stdio.h first, then the headers last-to-first) and x copied after."""
    install_rules(g, "autoinc", {"AIB": AIB}, classes={"identifier": ID})
    # per header: does some name have status exactly `called`?
    amap = autoinc_map()
    H = list(AUTOINC_ORDER)
    for h, hn in enumerate(H):
        names = [n for n in amap[hn] if n != "printf"]
        nxt_h = "AH%d_0" % (h + 1) if h + 1 < len(H) else "AEM"
        g.els("AH%d_0" % h, "AH%d_n0" % h, [("LDI", "NEED%d" % h, 0)])
        for k, nm in enumerate(names):
            install_rules(g, "autoinc-name", {"entry": "AH%d_n%d" % (h, k),
                "test": "AH%d_r%d" % (h, k), "found": nxt_h,
                "next": "AH%d_n%d" % (h, k + 1), "need": "NEED%d" % h, "AIB": AIB},
                {"name": sbconst(nm)})
        g.els("AH%d_n%d" % (h, len(names)), nxt_h)

    def line(hn):
        return [("OUT", c) for c in ("#include <%s>\n" % hn).encode()] + ([("ALUI","add","AI_LINES","AI_LINES",1)] if locations else [])
    install_rules(g, "autoinc-emit", {"entry": "AEM", "test": "AEMR",
        "need": "RTP", "next": "AEM%d" % (len(H) - 1)}, {"line": line("stdio.h")})
    for h in range(len(H) - 1, -1, -1):
        install_rules(g, "autoinc-emit", {"entry": "AEM%d" % h, "test": "AEM%dr" % h,
            "need": "NEED%d" % h, "next": "AEM%d" % (h - 1) if h else "ACP0"}, {"line": line(H[h])})



AL = set(range(97, 123)) | set(range(65, 91)) | {95}
DI = set(range(48, 58))
ID = AL | DI
WS = {32, 9}
SEGINF = 1000000000
# W regions (addresses; plain named slots are strings)
DIRB, NEWB, MACB, TAKEB, SEENB = 10 ** 7, 2 * 10 ** 7, 5 * 10 ** 7, 6 * 10 ** 7, 61 * 10 ** 6
SPLB, IRLN, IRNL = 11 * 10 ** 7, 12 * 10 ** 7, 121 * 10 ** 6
F_NAME, F_BODY, F_FN, F_FROM, F_TO, F_PREV, F_ACT, F_UP = 0, 1, 2, 3, 4, 5, 6, 7
F_NP, F_P0, MAXP = 8, 9, 8          # function-like: parameter count, parameter ids
F_HASH = F_P0 + MAXP     # 1: the body has `#` outside literals (s12)
F_VAR = F_HASH + 1      # final parameter is __VA_ARGS__
FSZ = F_VAR + 1
ARGB, ARGE = 62 * 10 ** 6, 63 * 10 ** 6   # argument blobs; the argument frame's entry
# s13: a function-like call's record, a stack (CL deep, CR the top's address):
# the macro, the argument index, the registers a pre-expansion saves, and per
# argument the raw blob and the fully expanded one
CRB, CRS = 69 * 10 ** 6, 40
C_ME, C_K, C_BDEP, C_PRE, C_EDEP, C_SEP, C_OST, C_SB, C_SB0 = 0, 1, 2, 3, 4, 5, 6, 7, 8
C_RAW, C_EXP = 9, 9 + MAXP
# F_FN: 0 object-like, 1 function-like (covered), 2 function-like not covered
# (more than MAXP or malformed)


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




def ea(dst, e):              # W[dst] := address of macro entry W[e]
    return [("ALUI", "mul", dst, e, FSZ), ("ALUI", "add", dst, dst, MACB)]


def sbconst(s):
    return [("SBCLR",)] + [("SBOUT", c) for c in s.encode()]


# ---- XE: #if expression evaluator (research/e2-pp-delta.md s11.2) ---------
# shunting-yard over an operator stack (XOB) and value/poison stacks (XVB,
# XPB) in W; 64-bit signed via A64/C64.  A division by zero sets the value's
# poison bit; && || ?: drop the poison of the operand they do not evaluate.
XOB, XVB, XPB, XPRB = 64 * 10 ** 6, 65 * 10 ** 6, 66 * 10 ** 6, 67 * 10 ** 6
# code: (spelling, prec, arity)
XOPS = {}
for line in (Path(HERE) / "operators.tsv").read_text().splitlines():
    if not line or line.startswith("#"):
        continue
    code, spelling, precedence, arity = line.split("\t")
    code, precedence, arity = int(code), int(precedence), int(arity)
    assert code not in XOPS and 0 < code < 257 and precedence >= 0 and arity in (0, 1, 2, 3)
    assert spelling and spelling not in {v[0] for v in XOPS.values()}
    XOPS[code] = spelling, precedence, arity
assert XOPS, "empty expression operator declarations"


def xe_init():
    a = []
    for c, (_, p, _) in list(XOPS.items()) + [(0, (None, -1, 0))]:
        a += [("LDI", "xc", XPRB + c), ("LDI", "xq", p), ("STX", "xc", 0, "xq")]
    return a


def build_xe(g):
    layout = {name: globals()[name] for name in ['XOB', 'XVB', 'XPB', 'XPRB', 'F_ACT', 'F_UP', 'FSZ', 'MACB', 'F_TO', 'F_FN', 'F_HASH', 'F_BODY']}
    layout["XOB_PREV"] = XOB - 1
    layout.update(("PREC_" + str(c), p) for c, (_, p, _) in XOPS.items())
    install_rules(g, "expression", layout)
    install_rules(g, "reduce", layout)


# ---- # and ## (research/e2-pp-delta.md s12) -------------------------------
# HSCAN (at #define): F_HASH := 1 when the body has `#` outside literals.
# HX (at expansion, `me` = the macro, arguments in ARGB): the body is
# rewritten into the string builder and saved as blob NB, which is then
# rescanned by the declared replacement-frame actions:
#   `# p`     -> `"` + the argument's spelling, blanks trimmed, inner blank
#               runs one space, `"` and `\` escaped inside literals + `"`
#   `a ## b`  -> the two spellings concatenated (no space); a placemarker
#               (empty argument) contributes nothing
#   other parameters -> the argument text (blank runs one space), padded by
#               a space each side, so the rescan tokenises it on its own
# Covered paste operands: identifier/digit boundaries, empty arguments and
# object-like # ## #. General punctuator/literal pastes remain not covered
# (the reference re-tokenises concatenated spellings).
# Vars: GLUE (1 ordinary paste, 2 comma/variadic special case), PSP (space
# owed), LASTK (1 identifier/digit, 2 placemarker, 3 hash, 4 comma, 0 other).
def build_hx(g):
    install_rules(g, "hash", {name: globals()[name] for name in ['C_EXP', 'C_RAW', 'F_BODY', 'F_FN', 'F_HASH', 'F_NP', 'F_P0', 'F_VAR']})



def build_cli(g, locations=False):
    install_rules(g, "cli", {name: globals()[name] for name in ['F_BODY', 'F_TO', 'NEWB', 'FSZ', 'MACB']},
        {"location_line": [("ALUI", "add", "CLI_PRELINES", "CLI_PRELINES", 1)] if locations else []})



def build(target="lnx/x86_64", locations=False):
    if target not in ("lnx/x86_64", "lnx/arm64", "osx/x86_64", "osx/arm64", "win/x86_64", "win/arm64"):
        raise ValueError("unsupported preprocessor target: "+target)
    target_os, target_arch = target.split("/")
    predef = PREDEF["os", target_os] + PREDEF["arch", target_arch] + PREDEF["common", "*"]
    if len(set(predef)) != len(predef):
        raise ValueError("overlapping target predefinitions: " + target)
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
    init += sbconst("printf") + [("SBINTERN", "ID_PRINTF")]
    init += sbconst("__VA_ARGS__") + [("SBINTERN", "ID_VA")]
    init += [("LDI", "RUN", 0), ("LDI", "FP", 0)] + xe_init()
    g.els("START", "CLI.FLAGS", init)
    build_cli(g, locations)

    # Declared text normalisation; only layout and inter-stage links are bound here.
    install_rules(g, "text", {"SPLB": SPLB, "after_comments": "AISTART" if AUTOINC else "P3START"})

    # Macro history and definition rules; bindings describe record layout only.
    macro_layout = {'NEWB': NEWB, 'FSZ': FSZ, 'MACB': MACB, 'F_TO': F_TO, 'SEGINF': SEGINF, 'F_FROM': F_FROM, 'F_PREV': F_PREV, 'F_NAME': F_NAME, 'F_BODY': F_BODY, 'F_FN': F_FN, 'F_VAR': F_VAR}
    install_rules(g, "macro", macro_layout)

    if AUTOINC:
        build_autoinc(g, locations)

    # ---- P3: directives -------------------------------------------------------
    g.els("P3START", "P3S2", [("RLD", "RUN")])
    # predef(): object-like macros with body "1" for the selected target
    chain = "P3PD0"
    g.r("P3S2", {0: ("CLI.D", [("LDI", "RUN", 1), ("LDI", "CURSEG", 0), ("SETOT", "CURSEG")]),
                 1: ("P3L0", [("LDI", "Z", 0), ("SPAN2", "Z", "RESUME"), ("JUMP", "RESUME")])})
    for k, nm in enumerate(predef):
        nxt = "P3PD%d" % (k + 1) if k + 1 < len(predef) else "CLI.U"
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
            # #if/#elif: integer constants, defined, the C operators;
            # evaluated by XE (64-bit); a dead #if is not evaluated (as the
            # reference); any macro name: not covered
            sub, pu = g.call("XE", st + "_x")
            if w == "if":
                g.els(st, st + "_l", [("RLD", "LIVE")])
                g.r(st + "_l", {0: (st + "_a0", [("RLD", "LIVE")]),
                                tuple(range(1, 257)): (sub, [("JUMP", "NS")] + pu)})
            else:
                g.els(st, sub, [("JUMP", "NS")] + pu)
            g.els(st + "_x", st + "_xp", [("CMPI", "XP", 0), ])
            g.r(st + "_xp", {1: (st + "_xv", [("C64", "XV", "xz")]),
                             (0, 2): ("DEAD", NC("#if division by zero"))})
            # (the reference reports it on stderr and still writes the text,
            # -E exit 0; one channel here, so: not covered)
            g.r(st + "_xv", {1: (st + "_a0", [("RLD", "LIVE")]), (0, 2): (st + "_a1", [("RLD", "LIVE")])})
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
    # Record parameter names and a final variadic slot. Unsupported/malformed
    # declarations remain visible to #ifdef but reject when invoked.
    g.els("DEFFN", "MDEF", [("PUSH", "DEFFNR")])
    g.labels.add("DEFFNR")
    fn2 = ("P3BLANK", [("LDI", "t", 2), ("STX", "EA", F_FN, "t"), ("JUMP", "LS")])
    g.els("DEFFNR", "DP0", [("ADV",), ("LDI", "NP", 0)])
    g.on("DP0", WS, "DP0", [("ADV",)])
    g.on("DP0", AL, "DPID", [("MARK", "PS_")])
    g.on("DP0", [41], "DPZERO", [("CMPI", "NP", 0)])
    g.r("DPZERO", {1:("DBF",[("ADV",),("STX","EA",F_NP,"NP")]),(0,2):fn2})
    g.on("DP0", [46], "DPDOT1", [("ADV",)])
    g.on("DPDOT1", [46], "DPDOT2", [("ADV",)])
    g.on("DPDOT2", [46], "DPVAR", [("ADV",),("CMPI","NP",MAXP)])
    g.els("DPDOT1", *fn2);g.els("DPDOT2", *fn2)
    g.r("DPVAR", {0:("DPVEND",[("ALUI","add","pa","EA",F_P0),("ALU","add","pa","pa","NP"),
        ("STX","pa",0,"ID_VA"),("ALUI","add","NP","NP",1),("LDI","t",1),("STX","EA",F_VAR,"t")]),(1,2):fn2})
    g.on("DPVEND", WS, "DPVEND", [("ADV",)])
    g.on("DPVEND", [41], "DBF", [("ADV",),("STX","EA",F_NP,"NP")])
    g.els("DPVEND", *fn2)
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
    subh, puh = g.call("HSCAN", "DBFR")
    g.els("DBF", subh, [("MARK", "VS"), ("BLOBSAVE", "BODY", "VS", "LE"),
                        ("STX", "EA", F_BODY, "BODY"), ("LDI", "one", 1),
                        ("STX", "EA", F_FN, "one"), ("JUMP", "LS")] + puh)
    g.els("DBFR", "P3BLANK")
    g.on("DB_WS", WS, "DB_WS", [("ADV",)])
    sub, pu = g.call("MDEF", "DB_R")
    g.els("DB_WS", sub, [("MARK", "VS"), ("BLOBSAVE", "BODY", "VS", "LE")] + pu)
    subh, puh = g.call("HSCAN", "DB_RR")
    g.els("DB_R", subh, [("STX", "EA", F_BODY, "BODY"), ("JUMP", "LS")] + puh)
    g.els("DB_RR", "P3BLANK")

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
                  60: ("CLI.IP", [])})
    g.on("SRCD", [47], "SRCD", [("ADV",), ("MARK", "DL")])
    g.on("SRCD", [EOF], "INC4", [("LDI", "Z", 0), ("SBCLR",), ("SBSPAN", "Z", "DL"), ("INPOP",),
                                 ("SBSPAN", "NM", "NME"), ("SBFIND", "HB"), ("CMPI", "HB", 0)])
    g.els("SRCD", "SRCD", [("ADV",)])
    g.r("INC4", {1: ("CLI.IP", []), (0, 2): ("INCOK", [])})
    g.els("CLI.IP", "CLI.IP.have", sbconst("\0cli/include-dir")+[("SBFIND","CLI_DIR"),("BLEN","CLI_LEN","CLI_DIR"),("CMPI","CLI_LEN",0)])
    g.r("CLI.IP.have", {1:("INC.BUILTIN",[("RLD","CLI_NOSTD")]),(0,2):("CLI.IP.try",[("SBCLR",),("SBBLOB","CLI_DIR"),("SBOUT",47),("SBSPAN","NM","NME"),("SBFIND","HB"),("CMPI","HB",0)])})
    g.r("CLI.IP.try", {1:("INC.BUILTIN",[("RLD","CLI_NOSTD")]),(0,2):("INCOK",[])})
    g.r("INC.BUILTIN", {0:("INC5",trydisk),
                              1:("DEAD",[("REJECT","no such file for #include")])})
    g.r("INC5", {1: ("INC6", sbconst("\0hdr/") + [("SBSPAN", "NM", "NME"), ("SBFIND", "HB"),
                                                  ("CMPI", "HB", 0)]),
                 (0, 2): ("INCOK", [])})
    g.r("INC6", {1: ("DEAD", [("REJECT", "no such file for #include")]), (0, 2): ("INCOK", [])})
    # found: o holds the buffer up to LS; write the file, a newline, the rest
    # of x from LE, and run P0, P1 and P3 again (P3 resumes at LS)
    inc_entry="INCOK"
    if locations:
        from locations import IRNAME
        # incname in the reference diagnostic map retains at most 62 bytes.
        g.els("INCOK","INC.location",[("ALUI","add","loc_end","NM",62),("CMP","NME","loc_end")])
        g.r("INC.location",{(0,1):("INC.record",[("COPYW","loc_end","NME")]),2:("INC.record",[])})
        g.els("INC.record","INC.body",[("BLOBSAVE","loc_name","NM","loc_end"),("STX","NIREG",IRNAME,"loc_name")])
        inc_entry="INC.body"
    g.els(inc_entry, "INCH", [("OLEN", "RESUME"), ("ALUI", "add", "a", "NIREG", IRLN),
                            ("ALUI", "add", "t", "LINES", 1), ("STX", "a", 0, "t"),
                            ("LDI", "HNL", 1), ("INPUSH", "HB")])
    g.on("INCH", [10], "INCH", [("COPYT",), ("ADV",), ("ALUI", "add", "HNL", "HNL", 1)])
    g.on("INCH", [EOF], "P0", [("INPOP",), ("OUT", 10), ("XLEN", "XE"), ("SPAN2", "LE", "XE"),
                               ("ALUI", "add", "a", "NIREG", IRNL), ("STX", "a", 0, "HNL"),
                               ("ALUI", "add", "NIREG", "NIREG", 1),
                               ("ALUI", "add", "NINCL", "NINCL", 1), ("SWAP",)])
    g.els("INCH", "INCH", [("COPYT",), ("ADV",)])

    install_rules(g, "rescan", {name: globals()[name] for name in ['ARGB', 'ARGE', 'CRB', 'CRS', 'C_BDEP', 'C_EDEP', 'C_EXP', 'C_K', 'C_ME', 'C_OST', 'C_PRE', 'C_RAW', 'C_SB', 'C_SB0', 'C_SEP', 'FSZ', 'F_ACT', 'F_BODY', 'F_FN', 'F_HASH', 'F_NP', 'F_P0', 'F_UP', 'F_VAR', 'MACB', 'MAXP']})
    if locations:
        from locations import install
        install(g, SPLB, IRLN, IRNL)
    else:
        g.els("ACC", "ACC", [("ACCEPT",)])
    build_xe(g)
    build_hx(g)
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
    locations="--locations" in sys.argv
    if locations: sys.argv.remove("--locations")
    if len(sys.argv)>3:
        sys.exit("usage: gen.py [OUT.json] [OS/ARCH] [--locations]")
    g = build(sys.argv[2] if len(sys.argv)>2 else "lnx/x86_64", locations=locations)
    out = sys.argv[1] if len(sys.argv) > 1 else "/tmp/e2delta.json"
    json.dump({"start": "START", "states": {k: [m, {str(kk): list(v) for kk, v in r.items()}]
                                            for k, (m, r) in g.st.items()},
               "seqs": [list(map(list, s)) for s in g.seqs]}, open(out, "w"))
    for k, v in sizes(g).items():
        print("%-14s %s" % (k, v))


if __name__ == "__main__":
    main()
