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
file supplies generation data. Transition rules live in the adjacent TSVs;
see rules.md for their inputs and the remaining Python assembly boundary.
This is source migration, not a claim of complete C99 coverage.
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
from finite_rules import install as install_rules

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


def build_autoinc(g, locations=False):
    """P2 autoinc, first run only (RUN == 0), between decomment and P3.
    One scan over x: every maximal identifier run followed (spaces, tabs,
    newlines) by `(` is a call; the `(`'s matching `)` followed by `{` is a
    definition; a call to printf sets RTP (991d337: stdio.h for any printf call).  Then, per header in AUTOINC_ORDER, a name of
    autoinc_map() (printf excluded, as hdrneeded does) with status exactly
    `called` pulls it in; the lines are emitted in prepend order (rtprintf's
    stdio.h first, then the headers last-to-first) and x copied after."""
    install_rules(g, HERE, "autoinc", {"AIB": AIB}, classes={"identifier": ID})
    # per header: does some name have status exactly `called`?
    amap = autoinc_map()
    H = list(AUTOINC_ORDER)
    for h, hn in enumerate(H):
        names = [n for n in amap[hn] if n != "printf"]
        nxt_h = "AH%d_0" % (h + 1) if h + 1 < len(H) else "AEM"
        install_rules(g, HERE, "assembly", {"entry": "AH%d_0" % h, "first": "AH%d_n0" % h,
            "last": "AH%d_n%d" % (h, len(names)), "next": nxt_h, "need": "NEED%d" % h}, section="header")
        for k, nm in enumerate(names):
            install_rules(g, HERE, "autoinc-name", {"entry": "AH%d_n%d" % (h, k),
                "test": "AH%d_r%d" % (h, k), "found": nxt_h,
                "next": "AH%d_n%d" % (h, k + 1), "need": "NEED%d" % h, "AIB": AIB},
                {"name": sbconst(nm)})

    def line(hn):
        return [("OUT", c) for c in ("#include <%s>\n" % hn).encode()] + ([("ALUI","add","AI_LINES","AI_LINES",1)] if locations else [])
    install_rules(g, HERE, "autoinc-emit", {"entry": "AEM", "test": "AEMR",
        "need": "RTP", "next": "AEM%d" % (len(H) - 1)}, {"line": line("stdio.h")})
    for h in range(len(H) - 1, -1, -1):
        install_rules(g, HERE, "autoinc-emit", {"entry": "AEM%d" % h, "test": "AEM%dr" % h,
            "need": "NEED%d" % h, "next": "AEM%d" % (h - 1) if h else "ACP0"}, {"line": line(H[h])})



AL = set(range(97, 123)) | set(range(65, 91)) | {95}
DI = set(range(48, 58))
ID = AL | DI
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




def sbconst(s):
    return [("SBCLR",)] + [("SBOUT", c) for c in s.encode()]


# ---- XE: #if expression evaluator (research/e2-pp-delta.md s11.2) ---------
# shunting-yard over an operator stack (XOB) and value/poison stacks (XVB,
# XPB) in W; signed intmax literals/ordinary signed-char constants via
# literal-*.tsv; decimal/hex/octal digit bounds are 19/16/21. Unsigned, wide
# and multichar constants remain explicit refusals. 64-bit signed via A64/C64.  A division by zero sets the value's
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
    from unisa.front.lex import ESC
    for ch, value in ESC.items():
        if ch not in '01234567x':
            g.on('XCESC', [ord(ch)], 'XCHEND', [('ADV',), ('LDI', 'xr', ord(value))])
    install_rules(g, HERE, "literal", layout)
    install_rules(g, HERE, "expression", layout)
    install_rules(g, HERE, "reduce", layout)


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
    install_rules(g, HERE, "hash", {name: globals()[name] for name in ['C_EXP', 'C_RAW', 'F_BODY', 'F_FN', 'F_HASH', 'F_NP', 'F_P0', 'F_VAR']})



