"""E3, structured (research/e3-structured.md): the delta is GENERATED from
declared data -- a grammar, attribute tables (the gold stages), and tape
templates -- by one generic compiler, instead of being grown state by state.

    python3 exec/parse2/gen2.py OUT.json

Step 1 covers: int functions and parameters, int locals, expression
statements, assignment, calls, unary - !, the binary operators of every
precedence level in weights/gold/prec.tsv except && ||, if/else, while,
return.  Anything else is rejected as not covered.  The frame size is
backpatched (ORES/OFILL), as the reference itself does: its `.frame` field is
always 7 characters wide.
"""
import json
import os
import re
import sys

import importlib.util
_spec = importlib.util.spec_from_file_location(
    "e3gen", os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "parse", "gen.py"))
E = importlib.util.module_from_spec(_spec)   # the token reader, the assembler P, the gold tables, the tape constants
_spec.loader.exec_module(E)

P, O, TK, TK_ID, TK_NUM, LOC = E.P, E.O, E.TK, E.TK_ID, E.TK_NUM, E.LOC
g = E.g

# ---- declared data 1: tape templates (measured once from the reference) -------------
# {name} prints W[name] in decimal; text is copied as is.
TEMPL = {
    "fn_head":  "{@name}:\n  .frame 8\n  store64 [r7+0], r6\n  mov r6, r7\n  .frame",
    "spill":    "  store64 [r6-{s}], r{pk}\n",
    "addr":     "  imm r2, {s}\n  sub64 r0, r6, r2\n",
    "gaddr":    "  .lea r0, g_{@var}\n",
    "load_int": "  .ld r0, [r0+0], 4\n",
    "store_int": "  .st [r1+0], r0, 4\n",
    "imm":      "  imm r0, {nv}\n",
    "push":     E.PUSH,
    "pop1":     E.POP1,
    "pop_arg":  "  load64 r{ak}, [r7+0]\n  .frame -8\n",
    "call":     "  call {@callee}\n",
    "neg":      "  imm r1, 0\n  sub64 r0, r1, r0\n",
    "not":      "  imm r1, 0\n  eq r0, r0, r1\n",
    "ret_int":  "  .frame 8\n  .st [r7+0], r0, 4\n  .ld r0, [r7+0], 4\n  .frame -8\n  jump R{rl}\n",
    "fn_tail":  "R{rl}:\n  mov r7, r6\n  load64 r6, [r7+0]\n  .frame -8\n  ret\n",
    "jumpz":    "  jumpz r0, L{a}\n",
    "jump_b":   "  jump L{b}\n",
    "jump_a":   "  jump L{a}\n",
    "label_a":  "L{a}:\n",
    "label_b":  "L{b}:\n",
    "label_c":  "L{c}:\n",
    "jumpz_b":  "  jumpz r0, L{b}\n",
    "bool":     "  imm r1, 0\n  ne r0, r0, r1\n",
    "and_skip": "  jumpz r0, L{e}\n",
    "label_e":  "L{e}:\n",
    "or_skip":  "  jumpz r0, L{on}\n  imm r0, 1\n  jump L{od}\nL{on}:\n",
    "label_d":  "L{od}:\n",
    "one":      "  imm r0, 1\n",
    "post_inc": "  imm r2, 1\n  sub64 r0, r0, r2\n",
    "post_dec": "  imm r2, 1\n  add64 r0, r0, r2\n",
    "pre_inc":  "  imm r1, 1\n  add64 r0, r0, r1\n",
    "pre_dec":  "  imm r1, 1\n  sub64 r0, r0, r1\n",
    # printf, the reference's builtin lowering (measured, examples/hello.c and a %d probe)
    "pf_spill": "  store64 [r6-{cur}], r0\n",
    "pf_write": "  .lea r0, S{sk}\n  imm r1, {cnt}\n  .write r0, r1\n",
    "pf_print": "  load64 r0, [r6-{as}]\n  .print r0\n",
    "pf_value": "  imm r0, 0\n",
    "pool_open": ".str S{sk} \"",
    "pool_close": "\\x00\"\n",
}
SPANS = {"@name": ("fns", "fne"), "@callee": ("cls", "cle"), "@var": ("ips", "ipe")}


def addr(p):
    """a variable's address: a local's frame slot, or a global's symbol"""
    gl, lc, dn = p.fresh("ga"), p.fresh("la"), p.fresh("ad")
    p.branch({1: gl}, lc, [("CMPI", "s", E.GMARK)])
    emit(P(gl), "gaddr").goto(dn)
    emit(P(lc), "addr").goto(dn)
    p.cur = dn
    return p


def emit(p, name):
    """a template as output actions: literal text and slot prints"""
    for part in re.split(r"(\{[^}]*\})", TEMPL[name]):
        if not part:
            continue
        if part[0] == "{":
            k = part[1:-1]
            if k in SPANS:
                p.a(("SPAN2",) + SPANS[k])
            else:
                p.num(k)
        else:
            p.o(part)
    return p


# ---- declared data 2: the operator ladder (derived from weights/gold/prec.tsv) --------
LEVELS = sorted({v for v in E.PREC.values()})
OPS = {lv: sorted(o for o, v in E.PREC.items() if v == lv) for lv in LEVELS}
SHORT = {"&&", "||"}   # short-circuit: not in step 1


def bad(k):
    return ("rej", "not covered: " + k)


def optail(o):
    """OPX.<o>: the left operand pushed (descriptor lt/lb), the right in r0 (vt/vb): one copy
    per operator, shared by both ladders -- double, pointer scaling, unsigned, int forms"""
    bn = "OPX." + o
    q = P(bn)
    q.a(("COPYW", "svt", "vt"), ("COPYW", "svb", "vb"), ("COPYW", "vt", "lt"), ("COPYW", "vb", "lb")).call("ISDV").a(("COPYW", "ldv", "u"),
        ("COPYW", "vt", "svt"), ("COPYW", "vb", "svb")).call("ISDV").a(("ALU", "or", "t", "u", "ldv"))
    q.branch({1: bn + ".f"}, bn + ".nf", [("CMPI", "t", 1)])
    f = P(bn + ".f")
    if o in FOPS:
        # right in r0: cvtid if an integer; push; the left from [r7+8] (cvtid if an integer); mov r1, r0; the right back; .frame -16
        f.branch({1: bn + ".f1"}, bn + ".fc", [("CMPI", "u", 1)])
        P(bn + ".fc").o("  cvtid r0, r0\n").goto(bn + ".f1")
        f = P(bn + ".f1")
        emit(f, "push").o("  load64 r0, [r7+8]\n").branch({1: bn + ".f2"}, bn + ".fl", [("CMPI", "ldv", 1)])
        P(bn + ".fl").o("  cvtid r0, r0\n").goto(bn + ".f2")
        cmpf = o in ("<", ">", "<=", ">=", "==")
        P(bn + ".f2").o("  mov r1, r0\n  load64 r0, [r7+0]\n  .frame -16\n  %s\n" % FOPS[o]).a(("LDI", "vt", 0), ("LDI", "vb", 4 if cmpf else DBL)).ret()
    else:
        f.goto("DEAD.dbl")
    q = P(bn + ".nf")
    if o in ("+", "-"):
        q.branch({1: bn + ".i"}, bad("pointer on the right"), [("CMPI", "vt", 0)])
        q = P(bn + ".i")
        q.branch({1: bn + ".n"}, bn + ".p", [("CMPI", "lt", 0)])
        r = P(bn + ".p")
        r.call("SCALE")
        emit(r, "pop1").o(E.optext(o)).a(("COPYW", "vt", "lt"), ("COPYW", "vb", "lb")).ret()
        q = P(bn + ".n")
    else:
        q.branch({1: bn + ".r"}, bn + ".pc", [("ALU", "or", "t", "vt", "lt"), ("CMPI", "t", 0)])
        P(bn + ".pc").goto(bn + ".r" if o in ("<", ">", "<=", ">=", "==", "!=") else "DEAD.pa")
        q = P(bn + ".r")
    emit(q, "pop1")
    cmp = o in ("<", ">", "<=", ">=", "==", "!=")
    q.branch({1: bn + ".u"}, bn + ".u0", [("CMPI", "lb", UNS + 8)])
    P(bn + ".u0").branch({1: bn + ".u"}, bn + ".s", [("CMPI", "vb", UNS + 8)])
    P(bn + ".u").o(E.optext(o, True)).a(("LDI", "vt", 0), ("LDI", "vb", 4 if cmp else UNS + 8)).ret()
    q = P(bn + ".s")
    q.o(E.optext(o))
    if cmp:
        q.a(("LDI", "vt", 0), ("LDI", "vb", 4)).ret()
    else:
        q.branch({1: bn + ".l8"}, bn + ".l0", [("CMPI", "lb", 8)])
        P(bn + ".l0").branch({1: bn + ".l8"}, bn + ".i4", [("CMPI", "vb", 8)])
        P(bn + ".l8").a(("LDI", "vt", 0), ("LDI", "vb", 8)).ret()
        P(bn + ".i4").a(("LDI", "vt", 0), ("LDI", "vb", 4)).ret()


def ladder(prefix, bottom):
    """E<lv>: operand, then (op E<lv+1>)* for the ops of level lv.
    prefix "E": from scratch (bottom = UNARY); prefix "C": the left operand is
    already in r0 (bottom = nothing) -- the same table, generated twice."""
    for i, lv in enumerate(LEVELS):
        nm, up = "%s%d" % (prefix, lv), ("%s%d" % (prefix, LEVELS[i + 1]) if i + 1 < len(LEVELS) else bottom)
        p = P(nm)
        if up:
            p.call(up)
        p.label(nm + ".l")
        cases = {o: "%s.%s" % (nm, o) for o in OPS[lv]}
        p.tok(cases, "RET")
        nxt = "E%d" % LEVELS[i + 1] if i + 1 < len(LEVELS) else "UNARY"
        for o in OPS[lv]:
            q = P("%s.%s" % (nm, o))
            if o == "&&":
                q.a(("ALUI", "add", "lab", "lab", 1), ("COPYW", "e", "lab"))
                emit(q, "and_skip").vpush("e").call("NEXT").call(nxt).vpop("e")
                emit(q, "bool").a(("LDI", "vt", 0), ("LDI", "vb", 4))
                emit(q, "label_e").goto(nm + ".l")
                continue
            if o == "||":
                q.a(("ALUI", "add", "lab", "lab", 1), ("COPYW", "od", "lab"), ("ALUI", "add", "lab", "lab", 1), ("COPYW", "on", "lab"))
                emit(q, "or_skip").vpush("od").call("NEXT").call(nxt).vpop("od")
                emit(q, "bool").a(("LDI", "vt", 0), ("LDI", "vb", 4))
                emit(q, "label_d").goto(nm + ".l")
                continue
            # left in r0: push; the right operand at the next level; then the operator's shared tail (OPX)
            emit(q, "push").vpush("vt", "vb").call("NEXT").call(nxt).vpop("lt", "lb").call("OPX." + o).goto(nm + ".l")
    if prefix == "E":
        for lv in LEVELS:
            for o in OPS[lv]:
                if o not in SHORT:
                    optail(o)
    g.on("DEAD.short", range(257), "DEAD", E.rej("not covered: && ||"), "r")
    g.on("DEAD.pa", range(257), "DEAD", E.rej("not covered: pointer arithmetic"), "r")


