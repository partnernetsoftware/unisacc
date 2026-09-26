"""E3 feasibility, MINIMUM slice: the parser/code walker as one finite delta.

    python3 exec/parse/gen.py [out.json]      -> writes the table, prints sizes

Machine and primitives: exec/pp/sim.py, unchanged (generic, option A).  x is
the reference's token dump with type spellings (`UA_TYPESPELL=1 ua_tdump
-dump-tokens FILE`, exec/parse/mkdump.sh; one token per line);
o is the reference's `-S` tape.  Design: research/e3-parse-delta.md.

Slice: `int f(int a, ...) { ... }` definitions only, <= 6 parameters; int
locals (block scoped, `int a = e, b;`); expressions: decimal literals,
locals, `=` to a local, unary - ! ~ +, every binary operator of the prec
stage (weights/gold/prec.tsv) incl. && ||, parentheses, calls (<= 6 args) to
a function defined earlier or itself; statements: block, expression, empty,
return e, if/else, while, for(e;e;e).  Anything else is rejected as
`not covered: ...`.

Derived, not typed in: the precedence ladder (prec.tsv), the ALU spelling of
each operator (binsel.tsv -> irsel.tsv; `_rev` = operands swapped).
Transcribed from the reference's tape (measured on probes): the prologue,
epilogue, push/pop, load/store, return, branch and label shapes, the header
and footer text, and the `.` prefix on div/mod (pseudo-ops).
"""
import json
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
         "typedef", "struct", "type=long", "type=char", "type=unsigned", "type=short", "type=signed", "[", "]"]
TK = {w: k + 1 for k, w in enumerate(WORDS)}
TK["type"] = TK["type=int"]   # x is the UA_TYPESPELL dump: every other spelling is TK_OTHER
TK_ID, TK_NUM, TK_BADNUM, TK_OTHER, TK_STR = 100, 101, 102, 103, 104
CASOPS = ("+", "-", "*", "/", "%", "<<", ">>", "&", "^", "|")
GMARK = 900000   # LOC[v] of a file-scope int (shadowed/restored like any local)
LOC, FND, UNDO, FR, DIG, VS = 10 ** 6, 2 * 10 ** 6, 3 * 10 ** 6, 5 * 10 ** 6, 6 * 10 ** 6, 7 * 10 ** 6
TDN = 8 * 10 ** 6  # TDN[v] = 1: v was declared a typedef name at file scope
ARR = 14 * 10 ** 6  # ARR[v] = 1: the visible v is an array (its value is its address; PTR[v] = depth after decay)
PTR = 9 * 10 ** 6  # PTR[v] = 1: the visible v is a pointer (8 bytes: load64/store64)
BASE = 11 * 10 ** 6  # BASE[v]: size of v's base type (int 4, char 1, long 8; 0 unknown), the scale of depth-1 +-
CUNK = 0          # base size unknown (void, a typedef name): +- and dereference to depth 0 not covered
FRD, FRB = 12 * 10 ** 6, 13 * 10 ** 6  # per function: return pointer depth and base size
TWORDS = ("type", "type=void", "type=long", "type=char", "type=unsigned", "type=short", "type=signed")

g = G()


def O(s):
    return [("OUT", c) for c in s.encode()]


def rej(k):
    return [("REJECT", k)]


# ---- the token reader: a byte trie over the dump's lines -------------------
def tokenizer():
    pre = {""}
    for w in WORDS + ["id=", "num=", "str=", "type=const", "type=volatile"]:
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
        if p in ("type=const", "type=volatile"):   # a qualifier: no code in the reference; skipped
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
    # decimal: copied as written (<= 19 digits), suffixes u/l dropped (measured);
    # hex/octal: value in W[nv] (64-bit), printed signed decimal (nx = 1)
    SUF = [ord(c) for c in "uUlL"]
    DIGS = range(48, 58)
    g.on("SPANNUM", [48], "NUM0", [("ADV",), ("LDI", "nx", 0), ("LDI", "nv", 0), ("LDI", "nd", 0)])
    for d in range(1, 10):   # the decimal value too (W[nv]): an array bound
        g.on("SPANNUM", [48 + d], "NUMD", [("ADV",), ("LDI", "nx", 0), ("LDI", "nv", d)])
    g.els("SPANNUM", "SKIPO", [("LDI", "tk", TK_BADNUM)])
    g.on("NUM0", [10], "RET", [("MARK", "pe"), ("ADV",), ("LDI", "tk", TK_NUM)])
    g.on("NUM0", SUF, "NUMS", [("MARK", "pe"), ("ADV",), ("LDI", "ns", 1)])
    g.on("NUM0", [ord("x"), ord("X")], "NUMX", [("ADV",), ("LDI", "nx", 1)])
    for d in range(8):
        g.on("NUM0", [48 + d], "NUMO", [("ADV",), ("LDI", "nx", 1), ("LDI", "nv", d), ("LDI", "nd", 1)])
    g.els("NUM0", "SKIPO", [("LDI", "tk", TK_BADNUM)])
    for d in range(10):
        g.on("NUMD", [48 + d], "NUMD", [("ADV",), ("A64I", "mul", "nv", "nv", 10), ("A64I", "add", "nv", "nv", d)])
    g.on("NUMD", [10], "NUMLEN", [("MARK", "pe"), ("ADV",), ("ALU", "sub", "t", "pe", "ps"), ("CMPI", "t", 20)])
    g.on("NUMD", SUF, "NUMDS", [("MARK", "pe"), ("ALU", "sub", "t", "pe", "ps"), ("CMPI", "t", 20)])
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


def addr(p, reg):             # address of local slot W[s] (or global x[gs..ge)) into reg
    gl, lc, dn = p.fresh("ga"), p.fresh("la"), p.fresh("ad")
    p.branch({1: gl}, lc, [("CMPI", "s", GMARK)])
    p.cur = gl
    p.o("  .lea %s, g_" % reg).a(("SPAN2", "gs", "ge")).o("\n").goto(dn)
    p.cur = lc
    p.o("  imm r2, ").a(("COPYW", "n", "s")).call("PRN").o("\n  sub64 %s, r6, r2\n" % reg).goto(dn)
    p.cur = dn


def lookup(p, lo, hi):        # s := slot of the local spelled x[W[lo]..W[hi])
    p.a(("INTERN", "v", lo, hi), ("LDX", "s", "v", LOC), ("LDX", "pt", "v", PTR), ("LDX", "pb", "v", BASE), ("LDX", "ar", "v", ARR), ("COPYW", "gs", lo), ("COPYW", "ge", hi))
    ok = p.fresh("ok")
    p.branch({1: "DEAD0"}, ok, [("CMPI", "s", 0)])
    p.cur = ok


def declare(p, lo="ps", hi="pe"):   # declare x[lo..hi) as a new local of W[dsz] bytes (array iff W[dar]); frame offset in W[s]
    p.a(("INTERN", "v", lo, hi), ("LDX", "o", "v", LOC), ("LDX", "op", "v", PTR),
        ("STX", "usp", UNDO, "v"), ("STX", "usp", UNDO + 1, "o"), ("STX", "usp", UNDO + 2, "op"),
        ("LDX", "ob", "v", BASE), ("STX", "usp", UNDO + 3, "ob"), ("LDX", "oa", "v", ARR), ("STX", "usp", UNDO + 4, "oa"),
        ("ALUI", "add", "usp", "usp", 5), ("STX", "v", ARR, "dar"),
        ("ALU", "add", "cur", "cur", "dsz"), ("STX", "v", LOC, "cur"), ("STX", "v", PTR, "ptd"), ("STX", "v", BASE, "bsz"), ("COPYW", "s", "cur"))
    up, nx = p.fresh("mx"), p.fresh("dn")
    p.branch({2: up}, nx, [("CMP", "cur", "max")])
    p.cur = up
    p.a(("COPYW", "max", "cur")).goto(nx)
    p.cur = nx


def unwind(p, saved):         # restore the scope to undo depth W[saved]
    top, body, done = p.fresh("uw"), p.fresh("ub"), p.fresh("ud")
    p.label(top)
    p.branch({2: body}, done, [("CMP", "usp", saved)])
    p.cur = body
    p.a(("ALUI", "sub", "usp", "usp", 5), ("LDX", "v", "usp", UNDO), ("LDX", "o", "usp", UNDO + 1), ("LDX", "oa", "usp", UNDO + 4), ("STX", "v", ARR, "oa"),
        ("LDX", "op", "usp", UNDO + 2), ("LDX", "ob", "usp", UNDO + 3), ("STX", "v", LOC, "o"), ("STX", "v", PTR, "op"),
        ("STX", "v", BASE, "ob")).goto(top)
    p.cur = done


def width(p, ptr, i4, i8):    # emit i8 if W[ptr] else i4
    a, b, d = p.fresh("w8"), p.fresh("w4"), p.fresh("wd")
    p.branch({(1, 2): a}, b, [("CMPI", ptr, 1)])
    P(a).o(i8).goto(d)
    P(b).o(i4).goto(d)
    p.cur = d


def tyinfo():                 # stage tyinfo (weights/gold/tyinfo.tsv): type key -> (size, unsigned)
    return {f[0]: (int(f[1]), int(f[2])) for f in gold("tyinfo") if len(f) >= 3 and f[1].isdigit()}


TY = tyinfo()
CTY = {"char": "i8", "short": "i16", "int": "i32", "long": "i64"}   # the signed spellings in this slice
SZ = {c: TY[k][0] for c, k in CTY.items()}
PSZ = TY["ptr"][0]
assert SZ == {"char": 1, "short": 2, "int": 4, "long": 8} and PSZ == 8, (SZ, PSZ)   # measured widths
assert not any(TY[k][1] for k in CTY.values())
LD = {n: ("  load64 r0, [r0+0]\n" if n == 8 else "  .ld r0, [r0+0], %d\n" % n) for n in set(SZ.values())}
ST = {n: ("  store64 [r1+0], r0\n" if n == 8 else "  .st [r1+0], r0, %d\n" % n) for n in set(SZ.values())}
# unsigned char / short: base code UNS + size; stored at the size, loaded then masked (measured:
# `imm r2, 255|65535; and64 r0, r0, r2` after the .ld).  unsigned int/long are not in the slice
# (the reference switches to unsigned compares and divides for them).
UNS = 16
for n in (SZ["char"], SZ["short"]):
    LD[UNS + n] = LD[n] + "  imm r2, %d\n  and64 r0, r0, r2\n" % ((1 << 8 * n) - 1)
    ST[UNS + n] = ST[n]