def build_cli(g, locations=False):
    install_rules(g, HERE, "cli", {name: globals()[name] for name in ['F_BODY', 'F_TO', 'NEWB', 'FSZ', 'MACB']},
        {"location_line": [("ALUI", "add", "CLI_PRELINES", "CLI_PRELINES", 1)] if locations else []})



def build(target="lnx/x86_64", locations=False):
    if target not in ("lnx/x86_64", "lnx/arm64", "osx/x86_64", "osx/arm64", "win/x86_64", "win/arm64"):
        raise ValueError("unsupported preprocessor target: "+target)
    target_os, target_arch = target.split("/")
    predef = PREDEF["os", target_os] + PREDEF["arch", target_arch] + PREDEF["common", "*"]
    if len(set(predef)) != len(predef):
        raise ValueError("overlapping target predefinitions: " + target)
    g = G()

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
    install_rules(g, HERE, "text", {"SPLB": SPLB, "after_comments": "AISTART" if AUTOINC else "P3START"})

    # Macro history and definition rules; bindings describe record layout only.
    macro_layout = {'NEWB': NEWB, 'FSZ': FSZ, 'MACB': MACB, 'F_TO': F_TO, 'SEGINF': SEGINF, 'F_FROM': F_FROM, 'F_PREV': F_PREV, 'F_NAME': F_NAME, 'F_BODY': F_BODY, 'F_FN': F_FN, 'F_VAR': F_VAR}
    install_rules(g, HERE, "macro", macro_layout)

    if AUTOINC:
        build_autoinc(g, locations)

    install_rules(g, HERE, "directive-scan", {"TAKEB": TAKEB, "SEENB": SEENB, "DIRB": DIRB})
    for k, nm in enumerate(predef):
        nxt = "P3PD%d" % (k + 1) if k + 1 < len(predef) else "CLI.U"
        install_rules(g, HERE, "assembly", {"entry": "P3PD%d" % k, "resume": "P3PDR%d" % k,
            "next": nxt, "F_BODY": F_BODY}, {"name": sbconst(nm)}, section="predefine")

    cases = {0: ("P3BLANK", [("JUMP", "LS")]), 100: ("PRAG", [("RLD", "LIVE")])}
    cases.update((k + 1, ("D_" + w, [])) for k, w in enumerate(DIRV))
    g.r("DSW", cases)
    for w in DIRV:
        st = "D_" + w
        for fl in (0, 1):
            name = st + "_a%d" % fl
            links = {suffix or "entry": name + suffix for suffix in ("", "b", "c", "m", "n")}
            links.update({key: globals()[key] for key in ['TAKEB', 'SEENB', 'F_TO', 'FSZ', 'MACB']})
            install_rules(g, HERE, "directive-action", links, section=w + "/" + PPT[(w, fl)])

    body_layout = {name: globals()[name] for name in ['F_BODY', 'F_FN', 'F_NP', 'F_P0', 'F_VAR', 'IRLN', 'IRNL', 'MAXP']}
    body_layout["include_body"] = "INC.body" if locations else "INCOK"
    install_rules(g, HERE, "directive-body", body_layout)
    if locations:
        from locations import IRNAME
        install_rules(g, HERE, "include-location", {"IRNAME": IRNAME})

    install_rules(g, HERE, "rescan", {name: globals()[name] for name in ['ARGB', 'ARGE', 'CRB', 'CRS', 'C_BDEP', 'C_EDEP', 'C_EXP', 'C_K', 'C_ME', 'C_OST', 'C_PRE', 'C_RAW', 'C_SB', 'C_SB0', 'C_SEP', 'FSZ', 'F_ACT', 'F_BODY', 'F_FN', 'F_HASH', 'F_NP', 'F_P0', 'F_UP', 'F_VAR', 'MACB', 'MAXP']})
    if locations:
        from locations import install
        install(g, SPLB, IRLN, IRNL)
    else:
        install_rules(g, HERE, "assembly", section="accept")
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
