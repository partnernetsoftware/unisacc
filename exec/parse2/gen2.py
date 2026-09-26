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
}
SPANS = {"@name": ("fns", "fne"), "@callee": ("cls", "cle")}


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
        for o in OPS[lv]:
            q = P("%s.%s" % (nm, o))
            nxt = "E%d" % LEVELS[i + 1] if i + 1 < len(LEVELS) else "UNARY"
            if o == "&&":
                q.a(("ALUI", "add", "lab", "lab", 1), ("COPYW", "e", "lab"))
                emit(q, "and_skip").vpush("e").call("NEXT").call(nxt).vpop("e")
                emit(q, "bool")
                emit(q, "label_e").goto(nm + ".l")
                continue
            if o == "||":
                q.a(("ALUI", "add", "lab", "lab", 1), ("COPYW", "od", "lab"), ("ALUI", "add", "lab", "lab", 1), ("COPYW", "on", "lab"))
                emit(q, "or_skip").vpush("od").call("NEXT").call(nxt).vpop("od")
                emit(q, "bool")
                emit(q, "label_d").goto(nm + ".l")
                continue
            # left in r0: push; the right operand at the next level; pop; the instruction (binsel -> irsel)
            emit(q, "push").call("NEXT").call("E%d" % LEVELS[i + 1] if i + 1 < len(LEVELS) else "UNARY")
            emit(q, "pop1").o(E.optext(o)).goto(nm + ".l")
    g.on("DEAD.short", range(257), "DEAD", E.rej("not covered: && ||"), "r")