LD[UNS + 8], ST[UNS + 8] = LD[8], ST[8]   # unsigned long: the long access; its ops are the unsigned ones
LDR = {k: LD[k % UNS] for k in LD}   # x++ / x--: the load is not masked (measured)
MSK = {k: LD[k][len(LD[k % UNS]):] for k in LD}   # op= and ++x: the result masked again before the store (measured)


def vwidth(p, ptr, bs, tab):  # a variable's access: W[ptr] >= 1 -> pointer size; else by W[bs] (tyinfo size); 0 (unknown) -> int
    d, pw, ot = p.fresh("vd"), p.fresh("vp"), p.fresh("vo")
    p.branch({(1, 2): pw}, ot, [("CMPI", ptr, 1)])
    P(pw).o(tab[PSZ]).goto(d)
    q = P(ot)
    for n in sorted(tab):
        if n == SZ["int"]:
            continue
        hit, nx = q.fresh("vs"), q.fresh("vn")
        q.branch({1: hit}, nx, [("CMPI", bs, n)])
        P(hit).o(tab[n]).goto(d)
        q = P(nx)
    q.o(tab[SZ["int"]]).goto(d)
    p.cur = d


def vload(p):                 # a variable's value: an array's is its address (decay, no load: measured)
    ld, dn = p.fresh("vl"), p.fresh("va")
    p.branch({1: dn}, ld, [("CMPI", "ar", 1)])
    p.cur = ld
    vwidth(p, "pt", "pb", LD)
    p.goto(dn)
    p.cur = dn


def noarr(p):                 # assignment / ++ -- / op= to an array: not C
    ok = p.fresh("na")
    p.branch({1: "DEADR"}, ok, [("CMPI", "ar", 1)])
    p.cur = ok


def noptr(p):                 # arithmetic on a pointer is not in this step
    ok = p.fresh("np")
    p.branch({(1, 2): "DEADP"}, ok, [("CMPI", "pt", 1)])
    p.cur = ok


def stars(p, then):           # '*'... then an identifier; W[ptd] = 1 iff any star; bni: base needs one
    lp, st, idk, bad, ok = p.fresh("sl"), p.fresh("ss"), p.fresh("si"), p.fresh("sb"), p.fresh("so")
    p.a(("LDI", "ptd", 0)).label(lp)
    p.tok({"*": st, TK_ID: idk}, ("rej", "not covered: declarator"))
    P(st).a(("ALUI", "add", "ptd", "ptd", 1)).call("NEXT").goto(lp)
    q = P(idk)
    q.branch({0: bad}, ok, [("CMP", "ptd", "bni")])
    g.on(bad, range(257), "DEAD", [("REJECT", "not covered: non-int, non-pointer declaration")], "r")
    P(ok).goto(then)


PUSH = "  .frame 8\n  store64 [r7+0], r0\n"
POP1 = "  load64 r1, [r7+0]\n  .frame -8\n"
NORM = "  imm r1, 0\n  ne r0, r0, r1\n"


M32 = "  imm r2, 4294967295\n  and64 r0, r0, r2\n"
M32 = [M32, M32 + "  and64 r1, r1, r2\n"]


def ubin(q, op, sub, cmp, after, pre=()):
    """left operand in r0 (its type in pt/pb): push, right operand, then the signed or
    unsigned spelling (binsel sign u iff either operand is
    unsigned long -- shifts too: s >> v is lshr64, measured); result: int for a comparison, else unsigned long iff u."""
    q.call("UFLAG").vpush("uf").o(PUSH).call("NEXT").call(sub)
    for x in pre:
        x(q)
    q.call("UFLAG").vpop("ul")
    # class (UFLAG): 1 unsigned long, 3 other 8-wide, 2 unsigned int, 0 int-width.  1 wins, then 3 (signed
    # 64-bit, result long), then 2: the 32-bit unsigned tape -- both operands masked to 32 bits, the u
    # spelling, the result masked again unless a comparison (measured: x - 0x80000000, 0x80000000 >> x,
    # c == 0xffffffff; y + 0x80000000 with long y is add64)
    uu, ss, sl, u4 = q.fresh("uu"), q.fresh("us"), q.fresh("ul"), q.fresh("u4")
    c1, c2, c3, c4 = q.fresh("c1"), q.fresh("c2"), q.fresh("c3"), q.fresh("c4")
    q.branch({1: uu}, c1, [("CMPI", "ul", 1)])
    P(c1).branch({1: uu}, c2, [("CMPI", "uf", 1)])
    P(c2).branch({1: sl}, c3, [("CMPI", "ul", 3)])
    P(c3).branch({1: sl}, c4, [("CMPI", "uf", 3)])
    t = P(c4)
    c5 = t.fresh("c5")
    t.branch({1: u4}, c5, [("CMPI", "ul", 2)])
    P(c5).branch({1: u4}, ss, [("CMPI", "uf", 2)])
    P(uu).o(POP1 + optext(op, True)).a(("LDI", "pt", 0), ("LDI", "pb", 0 if cmp else UNS + 8)).goto(after)
    P(sl).o(POP1 + optext(op)).a(("LDI", "pt", 0), ("LDI", "pb", 0 if cmp else SZ["long"])).goto(after)
    P(u4).o(POP1 + M32[1] + optext(op, True) + ("" if cmp else M32[0])).a(("LDI", "pt", 0), ("LDI", "pb", 0 if cmp else UNS + 4)).goto(after)
    P(ss).o(POP1 + optext(op)).a(("LDI", "pt", 0), ("LDI", "pb", 0)).goto(after)


