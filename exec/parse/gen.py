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
BINSEL = {f[0]: f[2] for f in gold("binsel") if f[1] == "s"}
IRSEL = {f[1]: f[2] for f in gold("irsel") if f[0] == "alu"}


def optext(op):
    sp = IRSEL[BINSEL[op]]
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
         "typedef", "struct", "type=long", "type=char", "type=unsigned", "type=short", "type=signed"]
TK = {w: k + 1 for k, w in enumerate(WORDS)}
TK["type"] = TK["type=int"]   # x is the UA_TYPESPELL dump: every other spelling is TK_OTHER
TK_ID, TK_NUM, TK_BADNUM, TK_OTHER, TK_STR = 100, 101, 102, 103, 104
CASOPS = ("+", "-", "*", "/", "%", "<<", ">>", "&", "^", "|")
GMARK = 900000   # LOC[v] of a file-scope int (shadowed/restored like any local)
LOC, FND, UNDO, FR, DIG, VS = 10 ** 6, 2 * 10 ** 6, 3 * 10 ** 6, 5 * 10 ** 6, 6 * 10 ** 6, 7 * 10 ** 6
TDN = 8 * 10 ** 6  # TDN[v] = 1: v was declared a typedef name at file scope
PTR = 9 * 10 ** 6  # PTR[v] = 1: the visible v is a pointer (8 bytes: load64/store64)
BASE = 11 * 10 ** 6  # BASE[v]: size of v's base type (int 4, char 1, long 8; 0 unknown), the scale of depth-1 +-
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
    for w in WORDS + ["id=", "num=", "str="]:
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
        if p in TK:
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
    # decimal, no suffix, no leading 0 unless the literal is 0, <= 9 digits
    g.on("SPANNUM", [48], "NUM0", [("ADV",)])
    g.on("SPANNUM", range(49, 58), "NUMD", [("ADV",)])
    g.els("SPANNUM", "SKIPO", [("LDI", "tk", TK_BADNUM)])
    g.on("NUM0", [10], "RET", [("MARK", "pe"), ("ADV",), ("LDI", "tk", TK_NUM)])
    g.els("NUM0", "SKIPO", [("LDI", "tk", TK_BADNUM)])
    g.on("NUMD", range(48, 58), "NUMD", [("ADV",)])
    g.on("NUMD", [10], "NUMLEN", [("MARK", "pe"), ("ADV",), ("ALU", "sub", "t", "pe", "ps"), ("CMPI", "t", 10)])
    g.els("NUMD", "SKIPO", [("LDI", "tk", TK_BADNUM)])
    g.r("NUMLEN", {0: ("RET", [("LDI", "tk", TK_NUM)]), (1, 2): ("RET", [("LDI", "tk", TK_BADNUM)])})


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


def addr(p, reg):             # address of local slot W[s] (or global x[gs..ge)) into reg
    gl, lc, dn = p.fresh("ga"), p.fresh("la"), p.fresh("ad")
    p.branch({1: gl}, lc, [("CMPI", "s", GMARK)])
    p.cur = gl
    p.o("  .lea %s, g_" % reg).a(("SPAN2", "gs", "ge")).o("\n").goto(dn)
    p.cur = lc
    p.o("  imm r2, ").a(("ALUI", "mul", "n", "s", 8)).call("PRN").o("\n  sub64 %s, r6, r2\n" % reg).goto(dn)
    p.cur = dn


def lookup(p, lo, hi):        # s := slot of the local spelled x[W[lo]..W[hi])
    p.a(("INTERN", "v", lo, hi), ("LDX", "s", "v", LOC), ("LDX", "pt", "v", PTR), ("LDX", "pb", "v", BASE), ("COPYW", "gs", lo), ("COPYW", "ge", hi))
    ok = p.fresh("ok")
    p.branch({1: "DEAD0"}, ok, [("CMPI", "s", 0)])
    p.cur = ok


