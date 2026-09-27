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
CRC = [("ALUI", "mul", "CR", "CL", CRS), ("ALUI", "add", "CR", "CR", CRB)]
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


PUSHM = [("STX", "me", F_UP, "CUR"), ("COPYW", "CUR", "me"), ("LDI", "one", 1),
         ("STX", "me", F_ACT, "one"), ("ALUI", "add", "DEP", "DEP", 1),
         ("LDX", "BB", "me", F_BODY), ("INPUSH", "BB")]
# the same, pushing the body HX rewrote (# and ## applied) instead of F_BODY
PUSHMB = PUSHM[:-2] + [("INPUSH", "NB")]


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
# rescanned exactly like a body (PUSHMB):
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
def build_hx(g, NC):
    OTH = set(range(256)) - ID - WS - {10, 34, 39, 35}
    # define-time scan
    g.els("HSCAN", "HS0", [("LDI", "hz", 0), ("STX", "EA", F_HASH, "hz"), ("INPUSH", "BODY")])
    g.on("HS0", [35], "HS0", [("ADV",), ("LDI", "hz", 1), ("STX", "EA", F_HASH, "hz")])
    g.on("HS0", [EOF], "RET", [("INPOP",)])
    for q in (34, 39):
        g.on("HS0", [q], "HSQ%d" % q, [("ADV",)])
        g.on("HSQ%d" % q, [92], "HSQ%dE" % q, [("ADV",)])
        g.on("HSQ%d" % q, [q], "HS0", [("ADV",)])
        g.on("HSQ%d" % q, [EOF], "RET", [("INPOP",)])
        g.els("HSQ%d" % q, "HSQ%d" % q, [("ADV",)])
        g.on("HSQ%dE" % q, [EOF], "RET", [("INPOP",)])
        g.els("HSQ%dE" % q, "HSQ%d" % q, [("ADV",)])
    g.els("HS0", "HS0", [("ADV",)])

    one = [("MARK", "hc0"), ("ADV",), ("MARK", "hc1"), ("SBSPAN", "hc0", "hc1")]
    g.els("HX", "HW", [("SBCLR",), ("LDI", "GLUE", 0), ("LDI", "PSP", 0), ("LDI", "LASTK", 0),
                       ("LDX", "hfn", "me", F_FN), ("LDX", "hb", "me", F_BODY), ("INPUSH", "hb")])

    # pre-emission of one byte (still at the cursor) in context c, then copy
    def pre(c, back):
        g.r("HPG" + c, {1: ("HPG1" + c, []), 0: ("HPS" + c, [("RLD", "PSP")])})
        g.on("HPG1" + c, ID, "HCP" + c, [("LDI", "GLUE", 0), ("LDI", "PSP", 0)])
        g.els("HPG1" + c, "DEAD", NC("## operand"))
        g.r("HPG" + c, {2: ("DEAD", NC("comma paste requires final variadic parameter"))})
        g.r("HPS" + c, {(1, 3): ("HCP" + c, [("SBOUT", 32), ("LDI", "PSP", 0)]),
                        2: ("HCP" + c, [("SBOUT", 2), ("LDI", "PSP", 0)]), 0: ("HCP" + c, [])})
        # copy: an identifier/digit byte, a literal, or one other byte
        g.on("HCP" + c, ID, back, one + [("LDI", "LASTK", 1)])
        g.on("HCP" + c, [44], back, [("SBSAVE", "HCOMMA")] + one + [("LDI", "LASTK", 4)])
        g.on("HCP" + c, OTH - {44}, back, one + [("LDI", "LASTK", 0)])
        g.on("HCP" + c, [35], back, one + [("LDI", "LASTK", 3)])
        for q in (34, 39):
            L = "HL%d%s" % (q, c)
            g.on("HCP" + c, [q], L, one + [("LDI", "LASTK", 0)])
            g.on(L, [92], L + "E", one)
            g.on(L, [q], back, one)
            g.on(L, [10, EOF], "DEAD", NC("unterminated literal in a body"))
            g.els(L, L, one)
            g.on(L + "E", [EOF], "DEAD", NC("unterminated literal in a body"))
            g.els(L + "E", L, one)
        g.els("HCP" + c, "DEAD", NC("hash rewrite"))
    pre("W", "HW")
    pre("A", "HA1")

    # body walk
    g.on("HW", WS | {10}, "HW", [("ADV",), ("ALUI", "or", "PSP", "PSP", 1)])
    g.on("HW", AL, "HID", [("MARK", "hs")])
    g.on("HW", [EOF], "HWE", [("RLD", "GLUE")])
    g.r("HWE", {0: ("RET", [("INPOP",), ("SBSAVE", "NB")]),
                tuple(range(1, 257)): ("DEAD", NC("## at the end of a body"))})
    g.on("HW", [35], "HH", [("ADV",)])
    g.els("HW", "HPGW", [("RLD", "GLUE")])
    # identifier: a parameter (function-like) or copied
    g.on("HID", ID, "HID", [("ADV",)])
    g.on("HID", [92], "DEAD", NC("UCN in a body"))
    g.els("HID", "HPL0I", [("MARK", "he"), ("INTERN", "hid", "hs", "he"), ("RLD", "hfn")])

    def plook(c, found, notp):
        g.r("HPL0" + c, {1: ("HPL" + c, [("LDI", "hk", 0), ("LDX", "hnp", "me", F_NP),
                                          ("CMP", "hk", "hnp")]),
                         tuple(k for k in range(257) if k != 1): notp})
        g.r("HPL" + c, {0: ("HPC" + c, [("ALUI", "add", "hpa", "me", F_P0), ("ALU", "add", "hpa", "hpa", "hk"),
                                         ("LDX", "hpv", "hpa", 0), ("CMP", "hpv", "hid")]),
                        (1, 2): notp})
        g.r("HPC" + c, {1: found, (0, 2): ("HPL" + c, [("ALUI", "add", "hk", "hk", 1), ("CMP", "hk", "hnp")])})
    getarg = [("ALU", "add", "hpa", "CR", "hk"), ("LDX", "hab", "hpa", C_RAW)]
    # not a parameter: the spelling (an identifier is a valid paste operand)
    plook("I", ("HAP", getarg + [("RLD", "GLUE")]), ("HNP", [("RLD", "GLUE")]))
    g.r("HNP", {2: ("DEAD", NC("comma paste requires final variadic parameter")), 1: ("HW", [("LDI", "GLUE", 0), ("LDI", "PSP", 0), ("SBSPAN", "hs", "he"), ("LDI", "LASTK", 1)]),
                0: ("HNP2", [("RLD", "PSP")])})
    g.r("HNP2", {(1, 3): ("HW", [("SBOUT", 32), ("LDI", "PSP", 0), ("SBSPAN", "hs", "he"), ("LDI", "LASTK", 1)]),
                 2: ("HW", [("SBOUT", 2), ("LDI", "PSP", 0), ("SBSPAN", "hs", "he"), ("LDI", "LASTK", 1)]),
                 0: ("HW", [("SBSPAN", "hs", "he"), ("LDI", "LASTK", 1)])})
    # a parameter: its argument, padded unless pasted
    g.r("HAP", {1: ("HA0", [("LDI","HRAW",1),("INPUSH", "hab")]), 0: ("HAQ", [("MARK", "hq")])})
    # GNU comma elision is special only for the final variadic parameter.
    # Save the builder before each comma; an empty raw argument restores it.
    g.r("HAP", {2: ("HV", [("LDX","hv","me",F_VAR),("RLD","hv")])})
    g.r("HV", {1: ("HVK", [("ALUI","sub","hv","hnp",1),("CMP","hk","hv")])})
    g.r("HV", {0: ("DEAD", NC("comma paste requires variadic macro"))})
    g.r("HVK", {1: ("HVE", [("INPUSH","hab"),("MARK","hvstart")]),
                  (0,2): ("DEAD", NC("comma paste requires final variadic parameter"))})
    g.on("HVE", WS | {10,2}, "HVE", [("ADV",)])
    g.on("HVE", [EOF], "HW", [("INPOP",),("SBCLR",),("SBBLOB","HCOMMA"),
                               ("LDI","GLUE",0),("LDI","PSP",2),("LDI","LASTK",2)])
    g.els("HVE", "HA0", [("JUMP","hvstart"),("LDI","GLUE",0),("LDI","PSP",0),("LDI","HRAW",1)])

    # not pasted on the left: pasted on the right (`p ##`) takes it raw, else expanded
    raw = ("HA0", [("LDI","HRAW",2),("JUMP", "hq"), ("ALUI", "or", "PSP", "PSP", 2), ("LDI", "LASTK", 2), ("INPUSH", "hab")])
    expd = ("HA0", [("LDI","HRAW",0),("JUMP", "hq"), ("LDX", "hab", "hpa", C_EXP), ("ALUI", "or", "PSP", "PSP", 2),
                    ("LDI", "LASTK", 2), ("INPUSH", "hab")])
    g.on("HAQ", WS | {10}, "HAQ", [("ADV",)])
    g.on("HAQ", [35], "HAQ2", [("ADV",)])
    g.els("HAQ", *expd)
    g.on("HAQ2", [35], *raw)
    g.els("HAQ2", *expd)
    # Internal token separators and hide marks are not paste operands.
    for q in ("HA0","HA1"):
        g.on(q,[2],q,[("ADV",)])
        # Pasting clears the hide mark of the joined boundary token only;
        # other tokens in a multi-token argument retain their hide marks.
        g.on(q,[1],q+".paint",[("RLD","GLUE")])
        g.r(q+".paint",{1:(q,[("ADV",)]),0:(q+".raw",[("RLD","HRAW")])})
        g.r(q+".raw",{2:(q+".last",[("MARK","hpaint"),("ADV",)]),
                       (0,1):("HPGA",[("RLD","GLUE")])})
        g.on(q+".last",ID,q+".last",[("ADV",)])
        g.els(q+".last",q+".tail")
        g.on(q+".tail",WS|{10,2},q+".tail",[("ADV",)])
        g.on(q+".tail",[EOF],q,[("JUMP","hpaint"),("ADV",)])
        g.els(q+".tail","HPGA",[("JUMP","hpaint"),("RLD","GLUE")])
    g.on("HA0", WS | {10}, "HA0", [("ADV",)])
    g.on("HA0", [EOF], "HW", [("INPOP",), ("ALUI", "or", "PSP", "PSP", 2), ("LDI", "GLUE", 0)])
    g.els("HA0", "HPGA", [("RLD", "GLUE")])
    g.on("HA1", WS | {10}, "HA1", [("ADV",), ("LDI", "PSP", 1)])
    g.on("HA1", [EOF], "HW", [("INPOP",), ("LDI", "PSP", 2), ("LDI", "GLUE", 0)])
    g.els("HA1", "HPGA", [("RLD", "GLUE")])
    # `#`: `##` or stringize
    g.on("HH", [35], "HPP", [("ADV",), ("RLD", "LASTK")])
    g.r("HPP", {4: ("HW", [("LDI", "GLUE", 2), ("LDI", "PSP", 0)]),
                (1, 2, 3): ("HW", [("LDI", "GLUE", 1), ("LDI", "PSP", 0)]),
                0: ("DEAD", NC("## operand"))})
    g.els("HH", "HH1", [("RLD", "hfn")])
    g.r("HH1", {1: ("HH2", [("RLD", "GLUE")]), 0: ("HOBJGLUE", [("RLD","GLUE")]),
                   2:("DEAD",NC("# in unsupported body"))})
    g.r("HOBJGLUE",{1:("HOBJPUT",[]),0:("HOBJ",[("RLD","PSP")])})
    g.r("HOBJ", {(1,3):("HOBJPUT",[("SBOUT",32)]),2:("HOBJPUT",[("SBOUT",2)]),0:("HOBJPUT",[])})
    g.els("HOBJPUT","HW",[("SBOUT",35),("LDI","LASTK",3),("LDI","PSP",0),("LDI","GLUE",0)])
    g.r("HH2", {0: ("HH3", []), 1: ("DEAD", NC("## operand"))})
    g.on("HH3", WS, "HH3", [("ADV",)])
    g.on("HH3", AL, "HHI", [("MARK", "hs")])
    g.els("HH3", "DEAD", NC("# not followed by a parameter"))
    g.on("HHI", ID, "HHI", [("ADV",)])
    g.els("HHI", "HPL0S", [("MARK", "he"), ("INTERN", "hid", "hs", "he"), ("LDI", "one", 1), ("RLD", "one")])
    plook("S", ("HSQ", getarg + [("RLD", "PSP")]), ("DEAD", NC("# not followed by a parameter")))
    g.r("HSQ", {(1, 3): ("HS", [("SBOUT", 32), ("SBOUT", 34), ("LDI", "SPS", 0), ("INPUSH", "hab")]),
                2: ("HS", [("SBOUT", 2), ("SBOUT", 34), ("LDI", "SPS", 0), ("INPUSH", "hab")]),
                0: ("HS", [("SBOUT", 34), ("LDI", "SPS", 0), ("INPUSH", "hab")])})
    # stringize walk: leading blanks dropped, inner runs pending as one space
    g.on("HS", WS | {10}, "HS", [("ADV",)])
    g.on("HS", [1], "HS", [("ADV",)])
    g.on("HS1", [1], "HS1", [("ADV",)])
    g.on("HS", [2], "HS", [("ADV",)])
    g.on("HS1", [2], "HS1", [("ADV",)])
    g.on("HS", [EOF], "HW", [("INPOP",), ("SBOUT", 34), ("LDI", "LASTK", 0), ("LDI", "PSP", 0)])
    g.els("HS", "HSP", [("RLD", "SPS")])
    g.r("HSP", {1: ("HSC", [("SBOUT", 32), ("LDI", "SPS", 0)]), 0: ("HSC", [])})
    g.on("HS1", WS | {10}, "HS1", [("ADV",), ("LDI", "SPS", 1)])
    g.on("HS1", [EOF], "HW", [("INPOP",), ("SBOUT", 34), ("LDI", "LASTK", 0), ("LDI", "PSP", 0)])
    g.els("HS1", "HSP", [("RLD", "SPS")])
    for q in (34, 39):
        L = "HSL%d" % q
        g.on("HSC", [q], L, ([("SBOUT", 92)] if q == 34 else []) + one)
        g.on(L, [92], L + "E", [("SBOUT", 92)] + one)
        if q == 39:
            g.on(L, [34], L, [("SBOUT", 92)] + one)
            g.on(L, [39], "HS1", one)
        else:
            g.on(L, [34], "HS1", [("SBOUT", 92)] + one)
        g.on(L, [10, EOF], "DEAD", NC("unterminated literal in a stringized argument"))
        g.els(L, L, one)
        g.on(L + "E", [92, 34], L, [("SBOUT", 92)] + one)
        g.on(L + "E", [EOF], "DEAD", NC("unterminated literal in a stringized argument"))
        g.els(L + "E", L, one)
    g.els("HSC", "HS1", one)


