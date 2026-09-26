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

WORDS = ["type=int", "type=void", "return", "if", "else", "while", "for", "eof",
         "(", ")", "{", "}", ";", ",", "=", "!", "~",
         "++", "--", "?", ":"] + [o + "=" for o in ("+", "-", "*", "/", "%", "<<", ">>", "&", "^", "|")] + sorted(PREC)
TK = {w: k + 1 for k, w in enumerate(WORDS)}
TK["type"] = TK["type=int"]   # x is the UA_TYPESPELL dump: every other spelling is TK_OTHER
TK_ID, TK_NUM, TK_BADNUM, TK_OTHER = 100, 101, 102, 103
CASOPS = ("+", "-", "*", "/", "%", "<<", ">>", "&", "^", "|")
LOC, FND, UNDO, FR, DIG, VS = 10 ** 6, 2 * 10 ** 6, 3 * 10 ** 6, 5 * 10 ** 6, 6 * 10 ** 6, 7 * 10 ** 6

g = G()


def O(s):
    return [("OUT", c) for c in s.encode()]


def rej(k):
    return [("REJECT", k)]


# ---- the token reader: a byte trie over the dump's lines -------------------
def tokenizer():
    pre = {""}
    for w in WORDS + ["id=", "num="]:
        for i in range(1, len(w) + 1):
            pre.add(w[:i])
    g.on("NEXT", range(257), "NX", [("MARK", "tpos")], "r")
    for p in sorted(pre):
        st = "NX" + p
        if p == "id=" or p == "num=":
            g.on(st, range(257), "SPANID" if p == "id=" else "SPANNUM", [("MARK", "ps")], "r")
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


def addr(p, reg):             # address of local slot W[s] into reg
    p.o("  imm r2, ").a(("ALUI", "mul", "n", "s", 8)).call("PRN").o("\n  sub64 %s, r6, r2\n" % reg)


def lookup(p, lo, hi):        # s := slot of the local spelled x[W[lo]..W[hi])
    p.a(("INTERN", "v", lo, hi), ("LDX", "s", "v", LOC))
    ok = p.fresh("ok")
    p.branch({1: "DEAD0"}, ok, [("CMPI", "s", 0)])
    p.cur = ok