def expr():
    p = P("UFLAG")    # W[uf]: 1 unsigned long (depth 0, base UNS + 8); 2 unsigned int (UNS + 4); 3 long or a pointer; else 0
    p.a(("LDI", "uf", 3)).branch({1: "UF.1"}, "RET", [("CMPI", "pt", 0)])
    P("UF.1").a(("LDI", "uf", 1)).branch({1: "RET"}, "UF.2", [("CMPI", "pb", UNS + 8)])
    P("UF.2").a(("LDI", "uf", 3)).branch({1: "RET"}, "UF.3", [("CMPI", "pb", SZ["long"])])
    P("UF.3").a(("LDI", "uf", 2)).branch({1: "RET"}, "UF.4", [("CMPI", "pb", UNS + 4)])
    P("UF.4").a(("LDI", "uf", 0)).ret()
    levels = sorted(set(PREC.values()))
    top = levels[-1]
    for L in levels:
        p = P("BIN%d" % L)
        g.labels.add("LOOP%d" % L)
        p.a(("PUSH", "LOOP%d" % L)).goto("BIN%d" % (L + 1) if L < top else "UNARY")
        p = P("LOOP%d" % L)
        cases = {}
        for op in sorted(k for k, v in PREC.items() if v == L):
            cases[op] = "LOOP%d.%s" % (L, op)
        p.tok(cases, "RET")
        sub = "BIN%d" % (L + 1) if L < top else "UNARY"
        for op in cases:
            q = P(cases[op])
            if op == "&&":
                q.newlab("a").vpush("a").o("  jumpz r0, ").lab("a").o("\n").call("NEXT").call(sub)
                q.o(NORM).vpop("a").lab("a").o(":\n").a(("LDI", "pt", 0), ("LDI", "pb", 0)).goto("LOOP%d" % L)
            elif op == "||":
                q.newlab("a").newlab("b").vpush("a")
                q.o("  jumpz r0, ").lab("b").o("\n  imm r0, 1\n  jump ").lab("a").o("\n").lab("b").o(":\n")
                q.call("NEXT").call(sub).o(NORM).vpop("a").lab("a").o(":\n").a(("LDI", "pt", 0), ("LDI", "pb", 0)).goto("LOOP%d" % L)
            elif op in ("+", "-"):   # p +- n: n scaled by 8 at depth >= 2, by BASE (4/1/8) at depth 1
                ok, pp, p1 = q.fresh("pa"), q.fresh("pp"), q.fresh("p1")    # (measured); unknown base -> not covered
                sc = {n: q.fresh("s%d" % n) for n in sorted(set(SZ.values()))}   # scale = tyinfo size of the base
                q.branch({2: pp, 1: p1}, ok, [("CMPI", "pt", 1)])
                t = P(p1)
                for n in sc:
                    nx = t.fresh("sn")
                    t.branch({1: sc[n]}, nx, [("CMPI", "pb", n)])
                    t = P(nx)
                    if n in (1, 2, 8):   # unsigned char/short/long *: scaled by the size (measured)
                        nx = t.fresh("sn")
                        t.branch({1: sc[n]}, nx, [("CMPI", "pb", UNS + n)])
                        t = P(nx)
                t.goto("DEADP")
                for k in sc:
                    r = P(sc[k])
                    r.vpush("pt", "pb").o(PUSH).call("NEXT").call(sub).call("NOPTR").vpop("pt", "pb")
                    r.o(("" if k == 1 else "  imm r2, %d\n  mul64 r0, r0, r2\n" % k) + POP1 + optext(op)).goto("LOOP%d" % L)
                ubin(P(ok), op, sub, False, "LOOP%d" % L, [lambda x: x.call("NOPTR")])
                r = P(pp)
                r.vpush("pt").o(PUSH).call("NEXT").call(sub).call("NOPTR").vpop("pt")
                r.o("  imm r2, %d\n  mul64 r0, r0, r2\n" % PSZ + POP1 + optext(op)).goto("LOOP%d" % L)
            elif op in ("==", "!=", "<", "<=", ">", ">="):   # pointers compare as the ints do (measured: p < q is slt64)
                ubin(q, op, sub, True, "LOOP%d" % L)
            else:     # a pointer operand is not covered: the reference scales it
                noptr(q)
                ubin(q, op, sub, False, "LOOP%d" % L, [noptr])
    # BINCONT: the operand is already emitted; resume every level's loop
    p = P("BINCONT")
    for L in levels[:-1]:
        p.a(("PUSH", "LOOP%d" % L))
    p.goto("LOOP%d" % top)

    # ASSIGN: id '=' ASSIGN | BIN
    p = P("EXPR")
    p.tok({TK_ID: "EXPR.id", "*": "EXPR.st"}, "EXPR.bin")
    p = P("EXPR.bin")
    p.call("BIN%d" % levels[0]).goto("EXPR.tail")
    p = P("EXPR.id")
    p.a(("COPYW", "sps", "ps"), ("COPYW", "spe", "pe")).call("NEXT")
    p.tok(dict([("=", "EXPR.as"), ("[", "EXPR.ix")] + [(o + "=", "EXPR.c" + o) for o in CASOPS]), "EXPR.use")
    for o in CASOPS:     # a op= e: address, load, push, e, op, store (measured)
        q = P("EXPR.c" + o)
        lookup(q, "sps", "spe")
        noptr(q)
        addr(q, "r0")
        q.o(PUSH)
        vwidth(q, "pt", "pb", LD)
        q.o(PUSH).vpush("pt", "pb").call("NEXT").call("EXPR").call("UFLAG").vpop("pt", "pb")
        uu, ss, u2, dn = q.fresh("cu"), q.fresh("cs"), q.fresh("c2"), q.fresh("cd")
        q.branch({1: uu}, u2, [("CMPI", "pb", UNS + 8)])    # unsigned iff either side is (measured)
        u3 = u2 + "x"
        P(u2).branch({1: uu, 2: "DEADU4"}, u3, [("CMPI", "uf", 1)])
        P(u3).branch({1: "DEADU4"}, ss, [("CMPI", "uf", 2)])
        P(uu).o(POP1 + optext(o, True)).goto(dn)
        P(ss).o(POP1 + optext(o)).goto(dn)
        q.cur = dn
        vwidth(q, "pt", "pb", MSK)
        q.o(POP1)
        vwidth(q, "pt", "pb", ST)
        q.ret()
    # *E = e  |  *E as an rvalue.  E is a pointer value (PV): W[pt] = its depth
    p = P("EXPR.st")
    p.call("NEXT").call("PV").call("PVCHK")
    p.tok({"=": "EXPR.sta"}, "EXPR.stu")
    p = P("VEXPR.st")   # statement level: `*p;` loads p only (measured)
    p.call("NEXT").call("PV").call("PVCHK")
    p.tok({"=": "VEXPR.sta", ";": "RET", ",": "EXPR.dis", ")": "EXPR.dis"}, "VEXPR.stu")
    g.on("EXPR.dis", range(257), "DEAD", rej("not covered: discarded dereference"), "r")
    P("VEXPR.sta").call("EXPR.sta").goto("VEXPR.c")
    P("VEXPR.stu").call("EXPR.stu").goto("VEXPR.c")
    p = P("EXPR.sta")
    p.a(("ALUI", "sub", "pt", "pt", 1)).o(PUSH).vpush("pt", "pb").call("NEXT").call("EXPR").vpop("pt", "pb").o(POP1)
    vwidth(p, "pt", "pb", ST)     # depth 0: the pointee's base width (char *p: .st 1, measured)
    p.ret()
    P("EXPR.stu").call("DEREF").call("BINCONT").goto("EXPR.tail")
    # PV: '*' PV | '&' id | id  -> r0 = the pointer value, W[pt] = its depth
    p = P("PV")
    p.tok({"*": "PV.st", "&": "PV.amp", TK_ID: "PV.id", "(": "PV.par"}, ("rej", "not covered: operand of *"))
    P("PV.par").call("U.par").call("NOPOST").ret()   # *(T *)e, *(p + 1)
    P("PV.st").call("NEXT").call("PV").call("PVCHK").call("DEREF").ret()
    p = P("PV.amp")
    p.call("NEXT").tok({TK_ID: "PV.amq"}, ("rej", "not covered: operand of &"))
    p = P("PV.amq")
    lookup(p, "ps", "pe")
    addr(p, "r0")
    p.call("NEXT").tok({"[": "PV.amx"}, "PV.amv")
    P("PV.amv").a(("ALUI", "add", "pt", "pt", 1)).call("NOPOST").ret()
    p = P("PV.amx")     # &a[i]: the element's address, as a[i] without the load (measured)
    vload(p)
    p.call("IDX").a(("ALUI", "add", "pt", "pt", 1)).call("NOPOST").ret()
    p = P("PV.id")
    lookup(p, "ps", "pe")
    addr(p, "r0")
    ld, dn = p.fresh("pl"), p.fresh("pd")
    p.branch({1: dn}, ld, [("CMPI", "ar", 1)])
    p.cur = ld
    width(p, "pt", "  .ld r0, [r0+0], 4\n", "  load64 r0, [r0+0]\n")
    p.goto(dn)
    p.cur = dn
    p.call("NEXT").call("NOPOST").ret()
    p = P("NOPOST")
    p.tok({"(": "DEADX", "++": "DEADX", "--": "DEADX"}, "RET")
    g.on("DEADX", range(257), "DEAD", rej("not covered: postfix on a * or & operand"), "r")
    p = P("PVCHK")    # dereferencing needs a pointer
    p.branch({(1, 2): "RET"}, ("rej", "not covered: dereference of a non-pointer"), [("CMPI", "pt", 1)])
    p = P("DEREF")    # r0 := *r0; the pointee's width follows the pointee type
    p.a(("ALUI", "sub", "pt", "pt", 1)).call("LDA").ret()
    p = P("LDA")      # r0 := *r0 for an address whose value has depth W[pt], base W[pb]
    d0, dk = p.fresh("d0"), p.fresh("dk")
    p.branch({1: d0}, dk, [("CMPI", "pt", 0)])
    P(d0).branch({(0, 2): dk}, ("rej", "not covered: dereference of an unknown base"), [("CMPI", "pb", CUNK)])
    p.cur = dk
    vwidth(p, "pt", "pb", LD)     # depth 0: the pointee's base width (char *p: .ld 1, measured)
    p.ret()
    p = P("IDX")      # p[i] = *(p + i) (measured, same tape); chained p[i][j] loads between
    p.label("IX.top").branch({(1, 2): "IX.ok"}, ("rej", "not covered: subscript of a non-pointer"), [("CMPI", "pt", 1)])
    p = P("IX.ok")
    p.o(PUSH).vpush("pt", "pb").call("NEXT").call("CEXPR").call("NOPTR").vpop("pt", "pb").expect("]")
    pp, p1 = p.fresh("xp"), p.fresh("x1")
    p.branch({2: pp}, p1, [("CMPI", "pt", 1)])
    P(pp).o("  imm r2, %d\n  mul64 r0, r0, r2\n" % PSZ).goto("IX.add")
    t = P(p1)
    for n in sorted(set(SZ.values())):
        hit, nx = t.fresh("xs"), t.fresh("xn")
        t.branch({1: hit}, nx, [("CMPI", "pb", n)])
        P(hit).o("" if n == 1 else "  imm r2, %d\n  mul64 r0, r0, r2\n" % n).goto("IX.add")
        t = P(nx)
        if n in (1, 2, 8):
            nx = t.fresh("xn")
            t.branch({1: hit}, nx, [("CMPI", "pb", UNS + n)])
            t = P(nx)
    t.goto("DEADP")
    p = P("IX.add")
    p.o(POP1 + "  add64 r0, r1, r0\n").a(("ALUI", "sub", "pt", "pt", 1)).call("NEXT")
    p.tok({"[": "IX.more"}, "RET")
    P("IX.more").call("LDA").goto("IX.top")
    p = P("IXV")      # id '[' ... as an rvalue
    lookup(p, "sps", "spe")
    addr(p, "r0")
    vload(p)
    p.call("IDX").call("LDA").call("NOPOST").ret()
    for nm, extra in (("EXPR.ix", {}), ("VEXPR.ixs", {";": "RET", ",": "EXPR.dis", ")": "EXPR.dis"})):
        p = P(nm)     # id '[' ... = e  |  as an rvalue; statement level `p[i];` computes the address only (measured)
        lookup(p, "sps", "spe")
        addr(p, "r0")
        vload(p)
        p.call("IDX").tok(dict([("=", "EXPR.ixa")] + [(o + "=", "EXPR.ixc") for o in CASOPS], **extra), "EXPR.ixu")
    g.on("EXPR.ixc", range(257), "DEAD", rej("not covered: compound assignment to a subscript"), "r")
    P("EXPR.ixu").call("LDA").call("NOPOST").call("BINCONT").goto("EXPR.tail")
    p = P("EXPR.ixa")
    d0, dk = p.fresh("a0"), p.fresh("ak")
    p.branch({1: d0}, dk, [("CMPI", "pt", 0)])
    P(d0).branch({(0, 2): dk}, ("rej", "not covered: dereference of an unknown base"), [("CMPI", "pb", CUNK)])
    p.cur = dk
    p.o(PUSH).vpush("pt", "pb").call("NEXT").call("EXPR").vpop("pt", "pb").o(POP1)
    vwidth(p, "pt", "pb", ST)
    p.ret()
    p = P("EXPR.use")
    p.call("IDTAIL").call("BINCONT").goto("EXPR.tail")
    p = P("EXPR.tail")
    p.tok({"=": "EXPR.bad", "?": "EXPR.q"}, "RET")
    p = P("EXPR.q")     # c ? a : b  (labels as if/else, measured)
    p.newlab("a").newlab("b").o("  jumpz r0, ").lab("a").o("\n").vpush("a", "b").call("NEXT").call("CEXPR").expect(":")
    p.call("UFLAG").branch({1: "DEADU", 2: "EXPR.qu2"}, "EXPR.q2", [("CMPI", "uf", 1)])
    P("EXPR.qu2").branch({1: "DEADU"}, "EXPR.q2", [("CMPI", "uf", 2)])
    p = P("EXPR.q2")
    p.vpop("a", "b").o("  jump ").lab("b").o("\n").lab("a").o(":\n").vpush("b").call("NEXT").call("EXPR")
    p.call("UFLAG").branch({1: "DEADU", 2: "EXPR.qu3"}, "EXPR.q3", [("CMPI", "uf", 1)])
    P("EXPR.qu3").branch({1: "DEADU"}, "EXPR.q3", [("CMPI", "uf", 2)])
    P("EXPR.q3").vpop("b").lab("b").o(":\n").ret()
    g.on("DEADU4", range(257), "DEAD", rej("not covered: unsigned int operand"), "r")
    g.on("DEADU", range(257), "DEAD", rej("not covered: unsigned long or unsigned int in ?:"), "r")
    # comma.  A bare identifier whose value is discarded emits its address
    # only (measured: `a;`, `a, 1`).  VEXPR: statement level / for clauses
    # (discarded when followed by ; , or )); CEXPR: value context (discarded
    # only when a comma follows).
    for nm, stops in (("VEXPR", (";", ",", ")")), ("CEXPR", (",",))):
        q = P(nm)
        q.tok({TK_ID: nm + ".id"} if nm == "CEXPR" else {TK_ID: nm + ".id", "*": nm + ".st"}, nm + ".e")
        P(nm + ".e").call("EXPR").goto(nm + ".c")
        q = P(nm + ".id")
        q.a(("COPYW", "sps", "ps"), ("COPYW", "spe", "pe")).call("NEXT")
        q.tok(dict([(k, nm + ".addr") for k in stops] + [("=", nm + ".as"), ("[", nm + ".ix")]
                   + [(o + "=", nm + ".c" + o) for o in CASOPS]), nm + ".use")
        P(nm + ".use").call("EXPR.use").goto(nm + ".c")
        P(nm + ".as").call("EXPR.as").goto(nm + ".c")
        P(nm + ".ix").call("EXPR.ix" if nm == "CEXPR" else "VEXPR.ixs").goto(nm + ".c")
        for o in CASOPS:
            P(nm + ".c" + o).call("EXPR.c" + o).goto(nm + ".c")
        q = P(nm + ".addr")
        lookup(q, "sps", "spe")
        addr(q, "r0")
        q.goto(nm + ".c")
        q = P(nm + ".c")
        q.tok({",": nm + ".more"}, "RET")
        P(nm + ".more").call("NEXT").goto(nm)
    g.on("EXPR.bad", range(257), "DEAD", rej("not covered: assignment to a non-identifier"), "r")
    p = P("EXPR.as")
    lookup(p, "sps", "spe")
    noarr(p)
    addr(p, "r0")
    p.o(PUSH).vpush("pt", "pb").call("NEXT").call("EXPR").vpop("pt", "pb").o(POP1)
    vwidth(p, "pt", "pb", ST)
    p.ret()
    p = P("NOPTR")
    noptr(p)
    p.ret()
    g.on("DEADR", range(257), "DEAD", rej("not covered: assignment to an array"), "r")
    g.on("DEADP", range(257), "DEAD", rej("not covered: pointer arithmetic"), "r")

    # UNARY
    p = P("UNARY")
    p.a(("LDI", "pt", 0), ("LDI", "pb", 0))   # a primary is an int unless it says otherwise
    p.tok({"-": "U.neg", "!": "U.not", "~": "U.cpl", "+": "U.pos", "(": "U.par",
           TK_NUM: "U.num", TK_STR: "U.str", TK_ID: "U.id", "*": "U.star", "&": "U.amp", "++": "U.pinc", "--": "U.pdec"}, ("rej", "not covered: expression"))
    for nm, sp in (("U.pinc", "add64"), ("U.pdec", "sub64")):
        P(nm).call("NEXT").tok({TK_ID: nm + ".id"}, ("rej", "not covered: operand of ++/--"))
        q = P(nm + ".id")
        lookup(q, "ps", "pe")
        noptr(q)
        addr(q, "r0")
        q.o(PUSH)
        vwidth(q, "pt", "pb", LD)
        q.o("  imm r1, 1\n  %s r0, r0, r1\n" % sp)
        vwidth(q, "pt", "pb", MSK)
        q.o(POP1)
        vwidth(q, "pt", "pb", ST)
        q.call("NEXT").ret()
    for nm, txt in (("U.neg", "  imm r1, 0\n  sub64 r0, r1, r0\n"), ("U.not", "  imm r1, 0\n  eq r0, r0, r1\n"),
                    ("U.cpl", "  imm r1, -1\n  xor64 r0, r0, r1\n")):
        q = P(nm).call("NEXT").call("UNARY")
        if nm != "U.not":      # - ~ of a pointer, or of an unsigned int: not covered
            noptr(q)
            ok = q.fresh("n4")
            q.branch({1: "DEADU4"}, ok, [("CMPI", "pb", UNS + 4)])
            q.cur = ok
        q.o(txt)
        if nm == "U.not":      # !x is an int whatever x was (C99 6.5.3.3p5): a + !q does not scale a
            q.a(("LDI", "pt", 0), ("LDI", "pb", 0))
        q.ret()
    P("U.pos").call("NEXT").goto("UNARY")
    P("U.star").call("NEXT").call("PV").call("PVCHK").call("DEREF").ret()
    P("U.amp").goto("PV")
    # '(' : a cast when a type word or a typedef name (TDN) follows, else a parenthesised expression
    P("U.par").call("NEXT").tok({"type": "CA.int", "type=char": "CA.char", "type=short": "CA.short",
                                 "type=long": "CA.long", "type=void": "CA.void", TK_ID: "U.pid"}, "U.pe")
    P("U.pe").call("CEXPR").expect(")").call("NEXT").ret()
    p = P("U.pid")
    p.a(("INTERN", "v", "ps", "pe"), ("LDX", "t", "v", TDN)).branch({1: "CA.void"}, "U.pe", [("CMPI", "t", 1)])
    for c in ("int", "char", "short", "long"):
        P("CA." + c).a(("LDI", "cb", SZ[c])).goto("CA.st")
    P("CA.void").a(("LDI", "cb", CUNK)).goto("CA.st")    # void / typedef name: only as a pointer cast
    p = P("CA.st")
    p.a(("LDI", "cd", 0)).call("NEXT").label("CA.sl")
    p.tok({"*": "CA.star", ")": "CA.cl"}, ("rej", "not covered: cast"))
    P("CA.star").a(("ALUI", "add", "cd", "cd", 1)).call("NEXT").goto("CA.sl")
    p = P("CA.cl")     # (T)e: the operand is a unary expression
    p.vpush("cb", "cd").call("NEXT").call("UNARY").vpop("cb", "cd")
    p.branch({(1, 2): "CA.ptr"}, "CA.sc", [("CMPI", "cd", 1)])
    P("CA.ptr").a(("COPYW", "pt", "cd"), ("COPYW", "pb", "cb")).ret()   # (T *)e: no code (measured)
    q = P("CA.sc")     # (T)e, T scalar: narrowed through the stack at T's size; long: no code (measured)
    for n in sorted(set(SZ.values())):
        hit, nx = q.fresh("cs"), q.fresh("cn")
        q.branch({1: hit}, nx, [("CMPI", "cb", n)])
        r = P(hit)
        if n != PSZ:
            r.o("  .frame 8\n  .st [r7+0], r0, %d\n  .ld r0, [r7+0], %d\n  .frame -8\n" % (n, n))
        r.a(("LDI", "pt", 0), ("LDI", "pb", n)).ret()
        q = P(nx)
    q.branch({}, ("rej", "not covered: cast to a non-scalar"))
    # a hex/octal literal in (INT_MAX, UINT_MAX] is an unsigned int (C99 6.4.4.1): the reference masks
    # the operands to 32 bits (measured: g() - 0x80000000; see ubin)
    p = P("U.num")
    p.branch({1: "U.nx"}, "U.num1", [("CMPI", "nx", 1)])
    p = P("U.nx")
    p.a(("LDI", "t", 0x7fffffff)).branch({2: "U.nx2"}, "U.num1", [("C64", "nv", "t")])
    p = P("U.nx2")
    p.a(("A64I", "shr", "t", "nv", 32), ("LDI", "z0", 0)).branch({1: "U.nxu"}, "U.num1", [("C64", "t", "z0")])
    P("U.nxu").o("  imm r0, ").call("NUMOUT").o("\n").call("NEXT").a(("LDI", "pb", UNS + 4)).ret()
    P("U.num1").o("  imm r0, ").call("NUMOUT").o("\n").call("NEXT").ret()
    # a string literal: `.lea r0, Sk`, k counted with the printf segments in source order; it also takes a label
    # number (measured: `p = "ab"; if (u)` -> L3); ?: takes both of its labels up front (u ? "x" : "y" -> L4 L5, then 6 7)
    p = P("U.str")
    p.o("  .lea r0, S").num("sk").o("\n").a(("ALUI", "add", "sk", "sk", 1), ("ALUI", "add", "lab", "lab", 1), ("LDI", "pt", 1), ("LDI", "pb", SZ["char"])).call("NEXT")
    p.tok({TK_STR: "U.strs"}, "RET")
    g.on("U.strs", range(257), "DEAD", rej("not covered: adjacent string literals"), "r")
    P("U.id").a(("COPYW", "sps", "ps"), ("COPYW", "spe", "pe")).call("NEXT").call("IDTAIL").ret()

    # IDTAIL: saved id x[sps..spe), current token follows it
    p = P("IDTAIL")
    p.tok({"(": "IT.call", "++": "IT.inc", "--": "IT.dec", "[": "IXV"}, "IT.var")
    for nm, o, undo in (("IT.inc", "+", "sub64"), ("IT.dec", "-", "add64")):
        q = P(nm)   # a++ : a += 1, then the old value back (measured)
        lookup(q, "sps", "spe")
        noptr(q)
        addr(q, "r0")
        q.o(PUSH)
        vwidth(q, "pt", "pb", LDR)
        q.o(PUSH + "  imm r0, 1\n" + POP1 + optext(o) + POP1)
        vwidth(q, "pt", "pb", ST)
        q.o("  imm r2, 1\n  %s r0, r0, r2\n" % undo).call("NEXT").ret()
    p = P("IT.var")
    lookup(p, "sps", "spe")
    addr(p, "r0")
    vload(p)
    p.ret()
    p = P("IT.call")
    p.a(("INTERN", "v", "sps", "spe"), ("LDX", "t", "v", FND))
    p.branch({1: "IT.ok"}, "IT.nd", [("CMP", "t", "pass")])
    p = P("IT.nd")
    p.branch({1: "PF"}, "IT.nw", [("CMP", "v", "pfid")])
    # syscall builtins (SYSCALLS): arguments pushed as for a call, popped to r(n-1)..r0,
    # r(n)..r(w-1) zeroed, then `.sys NAME, r0, r1, r2` (w 3) or `.sys6 NAME, r0..r5` (w 6) -- measured
    for k, (nm, _, _) in enumerate(SYSCALLS, 1):
        last = k == len(SYSCALLS)
        nxt = ("rej", "not covered: call to a function not defined before") if last else "IT.n%d" % (k + 1)
        P("IT.nw" if k == 1 else "IT.n%d" % k).branch({1: "IT.s%d" % k}, nxt, [("CMP", "v", "sy%d" % k)])
        P("IT.s%d" % k).a(("LDI", "sys", k)).goto("IT.ok3")
    p = P("IT.ok")
    p.a(("CMP", "v", "pfid"))
    p.branch({1: "DEADPF"}, "IT.ok1")
    g.on("DEADPF", range(257), "DEAD", rej("not covered: printf defined in the unit"), "r")
    p = P("IT.ok1")
    p.a(("LDX", "t", "v", LOC))
    p.branch({1: "IT.ok2"}, ("rej", "not covered: call through a local"), [("CMPI", "t", 0)])
    P("IT.ok2").a(("LDI", "sys", 0)).goto("IT.ok3")
    p = P("IT.ok3")
    p.vpush("sps", "spe", "sys").a(("LDI", "na", 0), ("LDI", "la", 0)).call("NEXT").tok({")": "IT.close"}, "IT.arg")
    p = P("IT.arg")
    p.vpush("na").call("EXPR").call("UFLAG").vpop("na").o(PUSH).a(("ALUI", "add", "na", "na", 1), ("COPYW", "la", "uf"))
    p.tok({",": "IT.comma", ")": "IT.close"}, ("rej", "not covered: argument list"))
    P("IT.comma").call("NEXT").goto("IT.arg")
    p = P("IT.close")
    p.branch({2: "DEADA"}, "IT.pop", [("COPYW", "nar", "na"), ("CMPI", "na", 6)])
    g.on("DEADA", range(257), "DEAD", rej("not covered: more than 6 arguments"), "r")
    p = P("IT.pop")
    p.branch({1: "IT.emit"}, "IT.pop1", [("CMPI", "na", 0)])
    p = P("IT.pop1")
    p.a(("ALUI", "sub", "na", "na", 1)).o("  load64 r").num("na").o(", [r7+0]\n  .frame -8\n").goto("IT.pop")
    p = P("IT.emit")
    p.vpop("sps", "spe", "sys").branch({1: "IT.ecall"}, "IT.esys", [("CMPI", "sys", 0)])
    for k, (_, _, w) in enumerate(SYSCALLS, 1):
        nxt = "IT.w%d" % (k + 1) if k < len(SYSCALLS) else ("rej", "not covered: syscall builtin")
        P("IT.esys" if k == 1 else "IT.w%d" % k).branch({1: "IT.v%d" % k}, nxt, [("CMPI", "sys", k)])
        P("IT.v%d" % k).a(("LDI", "w", w)).goto("IT.zf")
    P("IT.zf").branch({0: "IT.zf1"}, "IT.sd", [("CMP", "nar", "w")])
    P("IT.zf1").o("  imm r").num("nar").o(", 0\n").a(("ALUI", "add", "nar", "nar", 1)).goto("IT.zf")
    for k, (_, sc, w) in enumerate(SYSCALLS, 1):
        nxt = "IT.d%d" % (k + 1) if k < len(SYSCALLS) else ("rej", "not covered: syscall builtin")
        P("IT.sd" if k == 1 else "IT.d%d" % k).branch({1: "IT.x%d" % k}, nxt, [("CMPI", "sys", k)])
        regs = ", ".join("r%d" % i for i in range(w))
        P("IT.x%d" % k).o("  .sys%s %s, %s\n" % ("6" if w == 6 else "", sc, regs)).a(("LDI", "pt", 0), ("LDI", "pb", 0)).call("NEXT").ret()
    p = P("IT.ecall")
    p.o("  call ").a(("SPAN2", "sps", "spe")).o("\n").a(("INTERN", "v", "sps", "spe"), ("LDX", "pt", "v", FRD), ("LDX", "pb", "v", FRB))
    # a call's value is typed by the function's declared return type (FRD/FRB), not by its
    # last argument: g(v) / 2 is .div for long g(unsigned long)
    p.call("NEXT").ret()
    g.on("DEAD0", range(257), "DEAD", rej("not covered: identifier is not a local"), "r")


