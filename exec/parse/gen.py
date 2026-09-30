"""Shared transition assembler and token-reader support for the model pipeline.

The retired E3 grammar/code-walker was removed after parse2 became the sole
parser entry point. Historical versions remain in git; this module supplies
P, the token reader, formatting helpers and shared data to current generators.
It is not a second parser. Some retained helpers still encode language rules;
they are not claimed to be a fully language-independent constructor.
"""
import os
import sys
import importlib.util

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
# Loaded by path, not by name.  There are nine modules called `gen.py` under
# exec/, and `from gen import G` after a sys.path insert resolves against
# sys.modules first: measured, if another one has already been imported under
# that name this line raises
#     ImportError: cannot import name 'G' from 'gen' (.../exec/lex/gen.py)
# It works today only because this module happens to reach the name first.
# exec/prune/gen.py:12 already loads its dependency by explicit path for the
# same reason; this is that same pattern.
_spec = importlib.util.spec_from_file_location(
    "pp_gen", os.path.join(ROOT, "exec", "pp", "gen.py"))
_pp = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_pp)
G = _pp.G


def gold(name):
    rows = []
    for ln in open(os.path.join(ROOT, "weights", "gold", name + ".tsv"), encoding="utf-8"):
        if ln.startswith("#"):
            continue
        f = ln.rstrip("\n").split("\t")
        if "=>" in f:
            continue
        rows.append(f)
    return rows


PREC = {f[0]: int(f[1]) for f in gold("prec") if f[1].isdigit()}
BINSEL = {(f[0], f[1]): f[2] for f in gold("binsel") if f[1] in ("s", "u")}
IRSEL = {f[1]: f[2] for f in gold("irsel") if f[0] == "alu"}


def optext(op, u=False):
    sp = IRSEL[BINSEL[(op, "u" if u else "s")]]
    rev = sp.endswith("_rev")
    sp = sp[:-4] if rev else sp
    if sp in ("div", "mod", "udiv", "umod"):
        sp = "." + sp
    return "  %s r0, %s\n" % (sp, "r0, r1" if rev else "r1, r0")


HEADER = ("_start:\n  call __init\n  .argc r0\n  .lea r1, __argvv\n  imm r2, 0\n__argv_top:\n"
          "  slt64 r3, r2, r0\n  jumpz r3, __argv_done\n  .argv r4, r2\n"
          "  imm r5, 8\n  mul64 r5, r2, r5\n  add64 r5, r1, r5\n"
          "  store64 [r5+0], r4\n  imm r5, 1\n  add64 r2, r2, r5\n"
          "  jump __argv_top\n__argv_done:\n  call main\n  jump __main_ret\n.bss __argvv 32768\n")
FOOTER = "__init:\n  ret\n__main_ret:\n  .exit r0\n"

WORDS = ["type=int", "type=void", "type=static", "return", "if", "else", "while", "for", "eof",
         "(", ")", "{", "}", ";", ",", "=", "!", "~",
         "++", "--", "?", ":"] + [o + "=" for o in ("+", "-", "*", "/", "%", "<<", ">>", "&", "^", "|")] + sorted(PREC) + ["do", "break", "continue",
         "typedef", "struct", "type=long", "type=char", "type=unsigned", "type=short", "type=signed", "[", "]", "...", "type=double", "type=float", ".", "->", "sizeof", "switch", "case", "default", "enum", "goto", "union"]