def declare(p):               # declare x[ps..pe) as a new local; slot in W[s]
    p.a(("INTERN", "v", "ps", "pe"), ("LDX", "o", "v", LOC),
        ("STX", "usp", UNDO, "v"), ("STX", "usp", UNDO + 1, "o"), ("ALUI", "add", "usp", "usp", 2),
        ("ALUI", "add", "cur", "cur", 1), ("STX", "v", LOC, "cur"), ("COPYW", "s", "cur"))
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
    p.a(("ALUI", "sub", "usp", "usp", 2), ("LDX", "v", "usp", UNDO), ("LDX", "o", "usp", UNDO + 1),
        ("STX", "v", LOC, "o")).goto(top)
    p.cur = done


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
                q.call("NEXT").call(sub).o(NORM).vpop("a").lab("a").o(":\n").goto("LOOP%d" % L)
            else:
                q.o(PUSH).call("NEXT").call(sub).o(POP1 + optext(op)).goto("LOOP%d" % L)
    # BINCONT: the operand is already emitted; resume every level's loop
    p = P("BINCONT")
    for L in levels[:-1]:
        p.a(("PUSH", "LOOP%d" % L))
    p.goto("LOOP%d" % top)

    # ASSIGN: id '=' ASSIGN | BIN
    p = P("EXPR")
    p.tok({TK_ID: "EXPR.id"}, "EXPR.bin")
    p = P("EXPR.bin")
    p.call("BIN%d" % levels[0]).goto("EXPR.tail")
    p = P("EXPR.id")
    p.a(("COPYW", "sps", "ps"), ("COPYW", "spe", "pe")).call("NEXT")
    p.tok(dict([("=", "EXPR.as")] + [(o + "=", "EXPR.c" + o) for o in CASOPS]), "EXPR.use")
    for o in CASOPS:     # a op= e: address, load, push, e, op, store (measured)
        q = P("EXPR.c" + o)
        lookup(q, "sps", "spe")
        addr(q, "r0")
        q.o(PUSH + "  .ld r0, [r0+0], 4\n" + PUSH).call("NEXT").call("EXPR")
        q.o(POP1 + optext(o) + POP1 + "  .st [r1+0], r0, 4\n").ret()
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
        q.tok({TK_ID: nm + ".id"}, nm + ".e")
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
    p.o(PUSH).call("NEXT").call("EXPR").o(POP1 + "  .st [r1+0], r0, 4\n").ret()

    # UNARY
    p = P("UNARY")
    p.tok({"-": "U.neg", "!": "U.not", "~": "U.cpl", "+": "U.pos", "(": "U.par",
           TK_NUM: "U.num", TK_ID: "U.id", "++": "U.pinc", "--": "U.pdec"}, ("rej", "not covered: expression"))
    for nm, sp in (("U.pinc", "add64"), ("U.pdec", "sub64")):
        P(nm).call("NEXT").tok({TK_ID: nm + ".id"}, ("rej", "not covered: operand of ++/--"))
        q = P(nm + ".id")
        lookup(q, "ps", "pe")
        addr(q, "r0")
        q.o(PUSH + "  .ld r0, [r0+0], 4\n  imm r1, 1\n  %s r0, r0, r1\n" % sp + POP1
            + "  .st [r1+0], r0, 4\n").call("NEXT").ret()
    for nm, txt in (("U.neg", "  imm r1, 0\n  sub64 r0, r1, r0\n"), ("U.not", "  imm r1, 0\n  eq r0, r0, r1\n"),
                    ("U.cpl", "  imm r1, -1\n  xor64 r0, r0, r1\n")):
        P(nm).call("NEXT").call("UNARY").o(txt).ret()
    P("U.pos").call("NEXT").goto("UNARY")
    P("U.par").call("NEXT").call("CEXPR").expect(")").call("NEXT").ret()
    P("U.num").o("  imm r0, ").a(("SPAN2", "ps", "pe")).o("\n").call("NEXT").ret()
    P("U.id").a(("COPYW", "sps", "ps"), ("COPYW", "spe", "pe")).call("NEXT").call("IDTAIL").ret()

    # IDTAIL: saved id x[sps..spe), current token follows it
    p = P("IDTAIL")
    p.tok({"(": "IT.call", "++": "IT.inc", "--": "IT.dec"}, "IT.var")
    for nm, o, undo in (("IT.inc", "+", "sub64"), ("IT.dec", "-", "add64")):
        q = P(nm)   # a++ : a += 1, then the old value back (measured)
        lookup(q, "sps", "spe")
        addr(q, "r0")
        q.o(PUSH + "  .ld r0, [r0+0], 4\n" + PUSH + "  imm r0, 1\n" + POP1 + optext(o) + POP1
            + "  .st [r1+0], r0, 4\n  imm r2, 1\n  %s r0, r0, r2\n" % undo).call("NEXT").ret()
    p = P("IT.var")
    lookup(p, "sps", "spe")
    addr(p, "r0")
    p.o("  .ld r0, [r0+0], 4\n").ret()
    p = P("IT.call")
    p.a(("INTERN", "v", "sps", "spe"), ("LDX", "t", "v", FND))
    p.branch({1: "IT.ok"}, ("rej", "not covered: call to a function not defined before"), [("CMP", "t", "pass")])
    p = P("IT.ok")
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
    p.vpop("sps", "spe").o("  call ").a(("SPAN2", "sps", "spe")).o("\n").call("NEXT").ret()
    g.on("DEAD0", range(257), "DEAD", rej("not covered: identifier is not a local"), "r")