# builtin -> (.sys name, register width): the one declared table of syscall builtins
SYSCALLS = [("__write", "write", 3), ("__read", "read", 3), ("__open", "open", 3), ("__close", "close", 3),
            ("__lseek", "lseek", 3), ("__unlink", "unlink", 3), ("__rename", "rename", 3), ("__exit", "exit", 3),
            ("__mmap", "mmap", 6), ("__munmap", "munmap", 3)]


def printf():
    """printf("lit %d ...", e, ...) is expanded inline by the reference
    (measured): every argument is evaluated and spilled to a fresh slot of
    the current scope, then each literal segment is `.write`n from a pooled
    `.str Sk` and each %d is `.print`ed; the value is `imm r0, 0`.  Only %d
    and %% are in the slice; escapes n t r backslash dquote squote."""
    p = P("PF")
    p.call("NEXT").tok({TK_STR: "PF.s"}, ("rej", "not covered: printf format"))
    p = P("PF.s")
    p.vpush("ps", "pe").a(("COPYW", "pb", "vsp"), ("LDI", "na", 0)).call("NEXT")
    p.tok({",": "PF.arg", ")": "PF.go"}, ("rej", "not covered: printf format"))
    p = P("PF.arg")
    p.vpush("na", "pb").call("NEXT").call("EXPR").vpop("na", "pb")
    p.a(("ALUI", "add", "cur", "cur", 8), ("COPYW", "s", "cur"))
    up, nx = p.fresh("mx"), p.fresh("dn")
    p.branch({2: up}, nx, [("CMP", "cur", "max")])
    p.cur = up
    p.a(("COPYW", "max", "cur")).goto(nx)
    p.cur = nx
    p.o("  store64 [r6-").a(("COPYW", "n", "s")).call("PRN").o("], r0\n")
    p.vpush("s").a(("ALUI", "add", "na", "na", 1))
    p.tok({",": "PF.arg", ")": "PF.go"}, ("rej", "not covered: printf arguments"))
    p = P("PF.go")
    p.a(("ALUI", "sub", "t", "pb", 2), ("LDX", "fs", "t", VS), ("ALUI", "sub", "t", "pb", 1), ("LDX", "fe", "t", VS),
        ("ALUI", "add", "fs", "fs", 1), ("ALUI", "sub", "fe", "fe", 1), ("INPUSHXE", "fs", "fe"),
        ("LDI", "sl", 0), ("LDI", "ai", 0)).goto("PFW")
    g.on("PFW", [37], "PFW.pc", [("ADV",)])
    g.on("PFW", [92], "PFW.esc", [("ADV",)])
    g.on("PFW", [256], "PFW.e1", [])
    g.els("PFW", "PFW", [("ADV",), ("ALUI", "add", "sl", "sl", 1)])
    g.on("PFW.esc", [ord(c) for c in "ntr\\\"'"], "PFW", [("ADV",), ("ALUI", "add", "sl", "sl", 1)])
    g.els("PFW.esc", "DEAD", rej("not covered: escape in a printf format"))
    g.on("PFW.pc", [37], "PFW", [("ADV",), ("ALUI", "add", "sl", "sl", 1)])
    g.on("PFW.pc", [100], "PFW.d", [("ADV",)])
    g.els("PFW.pc", "DEAD", rej("not covered: printf conversion"))
    p = P("PFW.flush")      # a proc: write the pending literal segment
    p.branch({2: "PFW.f1"}, "RET", [("CMPI", "sl", 0)])
    p = P("PFW.f1")
    p.o("  .lea r0, S").num("sk").o("\n  imm r1, ").num("sl").o("\n  .write r0, r1\n")
    p.a(("ALUI", "add", "sk", "sk", 1), ("LDI", "sl", 0)).ret()
    p = P("PFW.d")
    p.call("PFW.flush")
    p.branch({0: "PFW.d1"}, ("rej", "not covered: printf arguments"), [("CMP", "ai", "na")])
    p = P("PFW.d1")
    p.a(("ALU", "add", "t", "pb", "ai"), ("LDX", "s", "t", VS), ("ALUI", "add", "ai", "ai", 1))
    p.o("  load64 r0, [r6-").a(("COPYW", "n", "s")).call("PRN").o("]\n  .print r0\n").goto("PFW")
    p = P("PFW.e1")
    p.call("PFW.flush")
    p.branch({1: "PFW.e2"}, ("rej", "not covered: printf arguments"), [("CMP", "ai", "na")])
    p = P("PFW.e2")
    p.a(("INPOP",), ("ALUI", "sub", "vsp", "pb", 2), ("LDI", "pt", 0), ("LDI", "pb", 0)).o("  imm r0, 0\n").call("NEXT").ret()

    # the pool: a third scan of x after the footer; every printf format's
    # literal segments again, in order, as `.str Sk "..\x00"`
    p = P("POOL")
    p.a(("JUMP", "x0"), ("LDI", "sk", 0)).call("NEXT").label("PO.loop")
    p.tok({"eof": "RET", TK_ID: "PO.id", TK_STR: "PO.lit"}, "PO.nx")
    P("PO.nx").call("NEXT").goto("PO.loop")
    p = P("PO.id")
    p.a(("INTERN", "v", "ps", "pe"), ("CMP", "v", "pfid"))
    p.branch({1: "PO.pf"}, "PO.nx")
    P("PO.pf").call("NEXT").tok({"(": "PO.par"}, "PO.loop")
    P("PO.par").call("NEXT").tok({TK_STR: "PO.s"}, "PO.loop")
    p = P("PO.s")
    p.a(("ALUI", "add", "fs", "ps", 1), ("ALUI", "sub", "fe", "pe", 1), ("INPUSHXE", "fs", "fe"), ("LDI", "st", 0))
    p.goto("PL")
    g.on("PL", [37], "PL.pc", [("ADV",)])
    g.on("PL", [92], "PL.esc", [("ADV",)])
    g.on("PL", [256], "PL.e1", [])
    g.els("PL", "PL.lit", [("BYTE", "c"), ("ADV",)])
    for ch, v in zip("ntr\\\"'", (10, 9, 13, 92, 34, 39)):
        g.on("PL.esc", [ord(ch)], "PL.lit", [("ADV",), ("LDI", "c", v)])
    g.els("PL.esc", "DEAD", rej("unreachable"))
    g.on("PL.pc", [37], "PL.lit", [("ADV",), ("LDI", "c", 37)])
    g.els("PL.pc", "PL.close", [("ADV",)])
    p = P("PL.lit")
    p.branch({1: "PL.hd"}, "PL.ch", [("CMPI", "st", 0)])
    p = P("PL.hd")
    p.o('.str S').num("sk").o(' "').a(("LDI", "st", 1)).goto("PL.ch")
    p = P("PL.ch")
    cases = {}
    for b in range(256):
        if 32 <= b < 127 and b not in (34, 92):
            t = chr(b)
        elif b in (34, 92):
            t = "\\" + chr(b)
        else:
            t = "\\x%02x" % b
        cases[b] = "PL.c%d" % b
        g.on("PL.c%d" % b, range(257), "PL", O(t), "r")
    p.branch(cases, ("rej", "unreachable"), [("RLD", "c")])
    # a string literal outside a printf format: the whole literal, escapes decoded, NUL-terminated
    p = P("PO.lit")
    p.a(("ALUI", "add", "fs", "ps", 1), ("ALUI", "sub", "fe", "pe", 1), ("INPUSHXE", "fs", "fe")).o('.str S').num("sk").o(' "').goto("PS")
    g.on("PS", [92], "PS.esc", [("ADV",)])
    g.on("PS", [256], "PS.e1", [])
    g.els("PS", "PS.ch", [("BYTE", "c"), ("ADV",)])
    for ch, v in zip("ntr\\\"'", (10, 9, 13, 92, 34, 39)):
        g.on("PS.esc", [ord(ch)], "PS.ch", [("ADV",), ("LDI", "c", v)])
    g.els("PS.esc", "DEAD", rej("not covered: escape in a string literal"))
    p = P("PS.ch")
    cases = {}
    for b in range(256):
        if 32 <= b < 127 and b not in (34, 92):
            t = chr(b)
        elif b in (34, 92):
            t = "\\" + chr(b)
        else:
            t = "\\x%02x" % b
        cases[b] = "PS.c%d" % b
        g.on("PS.c%d" % b, range(257), "PS", O(t), "r")
    p.branch(cases, ("rej", "unreachable"), [("RLD", "c")])
    P("PS.e1").o('\\x00"\n').a(("ALUI", "add", "sk", "sk", 1), ("INPOP",)).call("NEXT").goto("PO.loop")
    p = P("PL.close")
    p.branch({1: "PL.cl1"}, "PL", [("CMPI", "st", 1)])
    P("PL.cl1").o('\\x00"\n').a(("LDI", "st", 0), ("ALUI", "add", "sk", "sk", 1)).goto("PL")
    p = P("PL.e1")
    p.branch({1: "PL.e2"}, "PL.e3", [("CMPI", "st", 1)])
    P("PL.e2").o('\\x00"\n').a(("LDI", "st", 0), ("ALUI", "add", "sk", "sk", 1)).goto("PL.e3")
    P("PL.e3").a(("INPOP",)).call("NEXT").goto("PO.loop")