TK = {w: k + 1 for k, w in enumerate(WORDS)}
TK["type"] = TK["type=int"]   # x is the UA_TYPESPELL dump: every other spelling is TK_OTHER
TK_ID, TK_NUM, TK_BADNUM, TK_OTHER, TK_STR, TK_FNUM = 100, 101, 102, 103, 104, 105
CASOPS = ("+", "-", "*", "/", "%", "<<", ">>", "&", "^", "|")
GMARK = 900000   # LOC[v] of a file-scope int (shadowed/restored like any local)
LOC, FND, UNDO, FR, DIG, VS = 10 ** 6, 2 * 10 ** 6, 3 * 10 ** 6, 5 * 10 ** 6, 6 * 10 ** 6, 7 * 10 ** 6
TDD, TDB = 15 * 10 ** 6, 16 * 10 ** 6  # a typedef name's pointer depth and base size (typedef char *va_list: 1, 1)
TDN = 8 * 10 ** 6  # TDN[v] = 1: v was declared a typedef name at file scope
ARR = 14 * 10 ** 6  # ARR[v] = 1: the visible v is an array (its value is its address; PTR[v] = depth after decay)
PTR = 9 * 10 ** 6  # PTR[v] = 1: the visible v is a pointer (8 bytes: load64/store64)
BASE = 11 * 10 ** 6  # BASE[v]: size of v's base type (int 4, char 1, long 8; 0 unknown), the scale of depth-1 +-
CUNK = 0          # base size unknown (void, a typedef name): +- and dereference to depth 0 not covered
FRD, FRB = 12 * 10 ** 6, 13 * 10 ** 6  # per function: return pointer depth and base size
DPR = 18 * 10 ** 6  # DPR[f] = 1: f has a double parameter (an int argument would be converted: not covered)
VAR = 17 * 10 ** 6  # VAR[f] = 1: f was defined `(..., ...)` (its parameters arrive on the stack)
AUT, AUD = 19 * 10 ** 6, 20 * 10 ** 6  # AUT[v] = 1: v is a header function the reference auto-includes; AUD[v] = 2: defined here
VANAMES = ("va_start", "va_arg", "va_end")   # the reference's builtins (va_copy is undefined there: measured)
# struct layouts: STAG[tag] = sid (1..63); SSZ[sid] = size; member key v*64 + sid ->
# MOF offset, MSZ size, MPT pointer depth, MBS base size.  Measured: each member is
# aligned to its own size, the struct's size is rounded up to its largest member
# (struct { char c; long x; short s; int *p; int i; }: c@0 x@8 s@16 p@24 i@32, size 40).
SBB = 3840       # base code of a struct: SBB + sid (a local's BASE; its size is SSZ[sid]).
                 # 1000 until 2026-09-30: the function-signature pool of the E3 parser lives in
                 # [FPS_FIRST, SBB) and 872 codes ran out at thirty units, each with its own
                 # renamed static copy of the libc bodies (R13-0b #27, sbase); 3840 leaves
                 # 3712 signatures and still keeps SBB + STRUCT_MAX (128) below 4096.
STAG, SSZ, MOF, MSZ, MPT, MBS = (21 * 10 ** 6, 22 * 10 ** 6, 23 * 10 ** 6, 24 * 10 ** 6,
                                 25 * 10 ** 6, 26 * 10 ** 6)
TWORDS = ("type", "type=void", "type=long", "type=char", "type=unsigned", "type=short", "type=signed")

g = G()


def O(s):
    return [("OUT", c) for c in s.encode()]


def rej(k):
    return [("REJECT", k)]