HEX = "0123456789abcdef"
ESC = {"n": 10, "t": 9, "r": 13, "a": 7, "b": 8, "f": 12, "v": 11, "\\": 92, "'": 39, '"': 34, "?": 63, "0": 0}


def fmtwalk(pre, on_byte, on_d, on_end):
    """a printf format in the pushed reader frame, byte by byte: literal bytes (escapes
    decoded into W[bv]) -> on_byte state; %d -> on_d; %% is the byte '%'; the end -> on_end.
    Every other conversion or escape is not covered."""
    w = pre + ".w"
    g.on(w, [37], pre + ".pc", [("ADV",)])
    g.on(w, [92], pre + ".es", [("ADV",)])
    g.on(w, [256], on_end, [("INPOP",)])
    for c in range(256):
        if c not in (37, 92):
            g.on(w, [c], on_byte, [("ADV",), ("LDI", "bv", c)])
    g.on(pre + ".pc", [37], on_byte, [("ADV",), ("LDI", "bv", 37)])
    g.on(pre + ".pc", [ord("d")], on_d, [("ADV",)])
    g.els(pre + ".pc", "DEAD", E.rej("not covered: printf conversion"))
    for ch, v in ESC.items():
        g.on(pre + ".es", [ord(ch)], on_byte, [("ADV",), ("LDI", "bv", v)])
    g.els(pre + ".es", "DEAD", E.rej("not covered: printf escape"))
    return w


def printf():
    # PF: at '(' after printf.  The arguments are evaluated first, each into a fresh slot
    # (store64 [r6-N], r0); then the format's literal runs become .write of pooled strings
    # and each %d prints the next slot (.print); the value is 0.
    p = P("PF")
    p.call("NEXT").tok({E.TK_STR: "PF.s"}, bad("printf format"))
    p = P("PF.s")
    p.a(("ALUI", "add", "fs", "ps", 1), ("ALUI", "sub", "fe", "pe", 1)).vpush("fs", "fe").a(("ALU", "add", "as", "cur", "z0"), ("ALUI", "add", "as", "as", 8)).vpush("as")
    p.call("NEXT").label("PF.args")
    p.tok({",": "PF.arg", ")": "PF.go"}, bad("argument list"))
    q = P("PF.arg")
    q.call("NEXT").call("EXPR").a(("ALUI", "add", "cur", "cur", 8)).call("MAXF")
    emit(q, "pf_spill").goto("PF.args")
    q = P("PF.go")
    q.vpop("as").vpop("fs", "fe").a(("LDI", "cnt", 0), ("INPUSHXE", "fs", "fe")).goto("PF.w")
    fmtwalk("PF", "PF.b", "PF.d", "PF.end")
    P("PF.b").a(("ALUI", "add", "cnt", "cnt", 1)).goto("PF.w")
    q = P("PF.d")
    q.call("PF.flush")
    emit(q, "pf_print").a(("ALUI", "add", "as", "as", 8)).goto("PF.w")
    q = P("PF.end")
    q.call("PF.flush")
    emit(q, "pf_value").call("NEXT").ret()
    q = P("PF.flush")
    q.branch({1: "RET"}, "PF.fw", [("CMPI", "cnt", 0)])
    q = P("PF.fw")
    emit(q, "pf_write").a(("ALUI", "add", "sk", "sk", 1), ("LDI", "cnt", 0)).ret()
    # POOL: after the footer, the formats again from the top, in the same order: each
    # non-empty literal run is `.str Sk "..."` -- printable bytes as they are, others \xHH, then \x00
    p = P("POOL")
    p.call("NEXT").label("PO.l")
    p.tok({"eof": "RET", TK_ID: "PO.id", E.TK_STR: "PO.lit"}, "PO.nx")
    P("PO.lit").a(("ALUI", "add", "fs", "ps", 1), ("ALUI", "sub", "fe", "pe", 1), ("LDI", "cnt", 0), ("INPUSHXE", "fs", "fe")).goto("PL.w")
    g.on("PL.w", [92], "PL.es", [("ADV",)])
    g.on("PL.w", [256], "PL.end", [("INPOP",)])
    for c in range(256):
        if c != 92:
            g.on("PL.w", [c], "PL.b", [("ADV",), ("LDI", "bv", c)])
    for ch, v in ESC.items():
        g.on("PL.es", [ord(ch)], "PL.b", [("ADV",), ("LDI", "bv", v)])
    g.els("PL.es", "DEAD", E.rej("not covered: string escape"))
    q = P("PL.b")
    q.branch({1: "PL.open"}, "PL.byte", [("CMPI", "cnt", 0)])
    q = P("PL.open")
    emit(q, "pool_open").goto("PL.byte")
    P("PL.byte").a(("ALUI", "add", "cnt", "cnt", 1), ("RLD", "bv")).goto("PL.out")
    for c in range(256):
        if c in (34, 92):
            g.on("PL.out", [c], "PL.w", [("OUT", 92), ("OUT", c)], "r")
        elif 32 <= c < 127:
            g.on("PL.out", [c], "PL.w", [("OUT", c)], "r")
        else:
            g.on("PL.out", [c], "PL.w", [("OUT", 92), ("OUT", ord("x")), ("OUT", ord(HEX[c >> 4])), ("OUT", ord(HEX[c & 15]))], "r")
    q = P("PL.end")     # an empty literal still has its \x00
    q.branch({1: "PL.e0"}, "PL.e1", [("CMPI", "cnt", 0)])
    q = P("PL.e0")
    emit(q, "pool_open").goto("PL.e1")
    q = P("PL.e1")
    emit(q, "pool_close").a(("ALUI", "add", "sk", "sk", 1), ("LDI", "cnt", 0)).call("NEXT").goto("PO.l")
    P("PO.nx").call("NEXT").goto("PO.l")
    P("PO.id").a(("INTERN", "v", "ps", "pe")).branch({1: "PO.pf"}, "PO.nx", [("CMP", "v", "pfid")])
    P("PO.pf").call("NEXT").tok({"(": "PO.p1"}, "PO.l")
    P("PO.p1").call("NEXT").tok({E.TK_STR: "PO.s"}, "PO.l")
    P("PO.s").a(("ALUI", "add", "fs", "ps", 1), ("ALUI", "sub", "fe", "pe", 1), ("LDI", "cnt", 0), ("INPUSHXE", "fs", "fe")).goto("PO.w")
    fmtwalk("PO", "PO.b", "PO.d", "PO.end")
    q = P("PO.b")
    q.branch({1: "PO.open"}, "PO.byte", [("CMPI", "cnt", 0)])
    q = P("PO.open")
    emit(q, "pool_open").goto("PO.byte")
    q = P("PO.byte")
    q.a(("ALUI", "add", "cnt", "cnt", 1), ("RLD", "bv")).goto("PO.out")
    for c in range(256):
        if c in (34, 92):     # measured (probe p12): \" and \\
            g.on("PO.out", [c], "PO.w", [("OUT", 92), ("OUT", c)], "r")
        elif 32 <= c < 127:
            g.on("PO.out", [c], "PO.w", [("OUT", c)], "r")
        else:
            g.on("PO.out", [c], "PO.w", [("OUT", 92), ("OUT", ord("x")), ("OUT", ord(HEX[c >> 4])), ("OUT", ord(HEX[c & 15]))], "r")
    q = P("PO.d")
    q.call("PO.close").goto("PO.w")
    q = P("PO.end")
    q.call("PO.close").call("NEXT").goto("PO.l")
    q = P("PO.close")
    q.branch({1: "RET"}, "PO.cw", [("CMPI", "cnt", 0)])
    q = P("PO.cw")
    emit(q, "pool_close").a(("ALUI", "add", "sk", "sk", 1), ("LDI", "cnt", 0)).ret()


UNS = E.UNS   # unsigned char/short/long: UNS + size (unsigned int: not covered)
SBB = 1000   # a struct's base code: SBB + sid; layouts in the old E3's tables (measured rules)
STAG, SSZ, MOF, MSZ, MPT, MBS = E.STAG, E.SSZ, E.MOF, E.MSZ, E.MPT, E.MBS
FPB = E.FPB   # a function pointer: depth 1, base FPB (its call result is taken as int)
DBL = E.DBL
PDB = 27 * 10 ** 6   # PDB[f * 16 + k] = base of f's parameter k (a double parameter converts an int argument)
FOPS = {"+": "fadd64 r0, r1, r0", "-": "fsub64 r0, r1, r0", "*": "fmul64 r0, r1, r0", "/": "fdiv64 r0, r1, r0",
        "<": "flt64 r0, r1, r0", ">": "flt64 r0, r0, r1", "<=": "fle64 r0, r1, r0", ">=": "fle64 r0, r0, r1",
        "==": "feq64 r0, r1, r0"}   # measured (the reversed forms for > >=); != not measured: not covered   # double: stored, passed, returned and va_arg'd as a plain 64-bit move (measured, old E3 p63)
FLT = E.FLT   # float: only `*p = (float) d` (cvtds, .st 4 -- measured by the old E3); any other float value is not covered
TYPEW = {"type=float": FLT, "type=double": DBL, "type": E.SZ["int"], "type=char": E.SZ["char"], "type=short": E.SZ["short"], "type=long": E.SZ["long"], "type=void": 0}
TWORDS = tuple(TYPEW) + ("type=unsigned",)


def width_dispatch(p, name, tab8, tabn, masks=False):
    """one procedure per access kind, shared by every construct: the width comes from the
    value descriptor (vt >= 1 or vb == 8: 8 bytes; else vb), the text from the template"""
    q = P(name)
    q.branch({(1, 2): name + ".8"}, name + ".b", [("CMPI", "vt", 1)])
    P(name + ".b").branch({1: name + ".8"}, name + ".d", [("CMPI", "vb", 8)])
    P(name + ".d").branch({1: name + ".8"}, name + ".n", [("CMPI", "vb", DBL)])
    P(name + ".8").o(tab8).ret()
    q = P(name + ".n")
    for w in (1, 2, 4):
        hit, nx = q.fresh("w"), q.fresh("x")
        q.branch({1: hit}, nx, [("CMPI", "vb", w)])
        P(hit).o(tabn % w).ret()
        q = P(nx)
    for w, mask in ((1, 255), (2, 65535), (8, None)):
        hit, nx = q.fresh("u"), q.fresh("y")
        q.branch({1: hit}, nx, [("CMPI", "vb", UNS + w)])
        if w == 8:
            P(hit).o(tab8).ret()
        else:
            P(hit).o(tabn % w + (masks and "  imm r2, %d\n  and64 r0, r0, r2\n" % mask or "")).ret()
        q = P(nx)
    q.goto("DEAD.w")