def stmt():
    p = P("STMT")
    p.tok({"{": "BLOCK", "type": "S.decl", "type=char": "S.decl", "type=long": "S.decl", "type=void": "S.decl",
           "type=unsigned": "S.decl", "type=short": "S.decl", "type=signed": "S.decl", TK_ID: "S.idq", ";": "S.empty", "return": "S.ret", "if": "S.if",
           "while": "S.while", "for": "S.for",
           "do": "S.do", "break": "S.brk", "continue": "S.cnt"}, "S.expr")
    P("S.empty").call("NEXT").ret()
    p = P("S.idq")        # a typedef name starts a declaration
    p.a(("INTERN", "v", "ps", "pe"), ("LDX", "t", "v", TDN)).branch({1: "S.tdd"}, "S.expr", [("CMPI", "t", 1)])
    P("S.tdd").a(("LDI", "bni", 1), ("LDI", "bsz", 0)).call("NEXT").goto("D.one")
    P("S.expr").call("VEXPR").expect(";").call("NEXT").ret()
    # block: '{' ... '}' with its own scope
    p = P("BLOCK")
    p.vpush("usp", "cur").call("NEXT").label("B.loop")
    p.tok({"}": "B.end", "eof": "B.eof"}, "B.st")
    g.on("B.eof", range(257), "DEAD", rej("not covered: unterminated block"), "r")
    P("B.st").call("STMT").goto("B.loop")
    p = P("B.end")
    p.vpop("sv", "sc")
    unwind(p, "sv")
    p.a(("COPYW", "cur", "sc")).call("NEXT").ret()
    # declaration
    p = P("S.decl")
    p.a(("LDI", "bni", 1), ("LDI", "bsz", 0)).tok({"type": "S.dint", "type=char": "S.dch", "type=long": "S.dlg", "type=short": "S.dsh",
                                                   "type=unsigned": "S.dun"}, "S.dnx")
    P("S.dun").call("NEXT").tok({"type=char": "S.duc", "type=short": "S.dus", "type=long": "S.dul"}, ("rej", "not covered: unsigned int declaration"))
    P("S.dul").a(("LDI", "bni", 0), ("LDI", "bsz", UNS + SZ["long"])).goto("S.dnx")
    P("S.duc").a(("LDI", "bni", 0), ("LDI", "bsz", UNS + SZ["char"])).goto("S.dnx")
    P("S.dus").a(("LDI", "bni", 0), ("LDI", "bsz", UNS + SZ["short"])).goto("S.dnx")
    P("S.dint").a(("LDI", "bni", 0), ("LDI", "bsz", SZ["int"])).goto("S.dnx")
    P("S.dch").a(("LDI", "bni", 0), ("LDI", "bsz", SZ["char"])).goto("S.dnx")
    P("S.dlg").a(("LDI", "bni", 0), ("LDI", "bsz", SZ["long"])).goto("S.dnx")
    P("S.dsh").a(("LDI", "bni", 0), ("LDI", "bsz", SZ["short"])).goto("S.dnx")
    P("S.dnx").call("NEXT").tok(dict((w, "S.dw") for w in TWORDS), "D.one")
    P("S.dw").a(("LDI", "bni", 1), ("LDI", "bsz", 0)).goto("S.dnx")
    p = P("D.one")
    stars(p, "D.id")
    p = P("D.id")     # T x | T x[N]: an array takes N * (element size) bytes, packed (measured: char c[5]; int y; -> c at 5, y at 13)
    p.a(("COPYW", "dps", "ps"), ("COPYW", "dpe", "pe"), ("LDI", "dsz", 8), ("LDI", "dar", 0)).call("NEXT").tok({"[": "D.arr"}, "D.decl")
    p = P("D.decl")
    declare(p, "dps", "dpe")
    p.tok({"=": "D.init"}, "D.next")
    p = P("D.arr")
    p.call("NEXT").tok({TK_NUM: "D.an"}, ("rej", "not covered: array bound"))
    p = P("D.an")
    p.a(("COPYW", "dn", "nv")).call("NEXT").expect("]").call("NEXT")
    ep, e1 = p.fresh("ep"), p.fresh("e1")
    p.branch({(1, 2): ep}, e1, [("CMPI", "ptd", 1)])
    P(ep).a(("LDI", "es", PSZ)).goto("D.asz")
    q = P(e1)
    for n in sorted(set(SZ.values())):
        for b in ((n, UNS + n) if n in (1, 2, 8) else (n,)):
            hit, nx = q.fresh("es"), q.fresh("en")
            q.branch({1: hit}, nx, [("CMPI", "bsz", b)])
            P(hit).a(("LDI", "es", n)).goto("D.asz")
            q = P(nx)
    q.goto("D.abad")
    g.on("D.abad", range(257), "DEAD", rej("not covered: array of an unknown element type"), "r")
    p = P("D.asz")
    p.a(("ALU", "mul", "dsz", "dn", "es"), ("LDI", "dar", 1), ("ALUI", "add", "ptd", "ptd", 1)).call("D.decl1")
    p.a(("ALUI", "sub", "ptd", "ptd", 1)).tok({"=": "D.ainit"}, "D.next")
    g.on("D.ainit", range(257), "DEAD", rej("not covered: array initialiser"), "r")
    p = P("D.decl1")
    declare(p, "dps", "dpe")
    p.ret()
    p = P("D.init")
    p.vpush("s", "ptd", "bni", "bsz").call("NEXT").call("EXPR").vpop("s", "ptd", "bni", "bsz")
    addr(p, "r1")
    vwidth(p, "ptd", "bsz", ST)
    p.goto("D.next")
    p = P("D.next")
    p.tok({",": "D.comma", ";": "S.empty"}, ("rej", "not covered: declaration"))
    P("D.comma").call("NEXT").goto("D.one")
    # return e;
    p = P("S.ret")
    p.call("NEXT").tok({";": "S.retv"}, "S.rete")
    p = P("S.retv")
    p.o("  jump R").num("rl").o("\n").call("NEXT").ret()
    # break; continue; -- jump to the innermost loop's labels (0: none)
    for nm, sl in (("S.brk", "brk"), ("S.cnt", "cnt")):
        p = P(nm)
        p.branch({1: "S.noloop"}, nm + "1", [("CMPI", sl, 0)])
        p = P(nm + "1")
        p.o("  jump ").lab(sl).o("\n").call("NEXT").expect(";").call("NEXT").ret()
    g.on("S.noloop", range(257), "DEAD", rej("not covered: break/continue outside a loop"), "r")
    # do s while (e);  labels: a top, b break, c continue
    p = P("S.do")
    p.vpush("brk", "cnt").newlab("a").newlab("b").newlab("c").a(("COPYW", "brk", "b"), ("COPYW", "cnt", "c"))
    p.lab("a").o(":\n").vpush("a", "b", "c").call("NEXT").call("STMT").vpop("a", "b", "c")
    p.lab("c").o(":\n").vpush("a", "b", "c").expect("while").call("NEXT").expect("(").call("NEXT").call("CEXPR")
    p.expect(")").call("NEXT").expect(";").vpop("a", "b", "c")
    p.o("  jumpz r0, ").lab("b").o("\n  jump ").lab("a").o("\n").lab("b").o(":\n").vpop("brk", "cnt")
    p.call("NEXT").ret()
    p = P("S.rete")
    p.call("CEXPR").expect(";")
    p.branch({(1, 2): "S.retp"}, "S.reti", [("CMPI", "rptr", 1)])
    P("S.retp").o("  jump R").num("rl").o("\n").call("NEXT").ret()
    p = P("S.reti")     # a scalar return narrows through the stack at its tyinfo size; 8 (long) as a pointer (measured)
    q = p
    for n in sorted(set(SZ.values()) - {SZ["int"]}):
        hit, nx = q.fresh("rs"), q.fresh("rn")
        q.branch({1: "S.retp" if n == PSZ else hit}, nx, [("CMPI", "rbsz", n)])
        if n != PSZ:
            P(hit).o("  .frame 8\n  .st [r7+0], r0, %d\n  .ld r0, [r7+0], %d\n  .frame -8\n  jump R" % (n, n)).num("rl").o("\n").call("NEXT").ret()
        q = P(nx)
    q.o("  .frame 8\n  .st [r7+0], r0, 4\n  .ld r0, [r7+0], 4\n  .frame -8\n  jump R").num("rl").o("\n")
    q.call("NEXT").ret()
    # if (e) s [else s]
    p = P("S.if")
    p.call("NEXT").expect("(").call("NEXT").call("CEXPR").expect(")")
    p.newlab("a").o("  jumpz r0, ").lab("a").o("\n").vpush("a").call("NEXT").call("STMT").vpop("a")
    p.tok({"else": "IF.else"}, "IF.end")
    P("IF.end").lab("a").o(":\n").ret()
    p = P("IF.else")
    p.newlab("b").o("  jump ").lab("b").o("\n").lab("a").o(":\n").vpush("b").call("NEXT").call("STMT")
    p.vpop("b").lab("b").o(":\n").ret()
    # while (e) s
    p = P("S.while")
    p.vpush("brk", "cnt").newlab("a").newlab("b").a(("COPYW", "brk", "b"), ("COPYW", "cnt", "a")).lab("a").o(":\n").vpush("a", "b")
    p.call("NEXT").expect("(").call("NEXT").call("CEXPR").expect(")")
    p.vpop("a", "b").o("  jumpz r0, ").lab("b").o("\n").vpush("a", "b").call("NEXT").call("STMT")
    p.vpop("a", "b").o("  jump ").lab("a").o("\n").lab("b").o(":\n").vpop("brk", "cnt").ret()
    # for (e; e; e) s -- the step is parsed after the body: skip it, run the
    # body, JUMP the reader back to it, then JUMP forward past the body
    p = P("S.for")
    p.vpush("brk", "cnt").call("NEXT").expect("(").call("NEXT").tok({";": "F.no", "type": "F.no"}, "F.init")
    g.on("F.no", range(257), "DEAD", rej("not covered: for clause"), "r")
    p = P("F.init")
    p.call("VEXPR").expect(";").newlab("a").newlab("b").newlab("c").a(("COPYW", "brk", "b"), ("COPYW", "cnt", "c")).lab("a").o(":\n")
    p.vpush("a", "b", "c").call("NEXT").tok({";": "F.no"}, "F.cond")
    p = P("F.cond")
    p.call("CEXPR").expect(";").vpop("a", "b", "c").o("  jumpz r0, ").lab("b").o("\n").vpush("a", "b", "c")
    p.call("NEXT").tok({")": "F.no"}, "F.step")
    p = P("F.step")
    p.a(("COPYW", "sp", "tpos"), ("LDI", "dp", 0))
    p.label("F.skip").tok({"(": "F.open", ")": "F.close", "eof": "F.no"}, "F.sk1")
    P("F.sk1").call("NEXT").goto("F.skip")
    P("F.open").a(("ALUI", "add", "dp", "dp", 1)).call("NEXT").goto("F.skip")
    p = P("F.close")
    p.branch({1: "F.body"}, "F.cl1", [("CMPI", "dp", 0)])
    P("F.cl1").a(("ALUI", "sub", "dp", "dp", 1)).call("NEXT").goto("F.skip")
    p = P("F.body")
    p.vpush("sp").call("NEXT").call("STMT").vpop("sp").a(("COPYW", "ep", "tpos"), ("JUMP", "sp"))
    p.vpush("ep").call("NEXT").vpop("ep").vpop("a", "b", "c").lab("c").o(":\n").vpush("a", "b", "c", "ep")
    p.call("VEXPR").expect(")").vpop("a", "b", "c", "ep").o("  jump ").lab("a").o("\n").lab("b").o(":\n")
    p.a(("JUMP", "ep")).vpop("brk", "cnt").call("NEXT").ret()