def build():
    E.tokenizer()
    E.prn()
    E.numout()
    E.fconv()
    # ---- declared data 3: the grammar, compiled to procedures ---------------------------
    p = P("START")
    p.a(("LDI", "lab", 0), ("LDI", "vsp", 0), ("SBCLR",), [("SBOUT", c) for c in b"main"], ("SBINTERN", "mnid")).o(E.HEADER).call("NEXT").label("UNIT")
    p.tok({"type": "FN", "eof": "END"}, bad("top-level construct"))
    # a unit without main is an error in the reference (measured, probe r2)
    P("END").a(("LDX", "t", "mnid", E.FND)).branch({1: "END.ok"}, bad("no main"), [("CMPI", "t", 1)])
    P("END.ok").o(E.FOOTER).a(("ACCEPT",)).goto("DEAD")
    # function: int NAME ( params ) { body }
    p = P("FN")
    p.call("NEXT").tok({TK_ID: "FN.id"}, bad("declarator"))
    p = P("FN.id")
    p.a(("COPYW", "fns", "ps"), ("COPYW", "fne", "pe"), ("INTERN", "v", "ps", "pe"), ("LDI", "t", 1), ("STX", "v", E.FND, "t"), ("LDI", "cur", 0), ("ALUI", "add", "lab", "lab", 1), ("COPYW", "rl", "lab"))
    emit(p, "fn_head").a(("ORES", "frm", 7)).o("\n").call("NEXT").expect("(").call("NEXT").a(("LDI", "pk", 0))
    p.tok({")": "FN.body", "type": "FN.par", "type=void": "FN.void"}, bad("parameter"))
    P("FN.void").call("NEXT").tok({")": "FN.body"}, bad("parameter"))
    p = P("FN.par")
    p.call("NEXT").tok({TK_ID: "FN.pid"}, bad("parameter"))
    p = P("FN.pid")
    p.call("DECL")
    emit(p, "spill").a(("ALUI", "add", "pk", "pk", 1)).call("NEXT").tok({",": "FN.pn", ")": "FN.body"}, bad("parameter"))
    P("FN.pn").call("NEXT").tok({"type": "FN.par"}, bad("parameter"))
    p = P("FN.body")
    p.call("NEXT").expect("{").call("NEXT").call("STMTS")
    emit(p, "fn_tail").a(("OFILL", "frm", "cur", 7)).call("NEXT").goto("UNIT")
    # DECL: the identifier ps..pe becomes the next 8-byte slot (measured: params and int locals)
    p = P("DECL")
    p.a(("INTERN", "v", "ps", "pe"), ("ALUI", "add", "cur", "cur", 8), ("STX", "v", LOC, "cur"), ("COPYW", "s", "cur")).ret()
    # statements
    p = P("STMTS")
    p.tok({"}": "RET"}, "STMTS.one")
    P("STMTS.one").call("STMT").goto("STMTS")
    p = P("STMT")
    p.tok({"{": "S.blk", "type": "S.decl", "return": "S.ret", "if": "S.if", "while": "S.while", "for": "S.for", ";": "S.empty"}, "S.expr")
    P("S.blk").call("NEXT").call("STMTS").call("NEXT").ret()
    P("S.empty").call("NEXT").ret()
    p = P("S.decl")
    p.call("NEXT").tok({TK_ID: "S.did"}, bad("declaration"))
    P("S.did").call("DECL").call("NEXT").expect(";").call("NEXT").ret()
    p = P("S.ret")
    p.call("NEXT").call("EXPR").expect(";")
    emit(p, "ret_int").call("NEXT").ret()
    P("S.expr").call("EXPR").expect(";").call("NEXT").ret()
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
    q.o("  jumpz r0, L").num("b").o("\n").vpush("a", "b").call("NEXT").call("STMT").vpop("a", "b")
    emit(q, "jump_a")
    emit(q, "label_b").ret()
    p = P("S.for")
    p.a(("ALUI", "add", "lab", "lab", 1), ("COPYW", "a", "lab"), ("ALUI", "add", "lab", "lab", 1), ("COPYW", "b", "lab"),
        ("ALUI", "add", "lab", "lab", 1), ("COPYW", "c", "lab")).vpush("a", "b", "c")
    p.call("NEXT").expect("(").call("NEXT").call("EXPR").expect(";").vpop("a", "b", "c")
    emit(p, "label_a").vpush("a", "b", "c").call("NEXT").call("EXPR").expect(";").vpop("a", "b", "c")
    emit(p, "jumpz_b").call("NEXT").a(("COPYW", "stp", "tpos"), ("LDI", "dep", 0)).label("F.skip")
    p.tok({"(": "F.open", ")": "F.close"}, "F.nx")
    P("F.open").a(("ALUI", "add", "dep", "dep", 1)).goto("F.nx")
    P("F.close").branch({1: "F.body"}, "F.cl", [("CMPI", "dep", 0)])
    P("F.cl").a(("ALUI", "sub", "dep", "dep", 1)).goto("F.nx")
    P("F.nx").call("NEXT").goto("F.skip")
    p = P("F.body")
    p.vpush("a", "b", "c", "stp").call("NEXT").call("STMT").vpop("a", "b", "c", "stp").a(("COPYW", "aft", "tpos"))
    emit(p, "label_c").vpush("a", "b", "aft").a(("JUMP", "stp")).call("NEXT").call("EXPR").expect(")").vpop("a", "b", "aft")
    emit(p, "jump_a")
    emit(p, "label_b").a(("JUMP", "aft")).call("NEXT").ret()
    # expressions: EXPR = assignment | the ladder
    p = P("EXPR")
    p.tok({TK_ID: "X.id"}, "E%d" % LEVELS[0])
    P("X.id").a(("COPYW", "ips", "ps"), ("COPYW", "ipe", "pe")).call("NEXT").tok(dict({"=": "X.as", "(": "X.call", "++": "X.inc", "--": "X.dec"}, **{o + "=": "X.c" + o for o in E.CASOPS}), "X.var")
    p = P("X.as")
    p.call("LOOKUP")
    emit(p, "addr")
    emit(p, "push").call("NEXT").call("EXPR")
    emit(p, "pop1")
    emit(p, "store_int").ret()
    for o in E.CASOPS:
        q = P("X.c" + o)    # addr; push; load; push; rhs; pop; op; pop; store
        q.call("LOOKUP")
        emit(q, "addr")
        emit(q, "push")
        emit(q, "load_int")
        emit(q, "push").call("NEXT").call("EXPR")
        emit(q, "pop1").o(E.optext(o))
        emit(q, "pop1")
        emit(q, "store_int").ret()
    for nm, o, fix in (("X.inc", "+", "post_inc"), ("X.dec", "-", "post_dec")):
        q = P(nm)           # addr; push; load; push; 1; pop; op; pop; store; undo to the old value
        q.call("LOOKUP")
        emit(q, "addr")
        emit(q, "push")
        emit(q, "load_int")
        emit(q, "push")
        emit(q, "one")
        emit(q, "pop1").o(E.optext(o))
        emit(q, "pop1")
        emit(q, "store_int")
        emit(q, fix).call("NEXT").call("C%d" % LEVELS[0]).ret()
    p = P("X.var")      # an identifier operand, then the rest of the ladder with it as the left operand
    p.call("LOOKUP")
    emit(p, "addr")
    emit(p, "load_int").call("C%d" % LEVELS[0]).ret()
    P("X.call").call("CALL").call("C%d" % LEVELS[0]).ret()
    ladder("E", "UNARY")
    ladder("C", None)
    p = P("UNARY")
    p.tok({"-": "U.neg", "!": "U.not", "(": "U.par", TK_NUM: "U.num", TK_ID: "U.id", "++": "U.pinc", "--": "U.pdec"}, bad("expression"))
    for nm, fix in (("U.pinc", "pre_inc"), ("U.pdec", "pre_dec")):
        q = P(nm)           # addr; push; load; +-1; pop; store
        q.call("NEXT").tok({TK_ID: nm + ".id"}, bad("expression"))
        q = P(nm + ".id")
        q.a(("COPYW", "ips", "ps"), ("COPYW", "ipe", "pe")).call("LOOKUP")
        emit(q, "addr")
        emit(q, "push")
        emit(q, "load_int")
        emit(q, fix)
        emit(q, "pop1")
        emit(q, "store_int").call("NEXT").ret()
    q = P("U.neg")
    q.call("NEXT").call("UNARY")
    emit(q, "neg").ret()
    q = P("U.not")
    q.call("NEXT").call("UNARY")
    emit(q, "not").ret()
    P("U.par").call("NEXT").call("EXPR").expect(")").call("NEXT").ret()
    q = P("U.num")
    emit(q, "imm").call("NEXT").ret()
    P("U.id").a(("COPYW", "ips", "ps"), ("COPYW", "ipe", "pe")).call("NEXT").tok({"(": "U.call"}, "U.var")
    P("U.call").call("CALL").ret()
    q = P("U.var")
    q.call("LOOKUP")
    emit(q, "addr")
    emit(q, "load_int").ret()
    p = P("LOOKUP")     # s := the slot of ips..ipe (0: not a local of this slice)
    p.a(("INTERN", "v", "ips", "ipe"), ("LDX", "s", "v", LOC)).branch({1: "DEAD.nl"}, "RET", [("CMPI", "s", 0)])
    g.on("DEAD.nl", range(257), "DEAD", E.rej("not covered: identifier is not a local"), "r")
    # CALL: at '(' after ips..ipe: arguments pushed left to right, popped into r(n-1)..r0, call
    p = P("CALL")
    # the callee must be defined above (the reference rejects a call to an undefined function: probe r1)
    p.a(("INTERN", "v", "ips", "ipe"), ("LDX", "t", "v", E.FND)).branch({1: "CL.ok"}, bad("call to a function not defined before"), [("CMPI", "t", 1)])
    p = P("CL.ok")
    p.a(("COPYW", "cls", "ips"), ("COPYW", "cle", "ipe")).vpush("cls", "cle").a(("LDI", "na", 0)).call("NEXT").tok({")": "CL.done"}, "CL.arg")
    p = P("CL.arg")
    p.vpush("na").call("EXPR").vpop("na")
    emit(p, "push").a(("ALUI", "add", "na", "na", 1)).tok({",": "CL.more", ")": "CL.done"}, bad("argument list"))
    P("CL.more").call("NEXT").goto("CL.arg")
    p = P("CL.done")
    p.branch({2: "DEAD.na"}, "CL.pop", [("CMPI", "na", 6)])
    g.on("DEAD.na", range(257), "DEAD", E.rej("not covered: more than 6 arguments"), "r")
    p = P("CL.pop")
    p.branch({1: "CL.emit"}, "CL.p1", [("CMPI", "na", 0)])
    p = P("CL.p1")
    p.a(("ALUI", "sub", "na", "na", 1), ("COPYW", "ak", "na"))
    emit(p, "pop_arg").goto("CL.pop")
    p = P("CL.emit")
    p.vpop("cls", "cle")
    emit(p, "call").call("NEXT").ret()
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