def types():
    width_dispatch(None, "LOADV", "  load64 r0, [r0+0]\n", "  .ld r0, [r0+0], %d\n", masks=True)
    width_dispatch(None, "LOADRAW", "  load64 r0, [r0+0]\n", "  .ld r0, [r0+0], %d\n")   # va_arg: no mask (measured, p62)
    P("STOREV").branch({1: "STF"}, "STOREV0", [("CMPI", "vb", FLT)])
    P("STF").branch({1: "STF1"}, "STOREV0", [("CMPI", "vt", 0)])
    P("STF1").o("  .st [r1+0], r0, 4\n").ret()
    width_dispatch(None, "STOREV0", "  store64 [r1+0], r0\n", "  .st [r1+0], r0, %d\n")
    # NARROW: a value to vb bytes through the stack (a return, a cast); 8 bytes and pointers: nothing
    q = P("NARROW")
    q.branch({(1, 2): "RET"}, "NARROW.b", [("CMPI", "vt", 1)])
    P("NARROW.b").branch({1: "RET"}, "NARROW.u", [("CMPI", "vb", 8)])
    P("NARROW.u").branch({1: "RET"}, "NARROW.dd", [("CMPI", "vb", UNS + 8)])
    P("NARROW.dd").branch({1: "RET"}, "NARROW.n", [("CMPI", "vb", DBL)])
    q = P("NARROW.n")
    for w in (1, 2, 4):
        hit, nx = q.fresh("w"), q.fresh("x")
        q.branch({1: hit}, nx, [("CMPI", "vb", w)])
        P(hit).o("  .frame 8\n  .st [r7+0], r0, %d\n  .ld r0, [r7+0], %d\n  .frame -8\n" % (w, w)).ret()
        q = P(nx)
    q.goto("DEAD.w")
    g.on("DEAD.w", range(257), "DEAD", E.rej("not covered: width"), "r")
    P("ISTD").a(("INTERN", "t", "ps", "pe"), ("LDX", "u", "t", E.TDN), ("CMPI", "u", 1)).ret()
    p = P("FPDECL")
    p.call("NEXT").expect("*").call("NEXT").tok({TK_ID: "FPD.id"}, bad("declarator"))
    p = P("FPD.id")
    p.a(("COPYW", "ips", "ps"), ("COPYW", "ipe", "pe"), ("LDI", "fpn", 0)).call("NEXT").tok({"[": "FPD.a", ")": "FPD.c"}, bad("declarator"))
    P("FPD.a").call("NEXT").tok({TK_NUM: "FPD.n"}, bad("array bound"))
    P("FPD.n").a(("COPYW", "fpn", "nv")).call("NEXT").expect("]").call("NEXT").expect(")").goto("FPD.c")
    p = P("FPD.c")       # skip the parameter list
    p.call("NEXT").expect("(").a(("LDI", "dep", 1)).call("NEXT").label("FPD.l")
    p.tok({"(": "FPD.o", ")": "FPD.x"}, "FPD.k")
    P("FPD.k").call("NEXT").goto("FPD.l")
    P("FPD.o").a(("ALUI", "add", "dep", "dep", 1)).goto("FPD.k")
    P("FPD.x").a(("ALUI", "sub", "dep", "dep", 1)).branch({1: "FPD.d"}, "FPD.k", [("CMPI", "dep", 0)])
    P("FPD.d").a(("LDI", "td", 1), ("LDI", "tb", FPB)).call("NEXT").ret()
    # TSPEC: type words then stars -> tb (base size, 0 void), td (depth); current token after
    p = P("TSPEC")
    p.a(("LDI", "td", 0)).tok({**{w: "TS." + w for w in TWORDS}, TK_ID: "TS.id", "struct": "TS.struct", "type=unsigned": "TS.type=unsigned"}, bad("type"))
    P("TS.struct").call("NEXT").tok({TK_ID: "TS.tag", "{": "TS.anon"}, bad("type"))
    P("TS.anon").a(("LDI", "tg", 0)).call("SBODY").call("NEXT").goto("TS.sb")
    p = P("TS.tag")
    p.a(("INTERN", "tg", "ps", "pe")).call("NEXT").tok({"{": "TS.tbody"}, "TS.tref")
    P("TS.tbody").call("SBODY").call("NEXT").goto("TS.sb")
    p = P("TS.tref")      # a tag without a body: its sid (an incomplete struct gets one, size 0: pointers only)
    p.a(("LDX", "nsid2", "tg", STAG)).branch({1: "TS.new"}, "TS.old", [("CMPI", "nsid2", 0)])
    P("TS.new").a(("ALUI", "add", "nsid", "nsid", 1), ("STX", "tg", STAG, "nsid"), ("COPYW", "nsid2", "nsid")).goto("TS.old")
    P("TS.old").a(("ALUI", "add", "tb", "nsid2", SBB)).goto("TS.l")
    P("TS.sb").a(("ALUI", "add", "tb", "nsid", SBB)).goto("TS.l")
    # SBODY at '{': members `T [*]... name;` -- each aligned to its own size, the total to the largest (measured)
    p = P("SBODY")
    p.vpush("td", "tb").a(("LDX", "t", "tg", STAG)).branch({1: "SB.nw"}, "SB.re", [("CMPI", "t", 0)])
    P("SB.nw").a(("ALUI", "add", "nsid", "nsid", 1), ("COPYW", "t", "nsid")).goto("SB.re")
    p = P("SB.re")      # (a tag seen before without a body is completed in place)
    p.a(("COPYW", "sid", "t"), ("COPYW", "nsid", "t")).branch({1: "SB.go"}, "SB.tg", [("CMPI", "tg", 0)])
    P("SB.tg").a(("STX", "tg", STAG, "sid")).goto("SB.go")
    P("SB.go").a(("LDI", "soff", 0), ("LDI", "smal", 1)).call("NEXT").label("SB.m")
    P("SB.m").tok({"}": "SB.end"}, "SB.mem")
    p = P("SB.mem")
    p.vpush("sid", "soff", "smal").call("TSPEC").vpop("sid", "soff", "smal").tok({TK_ID: "SB.nm"}, bad("struct member"))
    p = P("SB.nm")
    p.branch({1: "SB.v"}, "SB.p", [("CMPI", "td", 0)])
    P("SB.p").a(("LDI", "msz", 8)).goto("SB.put")
    P("SB.v").branch({(1, 2): "DEAD.sm"}, "SB.v1", [("CMPI", "tb", SBB)])
    P("SB.v1").branch({1: "DEAD.void"}, "SB.v2", [("CMPI", "tb", 0)])
    P("SB.v2").a(("COPYW", "msz", "tb")).branch({2: "SB.vu"}, "SB.put", [("CMPI", "tb", UNS)])
    P("SB.vu").a(("ALUI", "sub", "msz", "tb", UNS)).goto("SB.put")
    g.on("DEAD.sm", range(257), "DEAD", E.rej("not covered: a struct member of struct type"), "r")
    p = P("SB.put")
    p.a(("INTERN", "v", "ps", "pe"), ("ALU", "add", "t", "soff", "msz"), ("ALUI", "sub", "t", "t", 1), ("ALU", "sub", "m", "z0", "msz"), ("ALU", "and", "soff", "t", "m"),
        ("ALUI", "mul", "k", "v", 64), ("ALU", "add", "k", "k", "sid"),
        ("STX", "k", MOF, "soff"), ("STX", "k", MSZ, "msz"), ("STX", "k", MPT, "td"), ("STX", "k", MBS, "tb"), ("ALU", "add", "soff", "soff", "msz"))
    p.branch({2: "SB.mx"}, "SB.nx", [("CMP", "msz", "smal")])
    P("SB.mx").a(("COPYW", "smal", "msz")).goto("SB.nx")
    P("SB.nx").call("NEXT").tok({";": "SB.semi"}, bad("struct member"))
    P("SB.semi").call("NEXT").goto("SB.m")
    p = P("SB.end")
    p.a(("ALU", "add", "t", "soff", "smal"), ("ALUI", "sub", "t", "t", 1), ("ALU", "sub", "m", "z0", "smal"), ("ALU", "and", "t", "t", "m"),
        ("STX", "sid", SSZ, "t"), ("COPYW", "nsid", "sid")).vpop("td", "tb").ret()
    P("TS.id").a(("INTERN", "t", "ps", "pe"), ("LDX", "u", "t", E.TDN)).branch({1: "TS.td"}, bad("type"), [("CMPI", "u", 1)])
    P("TS.td").a(("LDX", "tb", "t", E.TDB), ("LDX", "td", "t", E.TDD)).call("NEXT").goto("TS.l")
    for w, n in TYPEW.items():   # (unsigned is read by its own states below)
        P("TS." + w).a(("LDI", "tb", n)).call("NEXT").goto("TS.l" if w != "type=long" else "TS.ll")
    P("TS.ll").tok({"type=long": "TS.ll2"}, "TS.l")    # long long: a long
    P("TS.type=unsigned").call("NEXT").tok({"type=char": "TS.u1", "type=short": "TS.u2", "type=long": "TS.u8"}, bad("unsigned int"))
    P("TS.u1").a(("LDI", "tb", UNS + 1)).call("NEXT").goto("TS.l")
    P("TS.u2").a(("LDI", "tb", UNS + 2)).call("NEXT").goto("TS.l")
    P("TS.u8").a(("LDI", "tb", UNS + 8)).call("NEXT").goto("TS.ll")
    P("TS.ll2").call("NEXT").goto("TS.l")
    P("TS.l").tok({"*": "TS.st"}, "RET")
    P("TS.st").a(("ALUI", "add", "td", "td", 1)).call("NEXT").goto("TS.l")
    # the scale of p +- n: the pointee's size (depth 1: tb; deeper: 8); 1 needs no multiply (measured)
    q = P("SCALE")
    q.branch({2: "SC.8"}, "SC.1", [("CMPI", "lt", 1)])
    P("SC.8").o("  imm r2, 8\n  mul64 r0, r0, r2\n").ret()
    P("SC.1").branch({2: "SC.u"}, "SC.1b", [("CMPI", "lb", UNS)])
    P("SC.u").a(("ALUI", "sub", "lb", "lb", UNS)).goto("SC.1b")
    P("SC.1b").branch({1: "RET"}, "SC.n", [("CMPI", "lb", 1)])
    P("SC.n").o("  imm r2, ").num("lb").o("\n  mul64 r0, r0, r2\n").ret()