def spec():
    """SPEC: a run of type words (int char short long unsigned signed) -> W[bni] 0, W[bsz]
    (tyinfo size; UNS + size for unsigned char/short).  Anything the slice cannot
    type exactly is rejected: unsigned int/long, void mixed in, char/short twice."""
    p = P("SPEC")
    p.a(("LDI", "sz", 0), ("LDI", "un", 0)).label("SP.loop")
    p.tok({"type": "SP.int", "type=char": "SP.ch", "type=short": "SP.sh", "type=long": "SP.lg",
           "type=unsigned": "SP.un", "type=signed": "SP.sg"}, "SP.end")
    bad = ("rej", "not covered: type specifier")
    P("SP.int").branch({1: "SP.i4"}, "SP.nx", [("CMPI", "sz", 0)])
    P("SP.i4").a(("LDI", "sz", 4)).goto("SP.nx")
    P("SP.ch").branch({1: "SP.c1"}, bad, [("CMPI", "sz", 0)])
    P("SP.c1").a(("LDI", "sz", 1)).goto("SP.nx")
    P("SP.sh").branch({(0, 1): "SP.s2"}, bad, [("CMPI", "sz", 4)])   # sz 0 or 4
    P("SP.s2").branch({1: "SP.shx"}, "SP.s3", [("CMPI", "sz", 1)])
    g.on("SP.shx", range(257), "DEAD", rej("not covered: type specifier"), "r")
    P("SP.s3").a(("LDI", "sz", 2)).goto("SP.nx")
    P("SP.lg").branch({1: "SP.shx"}, "SP.l1", [("CMPI", "sz", 1)])
    P("SP.l1").branch({1: "SP.shx"}, "SP.l8", [("CMPI", "sz", 2)])
    P("SP.l8").a(("LDI", "sz", 8)).goto("SP.nx")
    P("SP.un").a(("LDI", "un", 1)).goto("SP.nx")
    P("SP.sg").goto("SP.nx")
    P("SP.nx").call("NEXT").goto("SP.loop")
    p = P("SP.end")
    p.a(("LDI", "bni", 0)).branch({1: "SP.u"}, "SP.s", [("CMPI", "un", 1)])
    P("SP.s").branch({1: "SP.s0"}, "SP.sk", [("CMPI", "sz", 0)])
    P("SP.s0").a(("LDI", "bsz", SZ["int"])).ret()
    P("SP.sk").a(("COPYW", "bsz", "sz")).ret()
    P("SP.u").branch({1: "SP.u1"}, "SP.u2", [("CMPI", "sz", 1)])
    P("SP.u1").a(("LDI", "bsz", UNS + 1)).ret()
    P("SP.u2").branch({1: "SP.u3"}, "SP.u4", [("CMPI", "sz", 2)])
    P("SP.u4").branch({1: "SP.u8"}, ("rej", "not covered: unsigned int"), [("CMPI", "sz", 8)])
    P("SP.u8").a(("LDI", "bsz", UNS + 8)).ret()
    P("SP.u3").a(("LDI", "bsz", UNS + 2)).ret()