def declare(p):               # declare x[ps..pe) as a new local (pointer iff W[ptd]); slot in W[s]
    p.a(("INTERN", "v", "ps", "pe"), ("LDX", "o", "v", LOC), ("LDX", "op", "v", PTR),
        ("STX", "usp", UNDO, "v"), ("STX", "usp", UNDO + 1, "o"), ("STX", "usp", UNDO + 2, "op"),
        ("LDX", "ob", "v", BASE), ("STX", "usp", UNDO + 3, "ob"), ("ALUI", "add", "usp", "usp", 4),
        ("ALUI", "add", "cur", "cur", 1), ("STX", "v", LOC, "cur"), ("STX", "v", PTR, "ptd"), ("STX", "v", BASE, "bsz"), ("COPYW", "s", "cur"))
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
    p.a(("ALUI", "sub", "usp", "usp", 4), ("LDX", "v", "usp", UNDO), ("LDX", "o", "usp", UNDO + 1),
        ("LDX", "op", "usp", UNDO + 2), ("LDX", "ob", "usp", UNDO + 3), ("STX", "v", LOC, "o"), ("STX", "v", PTR, "op"),
        ("STX", "v", BASE, "ob")).goto(top)
    p.cur = done


def width(p, ptr, i4, i8):    # emit i8 if W[ptr] else i4
    a, b, d = p.fresh("w8"), p.fresh("w4"), p.fresh("wd")
    p.branch({(1, 2): a}, b, [("CMPI", ptr, 1)])
    P(a).o(i8).goto(d)
    P(b).o(i4).goto(d)
    p.cur = d


def vwidth(p, ptr, bs, i4, i8, i1):   # a variable: W[ptr] >= 1 -> i8; else by W[bs]: 1 -> i1, 8 -> i8, else i4 (measured)
    a, c, e, b, d = p.fresh("v8"), p.fresh("vc"), p.fresh("vl"), p.fresh("v4"), p.fresh("vd")
    p.branch({(1, 2): a}, c, [("CMPI", ptr, 1)])
    P(c).branch({1: e}, b, [("CMPI", bs, 1)])
    P(b).branch({1: a}, d + "4", [("CMPI", bs, 8)])
    P(a).o(i8).goto(d)
    P(e).o(i1).goto(d)
    P(d + "4").o(i4).goto(d)
    p.cur = d


LD = ("  .ld r0, [r0+0], 4\n", "  load64 r0, [r0+0]\n", "  .ld r0, [r0+0], 1\n")
ST = ("  .st [r1+0], r0, 4\n", "  store64 [r1+0], r0\n", "  .st [r1+0], r0, 1\n")


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