def build():
    E.tokenizer()
    E.prn()
    E.numout()
    E.fconv()
    E.autoscan()
    types()
    # ---- declared data 3: the grammar, compiled to procedures ---------------------------
    p = P("START")
    p.a(("LDI", "lab", 0), ("LDI", "vsp", 0), ("SBCLR",), [("SBOUT", c) for c in b"main"], ("SBINTERN", "mnid"),
        ("SBCLR",), [("SBOUT", c) for c in b"printf"], ("SBINTERN", "pfid"), ("SBCLR",), [("SBOUT", c) for c in b"exit"], ("SBINTERN", "exid"), ("MARK", "x0"), ("LDI", "sk", 0))
    # the reference auto-includes a header when one of its functions is called and not defined here
    # (src/front_pp.c autoinc): the old E3's check, reused -- such a unit is not covered
    for k, (nm, _, _) in enumerate(E.SYSCALLS, 1):
        p.a(("SBCLR",), [("SBOUT", c) for c in nm.encode()], ("SBINTERN", "sy%d" % k))
    for k, nm in enumerate(("va_start", "va_arg", "va_end")):
        p.a(("SBCLR",), [("SBOUT", c) for c in nm.encode()], ("SBINTERN", "va%d" % k))
    p.a(("SBCLR",), [("SBOUT", c) for c in b"__argc"], ("SBINTERN", "acid"), ("SBCLR",), [("SBOUT", c) for c in b"__argv"], ("SBINTERN", "avid"))
    for nm in E.autonames():
        p.a(("SBCLR",), [("SBOUT", c) for c in nm.encode()], ("SBINTERN", "t"), ("LDI", "u", 1), ("STX", "t", E.AUT, "u"))
    p.call("AUTO").a(("JUMP", "x0")).o(E.HEADER).call("NEXT").label("UNIT")
    p.tok({**{w: "FN" for w in TWORDS}, "eof": "END", "typedef": "TD", "type=static": "TOP.st", TK_ID: "TOP.id", "struct": "FN"}, bad("top-level construct"))
    P("TOP.st").call("NEXT").goto("UNIT")        # static: the same code (measured)
    P("TOP.id").call("ISTD").branch({1: "FN"}, bad("top-level construct"))
    # typedef T [*]... NAME;  -- no code
    p = P("TD")
    p.call("NEXT").call("TSPEC").tok({TK_ID: "TD.id"}, bad("typedef"))
    P("TD.id").a(("INTERN", "t", "ps", "pe"), ("LDI", "u", 1), ("STX", "t", E.TDN, "u"), ("STX", "t", E.TDB, "tb"), ("STX", "t", E.TDD, "td")).call("NEXT").expect(";").call("NEXT").goto("UNIT")
    # a unit without main is an error in the reference (measured, probe r2)
    P("END").a(("LDX", "t", "mnid", E.FND)).branch({1: "END.ok"}, bad("no main"), [("CMPI", "t", 1)])
    P("END.ok").o("__init:\n").a(("JUMP", "x0"), ("LDI", "dep", 0)).call("INITS").o("  ret\n__main_ret:\n").a(("LDX", "t", "exid", E.FND)).branch({1: "END.ex"}, "END.x2", [("CMPI", "t", 1)])
    P("END.ex").o("  call exit\n").goto("END.x2")     # a unit that defines exit calls it on return from main (measured)
    P("END.x2").o("  .exit r0\n").a(("LDI", "sk", 0), ("JUMP", "x0")).call("POOL").a(("ACCEPT",)).goto("DEAD")
    p = P("INITS")
    p.call("NEXT").label("IN.l")
    p.tok({"eof": "RET", "{": "IN.o", "}": "IN.c", TK_ID: "IN.id"}, "IN.nx")
    P("IN.o").a(("ALUI", "add", "dep", "dep", 1)).goto("IN.nx")
    P("IN.c").a(("ALUI", "sub", "dep", "dep", 1)).goto("IN.nx")
    P("IN.nx").call("NEXT").goto("IN.l")
    P("IN.id").branch({1: "IN.id0"}, "IN.nx", [("CMPI", "dep", 0)])
    P("IN.id0").a(("COPYW", "ips", "ps"), ("COPYW", "ipe", "pe")).call("NEXT").tok({"=": "IN.eq"}, "IN.l")
    P("IN.eq").call("NEXT").tok({TK_NUM: "IN.v"}, "IN.l")
    q = P("IN.v")
    emit(q, "imm").o("  .lea r1, g_").a(("SPAN2", "ips", "ipe")).o("\n").a(("INTERN", "v", "ips", "ipe"), ("LDX", "vt", "v", E.PTR), ("LDX", "vb", "v", E.BASE)).call("STOREV").goto("IN.nx")
    # function: int NAME ( params ) { body }
    p = P("FN")
    p.call("TSPEC").a(("COPYW", "rd", "td"), ("COPYW", "rb", "tb")).tok({TK_ID: "FN.id", ";": "FN.semi", "(": "GV.fp"}, bad("declarator"))
    p = P("GV.fp")
    p.call("FPDECL").a(("COPYW", "fns", "ips"), ("COPYW", "fne", "ipe"), ("LDI", "gsz", 8), ("LDI", "gar", 0)).branch({1: "GV.fp1"}, "GV.fpa", [("CMPI", "fpn", 0)])
    P("GV.fpa").a(("ALUI", "mul", "gsz", "fpn", 8), ("LDI", "gar", 1)).goto("GV.fp1")
    P("GV.fp1").call("GV.emit").goto("GV.end")
    p = P("FN.id")
    p.a(("COPYW", "fns", "ps"), ("COPYW", "fne", "pe")).call("NEXT").tok({"(": "FN.fn", ";": "GV.sc", "=": "GV.sc", "[": "GV.ar"}, bad("declarator"))
    P("FN.semi").call("NEXT").goto("UNIT")      # `struct T { ... };` -- a definition only, no code
    # a global: `.bss g_NAME SIZE` where it is declared; its initialiser goes to __init (measured)
    p = P("GV.sc")
    p.branch({1: "GV.sc0"}, "GV.p8", [("CMPI", "td", 0)])
    P("GV.sc0").branch({1: "DEAD.void"}, "GV.scb", [("CMPI", "tb", 0)])
    P("GV.scb").call("ELSZ").a(("COPYW", "gsz", "es")).goto("GV.reg")
    P("GV.p8").a(("LDI", "gsz", 8)).goto("GV.reg")
    p = P("GV.reg")
    p.a(("LDI", "gar", 0)).call("GV.emit").tok({"=": "GV.init"}, "GV.end")
    P("GV.init").call("NEXT").tok({TK_NUM: "GV.iv"}, bad("global initialiser"))
    P("GV.iv").a(("LDI", "t", 2147483647), ("C64U", "nv", "t")).branch({2: "DEAD.big"}, "GV.iv1")
    P("GV.iv1").call("NEXT").goto("GV.end")
    P("GV.end").expect(";").call("NEXT").goto("UNIT")
    p = P("GV.ar")       # T NAME[N]: N * element size (a pointer element: 8)
    p.call("NEXT").tok({TK_NUM: "GV.an"}, bad("array bound"))
    p = P("GV.an")
    p.call("ELSZ").a(("ALU", "mul", "gsz", "nv", "es"), ("LDI", "gar", 1)).call("NEXT").expect("]").call("NEXT").call("GV.emit").goto("GV.end")
    p = P("GV.emit")
    p.o(".bss g_").a(("SPAN2", "fns", "fne")).o(" ").num("gsz").o("\n")
    p.a(("INTERN", "v", "fns", "fne"), ("LDI", "t", E.GMARK), ("STX", "v", LOC, "t"), ("STX", "v", E.BASE, "tb"), ("STX", "v", E.ARR, "gar"),
        ("ALU", "add", "t", "td", "gar"), ("STX", "v", E.PTR, "t")).ret()
    P("ELSZ").branch({1: "ELSZ.b0"}, "ELSZ.8", [("CMPI", "td", 0)])
    P("ELSZ.b0").branch({(1, 2): "ELSZ.st"}, "ELSZ.b", [("CMPI", "tb", SBB)])
    P("ELSZ.st").a(("ALUI", "sub", "t", "tb", SBB), ("LDX", "es", "t", SSZ)).branch({1: "DEAD.inc"}, "RET", [("CMPI", "es", 0)])
    g.on("DEAD.inc", range(257), "DEAD", E.rej("not covered: incomplete struct"), "r")
    P("ELSZ.b").branch({1: "DEAD.void"}, "ELSZ.s", [("CMPI", "tb", 0)])
    P("ELSZ.s").a(("COPYW", "es", "tb")).branch({1: "ELSZ.d"}, "ELSZ.sf", [("CMPI", "tb", DBL)])
    P("ELSZ.sf").branch({1: "ELSZ.f"}, "ELSZ.s2", [("CMPI", "tb", FLT)])
    P("ELSZ.f").a(("LDI", "es", 4)).ret()
    P("ELSZ.d").a(("LDI", "es", 8)).ret()
    P("ELSZ.s2").branch({2: "ELSZ.u"}, "RET", [("CMPI", "tb", UNS)])
    P("ELSZ.u").a(("ALUI", "sub", "es", "tb", UNS)).ret()
    P("ELSZ.8").a(("LDI", "es", 8)).ret()
    p = P("FN.fn")
    p.a(("INTERN", "v", "fns", "fne"), ("LDI", "t", 1), ("STX", "v", E.FND, "t"), ("STX", "v", E.FRD, "rd"), ("STX", "v", E.FRB, "rb"), ("LDI", "cur", 0), ("LDI", "max", 0), ("LDI", "usp", 0))
    p.call("NEXT").a(("LDI", "pk", 0), ("LDI", "vfn", 0))
    p.tok(dict({")": "FN.body", "type=void": "FN.void", TK_ID: "FN.ptk", "struct": "FN.par"}, **{w: "FN.par" for w in TWORDS if w != "type=void"}), bad("parameter"))
    P("FN.void").call("TSPEC").tok({")": "FN.vend", TK_ID: "FN.pid"}, bad("parameter"))
    P("FN.vend").branch({1: "FN.body"}, bad("parameter"), [("CMPI", "td", 0)])
    P("FN.ptk").call("ISTD").branch({1: "FN.par"}, bad("parameter"))
    p = P("FN.par")
    p.call("TSPEC").tok({TK_ID: "FN.pid", ",": "FN.pn", ")": "FN.body", "(": "FN.pfp"}, bad("parameter"))   # unnamed: a prototype
    P("FN.pfp").call("FPDECL").a(("COPYW", "ps", "ips"), ("COPYW", "pe", "ipe"), ("LDI", "dsz", 8), ("LDI", "dar", 0)).call("DECL").a(("ALUI", "add", "pk", "pk", 1)).tok({",": "FN.pn", ")": "FN.body"}, bad("parameter"))
    P("FN.dots").a(("LDI", "vfn", 1)).call("NEXT").tok({")": "FN.body"}, bad("parameter after ..."))
    p = P("FN.pid")
    p.a(("INTERN", "t", "fns", "fne"), ("ALUI", "mul", "t", "t", 16), ("ALU", "add", "t", "t", "pk"), ("STX", "t", PDB, "tb"))
    p.a(("LDI", "dsz", 8), ("LDI", "dar", 0)).call("DECL")
    p.a(("ALUI", "add", "pk", "pk", 1)).call("NEXT").tok({",": "FN.pn", ")": "FN.body"}, bad("parameter"))
    P("FN.pn").call("NEXT").tok({**{w: "FN.par" for w in TWORDS}, TK_ID: "FN.ptk", "struct": "FN.par", "...": "FN.dots"}, bad("parameter"))
    p = P("FN.body")
    p.call("NEXT").tok({"{": "FN.def", ";": "FN.proto"}, bad("expected {"))
    p = P("FN.proto")     # a prototype: nothing written; its parameters' names are dropped
    p.a(("LDI", "sv", 0)).call("UNWIND").a(("INTERN", "v", "fns", "fne"), ("LDI", "t", 0), ("STX", "v", E.FND, "t")).call("NEXT").goto("UNIT")
    p = P("FN.def")      # the return label is taken here: a prototype takes none (measured)
    p.a(("ALUI", "add", "lab", "lab", 1), ("COPYW", "rl", "lab"))
    emit(p, "fn_head").a(("ORES", "frm", 7)).o("\n").a(("LDI", "sk2", 0)).label("FN.sp")
    p.a(("INTERN", "v", "fns", "fne"), ("STX", "v", E.VAR, "vfn")).branch({0: "FN.sp0"}, "FN.go", [("CMP", "sk2", "pk")])
    P("FN.sp0").branch({1: "FN.spv"}, "FN.sp1", [("CMPI", "vfn", 1)])
    q = P("FN.spv")      # variadic: from the caller's stack (measured): load64 r1, [r6+16+8k]; imm r5, slot; sub64 r5, r6, r5; store64 [r5+0], r1
    q.a(("ALUI", "mul", "t", "sk2", 8), ("ALUI", "add", "t", "t", 16), ("ALUI", "add", "pks", "sk2", 1), ("ALUI", "mul", "pks", "pks", 8))
    q.o("  load64 r1, [r6+").num("t").o("]\n  imm r5, ").num("pks").o("\n  sub64 r5, r6, r5\n  store64 [r5+0], r1\n").a(("ALUI", "add", "sk2", "sk2", 1)).goto("FN.sp")
    q = P("FN.sp1")      # the parameters' spills: store64 [r6-8(k+1)], rk (measured)
    q.a(("ALUI", "add", "pks", "sk2", 1), ("ALUI", "mul", "pks", "pks", 8)).o("  store64 [r6-").num("pks").o("], r").num("sk2").o("\n").a(("ALUI", "add", "sk2", "sk2", 1)).goto("FN.sp")
    p = P("FN.go")
    p.call("NEXT").call("STMTS")
    emit(p, "fn_tail").a(("ALUI", "add", "t", "max", 7), ("ALUI", "and", "t", "t", -8), ("OFILL", "frm", "t", 7), ("LDI", "sv", 0)).call("UNWIND").call("NEXT").goto("UNIT")
    # DECL: the identifier ps..pe becomes the next 8-byte slot (measured: params and int locals)
    # DECLN: the name was saved in ips..ipe (the current token is after it)
    P("DECLN").a(("COPYW", "ps", "ips"), ("COPYW", "pe", "ipe")).goto("DECL")
    p = P("DECL")
    p.a(("INTERN", "v", "ps", "pe"), ("LDX", "t", "v", LOC), ("STX", "usp", E.UNDO, "v"), ("STX", "usp", E.UNDO + 1, "t"),
        ("LDX", "t", "v", E.PTR), ("STX", "usp", E.UNDO + 2, "t"), ("LDX", "t", "v", E.BASE), ("STX", "usp", E.UNDO + 3, "t"), ("ALUI", "add", "usp", "usp", 4),
        ("LDX", "t", "v", E.ARR), ("STX", "usp", E.UNDO + 4, "t"), ("ALUI", "add", "usp", "usp", 1),
        ("ALU", "add", "cur", "cur", "dsz"), ("STX", "v", LOC, "cur"), ("ALU", "add", "t", "td", "dar"), ("STX", "v", E.PTR, "t"), ("STX", "v", E.BASE, "tb"),
        ("STX", "v", E.ARR, "dar"), ("COPYW", "s", "cur")).call("MAXF").ret()
    # the frame is the deepest point reached: a block's slots are reused after it ends (measured, probe p12)
    P("MAXF").branch({2: "MAXF.u"}, "RET", [("CMP", "cur", "max")])
    P("MAXF.u").a(("COPYW", "max", "cur")).ret()
    # statements
    p = P("STMTS")
    p.tok({"}": "RET"}, "STMTS.one")
    P("STMTS.one").call("STMT").goto("STMTS")
    p = P("STMT")
    p.tok({"{": "S.blk", "*": "S.star", **{w: "S.decl" for w in TWORDS}, "return": "S.ret", "if": "S.if", "while": "S.while", "for": "S.for", "do": "S.do", "break": "S.brk", "continue": "S.cnt", ";": "S.empty", TK_ID: "S.idq", "struct": "S.decl"}, "S.expr")
    P("S.idq").call("ISTD").branch({1: "S.decl"}, "S.expr")
    p = P("S.blk")
    p.vpush("usp", "cur").call("NEXT").call("STMTS").vpop("sv", "cur").call("UNWIND").call("NEXT").ret()
    p = P("UNWIND")
    p.label("S.uw")
    p.branch({2: "S.uw1"}, "RET", [("CMP", "usp", "sv")])
    P("S.uw1").a(("ALUI", "sub", "usp", "usp", 5), ("LDX", "v", "usp", E.UNDO), ("LDX", "t", "usp", E.UNDO + 1), ("STX", "v", LOC, "t"),
                  ("LDX", "t", "usp", E.UNDO + 4), ("STX", "v", E.ARR, "t"),
                  ("LDX", "t", "usp", E.UNDO + 2), ("STX", "v", E.PTR, "t"), ("LDX", "t", "usp", E.UNDO + 3), ("STX", "v", E.BASE, "t")).goto("S.uw")

    P("S.empty").call("NEXT").ret()
    p = P("S.decl")
    p.call("TSPEC").tok({TK_ID: "S.did0", "(": "S.dfp"}, bad("declaration"))
    p = P("S.dfp")
    p.call("FPDECL").a(("LDI", "dsz", 8), ("LDI", "dar", 0)).branch({1: "S.dd"}, "S.dfa", [("CMPI", "fpn", 0)])
    P("S.dfa").a(("ALUI", "mul", "dsz", "fpn", 8), ("LDI", "dar", 1)).goto("S.dd")
    P("S.did0").a(("COPYW", "ips", "ps"), ("COPYW", "ipe", "pe")).goto("S.did")
    P("S.did").a(("LDI", "dsz", 8), ("LDI", "dar", 0)).branch({1: "S.dst"}, "S.dnx", [("CMPI", "td", 0)])
    P("S.dst").branch({(1, 2): "S.dst1"}, "S.dnx", [("CMPI", "tb", SBB)])
    P("S.dst1").call("ELSZ").a(("COPYW", "dsz", "es")).goto("S.dnx")
    P("S.dnx").call("NEXT").tok({"[": "S.darr"}, "S.dd")
    P("S.dd").call("DECLN").expect(";").call("NEXT").ret()
    P("S.darr").call("NEXT").tok({TK_NUM: "S.dan"}, bad("array bound"))
    P("S.dan").call("ELSZ").a(("ALU", "mul", "dsz", "nv", "es"), ("LDI", "dar", 1)).call("NEXT").expect("]").call("NEXT").goto("S.dd")
    p = P("S.ret")
    p.call("NEXT").tok({";": "S.rv"}, "S.re")
    P("S.rv").o("  jump R").num("rl").o("\n").call("NEXT").ret()     # return; (measured, old E3)
    p = P("S.re")
    p.call("EXPR").expect(";").a(("COPYW", "vt", "rd"), ("COPYW", "vb", "rb")).call("NARROW")
    p.o("  jump R").num("rl").o("\n").call("NEXT").ret()
    P("S.expr").a(("LDI", "stl", 1)).call("EXPR").expect(";").call("NEXT").ret()
    p = P("S.if")
    p.call("NEXT").expect("(").call("NEXT").call("EXPR").expect(")").a(("ALUI", "add", "lab", "lab", 1), ("COPYW", "a", "lab"))
    emit(p, "jumpz").vpush("a").call("NEXT").call("STMT").vpop("a").tok({"else": "S.else"}, "S.noelse")
    q = P("S.noelse")
    emit(q, "label_a").ret()
    q = P("S.else")
    q.a(("ALUI", "add", "lab", "lab", 1), ("COPYW", "b", "lab"))
    emit(q, "jump_b")
    emit(q, "label_a").vpush("b").call("NEXT").call("STMT").vpop("b")
    emit(q, "label_b").ret()
    p = P("S.while")
    p.a(("ALUI", "add", "lab", "lab", 1), ("COPYW", "a", "lab"), ("ALUI", "add", "lab", "lab", 1), ("COPYW", "b", "lab"))
    emit(p, "label_a").call("NEXT").expect("(").call("NEXT").vpush("a", "b").call("EXPR").vpop("a", "b").expect(")")
    q = p
    q.o("  jumpz r0, L").num("b").o("\n").vpush("a", "b", "lbrk", "lcnt").a(("COPYW", "lbrk", "b"), ("COPYW", "lcnt", "a")).call("NEXT").call("STMT").vpop("a", "b", "lbrk", "lcnt")
    emit(q, "jump_a")
    emit(q, "label_b").ret()
    p = P("S.for")
    p.a(("ALUI", "add", "lab", "lab", 1), ("COPYW", "a", "lab"), ("ALUI", "add", "lab", "lab", 1), ("COPYW", "b", "lab"),
        ("ALUI", "add", "lab", "lab", 1), ("COPYW", "c", "lab")).vpush("a", "b", "c")
    p.call("NEXT").expect("(").call("NEXT").tok({";": "F.i0"}, "F.i1")
    P("F.i1").call("EXPR").expect(";").goto("F.i0")
    p = P("F.i0")       # init done; the condition may be empty (then no jumpz: measured for(;;))
    p.vpop("a", "b", "c")
    emit(p, "label_a").vpush("a", "b", "c").call("NEXT").tok({";": "F.c0"}, "F.c1")
    q = P("F.c1")
    q.call("EXPR").expect(";").vpop("a", "b", "c")
    emit(q, "jumpz_b").goto("F.c2")
    P("F.c0").vpop("a", "b", "c").goto("F.c2")
    p = P("F.c2")
    p.call("NEXT").a(("COPYW", "stp", "tpos"), ("LDI", "dep", 0)).label("F.skip")
    p.tok({"(": "F.open", ")": "F.close"}, "F.nx")
    P("F.open").a(("ALUI", "add", "dep", "dep", 1)).goto("F.nx")
    P("F.close").branch({1: "F.body"}, "F.cl", [("CMPI", "dep", 0)])
    P("F.cl").a(("ALUI", "sub", "dep", "dep", 1)).goto("F.nx")
    P("F.nx").call("NEXT").goto("F.skip")
    p = P("F.body")
    p.vpush("a", "b", "c", "stp", "lbrk", "lcnt").a(("COPYW", "lbrk", "b"), ("COPYW", "lcnt", "c")).call("NEXT").call("STMT").vpop("a", "b", "c", "stp", "lbrk", "lcnt").a(("COPYW", "aft", "tpos"))
    emit(p, "label_c").vpush("a", "b", "aft").a(("JUMP", "stp")).call("NEXT").tok({")": "F.s0"}, "F.s1")
    P("F.s1").call("EXPR").expect(")").goto("F.s0")
    p = P("F.s0")
    p.vpop("a", "b", "aft")
    emit(p, "jump_a")
    emit(p, "label_b").a(("JUMP", "aft")).call("NEXT").ret()
    # do body while (cond);  labels a top, b break, c continue (measured)
    p = P("S.do")
    p.a(("ALUI", "add", "lab", "lab", 1), ("COPYW", "a", "lab"), ("ALUI", "add", "lab", "lab", 1), ("COPYW", "b", "lab"),
        ("ALUI", "add", "lab", "lab", 1), ("COPYW", "c", "lab"))
    emit(p, "label_a").vpush("a", "b", "c", "lbrk", "lcnt").a(("COPYW", "lbrk", "b"), ("COPYW", "lcnt", "c")).call("NEXT").call("STMT").vpop("a", "b", "c", "lbrk", "lcnt")
    emit(p, "label_c").expect("while").call("NEXT").expect("(").vpush("a", "b").call("NEXT").call("EXPR").vpop("a", "b").expect(")")
    emit(p, "jumpz_b")
    emit(p, "jump_a")
    emit(p, "label_b").call("NEXT").expect(";").call("NEXT").ret()
    for nm, slot in (("S.brk", "lbrk"), ("S.cnt", "lcnt")):
        q = P(nm)        # the innermost loop's label; outside a loop: not covered
        q.branch({1: "DEAD.nl2"}, nm + ".ok", [("CMPI", slot, 0)])
        P(nm + ".ok").o("  jump L").num(slot).o("\n").call("NEXT").expect(";").call("NEXT").ret()
    g.on("DEAD.nl2", range(257), "DEAD", E.rej("not covered: break/continue outside a loop"), "r")
    # expressions: EXPR = assignment | the ladder
    p = P("EXPR")     # st1: this EXPR is a whole expression statement (nested ones are not)
    p.a(("COPYW", "st1", "stl"), ("LDI", "stl", 0)).tok({TK_ID: "X.id"}, "EX.l")
    P("EX.l").call("E%d" % LEVELS[0]).call("QTAIL").ret()
    # c ? a : b -- labels as if/else (measured): jumpz L a; a; jump L b; L a: b; L b:
    P("QTAIL").tok({"?": "QT"}, "RET")
    p = P("QT")
    p.a(("ALUI", "add", "lab", "lab", 1), ("COPYW", "a", "lab"), ("ALUI", "add", "lab", "lab", 1), ("COPYW", "b", "lab"))
    emit(p, "jumpz").vpush("a", "b").call("NEXT").call("EXPR").vpop("a", "b").expect(":").vpush("vt", "vb")
    emit(p, "jump_b")
    emit(p, "label_a").vpush("b").call("NEXT").call("E%d" % LEVELS[0]).call("QTAIL").vpop("b").vpop("lt", "lb")
    emit(p, "label_b").branch({1: "QT.1"}, "DEAD.qt", [("CMP", "vt", "lt")])
    P("QT.1").branch({1: "RET"}, "QT.2", [("CMP", "vb", "lb")])
    P("QT.2").branch({1: "QT.3"}, "DEAD.qt", [("CMPI", "vt", 0)])          # two integer arms of different sizes
    P("QT.3").a(("LDI", "t", 0)).call("QT.int").a(("COPYW", "t2", "t"), ("COPYW", "x", "vb"), ("COPYW", "vb", "lb")).call("QT.int").a(("COPYW", "vb", "x"), ("LDI", "vb", 8)).ret()
    p = P("QT.int")      # t += 1 when vb is a signed int or long (the pair must be those)
    p.branch({1: "QT.i1"}, "QT.i8", [("CMPI", "vb", 4)])
    P("QT.i8").branch({1: "QT.i1"}, "DEAD.qt", [("CMPI", "vb", 8)])
    P("QT.i1").a(("ALUI", "add", "t", "t", 1)).ret()
    g.on("DEAD.qt", range(257), "DEAD", E.rej("not covered: ?: arms of different types"), "r")
    P("X.id").a(("COPYW", "ips", "ps"), ("COPYW", "ipe", "pe")).call("NEXT").tok(dict({"=": "X.as", "(": "X.cpf", "++": "X.inc", "--": "X.dec"}, **{o + "=": "X.c" + o for o in E.CASOPS}), "X.var")
    p = P("X.as")
    p.call("LOOKUP").call("NOARR")
    addr(p)
    emit(p, "push").vpush("vt", "vb").call("NEXT").call("EXPR").a(("COPYW", "rvb", "vb"), ("COPYW", "rvt", "vt")).vpop("vt", "vb").call("SAMEDBL")
    emit(p, "pop1").call("STOREV").ret()
    # = between a double and an integer needs a conversion (cvtid/cvtdi): not covered yet
    P("SAMEDBL").a(("LDI", "u", 0)).branch({1: "SD.l"}, "SD.r", [("CMPI", "vb", DBL)])
    P("SD.l").branch({1: "SD.l2"}, "SD.r", [("CMPI", "vt", 0)])
    P("SD.l2").a(("LDI", "u", 1)).goto("SD.r")
    P("SD.r").a(("LDI", "w", 0)).branch({1: "SD.r1"}, "SD.c", [("CMPI", "rvb", DBL)])
    P("SD.r1").branch({1: "SD.r2"}, "SD.c", [("CMPI", "rvt", 0)])
    P("SD.r2").a(("LDI", "w", 1)).goto("SD.c")
    P("SD.c").branch({1: "RET"}, "SD.cv", [("CMP", "u", "w")])
    P("SD.cv").branch({1: "SD.cv1"}, "DEAD.dbl", [("CMPI", "u", 1)])
    P("SD.cv1").branch({1: "SD.cv2"}, "DEAD.dbl", [("CMPI", "rvt", 0)])
    P("SD.cv2").o("  cvtid r0, r0\n").ret()
    for o in E.CASOPS:
        q = P("X.c" + o)    # addr; push; load; push; rhs; pop; op; pop; store
        q.call("LOOKUP").call("NOARR").call("INTONLY")
        addr(q)
        emit(q, "push")
        emit(q, "load_int")
        emit(q, "push").call("NEXT").call("EXPR")
        emit(q, "pop1").o(E.optext(o))
        emit(q, "pop1")
        emit(q, "store_int").ret()
    P("ISDV").a(("LDI", "u", 0)).branch({1: "ISDV.1"}, "ISDV.x", [("CMPI", "vb", DBL)])
    P("ISDV.1").branch({1: "ISDV.y"}, "ISDV.x", [("CMPI", "vt", 0)])
    P("ISDV.y").a(("LDI", "u", 1)).goto("ISDV.x")
    P("ISDV.x").a(("CMPI", "u", 1)).ret()
    P("NODBL").branch({1: "NODBL.l"}, "NODBL.r", [("CMPI", "lb", DBL)])      # a double VALUE (not a pointer to one)
    P("NODBL.l").branch({1: "DEAD.dbl"}, "NODBL.r", [("CMPI", "lt", 0)])
    P("NODBL.r").branch({1: "NODBL.r2"}, "RET", [("CMPI", "vb", DBL)])
    P("NODBL.r2").branch({1: "DEAD.dbl"}, "RET", [("CMPI", "vt", 0)])
    g.on("DEAD.dbl", range(257), "DEAD", E.rej("not covered: double operand"), "r")
    P("INTONLY").branch({1: "IO.b"}, bad("pointer or non-int in op= ++ --"), [("CMPI", "vt", 0)])
    P("IO.b").branch({1: "RET"}, bad("pointer or non-int in op= ++ --"), [("CMPI", "vb", 4)])
    for nm, o, fix in (("X.inc", "+", "post_inc"), ("X.dec", "-", "post_dec")):
        q = P(nm)           # addr; push; load; push; 1; pop; op; pop; store; undo to the old value
        q.call("LOOKUP").call("NOARR").call("INTONLY")
        addr(q)
        emit(q, "push")
        emit(q, "load_int")
        emit(q, "push")
        emit(q, "one")
        emit(q, "pop1").o(E.optext(o))
        emit(q, "pop1")
        emit(q, "store_int")
        emit(q, fix).call("NEXT").call("C%d" % LEVELS[0]).call("QTAIL").ret()
    p = P("X.var")      # an identifier operand, then the rest of the ladder with it as the left operand
    p.a(("LDI", "isfn", 0)).call("FNVAL").branch({1: "X.fnv"}, "X.v2", [("CMPI", "isfn", 1)])
    P("X.fnv").call("POSTIX").call("C%d" % LEVELS[0]).call("QTAIL").ret()
    p = P("X.v2")
    p.call("LOOKUP")
    addr(p)
    p.tok({".": "X.mb"}, "X.vl")
    P("X.vl").call("VLOAD").call("POSTIX").call("C%d" % LEVELS[0]).call("QTAIL").ret()
    P("X.mb").call("MEMB").call("C%d" % LEVELS[0]).call("QTAIL").ret()
    P("X.cpf").a(("INTERN", "v", "ips", "ipe")).branch({1: "X.pf"}, "X.call", [("CMP", "v", "pfid")])
    P("X.pf").call("PF").call("C%d" % LEVELS[0]).call("QTAIL").ret()
    P("X.call").call("CALL").call("C%d" % LEVELS[0]).call("QTAIL").ret()
    ladder("E", "UNARY")
    ladder("C", None)
    P("U.str").o("  .lea r0, S").num("sk").o("\n").a(("ALUI", "add", "sk", "sk", 1), ("ALUI", "add", "lab", "lab", 1), ("LDI", "vt", 1), ("LDI", "vb", 1)).call("NEXT").tok({E.TK_STR: "DEAD.adj"}, "RET")
    g.on("DEAD.adj", range(257), "DEAD", E.rej("not covered: adjacent string literals"), "r")
    p = P("UNARY")
    q = P("U.cpl")       # ~x: imm r1, -1; xor64 (measured); the operand's type is kept
    q.call("NEXT").call("UNARY").call("NODBL0").o("  imm r1, -1\n  xor64 r0, r0, r1\n").ret()
    P("NODBL0").branch({1: "NODBL0.1"}, "NODBL0.p", [("CMPI", "vb", DBL)])
    P("NODBL0.1").branch({1: "DEAD.dbl"}, "NODBL0.p", [("CMPI", "vt", 0)])
    P("NODBL0.p").branch({1: "RET"}, "DEAD.pa", [("CMPI", "vt", 0)])
    p = P("UNARY")
    P("U.fnum").o("  imm r0, ").a(("LDI", "nx", 1)).call("NUMOUT").o("\n").a(("LDI", "vt", 0), ("LDI", "vb", DBL)).call("NEXT").ret()
    p = P("UNARY")
    p.tok({E.TK_FNUM: "U.fnum", E.TK_STR: "U.str", "~": "U.cpl", "-": "U.neg", "!": "U.not", "(": "U.par", TK_NUM: "U.num", TK_ID: "U.id", "++": "U.pinc", "--": "U.pdec", "&": "U.amp", "*": "U.deref"}, bad("expression"))
    P("U.amp").call("NEXT").tok({TK_ID: "U.amp1"}, bad("address of"))
    q = P("U.amp1")
    q.a(("COPYW", "ips", "ps"), ("COPYW", "ipe", "pe")).call("LOOKUP")
    addr(q).a(("ALUI", "add", "vt", "vt", 1)).call("NEXT").ret()
    q = P("U.deref")     # * operand: its value is the address; one level down, then a load at the new width
    q.call("NEXT").tok({TK_ID: "UD.id"}, "UD.gen")
    P("UD.gen").call("UNARY").goto("UD.dn")
    P("UD.dn").call("DOWN").branch({1: "RET"}, "LOADV", [("CMPI", "vb", FPB)])
    P("UD.id").a(("COPYW", "ips", "ps"), ("COPYW", "ipe", "pe")).call("NEXT").tok({"(": "UD.call"}, "UD.v")
    P("UD.call").call("CALL").goto("UD.dn")
    p = P("UD.v")        # *f with f a local function pointer: `load64 r0, [r6-N]` as for a callee (measured, p70)
    p.a(("LDI", "isfn", 0)).call("FNVAL").branch({1: "RET"}, "UD.v2", [("CMPI", "isfn", 1)])
    p = P("UD.v2")
    p.call("LOOKUP").branch({1: "UD.f1"}, "UD.gv", [("CMPI", "vb", FPB)])
    P("UD.f1").branch({1: "UD.gv"}, "UD.f2", [("CMPI", "s", E.GMARK)])
    P("UD.f2").branch({1: "UD.gv"}, "UD.f3", [("CMPI", "ar", 1)])
    P("UD.f3").o("  load64 r0, [r6-").num("s").o("]\n").ret()
    p = P("UD.gv")
    addr(p)
    p.call("VLOAD").call("POSTIX").goto("UD.dn")
    P("DOWN").branch({1: "DOWN.fp"}, "DOWN.0", [("CMPI", "vb", FPB)])
    P("DOWN.fp").ret()
    P("DOWN.0").branch({(1, 2): "DOWN.1"}, bad("dereference of a non-pointer"), [("CMPI", "vt", 1)])
    P("DOWN.1").a(("ALUI", "sub", "vt", "vt", 1)).branch({1: "DOWN.2"}, "RET", [("CMPI", "vt", 0)])
    P("DOWN.2").branch({1: "DEAD.void"}, "RET", [("CMPI", "vb", 0)])
    g.on("DEAD.void", range(257), "DEAD", E.rej("not covered: dereference of void"), "r")
    for nm, fix in (("U.pinc", "pre_inc"), ("U.pdec", "pre_dec")):
        q = P(nm)           # addr; push; load; +-1; pop; store
        q.call("NEXT").tok({TK_ID: nm + ".id"}, bad("expression"))
        q = P(nm + ".id")
        q.a(("COPYW", "ips", "ps"), ("COPYW", "ipe", "pe")).call("LOOKUP").call("NOARR").call("INTONLY")
        addr(q)
        emit(q, "push")
        emit(q, "load_int")
        emit(q, fix)
        emit(q, "pop1")
        emit(q, "store_int").call("NEXT").ret()
    q = P("U.neg")
    q.call("NEXT").call("UNARY").call("INTONLY")
    emit(q, "neg").ret()
    q = P("U.not")
    q.call("NEXT").call("UNARY")
    emit(q, "not").a(("LDI", "vt", 0), ("LDI", "vb", 4)).ret()
    P("U.par").call("NEXT").tok({**{w: "U.cast" for w in TWORDS}, TK_ID: "U.pq", "struct": "U.cast"}, "U.pe")
    P("U.pq").call("ISTD").branch({1: "U.cast"}, "U.pe")
    P("U.pe").call("EXPR").expect(")").call("NEXT").call("POSTIX").ret()
    q = P("U.cast")      # (T) e: narrowed through the stack to T; long and pointers: no code (measured)
    q.call("TSPEC").expect(")").vpush("td", "tb").call("NEXT").call("UNARY").call("ISDV").a(("COPYW", "sdv", "u")).vpop("vt", "vb").call("ISDV")
    q.a(("COPYW", "tdv", "u")).branch({1: "UC.flt"}, "UC.nf", [("CMPI", "vb", FLT)])
    P("UC.flt").branch({1: "UC.flt1"}, "DEAD.dbl", [("CMPI", "sdv", 1)])
    P("UC.flt1").branch({1: "UC.flt2"}, "DEAD.dbl", [("CMPI", "vt", 0)])
    P("UC.flt2").o("  cvtds r0, r0\n").ret()
    P("UC.nf").branch({1: "UC.same"}, "UC.cv", [("CMP", "sdv", "tdv")])
    P("UC.same").call("NARROW").ret()
    P("UC.cv").branch({1: "UC.id"}, "UC.di", [("CMPI", "tdv", 1)])
    P("UC.id").branch({1: "UC.id1"}, "DEAD.dbl", [("CMPI", "vt", 0)])
    P("UC.id1").o("  cvtid r0, r0\n").ret()
    P("UC.di").branch({1: "UC.di1"}, "DEAD.dbl", [("CMPI", "vt", 0)])
    P("UC.di1").o("  cvtdi r0, r0\n").call("NARROW").ret()
    q = P("U.num")       # an int constant; beyond int: long when it fits (printed as NUMOUT does), unsigned kinds not covered
    q.a(("LDI", "t", 2147483647), ("C64U", "nv", "t")).branch({2: "U.big"}, "U.num1")
    q = P("U.big")       # decimal beyond int: long; hex/octal: unsigned int up to 0xffffffff (not covered), then long up to LONG_MAX
    q.branch({1: "U.bh"}, "U.bl", [("CMPI", "nx", 1)])
    P("U.bh").a(("LDI", "t", 4294967295), ("C64U", "nv", "t")).branch({2: "U.bl"}, "DEAD.big")
    P("U.bl").a(("LDI", "t", 0)).a(("C64", "nv", "t")).branch({0: "DEAD.big"}, "U.long")
    P("U.long").o("  imm r0, ").call("NUMOUT").o("\n").a(("LDI", "vt", 0), ("LDI", "vb", 8)).call("NEXT").ret()
    g.on("DEAD.big", range(257), "DEAD", E.rej("not covered: constant beyond int"), "r")
    q = P("U.num1")
    emit(q, "imm").a(("LDI", "vt", 0), ("LDI", "vb", 4)).call("NEXT").ret()
    P("U.id").a(("COPYW", "ips", "ps"), ("COPYW", "ipe", "pe")).call("NEXT").tok({"(": "U.call"}, "U.var0")
    P("U.call").call("CALL").ret()
    printf()
    q = P("U.var")
    q.call("LOOKUP")
    addr(q)
    q.tok({".": "MEMB"}, "U.vl")
    q = P("U.var0")
    q.a(("LDI", "isfn", 0)).call("FNVAL").branch({1: "POSTIX"}, "U.var", [("CMPI", "isfn", 1)])
    P("U.vl").call("VLOAD").call("POSTIX").ret()
    P("VLOAD").branch({1: "RET"}, "LOADV", [("CMPI", "ar", 1)])
    P("NOARR").branch({1: "DEAD.arr"}, "RET", [("CMPI", "ar", 1)])
    g.on("DEAD.arr", range(257), "DEAD", E.rej("not covered: assignment to an array"), "r")
    p = P("MEMB")         # current '.' (r0 = a struct's address) or '->' (r0 = a pointer to one)
    p.tok({".": "MB.dot", "->": "MB.arw"}, "RET")
    P("MB.dot").branch({1: "MB.d1"}, "DEAD.mb", [("CMPI", "vt", 0)])
    P("MB.arw").branch({1: "MB.d1"}, "DEAD.mb", [("CMPI", "vt", 1)])
    P("MB.d1").branch({(1, 2): "MB.ok"}, "DEAD.mb", [("CMPI", "vb", SBB)])
    g.on("DEAD.mb", range(257), "DEAD", E.rej("not covered: member access"), "r")
    p = P("MB.ok")
    p.a(("ALUI", "sub", "sid", "vb", SBB)).call("NEXT").tok({TK_ID: "MB.nm"}, bad("member access"))
    p = P("MB.nm")
    p.a(("INTERN", "v", "ps", "pe"), ("ALUI", "mul", "k", "v", 64), ("ALU", "add", "k", "k", "sid"),
        ("LDX", "mo", "k", MOF), ("LDX", "ms", "k", MSZ), ("LDX", "vt", "k", MPT), ("LDX", "vb", "k", MBS)).branch({1: "DEAD.mb"}, "MB.has", [("CMPI", "ms", 0)])
    P("MB.has").branch({1: "MB.z"}, "MB.off", [("CMPI", "mo", 0)])
    P("MB.off").o("  imm r2, ").num("mo").o("\n  add64 r0, r0, r2\n").goto("MB.z")
    P("MB.z").call("NEXT").tok({"=": "PX.as", "->": "MB.ptr"}, "MB.ld")
    P("MB.ptr").call("LOADV").goto("MEMB")
    P("MB.ld").call("LOADV").goto("POSTIX")
    p = P("POSTIX")
    p.tok({"[": "PX.i", "->": "MEMB", "(": "PX.fc"}, "RET")
    P("PX.fc").branch({1: "PX.fc1"}, "RET", [("CMPI", "vb", FPB)])
    P("PX.fc1").call("FPCALL").goto("POSTIX")
    q = P("PX.i")
    q.branch({(1, 2): "PX.ok"}, bad("subscript of a non-pointer"), [("CMPI", "vt", 1)])
    q = P("PX.ok")
    emit(q, "push").vpush("vt", "vb", "st1").call("NEXT").call("EXPR").expect("]").vpop("lt", "lb", "st1").call("SCALE")
    emit(q, "pop1").o("  add64 r0, r1, r0\n").a(("COPYW", "vt", "lt"), ("COPYW", "vb", "lb")).call("DOWN").call("NEXT").tok({"=": "PX.as"}, "PX.ld")
    # a statement that is only `p[i];` computes the address and stops (measured, probe p39)
    P("PX.ld").branch({1: "PX.st"}, "PX.l2", [("CMPI", "st1", 1)])
    P("PX.st").tok({";": "RET"}, "PX.l2")
    P("PX.l2").call("LOADV").goto("POSTIX")
    q = P("PX.as")
    emit(q, "push").vpush("vt", "vb").call("NEXT").call("EXPR").vpop("vt", "vb")
    emit(q, "pop1").call("STOREV").ret()
    # statement `*E = e` / `*E ...;`: E's value is the address
    q = P("S.star")
    q.call("NEXT").call("UNARY").call("DOWN").tok({"=": "SS.as"}, "SS.rv")
    q = P("SS.as")
    emit(q, "push").vpush("vt", "vb").call("NEXT").call("EXPR").vpop("vt", "vb")
    emit(q, "pop1").call("STOREV").expect(";").call("NEXT").ret()
    P("SS.rv").call("LOADV").call("C%d" % LEVELS[0]).expect(";").call("NEXT").ret()
    p = P("FNVAL")
    p.a(("INTERN", "v", "ips", "ipe"), ("LDX", "t", "v", LOC)).branch({1: "FNV.f"}, "RET", [("CMPI", "t", 0)])
    P("FNV.f").a(("LDX", "t", "v", E.FND)).branch({1: "FNV.y"}, "RET", [("CMPI", "t", 1)])
    P("FNV.y").o("  .lea r0, ").a(("SPAN2", "ips", "ipe")).o("\n").a(("LDI", "vt", 1), ("LDI", "vb", FPB), ("LDI", "isfn", 1)).ret()
    p = P("LOOKUP")     # s := the slot of ips..ipe (0: not a local of this slice)
    p.a(("INTERN", "v", "ips", "ipe"), ("LDX", "s", "v", LOC), ("LDX", "vt", "v", E.PTR), ("LDX", "vb", "v", E.BASE), ("LDX", "ar", "v", E.ARR)).branch({1: "DEAD.nl"}, "RET", [("CMPI", "s", 0)])
    g.on("DEAD.nl", range(257), "DEAD", E.rej("not covered: identifier is not a local"), "r")
    # CALL: at '(' after ips..ipe: arguments pushed left to right, popped into r(n-1)..r0, call
    p = P("CALL")
    # the callee must be defined above (the reference rejects a call to an undefined function: probe r1)
    # syscall builtins (the old E3's declared table E.SYSCALLS), __argc(), __argv(k) -- measured there
    p.a(("INTERN", "v", "ips", "ipe"), ("LDI", "sys", 0), ("LDX", "t", "v", LOC)).branch({1: "CL.va0"}, "CL.fpv", [("CMPI", "t", 0)])
    p = P("CL.fpv")      # the callee's value: a local's `load64 r0, [r6-N]`, a global's .lea + load64 (measured)
    p.call("LOOKUP").branch({1: "CL.fpv1"}, bad("call through a non-function"), [("CMPI", "vb", FPB)])
    q = P("CL.fpv1")
    q.branch({1: "CL.fpg"}, "CL.fpl", [("CMPI", "s", E.GMARK)])
    P("CL.fpl").o("  load64 r0, [r6-").num("s").o("]\n").goto("FPCALL")
    q = P("CL.fpg")
    emit(q, "gaddr").o("  load64 r0, [r0+0]\n").goto("FPCALL")
    p = P("FPCALL")      # r0 = the callee; current '(' -- pushed first, then the arguments; callr r5 (measured)
    emit(p, "push").vpush("cls", "cle").a(("LDI", "sys", 100), ("LDI", "fid", 0)).vpush("sys").a(("LDI", "na", 0)).call("NEXT").tok({")": "CL.done"}, "CL.arg")
    for k, nx in ((0, "CL.va1"), (1, "CL.va2"), (2, "CL.b1")):
        P("CL.va%d" % k).branch({1: "VA%d" % k}, nx, [("CMP", "v", "va%d" % k)])
    # va_start(ap, last): &ap pushed; last evaluated (unused); ap = r6 + 16 + 8 * named parameters; value 0 (measured)
    p = P("VA0")
    p.call("NEXT").tok({TK_ID: "VA0.ap"}, bad("va_start"))
    q = P("VA0.ap")
    q.a(("COPYW", "ips", "ps"), ("COPYW", "ipe", "pe")).call("LOOKUP")
    addr(q)
    emit(q, "push").call("NEXT").expect(",").call("NEXT").call("EXPR").expect(")").a(("ALUI", "mul", "t", "pk", 8), ("ALUI", "add", "t", "t", 16))
    q.o("  imm r0, ").num("t").o("\n  add64 r0, r6, r0\n")
    emit(q, "pop1").o("  store64 [r1+0], r0\n  imm r0, 0\n").a(("LDI", "vt", 0), ("LDI", "vb", 4)).call("NEXT").ret()
    # va_arg(ap, T): &ap and ap pushed; ap + 8 stored; the old ap loaded at T's width (measured)
    p = P("VA1")
    p.call("NEXT").tok({TK_ID: "VA1.ap"}, bad("va_arg"))
    q = P("VA1.ap")
    q.a(("COPYW", "ips", "ps"), ("COPYW", "ipe", "pe")).call("LOOKUP")
    addr(q)
    emit(q, "push").o("  load64 r0, [r0+0]\n")
    emit(q, "push").o("  imm r2, 8\n  add64 r0, r0, r2\n  load64 r1, [r7+8]\n  store64 [r1+0], r0\n  load64 r1, [r7+0]\n  .frame -8\n  .frame -8\n  mov r0, r1\n")
    q.call("NEXT").expect(",").call("NEXT").call("TSPEC").expect(")").a(("COPYW", "vt", "td"), ("COPYW", "vb", "tb")).call("LOADRAW").call("NEXT").ret()
    # va_end(ap): ap evaluated; value 0
    p = P("VA2")
    p.call("NEXT").call("EXPR").expect(")").o("  imm r0, 0\n").a(("LDI", "vt", 0), ("LDI", "vb", 4)).call("NEXT").ret()
    for k in range(1, len(E.SYSCALLS) + 1):
        P("CL.b%d" % k).branch({1: "CL.s%d" % k}, "CL.b%d" % (k + 1), [("CMP", "v", "sy%d" % k)])
        P("CL.s%d" % k).a(("LDI", "sys", k)).goto("CL.ok")
    P("CL.b%d" % (len(E.SYSCALLS) + 1)).branch({1: "CL.ac"}, "CL.av0", [("CMP", "v", "acid")])
    P("CL.ac").call("NEXT").expect(")").o("  .argc r0\n").a(("LDI", "vt", 0), ("LDI", "vb", 4)).call("NEXT").ret()
    P("CL.av0").branch({1: "CL.av"}, "CL.def", [("CMP", "v", "avid")])
    P("CL.av").call("NEXT").call("EXPR").expect(")").o("  .argv r0, r0\n").a(("LDI", "vt", 1), ("LDI", "vb", 1)).call("NEXT").ret()
    P("CL.def").a(("LDX", "t", "v", E.FND)).branch({1: "CL.def1"}, bad("call to a function not defined before"), [("CMPI", "t", 1)])
    P("CL.def1").a(("LDX", "t", "v", E.VAR)).branch({1: "DEAD.vc"}, "CL.ok", [("CMPI", "t", 1)])
    g.on("DEAD.vc", range(257), "DEAD", E.rej("not covered: call to a variadic function"), "r")
    p = P("CL.ok")
    p.a(("COPYW", "cls", "ips"), ("COPYW", "cle", "ipe"), ("INTERN", "fid", "ips", "ipe")).vpush("cls", "cle", "sys").a(("LDI", "na", 0)).call("NEXT").tok({")": "CL.done"}, "CL.arg")
    p = P("CL.arg")
    p.vpush("na", "fid").call("EXPR").vpop("na", "fid")
    p.a(("ALUI", "mul", "t", "fid", 16), ("ALU", "add", "t", "t", "na"), ("LDX", "t", "t", PDB)).branch({1: "CL.ad"}, "CL.a2", [("CMPI", "t", DBL)])
    P("CL.ad").call("ISDV").branch({1: "CL.a2"}, "CL.ad1", [("CMPI", "u", 1)])
    P("CL.ad1").branch({1: "CL.ad2"}, "DEAD.dbl", [("CMPI", "vt", 0)])
    P("CL.ad2").o("  cvtid r0, r0\n").goto("CL.a2")
    p = P("CL.a2")
    emit(p, "push").a(("ALUI", "add", "na", "na", 1)).tok({",": "CL.more", ")": "CL.done"}, bad("argument list"))
    P("CL.more").call("NEXT").goto("CL.arg")
    p = P("CL.done")
    p.a(("COPYW", "nar", "na")).branch({2: "DEAD.na"}, "CL.pop", [("CMPI", "na", 6)])
    g.on("DEAD.na", range(257), "DEAD", E.rej("not covered: more than 6 arguments"), "r")
    p = P("CL.pop")
    p.branch({1: "CL.emit"}, "CL.p1", [("CMPI", "na", 0)])
    p = P("CL.p1")
    p.a(("ALUI", "sub", "na", "na", 1), ("COPYW", "ak", "na"))
    emit(p, "pop_arg").goto("CL.pop")
    p = P("CL.emit")
    p.vpop("cls", "cle", "sys").branch({1: "CL.call"}, "CL.s100", [("CMPI", "sys", 0)])
    P("CL.s100").branch({1: "CL.callr"}, "CL.sysz", [("CMPI", "sys", 100)])
    P("CL.callr").o("  load64 r5, [r7+0]\n  .frame -8\n  callr r5\n").a(("LDI", "vt", 0), ("LDI", "vb", 4)).call("NEXT").ret()
    # a syscall: r(n)..r(w-1) zeroed, then `.sys NAME, r0, r1, r2` (w 3) or `.sys6 NAME, r0..r5`
    for k, (_, sc, w) in enumerate(E.SYSCALLS, 1):
        nx = "CL.w%d" % (k + 1) if k < len(E.SYSCALLS) else "DEAD"
        P("CL.sysz" if k == 1 else "CL.w%d" % k).branch({1: "CL.y%d" % k}, nx, [("CMPI", "sys", k)])
        q = P("CL.y%d" % k)
        q.a(("LDI", "w", w)).label("CL.z%d" % k)
        q.branch({0: "CL.zz%d" % k}, "CL.x%d" % k, [("CMP", "nar", "w")])
        P("CL.zz%d" % k).o("  imm r").num("nar").o(", 0\n").a(("ALUI", "add", "nar", "nar", 1)).goto("CL.z%d" % k)
        regs = ", ".join("r%d" % i for i in range(w))
        P("CL.x%d" % k).o("  .sys%s %s, %s\n" % ("6" if w == 6 else "", sc, regs)).a(("LDI", "vt", 0), ("LDI", "vb", 0)).call("NEXT").ret()
    p = P("CL.call")
    emit(p, "call").a(("INTERN", "v", "cls", "cle"), ("LDX", "vt", "v", E.FRD), ("LDX", "vb", "v", E.FRB)).call("NEXT").ret()
    g.finish()
    states = {n: [m, {str(k): v for k, v in row.items()}] for n, (m, row) in g.st.items()}
    return {"start": "START", "states": states, "seqs": [list(map(list, s)) for s in g.seqs]}


if __name__ == "__main__":
    d = build()
    s = json.dumps(d, separators=(",", ":"))
    open(sys.argv[1], "w").write(s)
    st, ent, live, ns, na = E.sizes(d)
    sys.stderr.write("states %d  entries %d (not 'unreachable' %d)  action seqs %d (%d actions)  json %d B\n"
                     % (st, ent, live, ns, na, len(s)))