def unit():
    p = P("START")
    p.a(("LDI", "x0", 0), ("LDI", "pass", 1), ("SBCLR",), [("SBOUT", c) for c in b"main"], ("SBINTERN", "mainid"),
        ("SBCLR",), [("SBOUT", c) for c in b"printf"], ("SBINTERN", "pfid"),
        [x for k, (nm, _, _) in enumerate(SYSCALLS, 1)
         for x in [("SBCLR",)] + [("SBOUT", c) for c in nm.encode()] + [("SBINTERN", "sy%d" % k)]])
    p.label("PASS").a(("JUMP", "x0"), ("LDI", "lab", 0), ("LDI", "fn", 0), ("LDI", "usp", 0),
                      ("LDI", "vsp", 0), ("LDI", "sk", 0), ("LDI", "brk", 0), ("LDI", "cnt", 0)).o(HEADER).call("NEXT")
    p.label("TOP").tok({"type": "FN", "type=void": "FN", "type=char": "FN", "type=long": "FN", "type=short": "FN", "type=static": "TOP.st", "typedef": "TD", "eof": "END"}, ("rej", "not covered: top-level construct"))
    # typedef <type words | struct TAG> *... NAME;  -- no code; NAME recorded in TDN
    TW = {"type": "TD.w", "type=void": "TD.w", "type=long": "TD.w", "type=char": "TD.w",
          "type=unsigned": "TD.w", "type=short": "TD.w", "type=signed": "TD.w"}
    P("TD").call("NEXT").tok(dict(TW, struct="TD.s"), ("rej", "not covered: typedef"))
    P("TD.s").call("NEXT").tok({TK_ID: "TD.w"}, ("rej", "not covered: typedef"))
    P("TD.w").call("NEXT").tok({**TW, "*": "TD.w", TK_ID: "TD.id"}, ("rej", "not covered: typedef"))
    p = P("TD.id")
    p.a(("INTERN", "v", "ps", "pe"), ("LDI", "t", 1), ("STX", "v", TDN, "t")).call("NEXT").expect(";").call("NEXT").goto("TOP")
    P("TOP.st").call("NEXT").tok({"type": "FN", "type=void": "FN", "type=char": "FN", "type=long": "FN", "type=short": "FN", TK_ID: "TOP.sid"}, ("rej", "not covered: static declaration"))
    p = P("TOP.sid")   # static TYPEDEFNAME ...: base size unknown (CUNK)
    p.a(("INTERN", "v", "ps", "pe"), ("LDX", "t", "v", TDN)).branch({1: "TOP.std"}, ("rej", "not covered: static declaration"), [("CMPI", "t", 1)])
    P("TOP.std").a(("LDI", "bni", 0), ("LDI", "bsz", CUNK)).goto("FN.n")
    p = P("FN")
    p.a(("LDI", "bni", 1), ("LDI", "bsz", 0)).tok({"type": "FN.i", "type=void": "FN.i", "type=char": "FN.c", "type=long": "FN.l", "type=short": "FN.s"}, "FN.n")
    P("FN.i").a(("LDI", "bni", 0)).tok({"type": "FN.i4"}, "FN.n")
    P("FN.i4").a(("LDI", "bsz", SZ["int"])).goto("FN.n")
    P("FN.c").a(("LDI", "bni", 0), ("LDI", "bsz", SZ["char"])).goto("FN.n")
    P("FN.l").a(("LDI", "bni", 0), ("LDI", "bsz", SZ["long"])).goto("FN.n")
    P("FN.s").a(("LDI", "bni", 0), ("LDI", "bsz", SZ["short"])).goto("FN.n")
    p = P("FN.n")
    p.call("NEXT")
    stars(p, "FN.r")
    P("FN.r").a(("COPYW", "rptr", "ptd"), ("COPYW", "rbsz", "bsz")).goto("FN.id")
    p = P("FN.id")
    p.a(("INTERN", "v", "ps", "pe"), ("STX", "v", FND, "pass"), ("COPYW", "fps", "ps"), ("COPYW", "fpe", "pe"),
        ("COPYW", "fv", "v"), ("STX", "v", FRD, "rptr"), ("STX", "v", FRB, "rbsz"), ("LDI", "cur", 0), ("LDI", "max", 0))
    p.call("NEXT").tok({"(": "FN.open", ";": "FN.gv", "=": "FN.gv", ",": "FN.gv"}, ("rej", "not covered: declarator"))
    P("FN.gv").branch({(1, 2): "FN.gp"}, "GV", [("CMPI", "rptr", 1)])
    g.on("FN.gp", range(257), "DEAD", rej("not covered: global pointer"), "r")
    # file-scope int: `.bss g_NAME 4` where declared; `= literal` goes to __init
    p = P("GV")
    # file-scope char/short/long: `.bss g_NAME <tyinfo size>` (measured); BASE[v] carries the width
    p.o(".bss g_").a(("SPAN2", "fps", "fpe"), ("LDI", "t", GMARK), ("STX", "v", LOC, "t"), ("LDI", "z0", 0), ("STX", "v", FND, "z0"), ("STX", "v", PTR, "z0"),
                     ("STX", "v", BASE, "rbsz"))
    q = p
    for n in [0] + sorted(set(SZ.values())):   # 0: plain `int` leaves bsz 0 (the int default)
        hit, nx = q.fresh("gz"), q.fresh("gn")
        q.branch({1: hit}, nx, [("CMPI", "rbsz", n)])
        P(hit).o(" %d\n" % (n or SZ["int"])).goto("GV.sz")
        q = P(nx)
    g.on(q.cur, range(257), "DEAD", rej("not covered: global of an unknown type"), "r")
    p = P("GV.sz")
    p.tok({"=": "GV.eq4"}, "GV.nx")
    P("GV.eq4").branch({1: "GV.eq"}, "GV.eq0", [("CMPI", "rbsz", SZ["int"])])
    P("GV.eq0").branch({1: "GV.eq"}, ("rej", "not covered: non-int global initialiser"), [("CMPI", "rbsz", 0)])
    P("GV.eq").call("NEXT").tok({TK_NUM: "GV.num"}, ("rej", "not covered: global initialiser"))
    P("GV.num").call("NEXT").goto("GV.nx")
    p = P("GV.nx")
    p.tok({";": "GV.end", ",": "GV.comma"}, ("rej", "not covered: global declaration"))
    P("GV.end").call("NEXT").goto("TOP")
    P("GV.comma").call("NEXT").tok({TK_ID: "GV.id"}, ("rej", "not covered: declarator"))
    P("GV.id").a(("INTERN", "v", "ps", "pe"), ("COPYW", "fps", "ps"), ("COPYW", "fpe", "pe")).call("NEXT").goto("GV")
    p = P("FN.open")
    p.call("NEXT").tok({")": "FN.close"}, "FN.par")
    p = P("FN.par")
    p.tok({TK_ID: "FN.ptd", "type=void": "FN.pv"}, "FN.psp")
    P("FN.psp").call("SPEC").goto("FN.pst")
    p = P("FN.ptd")
    p.a(("LDI", "bni", 1), ("LDI", "bsz", 0), ("INTERN", "v", "ps", "pe"), ("LDX", "t", "v", TDN))
    p.branch({1: "FN.pt"}, ("rej", "not covered: parameter"), [("CMPI", "t", 1)])
    P("FN.pt").call("NEXT").goto("FN.pst")
    P("FN.pv").a(("LDI", "bni", 1), ("LDI", "bsz", 0)).call("NEXT").tok({")": "FN.close", "*": "FN.pst"}, ("rej", "not covered: parameter"))
    p = P("FN.pst")
    stars(p, "FN.pid")
    p = P("FN.pid")
    p.a(("LDI", "dsz", 8), ("LDI", "dar", 0))
    declare(p)
    p.call("NEXT").tok({",": "FN.comma", ")": "FN.close"}, ("rej", "not covered: parameter list"))
    P("FN.comma").call("NEXT").goto("FN.par")
    p = P("FN.close")
    p.branch({2: "FN.many"}, "FN.hd", [("CMPI", "cur", 48)])
    g.on("FN.many", range(257), "DEAD", rej("not covered: more than 6 parameters"), "r")
    p = P("FN.hd")
    p.a(("ALUI", "div", "np", "cur", 8)).call("NEXT").tok({"{": "FN.body", ";": "FN.proto"}, ("rej", "not covered: declaration"))
    # prototype NAME(...);  -- no code; the name is not a definition
    p = P("FN.proto")
    p.a(("LDI", "z0", 0), ("STX", "fv", FND, "z0"), ("LDI", "z", 0))
    unwind(p, "z")
    p.call("NEXT").goto("TOP")
    p = P("FN.body")
    p.newlab("rl").a(("SPAN2", "fps", "fpe")).o(":\n  .frame 8\n  store64 [r7+0], r6\n  mov r6, r7\n  .frame ")
    p.branch({1: "FN.f2"}, "FN.f1", [("CMPI", "pass", 2)])
    P("FN.f1").a(("LDI", "n", 0)).goto("FN.fw")
    P("FN.f2").a(("LDX", "n", "fn", FR)).goto("FN.fw")
    p = P("FN.fw")
    p.call("PRNW").o("\n").a(("LDI", "k2", 0)).label("FN.st")
    p.branch({0: "FN.st1"}, "FN.go", [("CMP", "k2", "np")])
    p = P("FN.st1")
    p.a(("ALUI", "add", "k2", "k2", 1), ("ALUI", "mul", "n", "k2", 8)).o("  store64 [r6-").call("PRN")
    p.o("], r").a(("ALUI", "sub", "n", "k2", 1)).call("PRN").o("\n").goto("FN.st")
    p = P("FN.go")
    p.call("BLOCK").o("R").num("rl").o(":\n  mov r7, r6\n  load64 r6, [r7+0]\n  .frame -8\n  ret\n")
    p.a(("ALUI", "add", "t", "max", 7), ("ALUI", "div", "t", "t", 8), ("ALUI", "mul", "t", "t", 8), ("STX", "fn", FR, "t"), ("ALUI", "add", "fn", "fn", 1), ("LDI", "z", 0))
    unwind(p, "z")
    p.a(("CMP", "fv", "mainid"))
    p.branch({1: "FN.main"}, "TOP")
    P("FN.main").a(("COPYW", "hasmain", "pass")).goto("TOP")
    p = P("END")
    p.branch({1: "END2"}, "END1", [("CMPI", "pass", 2)])
    P("END1").a(("OCLR",), ("LDI", "pass", 2)).goto("PASS")
    p = P("END2")
    p.branch({1: "END3"}, ("rej", "undefined function 'main'"), [("CMPI", "hasmain", 2)])
    P("END3").o("__init:\n").call("INITS").o(FOOTER[len("__init:\n"):]).call("POOL").a(("ACCEPT",)).goto("DEAD")