# ---- the token reader: a byte trie over the dump's lines -------------------
def tokenizer(qualifiers=("type=const", "type=volatile")):
    from pathlib import Path
    from finite_rules import load as load_rules
    spans = dict(line.split("\t") for line in Path(HERE, "token-prefixes.tsv").read_text().splitlines() if line and not line.startswith("#"))
    def policy(section, state="NEXT", target="NX", token=TK_OTHER, domain=range(257), mode="b"):
        rules = load_rules(Path(HERE, "token-policy.tsv"), {}, domain=domain,
                           bindings=dict(state=state, target=target, token=token), section=section)
        for name, row in rules.items():
            for key, (nxt, acts) in row.items(): g.on(name, [key], nxt, acts, mode)
    pre = {""}
    for w in WORDS + list(spans) + list(qualifiers):
        for i in range(1, len(w) + 1):
            pre.add(w[:i])
    policy("entry", mode="r")
    for p in sorted(pre):
        st = "NX" + p
        if p in spans:
            policy("span", st, spans[p], mode="r")
            continue
        for b in range(256):
            c = chr(b)
            if p + c in pre:
                g.on(st, [b], "NX" + p + c, [("ADV",)])
        if p in qualifiers:   # declaration-only token, skipped by this reader
            policy("qualifier", st, domain=[10])
        elif p in TK:
            policy("word", st, token=TK[p], domain=[10])
        policy("tail", st)
    from finite_rules import install as install_rules
    bindings = {name: globals()[name] for name in ("TK_ID", "TK_STR", "TK_NUM", "TK_BADNUM")}
    # Derive each pre-multiply bound from the integer domain, never frozen answers.
    bindings.update(("limit" + str(d), (2**64 - 1 - d) // 10) for d in range(10))
    install_rules(g, HERE, "tokenread", bindings=bindings,
                  sequences={"truncated": rej("not covered: truncated token dump")})


# ---- a small structured assembler onto (state, r) rows ---------------------
class P:
    """Procedures as op lists; a label is a state; straight-line actions ride
    on the outgoing transition; a branch is a state reading r."""
    n = 0

    def __init__(self, name):
        self.cur = name
        self.acts = []

    def fresh(self, h="k"):
        P.n += 1
        return "%s.%s%d" % (self.cur.split(".")[0], h, P.n)

    def a(self, *acts):
        for x in acts:
            if isinstance(x, list):
                self.acts += x
            else:
                self.acts.append(x)
        return self

    def o(self, s):
        return self.a(O(s))

    def goto(self, lab):
        g.on(self.cur, range(257), lab, self.acts, "r")
        self.cur, self.acts = None, []

    def label(self, lab):
        if self.cur is not None:
            self.goto(lab)
        self.cur = lab
        return self

    def call(self, proc, ret=None):
        ret = ret or self.fresh("r")
        g.labels.add(ret)
        self.a(("PUSH", ret))
        self.goto(proc)
        self.cur = ret
        return self

    def ret(self):
        self.goto("RET")

    def branch(self, cases, other, acts=()):
        """cases: {r-value(s): label}; other: label or ('rej', k)"""
        b = self.fresh("b")
        self.a(*acts)
        self.goto(b)
        done = set()
        for ks, lab in cases.items():
            ks = ks if isinstance(ks, tuple) else (ks,)
            g.on(b, ks, lab, [], "r")
            done |= set(ks)
        if isinstance(other, tuple):
            g.on(b, [k for k in range(257) if k not in done], "DEAD", rej(other[1]), "r")
        else:
            g.on(b, [k for k in range(257) if k not in done], other, [], "r")

    def tok(self, cases, other):
        self.branch({(TK[k] if isinstance(k, str) else k): v for k, v in cases.items()}, other, [("RLD", "tk")])

    def expect(self, w):
        ok = self.fresh("e")
        self.tok({w: ok}, ("rej", "not covered: expected " + w))
        self.cur = ok
        return self

    def num(self, slot):          # print W[slot] in decimal
        return self.a(("COPYW", "n", slot)).call("PRN")

    def lab(self, slot):
        return self.o("L").num(slot)

    def vpush(self, *slots):
        for s in slots:
            self.a(("STX", "vsp", VS, s), ("ALUI", "add", "vsp", "vsp", 1))
        return self

    def vpop(self, *slots):
        for s in reversed(slots):
            self.a(("ALUI", "sub", "vsp", "vsp", 1), ("LDX", s, "vsp", VS))
        return self

    def newlab(self, slot):
        return self.a(("ALUI", "add", "lab", "lab", 1), ("COPYW", slot, "lab"))


def numeric_rules(section, bindings=None, owner=None):
    # Unique metadata instances also satisfy gen2's duplicate-definition guard.
    from pathlib import Path
    from finite_rules import install as install_rules
    bindings = dict(bindings or {}, DIG=DIG, TK_FNUM=TK_FNUM)
    for line in Path(HERE, "numeric-names.tsv").read_text().splitlines():
        if not line.startswith("#"):
            selected, name, prefix, kind = line.split("\t")
            if selected == section:
                bindings[name] = P((owner or prefix) + ".numeric_" + name).fresh(kind)
    install_rules(g, HERE, "numeric", bindings=bindings, section=section)


def prn():
    # One declared decimal algorithm, instantiated at widths zero and six.
    suffixes = ("", ".loop", ".out0", ".pad", ".sp", ".out", ".done") + tuple(".d%d" % i for i in range(20))
    for name, width in (("PRN", 0), ("PRNW", 6)):
        bindings = {"PRN" + suffix.replace(".", "_"): name + suffix for suffix in suffixes}
        numeric_rules("prn", dict(bindings, width=width), owner=name)


def numout():
    numeric_rules("numout")


def fconv():
    numeric_rules("fconv")


def tyinfo():                 # stage tyinfo (weights/gold/tyinfo.tsv): type key -> (size, unsigned)
    return {f[0]: (int(f[1]), int(f[2])) for f in gold("tyinfo") if len(f) >= 3 and f[1].isdigit()}


TY = tyinfo()
CTY = {"char": "i8", "short": "i16", "int": "i32", "long": "i64"}   # the signed spellings in this slice
SZ = {c: TY[k][0] for c, k in CTY.items()}
PSZ = TY["ptr"][0]
assert SZ == {"char": 1, "short": 2, "int": 4, "long": 8} and PSZ == 8, (SZ, PSZ)   # measured widths
assert not any(TY[k][1] for k in CTY.values())
# Value descriptor tags shared with the current parser.
UNS, DBL, FLT, FPB = 16, 64, 65, 67


PUSH = "  .frame 8\n  store64 [r7+0], r0\n"
POP1 = "  load64 r1, [r7+0]\n  .frame -8\n"


def autonames():
    """The reference's autoinc (src/front_pp.c): for each of these headers, a
    raw line opening `static` with `NAME(` and `{` on it names a function;
    if NAME is followed by `(` somewhere in the source and never by
    `( ... ) {`, the whole header is prepended -- tokens the dump does not
    show.  printf is the walker's own (exempt).  Read from include/, as the
    reference reads it."""
    out = []
    for h in "assert.h ctype.h stdlib.h string.h wchar.h stdio.h".split():
        for ln in open(os.path.join(ROOT, "include", h), encoding="utf-8", errors="replace"):
            ln = ln.rstrip("\n")
            if len(ln) <= 7 or not ln.startswith("static") or "(" not in ln or "{" not in ln:
                continue
            b = ln[:ln.index("(")].rstrip(" ")
            a = len(b)
            while a > 0 and (b[a - 1].isalnum() or b[a - 1] == "_"):
                a -= 1
            if a < len(b) and b[a:] != "printf" and b[a:] not in out:
                out.append(b[a:])
    return out


def autoscan():
    from pathlib import Path
    from finite_rules import install as install_rules
    bindings = {"AUT": AUT, "AUD": AUD}
    for line in Path(HERE, "autoscan-names.tsv").read_text().splitlines():
        if not line.startswith("#"):
            name, prefix, kind = line.split("\t")
            bindings[name] = P(prefix + ".autoscan_" + name).fresh(kind)
    classes = {name: [TK[token]] for name, token in (
        ("eof", "eof"), ("lparen", "("), ("rparen", ")"), ("lbrace", "{"))}
    classes["id"] = [TK_ID]
    install_rules(g, HERE, "autoscan", bindings=bindings, classes=classes, section="auto",
                  sequences={"reject_header": rej("not covered: the reference auto-includes a header")})


def sizes(d):
    unr = [i for i, s in enumerate(d["seqs"]) if s == [["REJECT", "unreachable"]]]
    ent = sum(len(r) for _, r in d["states"].values())
    live = sum(1 for _, r in d["states"].values() for v in r.values() if v[1] not in unr)
    return len(d["states"]), ent, live, len(d["seqs"]), sum(len(a) for a in d["seqs"])


if __name__ == "__main__":
    sys.exit("The retired E3 generator is no longer an entry point; use exec/parse2/gen2.py.")