def build_cli(g, NC, locations=False):
    """Raw NUL-separated CLI values from named byte resources. All language
    decisions live here: -include source prefixes, MDEF bodies, and -U masks.
    No CLI directive text is inserted for -D/-U, so source positions stay put.
    """
    g.els("CLI.FLAGS", "CLI.FLAGS.have", sbconst("\0cli/nostdinc")+
          [("SBFIND","CLI_B"),("BLEN","CLI_NOSTD","CLI_B"),("CMPI","CLI_NOSTD",0)])
    g.r("CLI.FLAGS.have", {1:("CLI.INC",[("LDI","CLI_NOSTD",0)]),
                                (0,2):("CLI.INC",[("LDI","CLI_NOSTD",1)])})
    g.els("CLI.INC", "CLI.INC.have", sbconst("\0cli/includes") + [("SBFIND", "CLI_B"), ("RLD", "CLI_B")])
    g.r("CLI.INC.have", {0: ("P0S", []), tuple(range(1,257)): ("CLI.INC.next", [("INPUSH", "CLI_B")])})
    g.on("CLI.INC.next", [EOF], "P0S", [("INPOP",), ("LDI", "CLI_Z", 0), ("XLEN", "CLI_E"),
                                         ("SPAN2", "CLI_Z", "CLI_E"), ("SWAP",)])
    g.els("CLI.INC.next", "CLI.INC.path", [("OUT", c) for c in b'#include "'])
    g.on("CLI.INC.path", [0], "CLI.INC.next", [("ADV",), ("OUT", 34), ("OUT", 10)] + ([("ALUI","add","CLI_PRELINES","CLI_PRELINES",1)] if locations else []))
    g.on("CLI.INC.path", [EOF], "DEAD", NC("unterminated CLI include"))
    g.els("CLI.INC.path", "CLI.INC.path", [("COPY",), ("ADV",)])

    for what,key,done in [('D','defines','P3PD0'),('U','undefines','P3L0')]:
        q='CLI.'+what
        g.els(q, q+'.have', sbconst('\0cli/'+key)+[("SBFIND","CLI_B"),("RLD","CLI_B")])
        g.r(q+'.have', {0:(done,[]),tuple(range(1,257)):(q+'.next',[("INPUSH","CLI_B")])})
        g.on(q+'.next',[EOF],done,[("INPOP",)])
        g.on(q+'.next',AL,q+'.name',[("MARK","CLI_S"),("ADV",)])
        g.els(q+'.next','DEAD',NC('invalid CLI macro name'))
        g.on(q+'.name',ID,q+'.name',[("ADV",)])
        g.els(q+'.name',q+'.end',[("MARK","CLI_E"),("INTERN","NID","CLI_S","CLI_E")])
        if what=='D':
            sub,pu=g.call('MDEF',q+'.store')
            g.on(q+'.end',[0],sub,sbconst('1')+[("SBSAVE","CLI_BODY")]+pu)
            g.on(q+'.end',[61],q+'.body',[("ADV",),("MARK","CLI_S")])
            g.els(q+'.end','DEAD',NC('invalid CLI macro name'))
            g.on(q+'.body',[0],sub,[("MARK","CLI_E"),("BLOBSAVE","CLI_BODY","CLI_S","CLI_E")]+pu)
            g.on(q+'.body',[EOF],'DEAD',NC('unterminated CLI macro body'))
            g.els(q+'.body',q+'.body',[("ADV",)])
            g.els(q+'.store',q+'.next',[("STX","EA",F_BODY,"CLI_BODY"),("ADV",)])
        else:
            g.on(q+'.end',[0],q+'.found',[("ALUI","add","a","NID",NEWB),("LDX","t","a",0),
                                          ("ALUI","sub","M","t",1),("CMPI","M",0)])
            g.els(q+'.end','DEAD',NC('invalid CLI undefine'))
            g.r(q+'.found',{0:(q+'.next',[("ADV",)]),(1,2):(q+'.next',ea('EA','M')+[("LDI","t",0),("STX","EA",F_TO,"t"),("ADV",)])})


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
    build_cli(g, NC, locations)

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

    # ---- P4: token rescan with replacement and argument frames ---------------
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
    g.r("ERPR", {1: ("PR.root", [("LDI","NLC",0)]),
                 (0, 2): (sub, [("COPYW", "SEGQ", "ESEG")] + pu)})
    g.els("ERM", "ERM2", [("CMPI", "M", 0)])
    g.r("ERM2", {0: ("ERL", [("SPANT", "IS")]),
                 (1, 2): ("ERM3", ea("me", "M") + [("LDX", "fn", "me", F_FN), ("RLD", "fn")])})
    # Unsupported function-like declarations reject only when invoked.
    start = [("OUT", 32), ("LDI", "DEP", 0), ("LDI", "SEP", 0), ("LDI", "CUR", 0),
             ("LDI", "BDEP", -1), ("LDI", "PRE", 0), ("LDI", "EDEP", 0),
             ("LDI", "SEPB", 32), ("LDI", "SEPB0", 32)]
    g.on("PR.root", WS, "PR.root", [("ADV",)])
    g.on("PR.root", [10], "PR.root", [("ADV",),("ALUI","add","NLC","NLC",1)])
    g.on("PR.root", [40], "PR.scan", [("ADV",)] + start + [("OLEN","O0"),("LDI","PRDEP",0)])
    g.els("PR.root", sub, [("JUMP","IE"),("COPYW","SEGQ","ESEG")] + pu)
    g.r("ERM3", {2: ("ERFN", []), 1: ("ERFC", [("LDI", "NLC", 0)]),
                 0: ("ERH", [("LDI", "NLC", 0), ("LDI", "FNE", 0)] + start
                     + [("OLEN", "O0"), ("LDX", "hh", "me", F_HASH), ("RLD", "hh")])})
    subx, pux = g.call("HX", "ERHR")
    g.r("ERH", {0: ("EB", PUSHM), 1: (subx, pux), tuple(range(2, 257)): ("DEAD", NC("hash flag"))})
    g.els("ERHR", "EB", PUSHMB)
    # function-like call: name, blanks/newlines (counted, re-emitted after
    # the expansion), `(`, then CF (s13)
    g.on("ERFC", [32, 9], "ERFC", [("ADV",)])
    g.on("ERFC", [10], "ERFC", [("ADV",), ("ALUI", "add", "NLC", "NLC", 1)])
    cfpre = [("ALU", "add", "t", "DEP", "PRE"), ("CMPI", "t", 0)]
    subc, puc = g.call("CF", "ERCR")
    g.on("ERFC", [40], subc, [("ADV",)] + start + [("OLEN", "O0")] + cfpre + puc)
    g.els("ERCR", "EB")
    g.els("ERFC", "ERL", [("JUMP", "IE"), ("SPANT", "IS")])
    # CF (s13): collect the arguments (nested parentheses: a depth counter,
    # top-level commas only) into a call record, expand each on its own (C99
    # 6.10.3.1: the body scan EB run on the argument as a frame whose end is a
    # barrier, BDEP, writing to o and cut off as a blob; a name left unexpanded
    # because its macro is active is painted, byte 1 before it), rewrite the
    # body (HX: `#`/`##` operands raw, other parameters expanded) and push it
    # as the macro's frame.  Newlines inside the call count only when it reads
    # the pass input itself (DEP + PRE == 0).
    init = ([("ALUI", "add", "CL", "CL", 1)] + CRC +
            [("STX", "CR", C_ME, "me"), ("LDI", "NA", 0), ("LDI", "DP", 0), ("LDI", "FNE", -1),
             ("MARK", "AS"), ("SBCLR",)])
    g.r("CF", {1: ("CFCOUNT", init + [("LDI", "NLI", 1)]), (0, 2): ("CFCOUNT", init + [("LDI", "NLI", 0)])})
    g.els("CFCOUNT","CFZERO",[("LDX","np","me",F_NP),("CMPI","np",0)])
    g.r("CFZERO",{1:("CFEMPTY",[]),(0,2):("ARG",[])})
    g.on("CFEMPTY",WS|{2},"CFEMPTY",[("ADV",)])
    g.on("CFEMPTY",[10],"CFEMPTY",[("ADV",),("ALU","add","NLC","NLC","NLI")])
    g.on("CFEMPTY",[41],"ARGN",[("ADV",),("CMP","NA","np")])
    g.on("CFEMPTY",[EOF],"ARGFRAME",[("CMP","DEP","BDEP")])
    g.els("CFEMPTY","DEAD",NC("argument count"))
    save = [("MARK", "AE"), ("SBSPAN", "AS", "AE"), ("SBSAVE", "ab"), ("ALU", "add", "a", "CR", "NA"),
            ("STX", "a", C_RAW, "ab"), ("ALUI", "add", "NA", "NA", 1), ("ADV",)]
    g.on("ARG", [40], "ARG", [("ADV",), ("ALUI", "add", "DP", "DP", 1)])
    g.on("ARG", [44], "ARGK", [("CMPI", "DP", 0)])
    g.r("ARGK", {1: ("ARGVAR", [("LDX","av","me",F_VAR),("RLD","av")]), (0, 2): ("ARG", [("ADV",)])})
    splitarg=("ARGC", save + [("CMPI", "NA", MAXP)])
    g.r("ARGVAR",{0:splitarg,1:("ARGVC",[("LDX","np","me",F_NP),("ALUI","sub","np","np",1),("CMP","NA","np")])})
    g.r("ARGVC",{1:("ARG",[("ADV",)]),(0,2):splitarg})
    g.r("ARGC", {0: ("ARG", [("MARK", "AS"),("SBCLR",)]), (1, 2): ("DEAD", NC("argument count"))})
    g.on("ARG", [41], "ARGR", [("CMPI", "DP", 0)])
    g.r("ARGR", {1: ("ARGEND", save + [("LDX", "np", "me", F_NP), ("CMP", "NA", "np")]),
                 (0, 2): ("ARG", [("ADV",), ("ALUI", "sub", "DP", "DP", 1)])})
    loop = [("STX", "CR", C_K, "kk"), ("LDX", "me", "CR", C_ME), ("LDX", "np", "me", F_NP),
            ("CMP", "kk", "np")]
    g.r("ARGEND",{1:("ARGN",[]),(0,2):("ARGMISS",[("LDX","av","me",F_VAR),("RLD","av")])})
    g.r("ARGMISS",{0:("DEAD",NC("argument count")),1:("ARGMISSC",[("ALUI","sub","need","np",1),("CMP","NA","need")])})
    g.r("ARGMISSC",{1:("ARGN",[("SBCLR",),("SBSAVE","ab"),("ALU","add","a","CR","NA"),("STX","a",C_RAW,"ab"),("LDI","one",1),("RLD","one")]),
                         (0,2):("DEAD",NC("argument count"))})
    g.r("ARGN", {1: ("CFL", [("LDI", "kk", 0)] + loop), (0, 2): ("DEAD", NC("argument count"))})
    g.on("ARG", [10], "ARG", [("ADV",), ("ALU", "add", "NLC", "NLC", "NLI")])
    # A call may start inside a replacement frame and finish in its parent.
    # Append each raw segment before popping; argument-expansion barriers stay.
    g.on("ARG", [EOF], "ARGFRAME", [("CMP","DEP","BDEP")])
    g.r("ARGFRAME",{1:("DEAD",NC("unterminated macro call")),(0,2):("ARGROOT",[("CMPI","DEP",0)])})
    g.r("ARGROOT",{1:("DEAD",NC("unterminated macro call")),(0,2):("ARGNEXT",[
        ("MARK","AE"),("SBSPAN","AS","AE"),("INPOP",),("STX","CUR",F_ACT,"Z0"),
        ("LDX","CUR","CUR",F_UP),("ALUI","sub","DEP","DEP",1),("MARK","AS"),
        ("ALU","add","t","DEP","PRE"),("CMPI","t",0)])})
    g.r("ARGNEXT",{1:("ARGRESUME",[("LDI","NLI",1)]),(0,2):("ARGRESUME",[("LDI","NLI",0)])})
    g.els("ARGRESUME","CFZERO",[("LDX","np","me",F_NP),("CMPI","np",0)])
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
    # argument kk: expanded on its own (EB, returning to CFR at the barrier)
    pre = [("STX", "CR", C_BDEP, "BDEP"), ("STX", "CR", C_PRE, "PRE"), ("STX", "CR", C_EDEP, "EDEP"),
           ("STX", "CR", C_SEP, "SEP"), ("OLEN", "ost"), ("STX", "CR", C_OST, "ost"),
           ("STX", "CR", C_SB, "SEPB"), ("STX", "CR", C_SB0, "SEPB0"), ("LDI", "SEPB", 2), ("LDI", "SEPB0", 2),
           ("LDI", "PRE", 1), ("LDI", "EDEP", -1), ("COPYW", "BDEP", "DEP"), ("LDI", "SEP", 0),
           ("ALU", "add", "a", "CR", "kk"), ("LDX", "ab", "a", C_RAW), ("INPUSH", "ab"), ("PUSH", "CFR")]
    g.labels.add("CFR")
    subh, puh = g.call("HX", "CFH")
    g.r("CFL", {1: (subh, puh), (0, 2): ("EB", pre)})
    g.els("CFR", "CFL", CRC + [("LDX", "ost", "CR", C_OST), ("OCUT", "eb", "ost"), ("LDX", "kk", "CR", C_K),
                               ("ALU", "add", "a", "CR", "kk"), ("STX", "a", C_EXP, "eb"),
                               ("LDX", "BDEP", "CR", C_BDEP), ("LDX", "PRE", "CR", C_PRE),
                               ("LDX", "EDEP", "CR", C_EDEP), ("LDX", "SEP", "CR", C_SEP),
                               ("LDX", "SEPB", "CR", C_SB), ("LDX", "SEPB0", "CR", C_SB0),
                               ("ALUI", "add", "kk", "kk", 1)] + loop)
    # rewritten: pop the record, push the replacement (nothing in it is a
    # parameter any more: FNE matches no entry)
    g.els("CFH", "RET", [("LDX", "me", "CR", C_ME), ("ALUI", "sub", "CL", "CL", 1)] + CRC +
          [("LDI", "FNE", -1)] + PUSHMB)
    g.on("ERFN", [32, 9, 10], "ERFN", [("ADV",)])
    g.on("ERFN", [40], "DEAD", NC("function-like macro invocation"))
    g.els("ERFN", "ERL", [("JUMP", "IE"), ("SPANT", "IS")])
    # token-level rescan of an object-like body (reference: tokens joined by
    # one space, the whole expansion padded by one space each side).  The
    # hide set of an object-like body token is the set of active macros:
    # F_ACT marks them, F_UP chains them (CUR is the innermost).
    popb = [("INPOP",), ("STX", "CUR", F_ACT, "Z0"), ("LDX", "CUR", "CUR", F_UP),
            ("ALUI", "sub", "DEP", "DEP", 1), ("CMP", "DEP", "EDEP")]
    g.on("EB", [EOF], "EBE", [("CMP", "DEP", "BDEP")])
    g.r("EBE", {1: ("RET", [("INPOP",)]), (0, 2): ("EBX", popb)})
    g.r("EBX", {1: ("EBX2", [("OLEN", "t"), ("CMP", "t", "O0")]), (0, 2): ("EB", [])})
    fin = [("LDI", "CHANGED", 1), ("LDI", "FNE", 0), ("CMPI", "NLC", 0)]
    g.r("EBX2", {1: ("EBNL", fin), (0, 2): ("EBNL", [("OUT", 32)] + fin)})
    g.r("EBNL", {2: ("EBNL", [("OUT", 10), ("ALUI", "sub", "NLC", "NLC", 1), ("CMPI", "NLC", 0)]),
                 (0, 1): ("ERL", [])})
    g.on("EB", [32, 9, 10], "EB", [("ADV",), ("LDI", "SEPB", 32)])
    g.on("EB", [2], "EB", [("ADV",)])
    sep = [("RLD", "SEP")]
    g.on("EB", AL, "EBSP", [("MARK", "BIS"), ("LDI", "PNT", 0)] + sep)
    g.on("EB", [1], "EBPT", [("ADV",), ("LDI", "PNT", 1)])
    g.on("EBPT", AL, "EBSP", [("MARK", "BIS")] + sep)
    g.els("EBPT", "DEAD", NC("byte 1 in a body"))
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
        g.r(pre, {1: (nxt, [("OUTW", "SEPB"), ("COPYW", "SEPB", "SEPB0")]), 0: (nxt, [("LDI", "SEP", 1), ("COPYW", "SEPB", "SEPB0")])})
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
    P = ["##", "...", "<<=", ">>=", "->", "++", "--", "<<", ">>", "<=", ">=", "==", "!=", "&&", "||",
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
    g.els("EBID", "EBPR", [("MARK", "BIE"),("INTERN","NID","BIS","BIE"),("CMP","NID","ID_PRAGMAOP")])
    g.r("EBPN", {1: ("EBNX", [("RLD", "PS")]),
                 (0, 2): ("EBPB", [("CMP","CUR","FNE")])})
    g.r("EBPR", {1: ("PR.before", [("LDI","PRCALL",1),("LDI","PRROOT",0),("COPYW","PRHIDE","PNT"),("BLOBSAVE","NMB","BIS","BIE"),("COPYW","PKW","SEPB0")]), (0, 2): ("EBPN", [("RLD", "PNT")])})
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
    pp = [("ALU", "and", "PP", "PNT", "PRE"), ("RLD", "PP")]
    g.r("EBNX", {1: ("EBNY", [("OUTW", "SEPB"), ("COPYW", "SEPB", "SEPB0")] + pp), 0: ("EBNY", [("LDI", "SEP", 1), ("COPYW", "SEPB", "SEPB0")] + pp)})
    g.r("EBNY", {1: ("EB", [("OUT", 1), ("SPANT", "BIS")]), (0, 2): ("EB", [("SPANT", "BIS")])})
    g.r("EBM3", {1: ("EBNX", [("LDI", "PNT", 1), ("RLD", "PS")]),
                 0: ("EBM4", [("LDX", "fn", "me", F_FN), ("RLD", "fn")])})
    # a function-like name: a call when `(` follows, looking past the ends of
    # body frames (popping them) and, at the outermost, into the pass input;
    # never past an argument's barrier (s13)
    g.r("EBM4", {1: ("EBF", [("LDI","PRCALL",0),("BLOBSAVE", "NMB", "BIS", "BIE"), ("COPYW", "PKW", "SEPB0")]),
                 2: ("DEAD", NC("function-like macro name in a body")),
                 0: ("EBH", [("LDX", "hh", "me", F_HASH), ("RLD", "hh")])})
    emit = [("INPUSH", "NMB"), ("LDI", "z0", 0), ("XLEN", "ze"), ("SPAN2", "z0", "ze"), ("INPOP",)]

    def ename(tag, nxt, tail):
        g.r("EN" + tag, {1: (nxt, [("OUTW", "SEPB")] + emit + tail),
                         (0, 2): (nxt, [("LDI", "SEP", 1)] + emit + tail)})
        return "EN" + tag
    subf, puf = g.call("CF", "EBCR")
    g.els("EBCR", "EB")
    g.on("EBF", [32, 9, 10], "EBF", [("ADV",), ("LDI", "PKW", 32)])
    g.on("EBF", [2], "EBF", [("ADV",)])
    g.on("EBF", [40], "PR.open", [("RLD","PRCALL")])
    g.on("EBF", [EOF], "EBFE", [("CMP", "DEP", "BDEP")])
    fk = [("COPYW", "SEPB", "PKW")]
    g.els("EBF", "PR.nobody", [("RLD","PRCALL")])
    g.r("PR.nobody",{0:(ename("F", "EB", fk),[("RLD","PS")]),1:("PR.lookup",[])})
    g.r("EBFE", {1: ("PR.nobody", [("RLD", "PRCALL")]), (0, 2): ("EBFP", popb)})
    g.r("EBFP", {1: ("EBFX", [("MARK", "PX"), ("COPYW", "NLC0", "NLC"), ("LDI","PRROOT",1)]), (0, 2): ("EBF", [])})
    g.on("EBFX", [32, 9], "EBFX", [("ADV",)])
    g.on("EBFX", [10], "EBFX", [("ADV",), ("ALUI", "add", "NLC", "NLC", 1)])
    g.on("EBFX", [40], "PR.open", [("RLD","PRCALL")])
    g.els("EBFX", "PR.noroot", [("JUMP","PX"),("COPYW","NLC","NLC0"),("RLD","PRCALL")])
    g.r("PR.noroot",{0:(ename("X", "EBX2", [("OLEN", "t"), ("CMP", "t", "O0")]),[("RLD","PS")]),
                        1:("PR.lookup",[])})
    prs,prpush=g.call("MFIND","PR.found")
    # Record suppression while the name's owning replacement is still active;
    # lookahead may pop that frame before discovering there is no call.
    prefind,prepush=g.call("MFIND","PR.preFound")
    g.els("PR.before",prefind,prepush)
    g.els("PR.preFound","PR.preHave",[("CMPI","M",0)])
    g.r("PR.preHave",{0:("EBF",[]),(1,2):("PR.preAct",ea("prme","M"))})
    g.els("PR.preAct","EBF",[("LDX","pra","prme",F_ACT),("ALU","or","PRHIDE","PRHIDE","pra")])
    g.els("PR.lookup","PR.hidden",[("RLD","PRHIDE")])
    g.r("PR.hidden",{1:("PR.name",[("LDI","PNT",1)]),0:(prs,prpush)})
    g.els("PR.found","PR.have",[("CMPI","M",0)])
    g.r("PR.have",{0:("PR.name",[("RLD","PRROOT")]),
                     (1,2):("PR.active",ea("me","M")+[("LDX","act","me",F_ACT),("RLD","act")])})
    g.r("PR.active",{1:("PR.name",[("LDI","PNT",1),("RLD","PRROOT")]),
                       0:("PR.kind",[("LDX","fn","me",F_FN),("RLD","fn")])})
    g.r("PR.kind",{0:("EBH",[("LDX","hh","me",F_HASH),("RLD","hh")]),
                     (1,2):("PR.name",[("RLD","PRROOT")])})
    g.els("PR.name","PR.sep",[("RLD","PS")])
    g.r("PR.sep",{1:("PR.paint",[("OUTW","SEPB"),("ALU","and","t","PNT","PRE"),("RLD","t")]),
                      (0,2):("PR.paint",[("LDI","SEP",1),("ALU","and","t","PNT","PRE"),("RLD","t")])})
    g.r("PR.paint",{1:("PR.emit",[("OUT",1)]),(0,2):("PR.emit",[])})
    g.els("PR.emit","PR.resume",emit+[("RLD","PRROOT")])
    g.r("PR.resume",{0:("EB",fk),1:("EBX2",[("OLEN","t"),("CMP","t","O0")])})
    g.r("PR.open", {0:(subf,[("ADV",)] + cfpre + puf),
                       1:("PR.scan",[("ADV",),("LDI","PRDEP",0)])})
    # No macro expansion inside the ignored call. A replacement can end in
    # the middle of the call, but an argument's frame is a hard boundary.
    g.on("PR.scan",[40],"PR.scan",[("ADV",),("ALUI","add","PRDEP","PRDEP",1)])
    g.on("PR.scan",[41],"PR.close",[("ADV",),("CMPI","PRDEP",0)])
    g.r("PR.close",{1:("PR.done",[("CMP","DEP","EDEP")]),
                       (0,2):("PR.scan",[("ALUI","sub","PRDEP","PRDEP",1)])})
    g.r("PR.done",{1:("EBX2",[("OLEN","t"),("CMP","t","O0")]),(0,2):("EB",[])})
    g.on("PR.scan",[10],"PR.nl",[("ADV",),("ALU","add","t","DEP","PRE"),("CMPI","t",0)])
    g.r("PR.nl",{1:("PR.scan",[("ALUI","add","NLC","NLC",1)]),(0,2):("PR.scan",[])})
    g.on("PR.scan",[EOF],"PR.frame",[("CMP","DEP","BDEP")])
    g.r("PR.frame",{1:("DEAD",NC("unterminated _Pragma call")),(0,2):("PR.base",[("CMPI","DEP",0)])})
    g.r("PR.base",{1:("DEAD",NC("unterminated _Pragma call")),(0,2):("PR.scan",popb)})
    for q in (34,39):
        state="PR.quote"+str(q)
        g.on("PR.scan",[q],state,[("ADV",)])
        g.on(state,[92],state+"E",[("ADV",)])
        g.on(state,[q],"PR.scan",[("ADV",)])
        g.on(state,[EOF,10],"DEAD",NC("unterminated _Pragma literal"))
        g.els(state,state,[("ADV",)])
        g.on(state+"E",[EOF],"DEAD",NC("unterminated _Pragma literal"))
        g.els(state+"E",state,[("ADV",)])
    g.els("PR.scan","PR.scan",[("ADV",)])
    subx, pux = g.call("HX", "EBHR")
    g.r("EBH", {0: ("EB", PUSHM), 1: (subx, pux), tuple(range(2, 257)): ("DEAD", NC("hash flag"))})
    g.els("EBHR", "EB", PUSHMB)
    # No expansion: preserve the original bytes; otherwise the rescan is done.
    g.r("P4END", {1: ("ACC", [("OCLR",), ("LDI", "Z", 0), ("XLEN", "XE"), ("SPAN2", "Z", "XE")]),
                  (0, 2): ("ACC", [])})  # the token rescan is complete: one round
    g.r("P4N", {1: ("ACC", []), (0, 2): ("P4", [("SWAP",)])})
    if locations:
        from locations import install
        install(g, SPLB, IRLN, IRNL)
    else:
        g.els("ACC", "ACC", [("ACCEPT",)])
    build_xe(g)
    build_hx(g, NC)
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