def inits():
    """__init body: a scan of x for `id = num` at brace depth 0 (only a
    global initialiser has that shape at file scope), in order."""
    p = P("INITS")
    p.a(("JUMP", "x0"), ("LDI", "dp", 0)).call("NEXT").label("IN.loop")
    p.tok({"eof": "RET", "{": "IN.o", "}": "IN.c", TK_ID: "IN.id"}, "IN.nx")
    P("IN.nx").call("NEXT").goto("IN.loop")
    P("IN.o").a(("ALUI", "add", "dp", "dp", 1)).goto("IN.nx")
    P("IN.c").a(("ALUI", "sub", "dp", "dp", 1)).goto("IN.nx")
    p = P("IN.id")
    p.branch({1: "IN.id0"}, "IN.nx", [("CMPI", "dp", 0)])
    p = P("IN.id0")
    p.a(("COPYW", "gs", "ps"), ("COPYW", "ge", "pe")).call("NEXT").tok({"=": "IN.eq"}, "IN.loop")
    P("IN.eq").call("NEXT").tok({TK_NUM: "IN.num"}, "IN.loop")
    p = P("IN.num")
    p.o("  imm r0, ").call("NUMOUT").o("\n  .lea r1, g_").a(("SPAN2", "gs", "ge"))
    p.o("\n  .st [r1+0], r0, 4\n").goto("IN.nx")


def build():
    tokenizer()
    prn()
    numout()
    expr()
    stmt()
    spec()
    printf()
    inits()
    unit()
    g.finish()
    states = {n: [m, {str(k): v for k, v in row.items()}] for n, (m, row) in g.st.items()}
    return {"start": "START", "states": states, "seqs": [list(map(list, s)) for s in g.seqs]}


def sizes(d):
    unr = [i for i, s in enumerate(d["seqs"]) if s == [["REJECT", "unreachable"]]]
    ent = sum(len(r) for _, r in d["states"].values())
    live = sum(1 for _, r in d["states"].values() for v in r.values() if v[1] not in unr)
    return len(d["states"]), ent, live, len(d["seqs"]), sum(len(a) for a in d["seqs"])


if __name__ == "__main__":
    d = build()
    out = sys.argv[1] if len(sys.argv) > 1 else "/dev/stdout"
    s = json.dumps(d, separators=(",", ":"))
    open(out, "w").write(s)
    st, ent, live, ns, na = sizes(d)
    sys.stderr.write("states %d  entries %d (not 'unreachable' %d)  action seqs %d (%d actions)  json %d B\n"
                     % (st, ent, live, ns, na, len(s)))