def stmt():
    p = P("STMT")
    p.tok({"{": "BLOCK", "type": "S.decl", ";": "S.empty", "return": "S.ret", "if": "S.if",
           "while": "S.while", "for": "S.for"}, "S.expr")
    P("S.empty").call("NEXT").ret()
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
    p.call("NEXT").label("D.one").tok({TK_ID: "D.id"}, ("rej", "not covered: declarator"))
    p = P("D.id")
    declare(p)
    p.call("NEXT").tok({"=": "D.init"}, "D.next")
    p = P("D.init")
    p.vpush("s").call("NEXT").call("EXPR").vpop("s")
    addr(p, "r1")
    p.o("  .st [r1+0], r0, 4\n").goto("D.next")
    p = P("D.next")
    p.tok({",": "D.comma", ";": "S.empty"}, ("rej", "not covered: declaration"))
    P("D.comma").call("NEXT").goto("D.one")
    # return e;
    p = P("S.ret")
    p.call("NEXT").tok({";": "S.retv"}, "S.rete")
    g.on("S.retv", range(257), "DEAD", rej("not covered: return without a value"), "r")
    p = P("S.rete")
    p.call("CEXPR").expect(";")
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
    p.newlab("a").newlab("b").lab("a").o(":\n").vpush("a", "b")
    p.call("NEXT").expect("(").call("NEXT").call("CEXPR").expect(")")
    p.vpop("a", "b").o("  jumpz r0, ").lab("b").o("\n").vpush("a", "b").call("NEXT").call("STMT")
    p.vpop("a", "b").o("  jump ").lab("a").o("\n").lab("b").o(":\n").ret()
    # for (e; e; e) s -- the step is parsed after the body: skip it, run the
    # body, JUMP the reader back to it, then JUMP forward past the body
    p = P("S.for")
    p.call("NEXT").expect("(").call("NEXT").tok({";": "F.no", "type": "F.no"}, "F.init")
    g.on("F.no", range(257), "DEAD", rej("not covered: for clause"), "r")
    p = P("F.init")
    p.call("VEXPR").expect(";").newlab("a").newlab("b").newlab("c").lab("a").o(":\n")
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
    p.a(("JUMP", "ep")).call("NEXT").ret()


def unit():
    p = P("START")
    p.a(("LDI", "x0", 0), ("LDI", "pass", 1), ("SBCLR",), [("SBOUT", c) for c in b"main"], ("SBINTERN", "mainid"))
    p.label("PASS").a(("JUMP", "x0"), ("LDI", "lab", 0), ("LDI", "fn", 0), ("LDI", "usp", 0),
                      ("LDI", "vsp", 0)).o(HEADER).call("NEXT")
    p.label("TOP").tok({"type": "FN", "eof": "END"}, ("rej", "not covered: top-level construct"))
    p = P("FN")
    p.call("NEXT").tok({TK_ID: "FN.id"}, ("rej", "not covered: declarator"))
    p = P("FN.id")
    p.a(("INTERN", "v", "ps", "pe"), ("STX", "v", FND, "pass"), ("COPYW", "fps", "ps"), ("COPYW", "fpe", "pe"),
        ("COPYW", "fv", "v"), ("LDI", "cur", 0), ("LDI", "max", 0))
    p.call("NEXT").expect("(").call("NEXT").tok({")": "FN.close"}, "FN.par")
    p = P("FN.par")
    p.tok({"type": "FN.pt", "type=void": "FN.pv"}, ("rej", "not covered: parameter"))
    P("FN.pv").call("NEXT").tok({")": "FN.close"}, ("rej", "not covered: parameter"))
    P("FN.pt").call("NEXT").tok({TK_ID: "FN.pid"}, ("rej", "not covered: parameter"))
    p = P("FN.pid")
    declare(p)
    p.call("NEXT").tok({",": "FN.comma", ")": "FN.close"}, ("rej", "not covered: parameter list"))
    P("FN.comma").call("NEXT").goto("FN.par")
    p = P("FN.close")
    p.branch({2: "FN.many"}, "FN.hd", [("CMPI", "cur", 6)])
    g.on("FN.many", range(257), "DEAD", rej("not covered: more than 6 parameters"), "r")
    p = P("FN.hd")
    p.a(("COPYW", "np", "cur")).call("NEXT").tok({"{": "FN.body"}, ("rej", "not covered: declaration"))
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
    P("END3").o(FOOTER).a(("ACCEPT",)).goto("DEAD")


def build():
    tokenizer()
    prn()
    expr()
    stmt()
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