def expr():
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
                q.o(NORM).vpop("a").lab("a").o(":\n").goto("LOOP%d" % L)
            elif op == "||":
                q.newlab("a").newlab("b").vpush("a")
                q.o("  jumpz r0, ").lab("b").o("\n  imm r0, 1\n  jump ").lab("a").o("\n").lab("b").o(":\n")
                q.call("NEXT").call(sub).o(NORM).vpop("a").lab("a").o(":\n").a(("LDI", "pt", 0)).goto("LOOP%d" % L)
            elif op in ("+", "-"):   # p +- n: n scaled by 8 at depth >= 2, by BASE (4/1/8) at depth 1
                ok, pp, p1 = q.fresh("pa"), q.fresh("pp"), q.fresh("p1")    # (measured); unknown base -> not covered
                sc = {4: q.fresh("s4"), 1: q.fresh("s1"), 8: q.fresh("s8")}
                s1b, s1c = q.fresh("sb"), q.fresh("sc")
                q.branch({2: pp, 1: p1}, ok, [("CMPI", "pt", 1)])
                P(p1).branch({1: sc[4]}, s1b, [("CMPI", "pb", 4)])
                P(s1b).branch({1: sc[1]}, s1c, [("CMPI", "pb", 1)])
                P(s1c).branch({1: sc[8]}, "DEADP", [("CMPI", "pb", 8)])
                for k in sc:
                    r = P(sc[k])
                    r.vpush("pt", "pb").o(PUSH).call("NEXT").call(sub).call("NOPTR").vpop("pt", "pb")
                    r.o(("" if k == 1 else "  imm r2, %d\n  mul64 r0, r0, r2\n" % k) + POP1 + optext(op)).goto("LOOP%d" % L)
                P(ok).o(PUSH).call("NEXT").call(sub).call("NOPTR").o(POP1 + optext(op)).a(("LDI", "pt", 0)).goto("LOOP%d" % L)
                r = P(pp)
                r.vpush("pt").o(PUSH).call("NEXT").call(sub).call("NOPTR").vpop("pt")
                r.o("  imm r2, 8\n  mul64 r0, r0, r2\n" + POP1 + optext(op)).goto("LOOP%d" % L)
            else:     # a pointer operand is not covered: the reference scales it
                noptr(q)
                q.o(PUSH).call("NEXT").call(sub)
                noptr(q)
                q.o(POP1 + optext(op)).a(("LDI", "pt", 0)).goto("LOOP%d" % L)
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
    p.tok(dict([("=", "EXPR.as")] + [(o + "=", "EXPR.c" + o) for o in CASOPS]), "EXPR.use")
    for o in CASOPS:     # a op= e: address, load, push, e, op, store (measured)
        q = P("EXPR.c" + o)
        lookup(q, "sps", "spe")
        noptr(q)
        addr(q, "r0")
        q.o(PUSH)
        vwidth(q, "pt", "pb", *LD)
        q.o(PUSH).vpush("pb").call("NEXT").call("EXPR").vpop("pb")
        q.o(POP1 + optext(o) + POP1)
        vwidth(q, "pt", "pb", *ST)
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
    p.a(("ALUI", "sub", "pt", "pt", 1)).o(PUSH).vpush("pt").call("NEXT").call("EXPR").vpop("pt").o(POP1)
    width(p, "pt", "  .st [r1+0], r0, 4\n", "  store64 [r1+0], r0\n")
    p.ret()
    P("EXPR.stu").call("DEREF").call("BINCONT").goto("EXPR.tail")
    # PV: '*' PV | '&' id | id  -> r0 = the pointer value, W[pt] = its depth
    p = P("PV")
    p.tok({"*": "PV.st", "&": "PV.amp", TK_ID: "PV.id"}, ("rej", "not covered: operand of *"))
    P("PV.st").call("NEXT").call("PV").call("PVCHK").call("DEREF").ret()
    p = P("PV.amp")
    p.call("NEXT").tok({TK_ID: "PV.amq"}, ("rej", "not covered: operand of &"))
    p = P("PV.amq")
    lookup(p, "ps", "pe")
    addr(p, "r0")
    p.a(("ALUI", "add", "pt", "pt", 1)).call("NEXT").call("NOPOST").ret()
    p = P("PV.id")
    lookup(p, "ps", "pe")
    addr(p, "r0")
    width(p, "pt", "  .ld r0, [r0+0], 4\n", "  load64 r0, [r0+0]\n")
    p.call("NEXT").call("NOPOST").ret()
    p = P("NOPOST")
    p.tok({"(": "DEADX", "++": "DEADX", "--": "DEADX"}, "RET")
    g.on("DEADX", range(257), "DEAD", rej("not covered: postfix on a * or & operand"), "r")
    p = P("PVCHK")    # dereferencing needs a pointer
    p.branch({(1, 2): "RET"}, ("rej", "not covered: dereference of a non-pointer"), [("CMPI", "pt", 1)])
    p = P("DEREF")    # r0 := *r0; the pointee's width follows the pointee type
    p.a(("ALUI", "sub", "pt", "pt", 1))
    width(p, "pt", "  .ld r0, [r0+0], 4\n", "  load64 r0, [r0+0]\n")
    p.ret()
    p = P("EXPR.use")
    p.call("IDTAIL").call("BINCONT").goto("EXPR.tail")
    p = P("EXPR.tail")
    p.tok({"=": "EXPR.bad", "?": "EXPR.q"}, "RET")
    p = P("EXPR.q")     # c ? a : b  (labels as if/else, measured)
    p.newlab("a").o("  jumpz r0, ").lab("a").o("\n").vpush("a").call("NEXT").call("CEXPR").expect(":")
    p.vpop("a").newlab("b").o("  jump ").lab("b").o("\n").lab("a").o(":\n").vpush("b").call("NEXT").call("EXPR")
    p.vpop("b").lab("b").o(":\n").ret()
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
        q.tok(dict([(k, nm + ".addr") for k in stops] + [("=", nm + ".as")]
                   + [(o + "=", nm + ".c" + o) for o in CASOPS]), nm + ".use")
        P(nm + ".use").call("EXPR.use").goto(nm + ".c")
        P(nm + ".as").call("EXPR.as").goto(nm + ".c")
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
    addr(p, "r0")
    p.o(PUSH).vpush("pt", "pb").call("NEXT").call("EXPR").vpop("pt", "pb").o(POP1)
    vwidth(p, "pt", "pb", *ST)
    p.ret()
    p = P("NOPTR")
    noptr(p)
    p.ret()
    g.on("DEADP", range(257), "DEAD", rej("not covered: pointer arithmetic"), "r")

    # UNARY
    p = P("UNARY")
    p.a(("LDI", "pt", 0))   # a primary is an int unless it says otherwise
    p.tok({"-": "U.neg", "!": "U.not", "~": "U.cpl", "+": "U.pos", "(": "U.par",
           TK_NUM: "U.num", TK_ID: "U.id", "*": "U.star", "&": "U.amp", "++": "U.pinc", "--": "U.pdec"}, ("rej", "not covered: expression"))
    for nm, sp in (("U.pinc", "add64"), ("U.pdec", "sub64")):
        P(nm).call("NEXT").tok({TK_ID: nm + ".id"}, ("rej", "not covered: operand of ++/--"))
        q = P(nm + ".id")
        lookup(q, "ps", "pe")
        noptr(q)
        addr(q, "r0")
        q.o(PUSH)
        vwidth(q, "pt", "pb", *LD)
        q.o("  imm r1, 1\n  %s r0, r0, r1\n" % sp + POP1)
        vwidth(q, "pt", "pb", *ST)
        q.call("NEXT").ret()
    for nm, txt in (("U.neg", "  imm r1, 0\n  sub64 r0, r1, r0\n"), ("U.not", "  imm r1, 0\n  eq r0, r0, r1\n"),
                    ("U.cpl", "  imm r1, -1\n  xor64 r0, r0, r1\n")):
        P(nm).call("NEXT").call("UNARY").o(txt).ret()
    P("U.pos").call("NEXT").goto("UNARY")
    P("U.star").call("NEXT").call("PV").call("PVCHK").call("DEREF").ret()
    P("U.amp").goto("PV")
    P("U.par").call("NEXT").call("CEXPR").expect(")").call("NEXT").ret()
    P("U.num").o("  imm r0, ").a(("SPAN2", "ps", "pe")).o("\n").call("NEXT").ret()
    P("U.id").a(("COPYW", "sps", "ps"), ("COPYW", "spe", "pe")).call("NEXT").call("IDTAIL").ret()

    # IDTAIL: saved id x[sps..spe), current token follows it
    p = P("IDTAIL")
    p.tok({"(": "IT.call", "++": "IT.inc", "--": "IT.dec"}, "IT.var")
    for nm, o, undo in (("IT.inc", "+", "sub64"), ("IT.dec", "-", "add64")):
        q = P(nm)   # a++ : a += 1, then the old value back (measured)
        lookup(q, "sps", "spe")
        noptr(q)
        addr(q, "r0")
        q.o(PUSH)
        vwidth(q, "pt", "pb", *LD)
        q.o(PUSH + "  imm r0, 1\n" + POP1 + optext(o) + POP1)
        vwidth(q, "pt", "pb", *ST)
        q.o("  imm r2, 1\n  %s r0, r0, r2\n" % undo).call("NEXT").ret()
    p = P("IT.var")
    lookup(p, "sps", "spe")
    addr(p, "r0")
    vwidth(p, "pt", "pb", *LD)
    p.ret()
    p = P("IT.call")
    p.a(("INTERN", "v", "sps", "spe"), ("LDX", "t", "v", FND))
    p.branch({1: "IT.ok"}, "IT.nd", [("CMP", "t", "pass")])
    p = P("IT.nd")
    p.branch({1: "PF"}, ("rej", "not covered: call to a function not defined before"), [("CMP", "v", "pfid")])
    p = P("IT.ok")
    p.a(("CMP", "v", "pfid"))
    p.branch({1: "DEADPF"}, "IT.ok1")
    g.on("DEADPF", range(257), "DEAD", rej("not covered: printf defined in the unit"), "r")
    p = P("IT.ok1")
    p.a(("LDX", "t", "v", LOC))
    p.branch({1: "IT.ok2"}, ("rej", "not covered: call through a local"), [("CMPI", "t", 0)])
    p = P("IT.ok2")
    p.vpush("sps", "spe").a(("LDI", "na", 0)).call("NEXT").tok({")": "IT.close"}, "IT.arg")
    p = P("IT.arg")
    p.vpush("na").call("EXPR").vpop("na").o(PUSH).a(("ALUI", "add", "na", "na", 1))
    p.tok({",": "IT.comma", ")": "IT.close"}, ("rej", "not covered: argument list"))
    P("IT.comma").call("NEXT").goto("IT.arg")
    p = P("IT.close")
    p.branch({2: "DEADA"}, "IT.pop", [("CMPI", "na", 6)])
    g.on("DEADA", range(257), "DEAD", rej("not covered: more than 6 arguments"), "r")
    p = P("IT.pop")
    p.branch({1: "IT.emit"}, "IT.pop1", [("CMPI", "na", 0)])
    p = P("IT.pop1")
    p.a(("ALUI", "sub", "na", "na", 1)).o("  load64 r").num("na").o(", [r7+0]\n  .frame -8\n").goto("IT.pop")
    p = P("IT.emit")
    p.vpop("sps", "spe").o("  call ").a(("SPAN2", "sps", "spe")).o("\n").a(("INTERN", "v", "sps", "spe"), ("LDX", "pt", "v", FRD), ("LDX", "pb", "v", FRB)).call("NEXT").ret()
    g.on("DEAD0", range(257), "DEAD", rej("not covered: identifier is not a local"), "r")


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
    p.a(("ALUI", "add", "cur", "cur", 1), ("COPYW", "s", "cur"))
    up, nx = p.fresh("mx"), p.fresh("dn")
    p.branch({2: up}, nx, [("CMP", "cur", "max")])
    p.cur = up
    p.a(("COPYW", "max", "cur")).goto(nx)
    p.cur = nx
    p.o("  store64 [r6-").a(("ALUI", "mul", "n", "s", 8)).call("PRN").o("], r0\n")
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
    p.o("  load64 r0, [r6-").a(("ALUI", "mul", "n", "s", 8)).call("PRN").o("]\n  .print r0\n").goto("PFW")
    p = P("PFW.e1")
    p.call("PFW.flush")
    p.branch({1: "PFW.e2"}, ("rej", "not covered: printf arguments"), [("CMP", "ai", "na")])
    p = P("PFW.e2")
    p.a(("INPOP",), ("ALUI", "sub", "vsp", "pb", 2)).o("  imm r0, 0\n").call("NEXT").ret()

    # the pool: a third scan of x after the footer; every printf format's
    # literal segments again, in order, as `.str Sk "..\x00"`
    p = P("POOL")
    p.a(("JUMP", "x0"), ("LDI", "sk", 0)).call("NEXT").label("PO.loop")
    p.tok({"eof": "RET", TK_ID: "PO.id"}, "PO.nx")
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
    p.a(("LDI", "bni", 1), ("LDI", "bsz", 0)).tok({"type": "S.dint", "type=char": "S.dch", "type=long": "S.dlg"}, "S.dnx")
    P("S.dint").a(("LDI", "bni", 0), ("LDI", "bsz", 4)).goto("S.dnx")
    P("S.dch").a(("LDI", "bni", 0), ("LDI", "bsz", 1)).goto("S.dnx")
    P("S.dlg").a(("LDI", "bni", 0), ("LDI", "bsz", 8)).goto("S.dnx")
    P("S.dnx").call("NEXT").tok(dict((w, "S.dw") for w in TWORDS), "D.one")
    P("S.dw").a(("LDI", "bni", 1), ("LDI", "bsz", 0)).goto("S.dnx")
    p = P("D.one")
    stars(p, "D.id")
    p = P("D.id")
    declare(p)
    p.call("NEXT").tok({"=": "D.init"}, "D.next")
    p = P("D.init")
    p.vpush("s", "ptd", "bni", "bsz").call("NEXT").call("EXPR").vpop("s", "ptd", "bni", "bsz")
    addr(p, "r1")
    vwidth(p, "ptd", "bsz", *ST)
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
    p = P("S.reti")
    p.o("  .frame 8\n  .st [r7+0], r0, 4\n  .ld r0, [r7+0], 4\n  .frame -8\n  jump R").num("rl").o("\n")
    p.call("NEXT").ret()
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


