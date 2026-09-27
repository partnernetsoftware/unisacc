"""Shared transition assembler and token-reader support for the model pipeline.

The retired E3 grammar/code-walker was removed after parse2 became the sole
parser entry point. Historical versions remain in git; this module supplies
P, the token reader, formatting helpers and shared data to current generators.
It is not a second parser. Some retained helpers still encode language rules;
they are not claimed to be a fully language-independent constructor.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "exec", "pp"))
from gen import G   # noqa: E402  (the pp table builder: seq dedup, RET, DEAD)


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
SBB = 1000       # base code of a struct: SBB + sid (a local's BASE; its size is SSZ[sid])
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
    pre = {""}
    for w in WORDS + ["id=", "num=", "str="] + list(qualifiers):
        for i in range(1, len(w) + 1):
            pre.add(w[:i])
    g.on("NEXT", range(257), "NX", [("MARK", "tpos")], "r")
    for p in sorted(pre):
        st = "NX" + p
        if p in ("id=", "num=", "str="):
            g.on(st, range(257), {"id=": "SPANID", "num=": "SPANNUM", "str=": "SPANSTR"}[p], [("MARK", "ps")], "r")
            continue
        for b in range(256):
            c = chr(b)
            if p + c in pre:
                g.on(st, [b], "NX" + p + c, [("ADV",)])
        if p in qualifiers:   # declaration-only token, skipped by this reader
            g.on(st, [10], "NEXT", [("ADV",)])
        elif p in TK:
            g.on(st, [10], "RET", [("ADV",), ("LDI", "tk", TK[p])])
        g.on(st, [256], "DEAD", rej("not covered: truncated token dump"))
        g.els(st, "SKIPO", [("LDI", "tk", TK_OTHER)])
    g.on("SKIPO", [10], "RET", [("ADV",)])
    g.on("SKIPO", [256], "DEAD", rej("not covered: truncated token dump"))
    g.els("SKIPO", "SKIPO", [("ADV",)])
    g.on("SPANID", [10], "RET", [("MARK", "pe"), ("ADV",), ("LDI", "tk", TK_ID)])
    g.on("SPANID", [256], "DEAD", rej("not covered: truncated token dump"))
    g.els("SPANID", "SPANID", [("ADV",)])
    g.on("SPANSTR", [10], "RET", [("MARK", "pe"), ("ADV",), ("LDI", "tk", TK_STR)])
    g.on("SPANSTR", [256], "DEAD", rej("not covered: truncated token dump"))
    g.els("SPANSTR", "SPANSTR", [("ADV",)])
    # decimal: copied as written (<= 20 digits, bounded by UINT64_MAX), suffixes u/l dropped (measured);
    # hex/octal: value in W[nv] (64-bit), printed signed decimal (nx = 1)
    SUF = [ord(c) for c in "uUlL"]
    DIGS = range(48, 58)
    g.on("SPANNUM", [48], "NUM0", [("ADV",), ("LDI", "nx", 0), ("LDI", "nv", 0), ("LDI", "nd", 0)])
    for d in range(1, 10):   # the decimal value too (W[nv]): an array bound
        g.on("SPANNUM", [48 + d], "NUMD", [("ADV",), ("LDI", "nx", 0), ("LDI", "nv", d)])
    # a character constant: its value, printed as a decimal (measured: '0' 48, '\\n' 10, '\\xff' 255, '\\101' 65)
    g.on("SPANNUM", [39], "CQ0", [("ADV",), ("LDI", "nx", 1), ("LDI", "nv", 0), ("LDI", "nd", 0)])
    for c in range(256):
        if c not in (10, 39, 92):
            g.on("CQ0", [c], "CQC", [("ADV",), ("LDI", "nv", c)])
    g.on("CQ0", [92], "CQE", [("ADV",)])
    g.els("CQ0", "SKIPO", [("LDI", "tk", TK_BADNUM)])
    for ch, v in (("n", 10), ("t", 9), ("r", 13), ("a", 7), ("b", 8), ("f", 12), ("v", 11), ("\\", 92), ("'", 39), ('"', 34), ("?", 63)):
        g.on("CQE", [ord(ch)], "CQC", [("ADV",), ("LDI", "nv", v)])
    for d in range(8):
        g.on("CQE", [48 + d], "CQO", [("ADV",), ("LDI", "nv", d), ("LDI", "nd", 1)])
        g.on("CQO", [48 + d], "CQO", [("ADV",), ("ALUI", "shl", "nv", "nv", 3), ("ALUI", "add", "nv", "nv", d), ("ALUI", "add", "nd", "nd", 1)])
    g.on("CQE", [ord("x")], "CQX", [("ADV",), ("LDI", "nd", 0)])
    for d, c in enumerate("0123456789abcdef"):
        for ch in {c, c.upper()}:
            g.on("CQX", [ord(ch)], "CQX", [("ADV",), ("ALUI", "shl", "nv", "nv", 4), ("ALUI", "add", "nv", "nv", d), ("ALUI", "add", "nd", "nd", 1)])
    g.els("CQE", "SKIPO", [("LDI", "tk", TK_BADNUM)])
    for st in ("CQC", "CQO", "CQX"):
        g.on(st, [39], "CQN", [("ADV",)])
        g.els(st, "SKIPO", [("LDI", "tk", TK_BADNUM)])
    g.on("CQN", [10], "CQV", [("MARK", "pe"), ("ADV",), ("CMPI", "nv", 256)])
    g.els("CQN", "SKIPO", [("LDI", "tk", TK_BADNUM)])
    g.r("CQV", {0: ("RET", [("LDI", "tk", TK_NUM)]), (1, 2): ("RET", [("LDI", "tk", TK_BADNUM)])})
    g.els("SPANNUM", "SKIPO", [("LDI", "tk", TK_BADNUM)])
    g.on("NUM0", [10], "RET", [("MARK", "pe"), ("ADV",), ("LDI", "tk", TK_NUM)])
    g.on("NUM0", SUF, "NUMS", [("MARK", "pe"), ("ADV",), ("LDI", "ns", 1)])
    g.on("NUM0", [ord("x"), ord("X")], "NUMX", [("ADV",), ("LDI", "nx", 1)])
    for d in range(8):
        g.on("NUM0", [48 + d], "NUMO", [("ADV",), ("LDI", "nx", 1), ("LDI", "nv", d), ("LDI", "nd", 1)])
    g.on("NUM0", [46], "NUMF", [("ADV",), ("LDI", "fk", 0)])
    g.els("NUM0", "SKIPO", [("LDI", "tk", TK_BADNUM)])
    g.on("NUMD", [46], "NUMF", [("ADV",), ("LDI", "fk", 0)])
    for d in range(10):
        g.on("NUMF", [48 + d], "NUMF", [("ADV",), ("A64I", "mul", "nv", "nv", 10), ("A64I", "add", "nv", "nv", d), ("ALUI", "add", "fk", "fk", 1)])
    g.on("NUMF", [10], "NUMFL", [("MARK", "pe"), ("ADV",), ("ALU", "sub", "t", "pe", "ps"), ("CMPI", "t", 20)])
    g.els("NUMF", "SKIPO", [("LDI", "tk", TK_BADNUM)])
    g.r("NUMFL", {0: ("FCONV", []), (1, 2): ("RET", [("LDI", "tk", TK_BADNUM)])})
    for d in range(10):
        # Check before multiplication; the 20th digit must not wrap W[nv].
        check, append = "NUMD.check%d" % d, "NUMD.append%d" % d
        g.on("NUMD", [48 + d], check, [("LDI", "numlimit", (2**64 - 1 - d) // 10), ("C64U", "nv", "numlimit")])
        g.r(check, {(0, 1): (append, []), 2: ("SKIPO", [("LDI", "tk", TK_BADNUM)])})
        g.els(append, "NUMD", [("ADV",), ("A64I", "mul", "nv", "nv", 10), ("A64I", "add", "nv", "nv", d)])
    g.on("NUMD", [10], "NUMLEN", [("MARK", "pe"), ("ADV",), ("ALU", "sub", "t", "pe", "ps"), ("CMPI", "t", 21)])
    g.on("NUMD", SUF, "NUMDS", [("MARK", "pe"), ("ALU", "sub", "t", "pe", "ps"), ("CMPI", "t", 21)])
    g.els("NUMD", "SKIPO", [("LDI", "tk", TK_BADNUM)])
    g.r("NUMLEN", {0: ("RET", [("LDI", "tk", TK_NUM)]), (1, 2): ("RET", [("LDI", "tk", TK_BADNUM)])})
    g.r("NUMDS", {0: ("NUMS", [("ADV",), ("LDI", "ns", 1)]), (1, 2): ("SKIPO", [("LDI", "tk", TK_BADNUM)])})
    # suffix: at most 3 of u U l L (the reference drops them)
    g.on("NUMS", SUF, "NUMS", [("ADV",), ("ALUI", "add", "ns", "ns", 1)])
    g.on("NUMS", [10], "NUMSN", [("ADV",), ("CMPI", "ns", 4)])
    g.els("NUMS", "SKIPO", [("LDI", "tk", TK_BADNUM)])
    g.r("NUMSN", {0: ("RET", [("LDI", "tk", TK_NUM)]), (1, 2): ("RET", [("LDI", "tk", TK_BADNUM)])})
    for d in range(8):   # octal: <= 21 digits (< 2**63)
        g.on("NUMO", [48 + d], "NUMO", [("ADV",), ("A64I", "shl", "nv", "nv", 3), ("A64I", "add", "nv", "nv", d), ("ALUI", "add", "nd", "nd", 1)])
    g.on("NUMO", [10], "NUMOK", [("MARK", "pe"), ("ADV",), ("CMPI", "nd", 22)])
    g.on("NUMO", SUF, "NUMOS", [("MARK", "pe"), ("CMPI", "nd", 22)])
    g.els("NUMO", "SKIPO", [("LDI", "tk", TK_BADNUM)])
    hx = [(c, c - 48) for c in DIGS] + [(c, c - 87) for c in range(97, 103)] + [(c, c - 55) for c in range(65, 71)]
    for c, d in hx:      # hex: 1..16 digits
        g.on("NUMX", [c], "NUMX", [("ADV",), ("A64I", "shl", "nv", "nv", 4), ("A64I", "add", "nv", "nv", d), ("ALUI", "add", "nd", "nd", 1)])
    g.on("NUMX", [10], "NUMOK", [("MARK", "pe"), ("ADV",), ("ALUI", "sub", "t", "nd", 1), ("CMPI", "t", 16)])
    g.on("NUMX", SUF, "NUMOS", [("MARK", "pe"), ("ALUI", "sub", "t", "nd", 1), ("CMPI", "t", 16)])
    g.els("NUMX", "SKIPO", [("LDI", "tk", TK_BADNUM)])
    g.r("NUMOK", {0: ("RET", [("LDI", "tk", TK_NUM)]), (1, 2): ("RET", [("LDI", "tk", TK_BADNUM)])})
    g.r("NUMOS", {0: ("NUMS", [("ADV",), ("LDI", "ns", 1)]), (1, 2): ("SKIPO", [("LDI", "tk", TK_BADNUM)])})


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


def prn():
    # PRN: W[n] >= 0 in decimal.  PRNW: the same, right-aligned in 6 columns
    for nm, width in (("PRN", 0), ("PRNW", 6)):
        p = P(nm)
        p.a(("LDI", "k", 0))
        p.label(nm + ".loop")
        cases = {}
        for r in range(20):
            lab = nm + ".d%d" % r
            cases[r] = lab
            g.on(lab, range(257), nm + (".out0" if r >= 10 else ".loop"),
                 [("LDI", "t", r % 10), ("STX", "k", DIG, "t"), ("ALUI", "add", "k", "k", 1)], "r")
        p.branch(cases, ("rej", "unreachable"), [("DIVMOD10", "n")])
        p.cur = nm + ".out0"
        p.a(("COPYW", "j", "k"))
        p.label(nm + ".pad")
        p.branch({0: nm + ".sp"}, nm + ".out", [("CMPI", "j", width)])
        p.cur = nm + ".sp"
        p.o(" ").a(("ALUI", "add", "j", "j", 1)).goto(nm + ".pad")
        p.cur = nm + ".out"
        p.a(("ALUI", "sub", "k", "k", 1), ("LDX", "t", "k", DIG), ("ALUI", "add", "t", "t", 48), ("OUTW", "t"))
        p.branch({1: nm + ".done"}, nm + ".out", [("CMPI", "k", 0)])
        p.cur = nm + ".done"
        p.ret()


def numout():
    # NUMOUT: the current literal as the reference prints it -- decimal: its
    # digits as written; hex/octal (nx = 1): W[nv] as signed 64-bit decimal
    p = P("NUMOUT")
    p.branch({1: "NO.v"}, "NO.s", [("CMPI", "nx", 1)])
    p.cur = "NO.s"
    p.a(("SPAN2", "ps", "pe")).ret()
    p.cur = "NO.v"
    p.a(("LDI", "z0", 0), ("LDI", "k", 0)).branch({0: "NO.neg"}, "NO.loop", [("C64", "nv", "z0")])
    p.cur = "NO.neg"
    p.o("-").a(("A64", "sub", "nv", "z0", "nv")).goto("NO.loop")
    p.cur = "NO.loop"
    p.a(("A64I", "urem", "t", "nv", 10), ("STX", "k", DIG, "t"), ("ALUI", "add", "k", "k", 1), ("A64I", "udiv", "nv", "nv", 10))
    p.branch({1: "NO.out"}, "NO.loop", [("LDI", "z0", 0), ("C64", "nv", "z0")])
    p.cur = "NO.out"
    p.a(("ALUI", "sub", "k", "k", 1), ("LDX", "t", "k", DIG), ("ALUI", "add", "t", "t", 48), ("OUTW", "t"))
    p.branch({1: "NO.done"}, "NO.out", [("CMPI", "k", 0)])
    p.cur = "NO.done"
    p.ret()


def fconv():
    """FCONV: the decimal floating constant M / 10^k (W[nv] = M < 10^18, W[fk] = k) as the IEEE-754
    double the reference prints (measured: `imm r0, <the 64 bits as signed decimal>`, 0.1 ->
    4591870180066957722).  Exact: M and D = 10^k are normalised to D <= M < 2D (value = M/D * 2^e),
    53 quotient bits by long division, then round-to-nearest-even on the remainder.  Result in W[nv]
    with nx = 1 (NUMOUT prints it signed); token TK_FNUM."""
    p = P("FCONV")
    p.a(("LDI", "tk", TK_FNUM), ("LDI", "nx", 1), ("LDI", "z0", 0)).branch({1: "RET"}, "FC.d", [("C64", "nv", "z0")])
    p = P("FC.d")
    p.a(("LDI", "fd", 1), ("LDI", "fe", 0)).label("FC.p")
    p.branch({1: "FC.n1"}, "FC.p1", [("CMPI", "fk", 0)])
    P("FC.p1").a(("A64I", "mul", "fd", "fd", 10), ("ALUI", "sub", "fk", "fk", 1)).goto("FC.p")
    p = P("FC.n1")       # M < D: M *= 2, e -= 1
    p.branch({0: "FC.n1a"}, "FC.n2", [("C64U", "nv", "fd")])
    P("FC.n1a").a(("A64I", "shl", "nv", "nv", 1), ("ALUI", "sub", "fe", "fe", 1)).goto("FC.n1")
    p = P("FC.n2")       # M >= 2D: D *= 2, e += 1
    p.a(("A64I", "shl", "t", "fd", 1)).branch({(1, 2): "FC.n2a"}, "FC.q", [("C64U", "nv", "t")])
    P("FC.n2a").a(("COPYW", "fd", "t"), ("ALUI", "add", "fe", "fe", 1)).goto("FC.n2")
    p = P("FC.q")
    p.a(("LDI", "fm", 0), ("LDI", "fc", 53)).label("FC.ql")
    p.a(("A64I", "shl", "fm", "fm", 1)).branch({(1, 2): "FC.q1"}, "FC.q2", [("C64U", "nv", "fd")])
    P("FC.q1").a(("A64I", "add", "fm", "fm", 1), ("A64", "sub", "nv", "nv", "fd")).goto("FC.q2")
    p = P("FC.q2")
    p.a(("A64I", "shl", "nv", "nv", 1), ("ALUI", "sub", "fc", "fc", 1)).branch({1: "FC.r"}, "FC.ql", [("CMPI", "fc", 0)])
    p = P("FC.r")        # round bit, sticky, even
    p.branch({(1, 2): "FC.r1"}, "FC.out", [("C64U", "nv", "fd")])
    p = P("FC.r1")
    p.a(("A64", "sub", "nv", "nv", "fd"), ("LDI", "z0", 0)).branch({1: "FC.r2"}, "FC.up", [("C64", "nv", "z0")])
    p = P("FC.r2")
    p.a(("A64I", "and", "t", "fm", 1)).branch({1: "FC.out"}, "FC.up", [("CMPI", "t", 0)])
    p = P("FC.up")
    p.a(("A64I", "add", "fm", "fm", 1), ("LDI", "t", 1 << 53)).branch({1: "FC.up1"}, "FC.out", [("C64", "fm", "t")])
    P("FC.up1").a(("A64I", "shr", "fm", "fm", 1), ("ALUI", "add", "fe", "fe", 1)).goto("FC.out")
    p = P("FC.out")
    p.a(("ALUI", "add", "t", "fe", 1023), ("A64I", "shl", "nv", "t", 52), ("A64I", "sub", "fm", "fm", 1 << 52),
        ("A64", "add", "nv", "nv", "fm")).ret()


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
    """Two scans of x before the first pass: definitions `NAME ( ... ) {`
    (AUD), then any `NAME (` of a header function not defined here is
    rejected -- the reference compiles the header too."""
    p = P("AUTO")
    p.a(("JUMP", "x0")).call("NEXT").label("AU.loop")
    p.tok({"eof": "AU.two", TK_ID: "AU.id"}, "AU.nx")
    P("AU.nx").call("NEXT").goto("AU.loop")
    p = P("AU.id")
    p.a(("INTERN", "av", "ps", "pe"), ("LDX", "t", "av", AUT)).branch({1: "AU.c"}, "AU.nx", [("CMPI", "t", 1)])
    P("AU.c").call("NEXT").tok({"(": "AU.p"}, "AU.loop")
    P("AU.p").a(("LDI", "ad", 1)).call("NEXT").label("AU.pl")
    P("AU.pl").tok({"(": "AU.po", ")": "AU.pc", "eof": "AU.two"}, "AU.pn")
    P("AU.pn").call("NEXT").goto("AU.pl")
    P("AU.po").a(("ALUI", "add", "ad", "ad", 1)).goto("AU.pn")
    P("AU.pc").a(("ALUI", "sub", "ad", "ad", 1)).branch({1: "AU.cl"}, "AU.pn", [("CMPI", "ad", 0)])
    P("AU.cl").call("NEXT").tok({"{": "AU.def"}, "AU.loop")
    P("AU.def").a(("LDI", "t", 2), ("STX", "av", AUD, "t")).goto("AU.nx")
    p = P("AU.two")
    p.a(("JUMP", "x0")).call("NEXT").label("AV.loop")
    p.tok({"eof": "RET", TK_ID: "AV.id"}, "AV.nx")
    P("AV.nx").call("NEXT").goto("AV.loop")
    p = P("AV.id")
    p.a(("INTERN", "av", "ps", "pe"), ("LDX", "t", "av", AUT)).branch({1: "AV.c"}, "AV.nx", [("CMPI", "t", 1)])
    P("AV.c").a(("LDX", "t", "av", AUD)).branch({1: "AV.nx"}, "AV.c2", [("CMPI", "t", 2)])
    P("AV.c2").call("NEXT").tok({"(": "AV.rej"}, "AV.loop")
    g.on("AV.rej", range(257), "DEAD", rej("not covered: the reference auto-includes a header"), "r")


def sizes(d):
    unr = [i for i, s in enumerate(d["seqs"]) if s == [["REJECT", "unreachable"]]]
    ent = sum(len(r) for _, r in d["states"].values())
    live = sum(1 for _, r in d["states"].values() for v in r.values() if v[1] not in unr)
    return len(d["states"]), ent, live, len(d["seqs"]), sum(len(a) for a in d["seqs"])


if __name__ == "__main__":
    sys.exit("The retired E3 generator is no longer an entry point; use exec/parse2/gen2.py.")