def unit():
    p = P("START")
    p.a(("LDI", "x0", 0), ("LDI", "pass", 1), ("SBCLR",), [("SBOUT", c) for c in b"main"], ("SBINTERN", "mainid"),
        ("SBCLR",), [("SBOUT", c) for c in b"printf"], ("SBINTERN", "pfid"))
    p.label("PASS").a(("JUMP", "x0"), ("LDI", "lab", 0), ("LDI", "fn", 0), ("LDI", "usp", 0),
                      ("LDI", "vsp", 0), ("LDI", "sk", 0), ("LDI", "brk", 0), ("LDI", "cnt", 0)).o(HEADER).call("NEXT")
    p.label("TOP").tok({"type": "FN", "type=void": "FN", "type=char": "FN", "type=long": "FN", "type=static": "TOP.st", "typedef": "TD", "eof": "END"}, ("rej", "not covered: top-level construct"))
    # typedef <type words | struct TAG> *... NAME;  -- no code; NAME recorded in TDN
    TW = {"type": "TD.w", "type=void": "TD.w", "type=long": "TD.w", "type=char": "TD.w",
          "type=unsigned": "TD.w", "type=short": "TD.w", "type=signed": "TD.w"}
    P("TD").call("NEXT").tok(dict(TW, struct="TD.s"), ("rej", "not covered: typedef"))
    P("TD.s").call("NEXT").tok({TK_ID: "TD.w"}, ("rej", "not covered: typedef"))
    P("TD.w").call("NEXT").tok({**TW, "*": "TD.w", TK_ID: "TD.id"}, ("rej", "not covered: typedef"))
    p = P("TD.id")
    p.a(("INTERN", "v", "ps", "pe"), ("LDI", "t", 1), ("STX", "v", TDN, "t")).call("NEXT").expect(";").call("NEXT").goto("TOP")
    P("TOP.st").call("NEXT").tok({"type": "FN", "type=void": "FN", "type=char": "FN", "type=long": "FN"}, ("rej", "not covered: static declaration"))
    p = P("FN")
    p.a(("LDI", "bni", 1), ("LDI", "bsz", 0)).tok({"type": "FN.i", "type=void": "FN.i", "type=char": "FN.c", "type=long": "FN.l"}, "FN.n")
    P("FN.i").a(("LDI", "bni", 0)).tok({"type": "FN.i4"}, "FN.n")
    P("FN.i4").a(("LDI", "bsz", 4)).goto("FN.n")
    P("FN.c").a(("LDI", "bsz", 1)).goto("FN.n")
    P("FN.l").a(("LDI", "bsz", 8)).goto("FN.n")
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
    p.o(".bss g_").a(("SPAN2", "fps", "fpe"), ("LDI", "t", GMARK), ("STX", "v", LOC, "t"), ("LDI", "z0", 0), ("STX", "v", FND, "z0"), ("STX", "v", PTR, "z0")).o(" 4\n")
    p.tok({"=": "GV.eq"}, "GV.nx")
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
    p.a(("LDI", "bni", 1), ("LDI", "bsz", 0)).tok({"type": "FN.pi", "type=void": "FN.pv", "type=char": "FN.pc", "type=long": "FN.pl",
                                 "type=unsigned": "FN.pt", "type=short": "FN.pt", "type=signed": "FN.pt",
                                 TK_ID: "FN.ptd"}, ("rej", "not covered: parameter"))
    p = P("FN.ptd")
    p.a(("INTERN", "v", "ps", "pe"), ("LDX", "t", "v", TDN)).branch({1: "FN.pt"}, ("rej", "not covered: parameter"), [("CMPI", "t", 1)])
    P("FN.pi").a(("LDI", "bni", 0), ("LDI", "bsz", 4)).goto("FN.pt")
    P("FN.pc").a(("LDI", "bsz", 1)).goto("FN.pt")
    P("FN.pl").a(("LDI", "bsz", 8)).goto("FN.pt")
    P("FN.pw").a(("LDI", "bsz", 0)).goto("FN.pt")
    P("FN.pv").call("NEXT").tok({")": "FN.close", "*": "FN.pvs"}, ("rej", "not covered: parameter"))
    P("FN.pvs").a(("LDI", "bni", 1)).goto("FN.pvk")
    P("FN.pt").call("NEXT").goto("FN.pvk")
    p = P("FN.pvk")
    p.tok(dict((w, "FN.pw") for w in TWORDS), "FN.pst")
    p = P("FN.pst")
    stars(p, "FN.pid")
    p = P("FN.pid")
    declare(p)
    p.call("NEXT").tok({",": "FN.comma", ")": "FN.close"}, ("rej", "not covered: parameter list"))
    P("FN.comma").call("NEXT").goto("FN.par")
    p = P("FN.close")
    p.branch({2: "FN.many"}, "FN.hd", [("CMPI", "cur", 6)])
    g.on("FN.many", range(257), "DEAD", rej("not covered: more than 6 parameters"), "r")
    p = P("FN.hd")
    p.a(("COPYW", "np", "cur")).call("NEXT").tok({"{": "FN.body", ";": "FN.proto"}, ("rej", "not covered: declaration"))
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
    p.a(("ALUI", "mul", "t", "max", 8), ("STX", "fn", FR, "t"), ("ALUI", "add", "fn", "fn", 1), ("LDI", "z", 0))
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
    p.o("  imm r0, ").a(("SPAN2", "ps", "pe")).o("\n  .lea r1, g_").a(("SPAN2", "gs", "ge"))
    p.o("\n  .st [r1+0], r0, 4\n").goto("IN.nx")


def build():
    tokenizer()
    prn()
    expr()
    stmt()
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
