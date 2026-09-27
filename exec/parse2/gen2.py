"""E3 delta generator: shared expression/type helpers and handwritten
state/action rules, with selected attributes read from the gold tables.

The declarative grammar compiler described in research/e3-structured.md is
an unfulfilled design, not the implementation of this file. Runtime model
construction does not by itself remove the handwritten compilation rules.

    python3 exec/parse2/gen2.py OUT.json
    python3 exec/parse2/gen2.py --locations OUT.json
    python3 exec/parse2/gen2.py --warnings OUT.json
    python3 exec/parse2/gen2.py --errors OUT.json

Warnings imply locations. Errors add located diagnostics and recovery.
Unsupported forms are rejected explicitly. Coverage is recorded in the
fixed probe lists and prd.md, not inferred from the presence of a rule.
"""
import json
import os
import re
import sys
from pathlib import Path

import importlib.util
_spec = importlib.util.spec_from_file_location(
    "e3gen", os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "parse", "gen.py"))
E = importlib.util.module_from_spec(_spec)   # the token reader, the assembler P, the gold tables, the tape constants
_spec.loader.exec_module(E)

# Intrinsic names come from the product declaration, not a second hand list.
sys.path.insert(0, E.ROOT)
from unisa.front.parse import INTRINSIC, INTRINSIC6
SYSCALLS = [(name, op, 3) for name, op in INTRINSIC.items()] + [(name, op, 6) for name, op in INTRINSIC6.items()]

O, TK, TK_ID, TK_NUM, LOC = E.O, E.TK, E.TK_ID, E.TK_NUM, E.LOC
VLSIZE, VLFRAME, VLDEP = 52 << 40, 53 << 40, 54 << 40
UNDO_SIZE = 39
# Shared rule/control entry points (reuse before adding a new state cluster):
# TSPEC/DSTARS: type specifiers and per-declarator pointer shape.
# FPDECL/PARAMS: function-pointer shape and balanced parameter scanning.
# FN.params: parameter declarations; definitions and block signatures share it.
# DIMS/DIMSAVE: dimensions and their object metadata; ELSZ: element size.
# DECLN/DECL, BIND/UNWIND: local allocation and scoped name restoration.
# ASSIGNCV, CKM/RESD: conversions and table-derived arithmetic type decisions.
# INITLIST/STRINGINIT: aggregate and string initialisation.
# These are existing helpers, not a claim that grammar duplication is gone.
DEFS = {}   # (name, how) -> count: a procedure or label defined twice merges two states silently


class P(E.P):
    def __init__(self, name):
        DEFS[name, "P"] = DEFS.get((name, "P"), 0) + 1
        super().__init__(name)

    def label(self, lab):
        DEFS[lab, "L"] = DEFS.get((lab, "L"), 0) + 1
        return super().label(lab)


E.P = P
g = E.g
from unresolved import install as ud_install
from finite_rules import install as install_rules, load as load_rules

# Tape text uses JSON string escaping; PUSH/POP1 retain their shared E bindings.
# {name} prints W[name] in decimal; declared spans print input slices.
def tape_rows(filename):
    with open(os.path.join(os.path.dirname(__file__), filename), encoding="utf-8") as source:
        next(source)  # column names
        return [line.rstrip("\n").split("\t") for line in source]


TEMPL = {}
for name, kind, value in tape_rows("tape-templates.tsv"):
    assert name not in TEMPL and kind in ("literal", "binding"), name
    TEMPL[name] = {"PUSH": E.PUSH, "POP1": E.POP1}[value] if kind == "binding" else json.loads(value)
SPANS = {name: (start, end) for name, start, end in tape_rows("tape-spans.tsv")}


def addr(p):
    """a variable's address: a local's frame slot, or a global's symbol"""
    gl, lc, dn = p.fresh("ga"), p.fresh("la"), p.fresh("ad")
    st, fr, auto = p.fresh("sa"), p.fresh("fa"), p.fresh("auto")
    p.branch({1: gl}, lc, [("CMPI", "s", E.GMARK)])
    emit(P(gl), "gaddr").goto(dn)
    P(lc).branch({0: st}, fr, [("CMPI", "s", 0)])
    P(st).a(("LDI", "t", 0), ("ALU", "sub", "t", "t", "s"), ("ALUI", "sub", "t", "t", 1)).o("  .lea r0, ls").num("t").o("\n").goto(dn)
    P(fr).branch({1: auto}, "DEAD.staticauto", [("CMPI", "si_active", 0)])
    emit(P(auto), "addr").goto(dn)
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
    bn = "OPX." + o
    modes = {name: (pointer, int(compare), int(invert)) for name, pointer, compare, invert in tape_rows("operator-modes.tsv")}
    pointer, compare, invert = modes.get(o, modes["*"])
    bindings = {name: bn + (suffix if suffix != "-" else "") for name, suffix in tape_rows("operator-states.tsv")}
    bindings.update(CKT=CKT, RST=RST, operator_offset=[x for lv in LEVELS for x in OPS[lv] if x not in SHORT].index(o) * 256,
                    axis_illegal=AX.index("illegal"), axis_f64=AX.index("f64"),
                    integer_entry=bn + (".n" if pointer in ("add", "sub") else ".r"),
                    pointer_compare_target=bn + ".r" if compare else "DEAD.pa")
    sequences = {name: row[0][1] for name, row in load_rules(Path(__file__).with_name("operator-actions.tsv"), {},
                 domain=[0], bindings=bindings, section="operator").items()}
    sequences.update(operator_signed=O(E.optext(o)), operator_unsigned=O(E.optext(o, True)))
    def install(section, suffix=""):
        structured_control("operator-" + section, False, dict(bindings, word_state=bn + "." + section + suffix), sequences)
    install("prefix")
    if o in FOPS:
        install("float-select")
        for suffix, base in (("d", DBL), ("s", FLT)):
            opcode = FPU[suffix + FOPS[o]]
            sequences.update(float_opcode=O(TYPE_TAPE["float_operator"] % (opcode.removesuffix("_rev"),
                             "r0, r1" if opcode.endswith("_rev") else "r1, r0")),
                             float_invert=O(TYPE_TAPE["float_invert"]) if invert else [])
            bindings.update(float_entry=bn + "." + suffix, float_convert="TO." + suffix, float_result=4 if compare else base)
            install("float-body", suffix)
    else:
        install("float-reject")
    install("pointer-" + pointer)
    install("integer")


def tytail():
    initials = {name: row[0][1] for name, row in load_rules(Path(__file__).with_name("operator-actions.tsv"),
                {}, domain=[0], section="initial").items()}
    current, initial = "CKM", initials["ckm"]
    for width, mask in [(sz, (1 << (8 * sz)) - 1) for _, _, sz, un, _ in TYINT if un and sz < 8]:
        hit, nxt = "CKM.m%d" % width, "CKM.k%d" % width
        structured_control("ckm-row", False, dict(word_state=current, tail_current=current,
            tail_test=P(current).fresh("b"), tail_hit=hit, tail_next=nxt, tail_axis=AX.index("u%d" % (8 * width))),
            dict(tail_initial=initial, tail_mask=O(TYPE_TAPE["mask_pair"] % mask)))
        current, initial = nxt, []
    structured_control("ckm-final", False, dict(tail_current=current, tail_test=P(current).fresh("b"),
        tail_axis=AX.index("u64")), dict(tail_initial=initial))
    current, initial = "RESD", initials["resd"]
    for name, code, size, unsigned, _ in TYINT:
        hit, nxt = "RESD." + name, "RESD.n" + name
        mask = O(TYPE_TAPE["mask"] % ((1 << (8 * size)) - 1)) if unsigned and size < 8 else []
        structured_control("resd-row", False, dict(word_state=current, tail_current=current,
            tail_test=P(current).fresh("b"), tail_hit=hit, tail_next=nxt, tail_axis=AX.index(name), tail_code=code),
            dict(tail_initial=initial, tail_mask=mask))
        current, initial = nxt, []
    structured_control("resd-final", False, dict(tail_current=current), dict(tail_initial=initial))


def strwalk(pre, body, done):
    from strings import walk
    walk(E, P, ESC, pre, body, done)


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
                q.call("FTRUTH").a(("ALUI", "add", "lab", "lab", 1), ("COPYW", "e", "lab"))
                emit(q, "and_skip").vpush("e").call("NEXT").call(nxt).vpop("e")
                emit(q.call("FTRUTH"), "bool").a(("LDI", "vt", 0), ("LDI", "vb", 4))
                emit(q, "label_e").goto(nm + ".l")
                continue
            if o == "||":
                q.call("FTRUTH").a(("ALUI", "add", "lab", "lab", 1), ("COPYW", "od", "lab"), ("ALUI", "add", "lab", "lab", 1), ("COPYW", "on", "lab"))
                emit(q, "or_skip").vpush("od").call("NEXT").call(nxt).vpop("od")
                emit(q.call("FTRUTH"), "bool").a(("LDI", "vt", 0), ("LDI", "vb", 4))
                emit(q, "label_d").goto(nm + ".l")
                continue
            # left in r0: push; the right operand at the next level; then the operator's shared tail (OPX)
            emit(q, "push").vpush("vt", "vb").call("NEXT").call(nxt).vpop("lt", "lb").call("OPX." + o).goto(nm + ".l")
    if prefix == "E":
        for lv in LEVELS:
            for o in OPS[lv]:
                if o not in SHORT:
                    optail(o)
        tytail()
    g.on("DEAD.short", range(257), "DEAD", E.rej("not covered: && ||"), "r")
    g.on("DEAD.pa", range(257), "DEAD", E.rej("not covered: pointer arithmetic"), "r")
    if prefix == "E":
        g.on("DEAD.ty", range(257), "DEAD", E.rej("not covered: operand types (the type table's result)"), "r")
    g.on("DEAD.ui", range(257), "DEAD", E.rej("not covered: unsigned int with this operand"), "r")


from printfallback import KINDS as PFKINDS, install as pf_install
PFCONV = {ord(k): PFKINDS.index(v) for k,v in E.gold("pfconv") if k != "conv"}

HEX = "0123456789abcdef"
ESC = {"n": 10, "t": 9, "r": 13, "a": 7, "b": 8, "f": 12, "v": 11, "\\": 92, "'": 39, '"': 34, "?": 63, "0": 0}


def fmtwalk(pre, on_byte, on_d, on_end):
    """Walk already-decoded format bytes; modifiers follow the product grammar.
    The fallback ignores their formatting effect, unlike declared libc printf.
    """
    w=pre+".w"
    g.on(w,[37],pre+".pc",[("ADV",)])
    g.on(w,[256],on_end,[("INPOP",)])
    for c in range(256):
        if c!=37:g.on(w,[c],on_byte,[("ADV",),("LDI","bv",c)])
    g.on(pre+".pc",list(b"-+ #0"),pre+".pc",[("ADV",)])
    g.els(pre+".pc",pre+".width",[])
    g.on(pre+".width",range(48,58),pre+".width",[("ADV",)])
    g.on(pre+".width",[46],pre+".precision",[("ADV",)])
    g.els(pre+".width",pre+".length",[])
    g.on(pre+".precision",range(48,58),pre+".precision",[("ADV",)])
    g.els(pre+".precision",pre+".length",[])
    g.on(pre+".length",list(b"hlLzjt"),pre+".length",[("ADV",)])
    g.on(pre+".length",[37],on_byte,[("ADV",),("LDI","bv",37)])
    for byte,kind in PFCONV.items():
        g.on(pre+".length",[byte],on_d,[("ADV",),("LDI","pfkind",kind)])
    g.els(pre+".length","DEAD",E.rej("not covered: printf conversion"))
    return w


def printf(warnings=False):
    pf_install(P)
    P("FMT.decode").a(("SBCLR",)).goto("FMT.walk")
    strwalk("FMT.walk","FMT.byte","FMT.end")
    P("FMT.byte").a(("RLD","bv")).goto("FMT.append")
    for b in range(256):g.on("FMT.append",[b],"FMT.walk.w",[("SBOUT",b)],"r")
    P("FMT.end").a(("SBSAVE","pfblob")).ret()
    # PF is at '(' with the callee name still in ips..ipe. Defined printf
    # uses the ordinary variadic call path, including nonliteral formats.
    # Only the undefined fallback requires a literal; pfconv selects routines.
    p = P("PF")
    if warnings: p.a(("LDI", "wi_called", 1))
    p.a(("LDX", "t", "pfid", E.FND)).branch({1: "PF.real"}, "PF.fallback", [("CMPI", "t", 1)])
    P("PF.real").call("CALL").ret()
    p = P("PF.fallback")
    p.call("NEXT").tok({E.TK_STR: "PF.s"}, bad("printf format"))
    p = P("PF.s")
    p.call("FMT.decode").vpush("pfblob").a(("ALUI","add","pfcalls","pfcalls",1),("COPYW","pfcid","pfcalls"),("LDI","pfan",0))
    if warnings: p.a(("COPYW","wf_format","pfblob"))
    p.call("NEXT").label("PF.args")
    p.tok({",": "PF.arg", ")": "PF.go"}, bad("argument list"))
    P("PF.arg").branch({0:"PF.argument"},bad("printf argument count"),[("CMPI","pfan",32)])
    q = P("PF.argument")
    q.vpush("pfcid","pfan").call("NEXT")
    if warnings: q.a(("COPYW","wf_at","tpos"),("LDI","wi_called",0)).vpush("wf_format","wf_at")
    q.call("EXPR")
    if warnings: q.vpop("wf_format","wf_at")
    q.vpop("pfcid","pfan")
    if warnings: q.a(("ALUI","add","wf_index","pfan",1)).call("WF.argcheck")
    q.a(("ALUI", "add", "cur", "cur", 8)).call("MAXF")
    emit(q, "pf_spill").a(("ALUI","mul","pfkey","pfcid",32),("ALU","add","pfkey","pfkey","pfan"),("STX","pfkey",32<<40,"cur"),("ALUI","add","pfan","pfan",1)).goto("PF.args")
    q = P("PF.go")
    q.vpop("pfblob").a(("LDI","pfx",0),("LDI", "cnt", 0), ("INPUSH", "pfblob")).goto("PF.w")
    fmtwalk("PF", "PF.b", "PF.d", "PF.end")
    P("PF.b").a(("ALUI", "add", "cnt", "cnt", 1)).goto("PF.w")
    P("PF.d").branch({0:"PF.convertarg"},bad("printf argument count"),[("CMP","pfx","pfan")])
    q = P("PF.convertarg")
    q.call("PF.flush").a(("ALUI","mul","pfkey","pfcid",32),("ALU","add","pfkey","pfkey","pfx"),("LDX","as","pfkey",32<<40))
    q.o("  load64 r0, [r6-").num("as").o("]\n").call("PF.convert").a(("ALUI","add","pfx","pfx",1)).goto("PF.w")
    P("PF.end").branch({1:"PF.finish"},bad("printf argument count"),[("CMP","pfx","pfan")])
    q = P("PF.finish")
    q.call("PF.flush")
    emit(q, "pf_value").call("NEXT").ret()
    q = P("PF.flush")
    q.branch({1: "RET"}, "PF.fw", [("CMPI", "cnt", 0)])
    q = P("PF.fw")
    emit(q, "pf_write").a(("ALUI", "add", "sk", "sk", 1), ("LDI", "cnt", 0)).ret()
    # POOL: after the footer, the formats again from the top, in the same order: each
    # non-empty literal run is `.str Sk "..."` -- printable bytes as they are, others \xHH, then \x00
    p = P("POOL")
    p.call("POOL.next").label("PO.l")
    p.tok({"eof": "RET", TK_ID: "PO.id", E.TK_STR: "PO.lit"}, "PO.nx")
    P("PO.lit").a(("LDX", "t", "tpos", SKIPS)).branch({1: "PO.nx"}, "PO.lit1", [("CMPI", "t", 1)])
    P("PO.lit1").a(("LDI", "cnt", 0)).goto("PL")
    strwalk("PL", "PL.b", "PL.end")
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
    emit(q, "pool_close").a(("ALUI", "add", "sk", "sk", 1), ("LDI", "cnt", 0)).call("POOL.next").goto("PO.l")
    P("PO.nx").call("POOL.next").goto("PO.l")
    P("PO.id").a(("LDX","ufblob","ps",FNSTR)).branch({1:"PO.idnormal"},"PO.func",[("CMPI","ufblob",0)])
    P("PO.idnormal").a(("INTERN", "v", "ps", "pe")).branch({1: "PO.pf0"}, "PO.nx", [("CMP", "v", "pfid")])
    emit(P("PO.func"),"pool_open").a(("INPUSH","ufblob")).goto("PO.funcbyte")
    g.on("PO.funcbyte",[256],"PO.funcend",[("INPOP",)])
    for byte in range(256):
        spelling=chr(byte) if 32<=byte<127 and byte not in (34,92) else ("\\"+chr(byte) if byte in (34,92) else "\\x%02x"%byte)
        g.on("PO.funcbyte",[byte],"PO.funcbyte",[("OUT",ord(c)) for c in spelling]+[("ADV",)])
    P("PO.funcend").o("\\x00").goto("PL.e1")
    P("PO.pf0").a(("LDX", "t", "pfid", E.FND)).branch({1: "PO.nx"}, "PO.pf", [("CMPI", "t", 1)])   # a real printf: its format pools whole
    P("PO.pf").call("POOL.next").tok({"(": "PO.p1"}, "PO.l")
    P("PO.p1").call("POOL.next").tok({E.TK_STR: "PO.s"}, "PO.l")
    # Evaluate/pool argument literals before the format fragments, including
    # nested fallback calls. Reader frames keep the original token positions.
    P("PO.s").call("FMT.decode").vpush("pfblob").call("POOL.next").a(("COPYW","pas","tpos"),("LDI","pdep",0)).goto("PO.seek")
    P("PO.seek").tok({"(":"PO.deeper",")":"PO.shallower","eof":"DEAD.pfpool"},"PO.seeknext")
    P("PO.deeper").a(("ALUI","add","pdep","pdep",1)).goto("PO.seeknext")
    P("PO.shallower").branch({1:"PO.arguments"},"PO.less",[("CMPI","pdep",0)])
    P("PO.less").a(("ALUI","sub","pdep","pdep",1)).goto("PO.seeknext")
    P("PO.seeknext").call("POOL.next").goto("PO.seek")
    P("PO.arguments").a(("INPUSHXE","pas","tpos")).call("POOL").a(("INPOP",)).vpop("pfblob").a(("LDI","cnt",0),("INPUSH","pfblob")).goto("PO.w")
    g.on("DEAD.pfpool",range(257),"DEAD",E.rej("not covered: unterminated printf pool"),"r")
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
    q.call("PO.close").call("POOL.next").goto("PO.l")
    q = P("PO.close")
    q.branch({1: "RET"}, "PO.cw", [("CMPI", "cnt", 0)])
    q = P("PO.cw")
    emit(q, "pool_close").a(("ALUI", "add", "sk", "sk", 1), ("LDI", "cnt", 0)).ret()

    g.on("POOL.next",[256],"RET",[("LDI","tk",TK["eof"])])
    g.els("POOL.next","POOL.next.real",[])
    P("POOL.next.real").call("NEXT").ret()


UNS = E.UNS   # unsigned char/short/int/long: UNS + size
SBB = E.SBB   # a struct's base code: SBB + sid; layouts in the old E3's tables (measured rules)
STAG, SSZ = E.STAG, E.SSZ
STRUCT_MAX, MEMBER_STRIDE = 128, 256
MOF, MSZ, MPT, MBS, MAR, MFLAT = (i << 40 for i in range(1, 7))
FPB = E.FPB   # register-call function pointer
FPV = 68      # stacked variadic function pointer; carried by the ordinary base descriptor
DBL = E.DBL
# ---- declared data: the product's type tables (weights/gold/type.tsv, tyinfo.tsv) -------------
# binary() asks two rows: ck = type(t1 "+" t2), the common type the operands are converted to
# (its signedness picks the spelling, a width below 8 masks both), and res = type(t1 op t2), the
# result (unsigned below 8: masked).  E3 reads the same rows instead of re-deriving them.
AX = [f[0] for f in E.gold("tyinfo") if f[1].isdigit()]   # the type axis, in tyinfo's own order (16 names)
TYROW = {(f[0], f[1], f[2]): f[3] for f in E.gold("type")}
TYINFO = {f[0]: (int(f[1]), int(f[2]), int(f[3])) for f in E.gold("tyinfo") if f[1].isdigit()}   # t -> (size, uns, narrow)
# the integer rows of tyinfo as value-descriptor codes: an unsigned type is UNS + size (E3's encoding)
TYINT = [(t, (UNS if TYINFO[t][1] else 0) + TYINFO[t][0], TYINFO[t][0], TYINFO[t][1], TYINFO[t][2])
         for t in ("i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64")]      # (t, vb, size, uns, narrow)
U32M = (1 << (8 * TYINFO["u32"][0])) - 1          # an unsigned int kept to 32 bits (measured), from tyinfo
TYPE_TAPE = {name: json.loads(text) for name, text in tape_rows("type-tape.tsv")}
UIM = TYPE_TAPE["mask"] % U32M
assert TYINFO["u32"][1] == 1
# the premises the derivations lean on, pinned: a table that changes shape must fail here, not silently
assert len(AX) == 16 and AX[-1] == "illegal"
assert len(TYINT) == 8 and all(TYINFO[t][0] in (1, 2, 4, 8) for t, *_ in TYINT)
TYOP = {"&": "|", "<": "<", ">": "<", "<=": "<", ">=": "<", "==": "==", "!=": "=="}   # tycanon: the row an operator asks
CKT, RST = 30 * 10 ** 6, 31 * 10 ** 6   # CKT[l * 16 + r] = ck; RST[opi * 256 + l * 16 + r] = res (AX indices)
CSV, CSL = 33 * 10 ** 6, 34 * 10 ** 6   # a switch's cases: value and label, a stack (csp)
SAL = 37 * 10 ** 6
POSSPAN = 1 << 26   # disjoint byte-position-keyed regions; checked at START
TIX, SINIT, SIEND = 8 * POSSPAN, 9 * POSSPAN, 10 * POSSPAN
SFLAT = 16 * POSSPAN  # scalar slots in a struct/member
GUNIT = 15 * POSSPAN  # global symbol -> last declaration unit epoch
PIDS = 14 * POSSPAN  # parameter index -> bound object, for deferred aggregate copies
GINPS, GINPE = 12 * POSSPAN, 13 * POSSPAN  # declaration name at its initializer = token
TAGLEVEL, TAGUNDO = 19 * POSSPAN, 20 * POSSPAN
ETAG = 11 * POSSPAN   # named enum tags, separate from typedef and value namespaces
GIBLOB, GIEND = 5 * POSSPAN, 6 * POSSPAN  # initialiser tape and end token, produced once in source order
FNSTR = 18 * POSSPAN  # __func__ token byte position -> function-name blob
SKIPS = 7 * POSSPAN   # SKIPS[the token position of a string literal] = 1: it initialises a char array, not pooled
GSZ, SMN, SMEM = 39 * 10 ** 6, 40 * 10 ** 6, 41 * 10 ** 6   # a global's size; a struct's members, in order   # MAR[member key] = array length (0: scalar, -1: flexible)   # a struct's alignment (its widest member's)
ENV, END_ = 35 * 10 ** 6, 36 * 10 ** 6   # an enum constant's value; END_[v] = 1 when v names one
DIM, TDIM = 28 * 10 ** 6, 29 * 10 ** 6   # DIM[v * 8 + k]: an array's k-th dimension; TDIM[k]: while declaring
PDB = 27 * 10 ** 6   # PDB[f * 16 + k] = base of f's parameter k (a double parameter converts an int argument)
FOPS = {"+":"add", "-":"sub", "*":"mul", "/":"div", "<":"lt", ">":"gt", "<=":"le", ">=":"ge", "==":"eq", "!=":"eq"}
FPU = {row[1]: row[2] for row in E.gold("irsel") if row[0] == "fpu"}
FLT = E.FLT   # f32 value descriptor; pointer depth keeps pointee types distinct
BOOL = 66  # distinct value kind; arithmetic maps to tyinfo u8
assert len({BOOL, DBL, FLT, FPB, FPV}) == 5 and FPV < SBB
TYPEW = {"type=_Bool": BOOL, "type=float": FLT, "type=double": DBL, "type": E.SZ["int"], "type=char": E.SZ["char"], "type=short": E.SZ["short"], "type=long": E.SZ["long"], "type=void": 0}
TWORDS = tuple(TYPEW) + ("type=unsigned",)


def width_dispatch(name, tape, masks=False):
    bindings = {key: name + suffix for key, suffix in
                (("entry", ""), ("base", ".b"), ("double", ".d"), ("float", ".f"),
                 ("f32", ".f32"), ("wide", ".8"), ("number", ".n"), ("bool", ".bool"), ("integer", ".integer"))}
    bindings.update((key + "_test", P(bindings[key]).fresh("b")) for key in ("entry", "base", "double", "float", "number"))
    bindings.update(DBL=DBL, FLT=FLT, BOOL=BOOL, SBB=SBB)
    seq = {"wide": O(TYPE_TAPE[tape + "8"]), "float": O(TYPE_TAPE[tape + "n"] % TYINFO["f32"][0]),
           "bool": O(TYPE_TAPE[tape + "n"] % 1 + (TYPE_TAPE["mask"] % 255 if masks else ""))}
    install_rules(g, os.path.dirname(__file__), "width", bindings=bindings, sequences=seq, section="prefix")
    q = P(bindings["integer"])
    for _, vb, size, uns, _ in (row for row in TYINT if row[1] != 8):
        bindings.update(current=q.cur, hit=q.fresh("w"), next=q.fresh("x"), test=q.fresh("b"), code=vb)
        seq["row"] = O(TYPE_TAPE[tape + "8"] if size == 8 else TYPE_TAPE[tape + "n"] % size +
                       (TYPE_TAPE["mask"] % ((1 << (8 * size)) - 1) if masks and uns else ""))
        install_rules(g, os.path.dirname(__file__), "width", bindings=bindings, sequences=seq, section="row")
        q = P(bindings["next"])
    bindings["current"] = q.cur
    if name == "LOADV": bindings["tail_test"] = q.fresh("b")
    install_rules(g, os.path.dirname(__file__), "width", bindings=bindings, section="aggregate" if name == "LOADV" else "reject")


def types():
    width_dispatch("LOADV", "load", masks=True)
    width_dispatch("LOADRAW", "load")
    bindings = dict(FLT=FLT, store_test=P("STOREV").fresh("b"), scalar_test=P("STF").fresh("b"))
    install_rules(g, os.path.dirname(__file__), "width", bindings=bindings,
                  sequences={"store_float": O(TYPE_TAPE["storen"] % 4)}, section="store")
    width_dispatch("STOREV0", "store")
    bindings = {key: P(state).fresh("b") for key, state in
                (("pointer_test", "NARROW"), ("wide_test", "NARROW.b"), ("unsigned_test", "NARROW.u"),
                 ("double_test", "NARROW.dd"), ("bool_test", "NARROW.ui"))}
    bindings.update(UNSIGNED_WIDE=UNS + 8, DBL=DBL, BOOL=BOOL, TDN=E.TDN)
    install_rules(g, os.path.dirname(__file__), "width", bindings=bindings, sequences={"uim": O(UIM)}, section="narrow")
    q = P("NARROW.n")
    for _, vb, size, uns, _ in (row for row in TYINT if row[4]):
        bindings.update(current=q.cur, hit=q.fresh("w"), next=q.fresh("x"), test=q.fresh("b"), code=vb)
        text = TYPE_TAPE["mask"] % ((1 << (8 * size)) - 1) if uns else TYPE_TAPE["narrow"] % (size, size)
        install_rules(g, os.path.dirname(__file__), "width", bindings=bindings, sequences={"row": O(text)}, section="row")
        q = P(bindings["next"])
    bindings["current"] = q.cur
    install_rules(g, os.path.dirname(__file__), "width", bindings=bindings, sequences={"reject": E.rej("not covered: width")}, section="finish")
    structured_control("dimensions", False)
    strwalk("DM.s", "DM.sb", "DM.se")
    structured_control("dimensions-tail", False)
    # TSPEC: type words then stars -> tb (base size, 0 void), td (depth); current token after
    dispatch = P("TSPEC").fresh("b")
    structured_control("type-entry", False, dict(type_dispatch=dispatch))
    targets = {TK[word]: "TS." + word for word in TWORDS}
    targets.update((TK_ID if word == "identifier" else TK[word], target)
                   for word, target in tape_rows("type-entry.tsv"))
    for key, target in targets.items():
        g.on(dispatch, [key], target, [], "r")
    for state, row in load_rules(Path(__file__).with_name("type-default.tsv"),
            {"reject": E.rej("not covered: type")}, domain=set(range(257)) - targets.keys(),
            bindings=dict(type_dispatch=dispatch)).items():
        for key, (target, actions) in row.items(): g.on(state, [key], target, actions, "r")
    structured_control("type-prefix", False)
    structured_control("structure", False)
    structured_control("type-typedef", False)
    follows = dict(tape_rows("type-follow.tsv"))
    for word, value in TYPEW.items():
        p = P("TS." + word)
        structured_control("type-word", False, dict(word_state=p.cur, word_return=p.fresh("r"),
                           type_value=value, word_follow=follows.get(word, follows["*"])))
    structured_control("type-tail", False)


def structured_control(section, warnings, extra=None, sequence_bindings=None):
    section += "-warnings" if warnings and section in ("block", "if") else ""
    p = P("control." + section + (extra or {}).get("word_state", ""))
    bindings = dict(VLDEP=VLDEP, CSV=CSV, CSL=CSL, U32M=U32M, DIM=DIM, TDIM=TDIM, FPB=FPB, FPV=FPV,
                    UNSIGNED_INT=UNS + 4, UNSIGNED_LONG=UNS + 8,
                    statement="STMT.body" if warnings else "STMT")
    bindings.update((name, globals()[name]) for name in
                    ("STAG", "TAGLEVEL", "TAGUNDO", "ETAG", "SBB", "SSZ", "SAL", "SMN", "SMEM", "SFLAT",
                     "MOF", "MSZ", "MPT", "MBS", "MAR", "MFLAT", "MEMBER_STRIDE"))
    bindings.update(STRUCT_LIMIT=STRUCT_MAX + 1, MEMBER_MASK=-MEMBER_STRIDE,
                    TAGUNDO1=TAGUNDO + 1, TAGUNDO2=TAGUNDO + 2, TAGUNDO3=TAGUNDO + 3)
    bindings.update(TDN=E.TDN, TDB=E.TDB, TDD=E.TDD, UNS=UNS, UNSIGNED_CHAR=UNS + 1, UNSIGNED_SHORT=UNS + 2)
    bindings.update(extra or {})
    for part, prefix, kind, key in tape_rows("control-fresh.tsv"):
        if part == section:
            p.cur = prefix
            bindings[key] = p.fresh(kind)
    sequences = {name: O(re.split(r"(\{[^}]*\})", TEMPL[template])[int(fragment)])
                 for name, template, fragment in tape_rows("control-text.tsv")}
    sequences.update((name, E.rej(message)) for name, message in tape_rows("control-reject.tsv"))
    for name, method, slots in tape_rows("control-stack.tsv"):
        p.acts = []
        sequences[name] = getattr(p, method)(*slots.split(",")).acts
    tokens = dict(TK, identifier=TK_ID, number=TK_NUM, string=E.TK_STR)
    sequences.update(sequence_bindings or {})
    classes = {name: [AX.index("f32"), AX.index("f64")] if kind == "float_axes" else
               [TK[token] for token in TWORDS] if kind == "typewords" else [tokens[value]]
               for name, kind, value in tape_rows("control-classes.tsv")}
    install_rules(g, os.path.dirname(__file__), "control", bindings=bindings,
                  sequences=sequences, classes=classes, section=section)


def build(locations=False, warnings=False, errors=False):
    # Unit markers are emitted only by the model framing pass. Each scan's
    # first marker resets the epoch; single-unit token dumps keep epoch zero.
    E.WORDS.append("type=extern"); E.TK["type=extern"] = max(E.TK.values()) + 1
    E.WORDS.append("type=_Bool"); E.TK["type=_Bool"] = max(E.TK.values()) + 1
    E.tokenizer(("type=const", "type=volatile", "type=restrict", "type=inline"))
    del g.st["NX"][1][64]
    g.on("NX", [64], "MU0", [("ADV",)])
    for i, c in enumerate(b"unit"):
        g.on("MU"+str(i), [c], "MU"+str(i+1), [("ADV",)])
        g.els("MU"+str(i), "DEAD", E.rej("not covered: unit marker"))
    g.on("MU4", [48], "MUend", [("ADV",), ("LDI", "unit_epoch", 0), ("LDI", "ixcount", 0)])
    g.on("MU4", [43], "MUend", [("ADV",), ("ALUI", "add", "unit_epoch", "unit_epoch", 1), ("LDI", "ixcount", 0)])
    g.els("MU4", "DEAD", E.rej("not covered: unit marker"))
    g.on("MUend", [10], "NEXT", [("ADV",)])
    g.els("MUend", "DEAD", E.rej("not covered: unit marker"))
    from strings import token_span
    token_span(E, P)
    from strings import initializer as string_initializer
    string_initializer(E, P, ESC)
    E.prn()
    E.numout()
    from floatconst import install as floatconst_install
    floatconst_install(E, P)
    E.autoscan()
    types()
    from truth import install as truth_install
    truth_install(P, DBL, FLT)
    from booleans import install as bool_install
    bool_install(P, BOOL, DBL, FLT)
    from constexpr import install as const_install
    const_install(E, P, LEVELS, OPS, ENV, END_)
    from statics import install as static_install
    static_install(E, P, TIX, SINIT, SIEND, LOC, SKIPS, BOOL)
    from initializers import install as init_install
    init_install(E, P, SBB, LOC, DIM, SSZ, SMN, SMEM, MOF, MSZ, MPT, MBS, MAR, SFLAT, MFLAT, MEMBER_STRIDE)
    g.on("DEAD.staticauto", range(257), "DEAD", E.rej("not covered: static initializer uses automatic storage"), "r")
    # ---- declared data 3: the grammar, compiled to procedures ---------------------------
    P("START").branch({0: "START.ok"}, bad("token input exceeds position domain"), [("XLEN", "toklen"), ("CMPI", "toklen", POSSPAN)])
    p = P("START.ok")
    tops = [o for lv in LEVELS for o in OPS[lv] if o not in SHORT]
    for l in range(16):
        for r in range(16):
            ck = TYROW.get((AX[l], "+", AX[r]), "illegal")
            p.a(("LDI", "t", l * 16 + r), ("LDI", "u", AX.index(ck)), ("STX", "t", CKT, "u"))
            for i, o in enumerate(tops):
                y = TYROW.get((AX[l], TYOP.get(o, o), AX[r]), "illegal")
                p.a(("LDI", "t", i * 256 + l * 16 + r), ("LDI", "u", AX.index(y)), ("STX", "t", RST, "u"))
    p.a(("LDI", "lab", 0), ("LDI", "vsp", 0), ("LDI", "csp", 0), ("SBCLR",), [("SBOUT", c) for c in b"main"], ("SBINTERN", "mnid"),
        ("SBCLR",), [("SBOUT", c) for c in b"printf"], ("SBINTERN", "pfid"), ("SBCLR",), [("SBOUT", c) for c in b"exit"], ("SBINTERN", "exid"), ("MARK", "x0"), ("LDI", "sk", 0))
    p.a(("SBCLR",), [("SBOUT", c) for c in b"__func__"], ("SBINTERN", "funcid"))
    # the reference auto-includes a header when one of its functions is called and not defined here
    # (src/front_pp.c autoinc): the old E3's check, reused -- such a unit is not covered
    for k, (nm, _, _) in enumerate(SYSCALLS, 1):
        p.a(("SBCLR",), [("SBOUT", c) for c in nm.encode()], ("SBINTERN", "sy%d" % k))
    for k, nm in enumerate(("va_start", "va_arg", "va_end")):
        p.a(("SBCLR",), [("SBOUT", c) for c in nm.encode()], ("SBINTERN", "va%d" % k))
    for k, nm in enumerate(("__builtin_sqrt", "__builtin_sqrtf")):
        p.a(("SBCLR",), [("SBOUT", c) for c in nm.encode()], ("SBINTERN", "sqrt%d" % k))
    p.a(("SBCLR",), [("SBOUT", c) for c in b"__argc"], ("SBINTERN", "acid"), ("SBCLR",), [("SBOUT", c) for c in b"__argv"], ("SBINTERN", "avid"))
    for nm in E.autonames():
        p.a(("SBCLR",), [("SBOUT", c) for c in nm.encode()], ("SBINTERN", "t"), ("LDI", "u", 1), ("STX", "t", E.AUT, "u"))
    p.call("AUTO").a(("JUMP", "x0")).call("INDEX").a(("JUMP", "x0")).o(E.HEADER).call("NEXT").label("UNIT")
    p.tok({**{w: "FN" for w in TWORDS}, "eof": "END", "typedef": "TD", "type=static": "TOP.st", "type=extern": "TOP.st", TK_ID: "TOP.id", "struct": "FN", "union": "FN", "enum": "FN"}, bad("top-level construct"))
    # enum is a type specifier in both declarations and typedefs.
    P("ENUM").call("NEXT").tok({TK_ID: "EN.tag", "{": "EN.b"}, bad("enum"))
    P("EN.tag").a(("INTERN", "entag", "ps", "pe")).call("NEXT").tok({"{": "EN.register"}, "EN.reference")
    P("EN.register").a(("COPYW", "tg", "entag")).call("TAG.bind").a(("LDI", "u", 1), ("STX", "entag", ETAG, "u")).goto("EN.b")
    P("EN.reference").branch({1: "RET"}, bad("unknown enum tag"), [("LDX", "u", "entag", ETAG), ("CMPI", "u", 1)])
    p = P("EN.b")
    p.a(("LDI", "env", 0)).call("NEXT").label("EN.l")
    p.tok({TK_ID: "EN.id", "}": "EN.e"}, bad("enum"))
    P("EN.id").a(("COPYW", "enps", "ps"), ("COPYW", "enpe", "pe")).call("NEXT").tok({"=": "EN.eq"}, "EN.bind")
    P("EN.eq").call("NEXT").call("CE").a(("COPYW", "env", "cv")).goto("EN.bind")
    P("EN.bind").a(("COPYW", "ps", "enps"), ("COPYW", "pe", "enpe"), ("INTERN", "v", "ps", "pe")).branch({1: "EN.put"}, "EN.local", [("CMPI", "tagscope", 0)])
    P("EN.local").call("BIND").goto("EN.put")
    P("EN.put").a(("STX", "v", ENV, "env"), ("LDI", "t", 1), ("STX", "v", END_, "t"), ("ALUI", "add", "env", "env", 1)).tok({",": "EN.c", "}": "EN.e"}, bad("enum"))
    P("EN.c").call("NEXT").goto("EN.l")
    P("EN.e").call("NEXT").ret()
    P("TOP.st").call("NEXT").goto("UNIT")        # static: the same code (measured)
    P("TOP.id").call("ISTD").branch({1: "FN"}, bad("top-level construct"))
    # typedef T [*]... NAME;  -- no code
    P("TD").call("TD.parse").goto("UNIT")
    p = P("TD.parse")
    p.call("NEXT").call("TSPEC").tok({TK_ID: "TD.id", "(": "TD.fp"}, bad("typedef"))
    P("TD.fp").call("FPDECL").branch({1: "TD.fpshape"}, bad("function typedef shape"), [("CMPI", "fp_isfunction", 0)])
    P("TD.fpshape").branch({1: "TD.fpput"}, bad("function pointer array typedef"), [("CMPI", "fpn", 0)])
    P("TD.fpput").a(("COPYW", "tdps", "ips"), ("COPYW", "tdpe", "ipe")).goto("TD.bind")
    P("TD.id").a(("COPYW", "tdps", "ps"), ("COPYW", "tdpe", "pe")).call("NEXT").goto("TD.bind")
    P("TD.bind").a(("COPYW", "ps", "tdps"), ("COPYW", "pe", "tdpe"), ("INTERN", "v", "ps", "pe")).branch({1: "TD.put"}, "TD.local", [("CMPI", "tagscope", 0)])
    P("TD.local").call("BIND").goto("TD.put")
    P("TD.put").a(("LDI", "u", 1), ("STX", "v", E.TDN, "u"), ("STX", "v", E.TDB, "tb"), ("STX", "v", E.TDD, "td")).expect(";").call("NEXT").ret()
    # a unit without main is an error in the reference (measured, probe r2)
    P("END").a(("LDX", "t", "mnid", E.FND)).branch({1: "END.ok"}, bad("no main"), [("CMPI", "t", 1)])
    P("END.ok").o("__init:\n").a(("JUMP", "x0"), ("LDI", "dep", 0)).call("INITS").o("  ret\n__main_ret:\n").a(("LDX", "t", "exid", E.FND)).branch({1: "END.ex"}, "END.x2", [("CMPI", "t", 1)])
    P("END.ex").o("  call exit\n").goto("END.x2")     # a unit that defines exit calls it on return from main (measured)
    p = P("END.x2").o("  .exit r0\n").call("PF.helpers").a(("LDI", "sk", 0), ("JUMP", "x0")).call("POOL").call("UD.check")
    if warnings: p.call("WR.summary").call("WR.error")
    p.a(("ACCEPT",)).goto("DEAD")
    p = P("INITS")
    p.call("NEXT").label("IN.l")
    p.branch({1: "IN.scan"}, "IN.static", [("LDX", "si_blob", "tpos", SINIT), ("CMPI", "si_blob", 0)])
    p = P("IN.scan")
    p.tok({"eof": "RET", "{": "IN.o", "}": "IN.c", TK_ID: "IN.id", "=": "IN.eqd"}, "IN.nx")
    P("IN.eqd").branch({1: "IN.eq"}, "IN.nx", [("CMPI", "dep", 0)])
    P("IN.o").a(("ALUI", "add", "dep", "dep", 1)).goto("IN.nx")
    P("IN.c").a(("ALUI", "sub", "dep", "dep", 1)).goto("IN.nx")
    P("IN.nx").call("NEXT").goto("IN.l")
    P("IN.id").branch({1: "IN.id0"}, "IN.nx", [("CMPI", "dep", 0)])
    P("IN.id0").a(("COPYW", "ips", "ps"), ("COPYW", "ipe", "pe")).goto("IN.nx")
    P("IN.eq").a(("LDX", "ips", "tpos", GINPS), ("LDX", "ipe", "tpos", GINPE)).branch({2: "IN.eq0"}, "DEAD.ginit", [("CMP", "ipe", "ips")])
    P("IN.eq0").call("NEXT").tok({E.TK_STR: "IN.s0"}, "IN.v")
    P("IN.s0").a(("LDX", "t", "tpos", SKIPS)).branch({1: "IN.s1"}, "IN.v", [("CMPI", "t", 1)])
    P("IN.s1").a(("INTERN", "iv", "ips", "ipe"), ("LDX", "ibytes", "iv", GSZ),
        ("LDI", "imode", 1), ("COPYW", "inps", "ips"), ("COPYW", "inpe", "ipe")).call("STRINGINIT").goto("IN.l")
    # Every non-string-array initializer was emitted once at its declaration.
    P("IN.v").a(("LDX", "gi_blob", "tpos", GIBLOB), ("LDX", "gi_end", "tpos", GIEND), ("INPUSH", "gi_blob")).goto("IN.vcopy")
    g.on("IN.vcopy", [256], "IN.vdone", [("INPOP",), ("JUMP", "gi_end")])
    g.els("IN.vcopy", "IN.vcopy", [("COPY",), ("ADV",)])
    P("IN.vdone").call("NEXT").goto("IN.l")
    # function: int NAME ( params ) { body }
    p = P("FN")
    p.a(("LDI", "fn_fpwrap", 0)).call("TSPEC").a(("COPYW", "rd", "td"), ("COPYW", "rb", "tb")).tok({TK_ID: "FN.id", ";": "FN.semi", "(": "GV.fp"}, bad("declarator"))
    p = P("GV.fp")
    p.call("FPDECL").a(("COPYW", "fns", "ips"), ("COPYW", "fne", "ipe")).branch({1: "FN.returnfp"}, "GV.fpobject", [("CMPI", "fp_isfunction", 1)])
    P("FN.returnfp").a(("LDI", "fn_fpwrap", 1), ("LDI", "rd", 1), ("LDI", "rb", FPB)).goto("FN.fn")
    p = P("GV.fpobject")
    p.a(("LDI", "gsz", 8), ("LDI", "gar", 0)).branch({1: "GV.fp1"}, "GV.fpa", [("CMPI", "fpn", 0)])
    P("GV.fpa").a(("ALUI", "mul", "gsz", "fpn", 8), ("LDI", "gar", 1)).goto("GV.fp1")
    P("GV.fp1").call("GV.emit").tok({"=": "GV.init"}, "GV.end")
    p = P("FN.id")
    p.a(("COPYW", "fns", "ps"), ("COPYW", "fne", "pe")).call("NEXT").tok({"(": "FN.fn", ";": "GV.sc", "=": "GV.sc", ",": "GV.sc", "[": "GV.ar"}, bad("declarator"))
    P("FN.semi").call("NEXT").goto("UNIT")      # `struct T { ... };` -- a definition only, no code
    # a global: `.bss g_NAME SIZE` where it is declared; its initialiser goes to __init (measured)
    p = P("GV.sc")
    p.branch({1: "GV.sc0"}, "GV.p8", [("CMPI", "td", 0)])
    P("GV.sc0").branch({1: "DEAD.void"}, "GV.scb", [("CMPI", "tb", 0)])
    P("GV.scb").call("ELSZ").a(("COPYW", "gsz", "es")).goto("GV.reg")
    P("GV.p8").a(("LDI", "gsz", 8)).goto("GV.reg")
    p = P("GV.reg")
    p.a(("LDI", "gar", 0)).call("GV.emit").tok({"=": "GV.init"}, "GV.end")
    P("GV.init").a(("STX", "tpos", GINPS, "fns"), ("STX", "tpos", GINPE, "fne")).call("NEXT").tok({"{": "GV.bi", E.TK_STR: "GV.gs"}, "GV.iv")
    P("GV.gs").call("CHARR").branch({1: "GV.gs1"}, "GV.iv", [("CMPI", "u", 1)])
    P("GV.gs1").a(("LDI", "t", 1), ("STX", "tpos", SKIPS, "t")).call("NEXT").goto("GV.end")
    # CHARR: u := 1 when the declaration being read (gar/dar, td, tb) is an array of char
    p = P("CHARR")
    p.a(("LDI", "u", 0), ("ALU", "or", "t", "gar", "dar")).branch({1: "RET"}, "CH.1", [("CMPI", "t", 0)])
    P("CH.1").branch({1: "CH.2"}, "RET", [("CMPI", "td", 0)])
    P("CH.2").branch({1: "CH.y"}, "CH.3", [("CMPI", "tb", 1)])
    P("CH.3").branch({1: "CH.y"}, "RET", [("CMPI", "tb", UNS + 1)])
    P("CH.y").a(("LDI", "u", 1)).ret()
    p = P("GV.bi")
    p.a(("OLEN", "gi_out"), ("COPYW", "gi_pos", "tpos"), ("INTERN", "ivv", "fns", "fne"), ("LDI", "imode", 1),
        ("COPYW", "inps", "fns"), ("COPYW", "inpe", "fne"), ("COPYW", "ibytes", "gsz"))
    p.vpush("gi_out", "gi_pos", "fns", "fne", "td", "tb", "bd", "gar", "gsz").call("INITLIST").vpop("gi_out", "gi_pos", "fns", "fne", "td", "tb", "bd", "gar", "gsz")
    p.goto("GV.cache")
    p = P("GV.iv").a(("OLEN", "gi_out"), ("COPYW", "gi_pos", "tpos"))
    p.vpush("gi_out", "gi_pos", "fns", "fne", "td", "tb", "bd", "gar", "gsz").call("EXPR").a(("COPYW", "rvt", "vt"), ("COPYW", "rvb", "vb")).vpop("gi_out", "gi_pos", "fns", "fne", "td", "tb", "bd", "gar", "gsz")
    p.a(("COPYW", "vt", "td"), ("COPYW", "vb", "tb")).call("ASSIGNCV").o("  .lea r1, g_").a(("SPAN2", "fns", "fne")).o("\n").call("STOREV").goto("GV.cache")
    P("GV.cache").a(("OCUT", "gi_blob", "gi_out"), ("STX", "gi_pos", GIBLOB, "gi_blob"), ("STX", "gi_pos", GIEND, "tpos")).goto("GV.end")
    P("GV.end").tok({",": "GV.cm", "=": "GV.init"}, "GV.e0")
    g.on("DEAD.ginit", range(257), "DEAD", E.rej("not covered: global initialiser"), "r")
    P("GV.e0").expect(";").call("NEXT").goto("UNIT")
    P("GV.cm").call("DSTARS").tok({TK_ID: "GV.cid"}, bad("declarator"))
    P("GV.cid").a(("COPYW", "fns", "ps"), ("COPYW", "fne", "pe")).call("NEXT").tok({";": "GV.sc", "=": "GV.sc", ",": "GV.sc", "[": "GV.ar"}, bad("declarator"))
    p = P("GV.ar")       # T NAME[N][M]...: the product * element size (a pointer element: 8)
    p.call("DIMS").goto("GV.an")
    p = P("GV.an")
    p.call("ELSZ").a(("ALU", "mul", "gsz", "prd", "es"), ("COPYW", "gar", "drk")).call("GV.emit").goto("GV.end")
    p = P("GV.emit")
    p.a(("INTERN", "v", "fns", "fne"), ("LDX", "u", "v", LOC)).branch({1: "GV.epoch"}, "GV.storage", [("CMPI", "u", E.GMARK)])
    P("GV.epoch").branch({1: "GV.record"}, "GV.storage", [("LDX", "u", "v", GUNIT), ("CMP", "u", "unit_epoch")])
    p = P("GV.storage")
    p.a(("STX", "v", GUNIT, "unit_epoch")).o(".bss g_").a(("SPAN2", "fns", "fne")).o(" ").num("gsz").o("\n")
    p.goto("GV.record")
    p = P("GV.record")
    p.a(("INTERN", "v", "fns", "fne"), ("STX", "v", GSZ, "gsz"), ("LDI", "t", E.GMARK), ("STX", "v", LOC, "t"), ("STX", "v", E.BASE, "tb"), ("STX", "v", E.ARR, "gar"),
        ("COPYW", "t", "td")).branch({1: "GV.e1"}, "GV.e2", [("CMPI", "gar", 0)])
    P("GV.e2").a(("ALUI", "add", "t", "t", 1)).call("DIMSAVE").goto("GV.e1")
    P("GV.e1").a(("STX", "v", E.PTR, "t")).ret()
    P("ELSZ").branch({1: "ELSZ.b0"}, "ELSZ.8", [("CMPI", "td", 0)])
    P("ELSZ.b0").branch({(1, 2): "ELSZ.st"}, "ELSZ.b", [("CMPI", "tb", SBB)])
    P("ELSZ.st").a(("ALUI", "sub", "t", "tb", SBB), ("LDX", "es", "t", SSZ)).branch({1: "DEAD.inc"}, "RET", [("CMPI", "es", 0)])
    g.on("DEAD.inc", range(257), "DEAD", E.rej("not covered: incomplete struct"), "r")
    P("ELSZ.b").branch({1: "DEAD.void"}, "ELSZ.s", [("CMPI", "tb", 0)])
    P("ELSZ.s").a(("COPYW", "es", "tb")).branch({1: "ELSZ.d"}, "ELSZ.sf", [("CMPI", "tb", DBL)])
    P("ELSZ.sf").branch({1: "ELSZ.f"}, "ELSZ.bool", [("CMPI", "tb", FLT)])
    P("ELSZ.bool").branch({1: "ELSZ.one"}, "ELSZ.s2", [("CMPI", "tb", BOOL)])
    P("ELSZ.one").a(("LDI", "es", 1)).ret()
    P("ELSZ.f").a(("LDI", "es", 4)).ret()
    P("ELSZ.d").a(("LDI", "es", 8)).ret()
    P("ELSZ.s2").branch({2: "ELSZ.u"}, "RET", [("CMPI", "tb", UNS)])
    P("ELSZ.u").a(("ALUI", "sub", "es", "tb", UNS)).ret()
    P("ELSZ.8").a(("LDI", "es", 8)).ret()
    p = P("FN.fn")
    p.a(("INTERN", "v", "fns", "fne"), ("LDI", "t", 1), ("STX", "v", E.FND, "t"), ("STX", "v", E.FRD, "rd"), ("STX", "v", E.FRB, "rb"), ("LDI", "cur", 0), ("LDI", "max", 0), ("LDI", "usp", 0))
    p.goto("FN.params")
    p = P("FN.params")
    p.call("NEXT").a(("LDI", "pk", 0), ("LDI", "vfn", 0))
    p.tok(dict({")": "FN.body", "type=void": "FN.void", TK_ID: "FN.ptk", "struct": "FN.par", "union": "FN.par"}, **{w: "FN.par" for w in TWORDS if w != "type=void"}), bad("parameter"))
    P("FN.void").call("TSPEC").tok({")": "FN.vend", TK_ID: "FN.pid", "(": "FN.pfp"}, bad("parameter"))
    P("FN.vend").branch({1: "FN.body"}, "FN.unnamed", [("CMPI", "td", 0)])
    P("FN.ptk").call("ISTD").branch({1: "FN.par"}, bad("parameter"))
    p = P("FN.par")
    p.call("TSPEC").tok({TK_ID: "FN.pid", ",": "FN.unnamed", ")": "FN.unnamed", "(": "FN.pfp"}, bad("parameter"))   # unnamed: a prototype
    P("FN.pfp").call("FPDECL").branch({1: "FN.pfpbind"}, "FN.pfparray", [("CMPI", "fpn", 0)])
    P("FN.pfparray").a(("ALUI", "add", "td", "td", 1)).goto("FN.pfpbind")
    P("FN.pfpbind").branch({1: "FN.pfpstacked"}, "FN.pfpdecl", [("CMPI", "tb", FPV)])
    # The reference lookahead counts a literal ... in a nested parameter too.
    P("FN.pfpstacked").a(("LDI", "vfn", 1)).goto("FN.pfpdecl")
    P("FN.pfpdecl").branch({1: "FN.unnamed"}, "FN.pfpdecl1", [("CMPI", "sigmode", 1)])
    P("FN.pfpdecl1").a(("COPYW", "ps", "ips"), ("COPYW", "pe", "ipe"), ("LDI", "dsz", 8), ("LDI", "dar", 0)).call("DECL").a(("STX", "pk", PIDS, "v"), ("ALUI", "add", "pk", "pk", 1)).tok({",": "FN.pn", ")": "FN.body"}, bad("parameter"))
    P("FN.dots").a(("LDI", "vfn", 1)).call("NEXT").tok({")": "FN.body"}, bad("parameter after ..."))
    # Array parameters adjust to pointers before their descriptor is bound.
    # Only a single, side-effect-free bound token is covered here; do not
    # silently discard arbitrary VLA expressions as the reference does.
    P("FN.pid").a(("COPYW", "par_s", "ps"), ("COPYW", "par_e", "pe")).call("NEXT").tok({"[": "FN.array"}, "FN.bind")
    P("FN.array").a(("ALUI", "add", "td", "td", 1)).call("NEXT").goto("FN.aqual")
    P("FN.aqual").tok({"type=static": "FN.aqnext", "]": "FN.aend", TK_NUM: "FN.abound", TK_ID: "FN.abound"}, bad("array parameter bound"))
    P("FN.aqnext").call("NEXT").goto("FN.aqual")
    P("FN.abound").call("NEXT").expect("]").goto("FN.aend")
    P("FN.aend").call("NEXT").tok({",": "FN.bind", ")": "FN.bind"}, bad("array parameter suffix"))
    p = P("FN.bind")
    p.a(("COPYW", "ps", "par_s"), ("COPYW", "pe", "par_e"))
    p.call("SIG.store").branch({1: "FN.pcount"}, "FN.bindslot", [("CMPI", "sigmode", 1)])
    P("FN.unnamed").call("SIG.store").goto("FN.pcount")
    P("FN.pcount").a(("ALUI", "add", "pk", "pk", 1)).tok({",": "FN.pn", ")": "FN.body"}, bad("parameter"))
    P("SIG.store").branch({0: "SIG.put"}, "RET", [("CMPI", "pk", 8)])
    P("SIG.put").a(("INTERN", "t", "fns", "fne"), ("ALUI", "mul", "t", "t", 16), ("ALU", "add", "t", "t", "pk"), ("ALUI", "mul", "u", "td", 4096), ("ALU", "add", "u", "u", "tb"), ("STX", "t", PDB, "u")).ret()
    p = P("FN.bindslot")
    p.a(("LDI", "dsz", 8), ("LDI", "dar", 0)).call("DECL").a(("STX", "pk", PIDS, "v"))
    p.a(("ALUI", "add", "pk", "pk", 1)).tok({",": "FN.pn", ")": "FN.body"}, bad("parameter"))
    P("FN.pn").call("NEXT").tok({**{w: "FN.par" for w in TWORDS}, TK_ID: "FN.ptk", "struct": "FN.par", "union": "FN.par", "...": "FN.dots"}, bad("parameter"))
    p = P("FN.body")
    p.call("NEXT").branch({1: "BP.end"}, "FN.body0", [("CMPI", "sigmode", 1)])
    P("FN.body0").branch({1: "FN.fpclose"}, "FN.bodykind", [("CMPI", "fn_fpwrap", 1)])
    P("FN.fpclose").expect(")").call("FPD.c").goto("FN.bodykind")
    P("FN.bodykind").tok({"{": "FN.def", ";": "FN.proto"}, bad("expected {"))
    p = P("FN.proto")     # a prototype: nothing written; its parameters' names are dropped
    # a prototype after the definition (the header defines exit, the unit declares it) keeps it defined
    p.a(("LDI", "sv", 0)).call("UNWIND").a(("INTERN", "v", "fns", "fne"), ("LDX", "t", "v", E.FND)).branch({1: "FN.pr1"}, "FN.pr0", [("CMPI", "t", 1)])
    P("FN.pr0").a(("LDI", "t", 0), ("STX", "v", E.FND, "t")).goto("FN.pr1")
    P("FN.pr1").call("NEXT").goto("UNIT")
    p = P("FN.def")      # the return label is taken here: a prototype takes none (measured)
    # more than six parameters: all of them come on the tape stack, as for a variadic function (measured, b_args8)
    p.branch({2: "FN.many"}, "FN.def1", [("CMPI", "pk", 6)])
    P("FN.many").a(("LDI", "vfn", 1)).goto("FN.def1")
    p = P("FN.def1")
    p.a(("LDI","infunc",1),("ALUI", "add", "lab", "lab", 1), ("COPYW", "rl", "lab"))
    emit(p, "fn_head").a(("ORES", "frm", 7)).o("\n").a(("LDI", "sk2", 0)).label("FN.sp")
    p.a(("INTERN", "v", "fns", "fne"), ("STX", "v", E.VAR, "vfn")).branch({0: "FN.sp0"}, "FN.copies", [("CMP", "sk2", "pk")])
    P("FN.sp0").branch({1: "FN.spv"}, "FN.sp1", [("CMPI", "vfn", 1)])
    q = P("FN.spv")      # variadic: from the caller's stack (measured): load64 r1, [r6+16+8k]; imm r5, slot; sub64 r5, r6, r5; store64 [r5+0], r1
    q.a(("ALUI", "mul", "t", "sk2", 8), ("ALUI", "add", "t", "t", 16), ("ALUI", "add", "pks", "sk2", 1), ("ALUI", "mul", "pks", "pks", 8))
    q.o("  load64 r1, [r6+").num("t").o("]\n  imm r5, ").num("pks").o("\n  sub64 r5, r6, r5\n  store64 [r5+0], r1\n").a(("ALUI", "add", "sk2", "sk2", 1)).goto("FN.sp")
    q = P("FN.sp1")      # the parameters' spills: store64 [r6-8(k+1)], rk (measured)
    q.a(("ALUI", "add", "pks", "sk2", 1), ("ALUI", "mul", "pks", "pks", 8)).o("  store64 [r6-").num("pks").o("], r").num("sk2").o("\n").a(("ALUI", "add", "sk2", "sk2", 1)).goto("FN.sp")
    # Spill every incoming argument before any copy clobbers r0-r2.
    P("FN.copies").a(("LDI", "cpi", 0)).goto("FN.copyloop")
    P("FN.copyloop").branch({0: "FN.copytype"}, "FN.go", [("CMP", "cpi", "pk")])
    P("FN.copytype").a(("LDX", "cpv", "cpi", PIDS), ("LDX", "t", "cpv", E.PTR)).branch({1: "FN.copybase"}, "FN.copynext", [("CMPI", "t", 0)])
    P("FN.copybase").a(("LDX", "cpb", "cpv", E.BASE)).branch({(1, 2): "FN.copy"}, "FN.copynext", [("CMPI", "cpb", SBB)])
    q = P("FN.copy")
    q.a(("ALUI", "sub", "t", "cpb", SBB), ("LDX", "sz", "t", SSZ), ("ALU", "add", "cur", "cur", "sz"), ("LDX", "cps", "cpv", LOC)).call("MAXF")
    q.o("  load64 r1, [r6-").num("cps").o("]\n  imm r0, ").num("cur").o("\n  sub64 r0, r6, r0\n").call("COPYSTRUCT")
    q.a(("STX", "cpv", LOC, "cur")).goto("FN.copynext")
    P("FN.copynext").a(("ALUI", "add", "cpi", "cpi", 1)).goto("FN.copyloop")
    p = P("FN.go")
    if warnings: p.a(("LDI", "wr_last", 0), ("COPYW", "wu_fn", "usp"))
    p.a(("LDI","vl_depth",0)).call("VL.enter").call("TAG.enter").call("NEXT").call("STMTS").call("TAG.leave").call("VL.leave")
    if warnings: p.a(("COPYW", "wu_lo", "wu_fn")).call("WU.block").call("WR.return")
    p.a(("INTERN", "v", "fns", "fne")).branch({1: "FN.m0"}, "FN.tl", [("CMP", "v", "mnid")])
    P("FN.m0").o("  imm r0, 0\n").goto("FN.tl")      # reaching main's } returns 0 (C99 5.1.2.2.3; product 18c8f22)
    p = P("FN.tl")
    emit(p, "fn_tail").a(("ALUI", "add", "t", "max", 7), ("ALUI", "and", "t", "t", -8), ("OFILL", "frm", "t", 7), ("LDI", "sv", 0)).call("UNWIND")
    p.branch({1: "FN.rv0"}, "FN.nx", [("CMPI", "rd", 0)])
    P("FN.rv0").branch({(1, 2): "FN.rv"}, "FN.nx", [("CMPI", "rb", SBB)])
    P("FN.rv").a(("ALUI", "sub", "t", "rb", SBB), ("LDX", "sz", "t", SSZ)).o(".bss __rv_").a(("SPAN2", "fns", "fne")).o(" ").num("sz").o("\n").goto("FN.nx")
    P("FN.nx").a(("LDI","infunc",0)).call("NEXT").goto("UNIT")
    # DECL: the identifier ps..pe becomes the next 8-byte slot (measured: params and int locals)
    # DECLN: the name was saved in ips..ipe (the current token is after it)
    install_rules(g, os.path.dirname(__file__), "declaration", section="name")
    # Scope record fields are declared once; bind/unwind share their layout bindings.
    scope_bindings = {name: getattr(E, name) for name in
                      ("UNDO", "PTR", "BASE", "ARR", "TDN", "TDB", "TDD", "FND", "FRD", "FRB", "VAR")}
    scope_bindings.update(LOC=LOC, END_=END_, ENV=ENV, VLSIZE=VLSIZE, UNDO_SIZE=UNDO_SIZE)
    for name, base, size in (("UNDO", E.UNDO, UNDO_SIZE), ("DIM", DIM, 8), ("PDB", PDB, 16)):
        scope_bindings.update((name + "_" + str(i), base + i) for i in range(size))
    scope_sequences = {name: row[0][1] for name, row in load_rules(
        Path(__file__).with_name("scope-actions.tsv"), {}, domain=[0], bindings=scope_bindings).items()}
    if warnings: scope_bindings["bind_return"] = P("BIND").fresh("r")
    install_rules(g, os.path.dirname(__file__), "scope", bindings=scope_bindings,
                  sequences=scope_sequences, section="bind-warnings" if warnings else "bind")
    # Preserve fresh continuation identities; declaration semantics live in TSV.
    p = P("DECL")
    decl_bindings = dict(scope_bindings, after_bind=p.fresh("r"))
    decl_bindings["after_local"] = p.fresh("r") if warnings else decl_bindings["after_bind"]
    decl_bindings.update(after_frame=p.fresh("r"), array_test=p.fresh("b"),
                         after_dims=P("DC.a").fresh("r"), frame_test=P("MAXF").fresh("b"))
    for section in ("entry-warnings" if warnings else "entry", "body"):
        install_rules(g, os.path.dirname(__file__), "declaration", bindings=decl_bindings, section=section)
    from vla import install as vla_install
    vla_install(E,P,VLSIZE,VLFRAME,VLDEP,END_,bad,UNS)
    # statements
    structured_control("dispatch", warnings)
    structured_control("block", warnings)
    scope_bindings["scope_compare"] = P("S.uw").fresh("b")
    if warnings: scope_bindings["unbind_return"] = P("S.uw1").fresh("r")
    install_rules(g, os.path.dirname(__file__), "scope", bindings=scope_bindings,
                  sequences=scope_sequences, section="unwind-warnings" if warnings else "unwind")

    P("S.empty").call("NEXT").ret()
    p = P("S.decl")
    p.call("TSPEC").tok({TK_ID: "S.did0", "(": "S.dfp", ";": "S.empty"}, bad("declaration"))
    p = P("S.dfp")
    p.call("FPDECL").a(("LDI", "dsz", 8), ("LDI", "dar", 0)).branch({1: "S.dd"}, "S.dfa", [("CMPI", "fpn", 0)])
    P("S.dfa").a(("ALUI", "mul", "dsz", "fpn", 8), ("LDI", "dar", 1)).goto("S.dd")
    P("S.did0").a(("COPYW", "ips", "ps"), ("COPYW", "ipe", "pe")).goto("S.did")
    P("S.did").a(("LDI", "dsz", 8), ("LDI", "dar", 0)).branch({1: "S.dst"}, "S.dnx", [("CMPI", "td", 0)])
    P("S.dst").branch({(1, 2): "S.dst1"}, "S.dnx", [("CMPI", "tb", SBB)])
    P("S.dst1").call("ELSZ").a(("COPYW", "dsz", "es")).goto("S.dnx")
    P("S.dnx").call("NEXT").tok({"[": "S.darr", "(": "S.prototype"}, "S.dd")
    # Same parameter parser, no frame or parameter bindings for a prototype.
    sigsaved = ("sigmode", "fns", "fne", "pk", "vfn", "fn_fpwrap", "td", "tb", "bd")
    p = P("S.prototype").vpush(*sigsaved).a(("COPYW", "ps", "ips"), ("COPYW", "pe", "ipe")).call("BIND")
    p.a(("LDI", "t", 0), ("STX", "v", LOC, "t"), ("STX", "v", E.VAR, "t"), ("ALUI", "mul", "u", "v", 16))
    for j in range(16): p.a(("STX", "u", PDB+j, "t"))
    p.a(("LDI", "t", 1), ("STX", "v", E.FND, "t"), ("STX", "v", E.FRD, "td"), ("STX", "v", E.FRB, "tb"), ("COPYW", "fns", "ips"), ("COPYW", "fne", "ipe"), ("LDI", "sigmode", 1)).call("FN.params").vpop(*sigsaved).tok({",": "S.dcm"}, "S.dend")
    P("BP.end").branch({2: "BP.many"}, "BP.put", [("CMPI", "pk", 6)])
    P("BP.many").a(("LDI", "vfn", 1)).goto("BP.put")
    P("BP.put").a(("INTERN", "v", "fns", "fne"), ("STX", "v", E.VAR, "vfn")).ret()
    P("S.dd").call("DECLN").tok({"=": "S.din", ",": "S.dcm"}, "S.dend")
    P("S.dend").expect(";").call("NEXT").ret()
    P("S.dcm").call("DSTARS").tok({TK_ID: "S.did0"}, bad("declarator"))
    p = P("S.din")
    p.a(("COPYW", "lpp", "tpos")).call("NEXT").tok({"{": "S.lbr", E.TK_STR: "S.ls"}, "S.din0")
    p = P("S.ls")           # (measured, s34) imm r2, S; sub64 r1, r6, r2; .zero; each byte then the 0 at S - k
    p.call("CHARR").branch({1: "S.ls1"}, "S.din0", [("CMPI", "u", 1)])   # char *p = "...": the expression path
    P("S.ls1").a(("LDI", "t", 1), ("STX", "tpos", SKIPS, "t"), ("COPYW", "isl", "s"),
        ("COPYW", "ibytes", "dsz"), ("LDI", "imode", 0)).call("STRINGINIT").tok({",": "S.dcm"}, "S.dend")
    P("S.din0").a(("JUMP", "lpp")).call("NEXT").goto("S.din00")       # back to '=' (re-read) for the scalar path
    P("S.din00").branch({1: "S.initkind"}, bad("array initialiser"), [("CMPI", "dar", 0)])
    P("S.initkind").branch({1:"S.initbase"},"S.din1",[("LDX","lt","v",E.PTR),("CMPI","lt",0)])
    P("S.initbase").branch({(1,2):"S.dstruct"},"S.din1",[("LDX","lb","v",E.BASE),("CMPI","lb",SBB)])
    P("S.dstruct").call("NEXT").call("CP.tryinit").branch({1:"S.cpinit"},"S.structexpr",[("CMPI","cp_match",1)])
    P("S.cpinit").a(("LDI","imode",0),("COPYW","ivv","v"),("COPYW","ibytes","dsz")).vpush("bd","tb","cp_pars").call("INITLIST").vpop("bd","tb","cp_pars").goto("S.cpclose")
    P("S.cpclose").branch({1:"S.cpdone"},"S.cpnext",[("CMPI","cp_pars",0)])
    P("S.cpnext").expect(")").a(("ALUI","sub","cp_pars","cp_pars",1)).call("NEXT").goto("S.cpclose")
    P("S.cpdone").tok({",":"S.dcm"},"S.dend")
    q = P("S.structexpr")
    q.o("  imm r0, ").num("s").o("\n  sub64 r0, r6, r0\n"); emit(q,"push").vpush("s","v","bd","tb","lt","lb").call("EXPR").vpop("s","v","bd","tb","lt","lb").call("AS.struct").tok({",":"S.dcm"},"S.dend")
    # Reference cplit lookahead: token recognition only, never trial-parse an expression.
    P("CP.tryinit").a(("COPYW","cp_scanback","tpos"),("LDI","cp_pars",0),("LDI","cp_match",0)).goto("CP.leading")
    P("CP.leading").tok({"(":"CP.morepar"},"CP.istype")
    P("CP.morepar").a(("ALUI","add","cp_pars","cp_pars",1)).call("NEXT").goto("CP.leading")
    P("CP.istype").branch({2:"CP.istype1"},"CP.nomatch",[("CMPI","cp_pars",0)])
    P("CP.istype1").tok({**{w:"CP.scanstart" for w in TWORDS},"struct":"CP.scanstart","union":"CP.scanstart","enum":"CP.scanstart",TK_ID:"CP.typedef"},"CP.nomatch")
    P("CP.typedef").call("ISTD").branch({1:"CP.scanstart"},"CP.nomatch")
    P("CP.scanstart").a(("LDI","cp_depth",0)).goto("CP.scan")
    P("CP.scan").tok({"(":"CP.scanopen",")":"CP.scanclose","eof":"CP.nomatch"},"CP.scanmore")
    P("CP.scanopen").a(("ALUI","add","cp_depth","cp_depth",1)).goto("CP.scanmore")
    P("CP.scanclose").branch({1:"CP.aftertype"},"CP.scandown",[("CMPI","cp_depth",0)])
    P("CP.scandown").a(("ALUI","sub","cp_depth","cp_depth",1)).goto("CP.scanmore")
    P("CP.scanmore").call("NEXT").goto("CP.scan")
    P("CP.aftertype").call("NEXT").tok({"{":"CP.matched"},"CP.nomatch")
    P("CP.matched").a(("LDI","cp_match",1),("ALUI","sub","cp_pars","cp_pars",1)).ret()
    P("CP.nomatch").a(("JUMP","cp_scanback")).call("NEXT").ret()
    # One initializer walker; only the address mode differs across storage classes.
    P("S.lbr").a(("LDI", "imode", 0), ("COPYW", "ivv", "v"), ("COPYW", "ibytes", "dsz")).vpush("bd", "tb").call("INITLIST").vpop("bd", "tb").tok({",": "S.dcm"}, "S.dend")
    P("INITADDR").branch({1: "IA.local"}, "IA.named", [("CMPI", "imode", 0)])
    P("IA.local").a(("ALU", "sub", "t", "isl", "ioff")).o("  imm r2, ").num("t").o("\n  sub64 r1, r6, r2\n").ret()
    P("IA.named").branch({1: "IA.global"}, "IA.static", [("CMPI", "imode", 1)])
    P("IA.global").o("  .lea r1, g_").a(("SPAN2", "inps", "inpe")).goto("IA.offset")
    P("IA.static").o("  .lea r1, ls").num("inlabel").goto("IA.offset")
    P("IA.offset").o("\n").branch({1: "RET"}, "IA.add", [("CMPI", "ioff", 0)])
    P("IA.add").o("  imm r2, ").num("ioff").o("\n  add64 r1, r1, r2\n").ret()
    q = P("S.din1")
    q.vpush("s", "v", "bd", "tb").call("NEXT")
    if warnings: q.a(("LDX", "wi_target", "v", E.PTR)).call("WI.expr")
    else: q.call("EXPR")
    q.a(("COPYW", "rvt", "vt"), ("COPYW", "rvb", "vb")).vpop("s", "v", "bd", "tb")
    q.a(("LDX", "vt", "v", E.PTR), ("LDX", "vb", "v", E.BASE)).call("ASSIGNCV").goto("S.din2")
    q = P("S.din2")
    q.o("  imm r2, ").num("s").o("\n  sub64 r1, r6, r2\n").call("STOREV").tok({",": "S.dcm"}, "S.dend")
    P("S.darr").call("VL.classify").branch({1:"VL.decl"},"S.fixedarray",[("CMPI","vl_dynamic",1)])
    P("S.fixedarray").call("DIMS").call("ELSZ").a(("ALU", "mul", "dsz", "prd", "es"), ("COPYW", "dar", "drk")).goto("S.dd")
    p = P("S.ret")
    p.call("NEXT").tok({";": "S.rv"}, "S.re")
    P("S.rv").o("  jump R").num("rl").o("\n").call("NEXT").ret()     # return; (measured, old E3)
    p = P("S.re")
    p.branch({1: "S.rs0"}, "S.re1", [("CMPI", "rd", 0)])
    P("S.rs0").branch({(1, 2): "S.rs"}, "S.re1", [("CMPI", "rb", SBB)])
    p = P("S.rs")
    p.tok({TK_ID: "S.rs1"}, bad("struct return"))
    q = P("S.rs1")
    q.a(("COPYW", "ips", "ps"), ("COPYW", "ipe", "pe")).call("LOOKUP").call("NOARR").branch({1: "S.rs2"}, bad("struct return"), [("CMP", "vb", "rb")])
    q = P("S.rs2")
    q.branch({1: "S.rs3"}, bad("struct return"), [("CMPI", "vt", 0)])
    q = P("S.rs3")
    addr(q)
    q.o("  mov r1, r0\n  .lea r0, __rv_").a(("SPAN2", "fns", "fne")).o("\n").call("NEXT").expect(";").call("WCOPY")
    q.o("  jump R").num("rl").o("\n").call("NEXT").ret()
    # Returns, arguments and assignment all use the same aggregate copy.
    P("WCOPY").a(("ALUI", "sub", "t", "rb", SBB), ("LDX", "sz", "t", SSZ)).goto("COPYSTRUCT")
    p = P("S.re1")
    p.call("EXPR").expect(";").a(("COPYW", "rvt", "vt"), ("COPYW", "rvb", "vb"),
        ("COPYW", "vt", "rd"), ("COPYW", "vb", "rb")).call("ASSIGNCV")
    p.branch({1: "S.rscalar"}, "S.rj", [("CMPI", "vt", 0)])
    P("S.rscalar").branch({(BOOL, DBL, FLT): "S.rj"}, "S.rn", [("RLD", "vb")])
    P("S.rn").call("NARROW").goto("S.rj")
    p = P("S.rj")
    p.o("  jump R").num("rl").o("\n").call("NEXT").ret()
    P("S.expr").a(("LDI", "stl", 1)).call("CEXPR").expect(";").call("NEXT").ret()
    structured_control("if", warnings)
    structured_control("switch", warnings)
    structured_control("loops", warnings)
    # expressions: EXPR = assignment | the ladder
    p = P("CEXPR")    # e , e , ...: the value is the last; a discarded bare identifier gives its address only (measured)
    p.vpush("cv").a(("LDI", "cv", 1)).label("CX.l")
    p.a(("COPYW", "sst", "stl")).call("EXPR").tok({",": "CX.c"}, "CX.e")
    P("CX.c").a(("COPYW", "stl", "sst")).call("NEXT").goto("CX.l")
    P("CX.e").vpop("cv").ret()
    p = P("EXPR")     # st1: this EXPR is a whole expression statement (nested ones are not); cv1: a comma operand
    p.a(("COPYW", "st1", "stl"), ("LDI", "stl", 0), ("COPYW", "cv1", "cv"), ("LDI", "cv", 0)).tok({TK_ID: "X.id", "(": "LP.scan"}, "EX.l")
    P("EX.l").call("E%d" % LEVELS[0]).call("QTAIL").ret()
    # Parentheses preserve lvalues. Scan syntax before emitting anything;
    # the address walker evaluates the accepted operand exactly once.
    P("LP.scan").a(("COPYW", "lp_start", "tpos"), ("LDI", "lp_depth", 1)).call("NEXT").tok(
        {**{w: "LP.fallback" for w in TWORDS}, "struct": "LP.fallback", "union": "LP.fallback", "enum": "LP.fallback", TK_ID: "LP.typedef"}, "LP.scan0")
    P("LP.typedef").call("ISTD").branch({1: "LP.fallback"}, "LP.scan0")
    P("LP.scan0").tok({"(": "LP.open", ")": "LP.close", "eof": "LP.fallback"}, "LP.next")
    P("LP.open").a(("ALUI", "add", "lp_depth", "lp_depth", 1)).goto("LP.next")
    P("LP.close").a(("ALUI", "sub", "lp_depth", "lp_depth", 1)).branch({1: "LP.after"}, "LP.next", [("CMPI", "lp_depth", 0)])
    P("LP.next").call("NEXT").goto("LP.scan0")
    lops = {"=": "PX.as", "++": "LP.inc", "--": "LP.dec", **{o+"=": "LV.c"+o for o in E.CASOPS}}
    P("LP.after").call("NEXT").tok({o: "LP.parse" for o in lops}, "LP.fallback")
    P("LP.fallback").a(("JUMP", "lp_start")).call("NEXT").goto("EX.l")
    P("LP.parse").a(("JUMP", "lp_start")).call("NEXT").call("LP.addr").tok(lops, bad("parenthesized lvalue"))
    P("LP.addr").tok({"(": "LP.par", "*": "LP.star", TK_ID: "PRE.addr"}, bad("parenthesized lvalue"))
    P("LP.par").call("NEXT").call("LP.addr").expect(")").call("NEXT").ret()
    P("LP.star").call("NEXT").call("UNARY").call("DOWN").ret()
    for name, op in (("inc", "+"), ("dec", "-")):
        P("LP."+name).call("CSTEP").call("POST."+op).call("C%d" % LEVELS[0]).call("QTAIL").ret()
    # c ? a : b -- labels as if/else (measured): jumpz L a; a; jump L b; L a: b; L b:
    P("QTAIL").tok({"?": "QT"}, "RET")
    p = P("QT")
    p.a(("ALUI", "add", "lab", "lab", 1), ("COPYW", "a", "lab"), ("ALUI", "add", "lab", "lab", 1), ("COPYW", "b", "lab"))
    emit(p.call("FTRUTH"), "jumpz").call("NEXT").a(("COPYW", "qtpos", "tpos"), ("OLEN", "qtout")).vpush("a", "b", "qtpos", "qtout").call("EXPR").vpop("a", "b", "qtpos", "qtout").expect(":")
    emit(p, "jump_b")
    emit(p, "label_a").vpush("a", "b", "qtpos", "qtout", "vt", "vb").call("NEXT").call("E%d" % LEVELS[0]).call("QTAIL").vpop("a", "b", "qtpos", "qtout", "lt", "lb")
    p.branch({1: "QT.merge"}, "QT.floatcheck", [("CMP", "vb", "lb")])
    P("QT.floatcheck").branch({(DBL, FLT): "QT.common"}, "QT.floatleft", [("RLD", "vb")])
    P("QT.floatleft").branch({(DBL, FLT): "QT.common"}, "QT.merge", [("RLD", "lb")])
    P("QT.common").branch({1: "QT.common0"}, "DEAD.qt", [("CMPI", "vt", 0)])
    P("QT.common0").branch({1: "QT.common1"}, "DEAD.qt", [("CMPI", "lt", 0)])
    P("QT.common1").call("TAX").a(("COPYW", "axr", "ax"), ("COPYW", "vb", "lb")).call("TAX").a(("ALUI", "mul", "t", "ax", 16), ("ALU", "add", "t", "t", "axr"), ("LDX", "qtc", "t", CKT)).branch({AX.index("f64"): "QT.double", AX.index("f32"): "QT.single"}, "DEAD.qt", [("RLD", "qtc")])
    for label, cv in (("double", "d"), ("single", "s")):
        q = P("QT." + label).a(("OCUT", "qtdiscard", "qtout"), ("JUMP", "qtpos")).call("NEXT").vpush("a", "b").call("EXPR").call("TO." + cv).vpop("a", "b").expect(":")
        emit(q, "jump_b")
        emit(q, "label_a").vpush("b").call("NEXT").call("E%d" % LEVELS[0]).call("QTAIL").call("TO." + cv).vpop("b")
        emit(q, "label_b").a(("LDI", "vt", 0), ("LDI", "vb", DBL if cv == "d" else FLT)).ret()
    p = P("QT.merge")
    emit(p, "label_b").branch({1: "QT.1"}, "DEAD.qt", [("CMP", "vt", "lt")])
    P("QT.1").branch({1: "QT.scalar"}, "QT.same", [("CMPI", "vt", 0)])
    P("QT.same").branch({1: "RET"}, "DEAD.qt", [("CMP", "vb", "lb")])
    q = P("QT.scalar")
    for _, code, *_ in TYINT:
        nx = q.fresh("next")
        q.branch({1: "QT.int"}, nx, [("CMPI", "vb", code)])
        q = P(nx)
    q.goto("QT.same")
    # Common integer type is the existing type(t1 + t2) row, not a new ladder.
    P("QT.int").call("TAX").a(("COPYW", "axr", "ax"), ("COPYW", "vb", "lb")).call("TAX").a(("ALUI", "mul", "t", "ax", 16), ("ALU", "add", "t", "t", "axr"), ("LDX", "rs", "t", CKT)).goto("RESD")
    g.on("DEAD.qt", range(257), "DEAD", E.rej("not covered: ?: arms of different types"), "r")
    P("X.id").a(("COPYW", "ips", "ps"), ("COPYW", "ipe", "pe")).call("NEXT").tok(dict({"=": "X.as", "(": "X.cpf", "++": "X.inc", "--": "X.dec"}, **{o + "=": "X.c" + o for o in E.CASOPS}), "X.var")
    p = P("X.as")
    p.call("LOOKUP").call("NOARR")
    addr(p).goto("PX.as")       # names and computed lvalues share assignment
    # One assignment conversion: target vt/vb, source rvt/rvb, value in r0.
    P("ASSIGNCV").branch({1: "ACV.scalar"}, "ACV.u", [("CMPI", "vt", 0)])
    P("ACV.scalar").branch({1: "BOOLCV"}, "ACV.double", [("CMPI", "vb", BOOL)])
    P("ACV.double").branch({1: "ACV.d"}, "ACV.float", [("CMPI", "vb", DBL)])
    P("ACV.float").branch({1: "ACV.s"}, "ACV.int", [("CMPI", "vb", FLT)])
    P("ACV.int").branch({1: "ACV.u"}, "ACV.i", [("CMPI", "vb", UNS + 8)])
    for suffix in ("d", "s", "i", "u"):
        P("ACV." + suffix).vpush("vt", "vb").a(("COPYW", "vt", "rvt"), ("COPYW", "vb", "rvb")).call("TO." + suffix).vpop("vt", "vb").ret()
    P("CSTEP").branch({1: "CSTEP.scalar"}, "STEPTY", [("CMPI", "vt", 0)])
    P("CSTEP.scalar").branch({(DBL, FLT): "CSTEP.fp"}, "STEPTY", [("RLD", "vb")])
    P("CSTEP.fp").a(("LDI", "stp", 1)).ret()
    for o in E.CASOPS:
        q = P("X.c" + o)    # addr; push; load; push; rhs (a pointer's scaled); pop; op; pop; store
        q.call("LOOKUP").call("NOARR")
        addr(q).goto("LV.c" + o)
        q = P("LV.c" + o).call("CSTEP")
        if o not in ("+", "-"):
            q.branch({1: (nx2 := "X.c%s.i" % o)}, "DEAD.nint", [("CMPI", "vt", 0)])
            q = P(nx2)
        emit(q, "push").call("LOADV")
        emit(q, "push").vpush("vt", "vb", "stp").call("NEXT").call("EXPR").a(("COPYW", "rvb", "vb"), ("COPYW", "rvt", "vt")).vpop("vt", "vb", "stp")
        q.call("TAX").branch({(AX.index("f32"), AX.index("f64")): "BC.float" + o}, "BC.rtype" + o, [("RLD", "ax")])
        P("BC.rtype" + o).vpush("vt", "vb").a(("COPYW", "vt", "rvt"), ("COPYW", "vb", "rvb")).call("TAX").vpop("vt", "vb").branch({(AX.index("f32"), AX.index("f64")): "BC.float" + o}, "BC.ordinary" + o, [("RLD", "ax")])
        bf = P("BC.float" + o).vpush("vt", "vb").a(("COPYW", "lt", "vt"), ("COPYW", "lb", "vb"), ("COPYW", "vt", "rvt"), ("COPYW", "vb", "rvb")).call("OPX." + o)
        bf.a(("COPYW", "rvt", "vt"), ("COPYW", "rvb", "vb")).vpop("vt", "vb").call("ASSIGNCV")
        emit(bf, "pop1").call("STOREV").ret()
        P("BC.ordinary" + o).vpush("vt","vb").a(("COPYW","vt","rvt"),("COPYW","vb","rvb")).call("NODBL0").vpop("vt","vb").goto("BC.integer" + o)
        q = P("BC.integer" + o)
        q.branch({1: "X.c%s.m" % o}, "X.c%s.s" % o, [("CMPI", "stp", 1)])
        P("X.c%s.s" % o).o("  imm r2, ").num("stp").o("\n  mul64 r0, r0, r2\n").goto("X.c%s.m" % o)
        q = P("X.c%s.m" % o)
        # the variable's class picks the form: unsigned long (the unsigned spelling), unsigned int (masked),
        # else signed; an unsigned right side of another class is not covered (measured, p49/p50)
        emit(q, "pop1").branch({1: "X.c%s.v" % o}, "X.c%s.n" % o, [("CMPI", "vt", 0)])
        P("X.c%s.v" % o).branch({1: "X.c%s.U" % o}, "X.c%s.v1" % o, [("CMPI", "vb", UNS + 8)])
        P("X.c%s.U" % o).o(E.optext(o, True)).goto("X.c%s.st" % o)
        P("X.c%s.v1" % o).branch({1: "X.c%s.w" % o}, "X.c%s.v2" % o, [("CMPI", "vb", UNS + 4)])
        P("X.c%s.v2" % o).branch({1: "X.c%s.v3" % o}, "X.c%s.n" % o, [("CMPI", "rvt", 0)])
        P("X.c%s.v3" % o).branch({1: "X.c%s.U" % o}, "X.c%s.v4" % o, [("CMPI", "rvb", UNS + 8)])   # an unsigned long right side: the unsigned spelling (p50)
        P("X.c%s.v4" % o).branch({1: "DEAD.ui"}, "X.c%s.n" % o, [("CMPI", "rvb", UNS + 4)])
        P("X.c%s.w" % o).branch({1: "DEAD.ui"}, "X.c%s.w1" % o, [("CMPI", "rvb", UNS + 8)])
        P("X.c%s.w1" % o).o("  imm r2, %d\n  and64 r0, r0, r2\n  and64 r1, r1, r2\n" % U32M + E.optext(o, True) + UIM).goto("X.c%s.st" % o)
        q = P("X.c%s.n" % o)
        q.o(E.optext(o)).call("NARU").goto("X.c%s.st" % o)
        q = P("X.c%s.st" % o)
        emit(q, "pop1").call("STOREV").ret()
    P("NODBL").branch({1: "NODBL.l"}, "NODBL.r", [("CMPI", "lb", DBL)])      # a double VALUE (not a pointer to one)
    P("NODBL.l").branch({1: "DEAD.dbl"}, "NODBL.r", [("CMPI", "lt", 0)])
    P("NODBL.r").branch({1: "NODBL.r2"}, "RET", [("CMPI", "vb", DBL)])
    P("NODBL.r2").branch({1: "DEAD.dbl"}, "RET", [("CMPI", "vt", 0)])
    g.on("DEAD.dbl", range(257), "DEAD", E.rej("not covered: double operand"), "r")
    # NARU: an unsigned char/short result masked back before its store (measured, p46); others as they are
    P("TAX").branch({(1, 2): "TAX.p"}, "TAX.0", [("CMPI", "vt", 1)])
    P("TAX.p").a(("LDI", "ax", AX.index("ptr"))).ret()
    q = P("TAX.0")
    for code, name in ((1, "i8"), (2, "i16"), (4, "i32"), (8, "i64"), (UNS + 1, "u8"), (UNS + 2, "u16"), (UNS + 4, "u32"), (UNS + 8, "u64"),
                       (BOOL, "u8"), (0, "void"), (DBL, "f64"), (FLT, "f32"), (FPB, "ptr"), (FPV, "ptr")):
        hit, nx = q.fresh("h"), q.fresh("n")
        q.branch({1: hit}, nx, [("CMPI", "vb", code)])
        P(hit).a(("LDI", "ax", AX.index(name))).ret()
        q = P(nx)
    q.branch({(1, 2): "TAX.s"}, "DEAD.w", [("CMPI", "vb", SBB)])
    P("TAX.s").a(("LDI", "ax", AX.index("struct"))).ret()
    P("NARU").branch({1: "NARU.1"}, "NARU.b", [("CMPI", "vt", 0)])
    P("NARU.1").branch({1: "TO.b"}, "NARU.integer", [("CMPI", "vb", BOOL)])
    q = P("NARU.integer")          # the unsigned narrow rows of tyinfo: masked back to their size
    for t, vb, sz, un, nr in TYINT:
        if not (un and sz < 8):
            continue
        hit, nx = q.fresh("h"), q.fresh("n")
        q.branch({1: hit}, nx, [("CMPI", "vb", vb)])
        P(hit).o("  imm r2, %d\n  and64 r0, r0, r2\n" % ((1 << (8 * sz)) - 1)).ret()
        q = P(nx)
    q.ret()
    P("NARU.b").ret()
    P("STEPTY").a(("LDI", "stp", 1)).branch({1: "STY.s"}, "STY.p", [("CMPI", "vt", 0)])
    P("STY.s").branch({1: "RET"}, "STY.s0", [("CMPI", "vb", BOOL)])
    P("STY.s0").branch({1: "DEAD.nint"}, "STY.s1", [("CMPI", "vb", 0)])
    P("STY.s1").branch({(0, 1): "RET"}, "STY.s2", [("CMPI", "vb", 8)])
    P("STY.s2").branch({(0, 1): "STY.s3"}, "DEAD.nint", [("CMPI", "vb", UNS + 8)])
    P("STY.s3").branch({2: "RET"}, "DEAD.nint", [("CMPI", "vb", UNS)])
    P("STY.p").call("ISFP").branch({1: "DEAD.nint"}, "STY.p1", [])
    P("STY.p1").a(("COPYW", "td", "vt"), ("ALUI", "sub", "td", "td", 1), ("COPYW", "tb", "vb")).call("ELSZ").a(("COPYW", "stp", "es")).ret()
    g.on("DEAD.nint", range(257), "DEAD", E.rej("not covered: pointer or non-int in op= ++ --"), "r")
    P("INTONLY").branch({1: "IO.b"}, bad("pointer or non-int in op= ++ --"), [("CMPI", "vt", 0)])
    P("IO.b").branch({1: "RET"}, bad("pointer or non-int in op= ++ --"), [("CMPI", "vb", 4)])
    for op, name in (("+", "add"), ("-", "sub")):
        P("FPSTEP." + op).branch({1: "FPSTEP.d" + op}, "FPSTEP.s" + op, [("CMPI", "vb", DBL)])
        for suffix, bits in (("d", 4607182418800017408), ("s", 1065353216)):
            P("FPSTEP." + suffix + op).o("  imm r1, %d\n  %s r0, r0, r1\n" % (bits, FPU[suffix + name])).ret()
    for nm, o, fix in (("X.inc", "+", "post_inc"), ("X.dec", "-", "post_dec")):
        q = P(nm)           # addr; push; load; push; 1; pop; op; pop; store; undo to the old value
        q.call("LOOKUP").call("NOARR").call("CSTEP")
        addr(q)
        q.call("POST." + o).call("C%d" % LEVELS[0]).call("QTAIL").ret()
        # A computed member/element address uses the same update as a name.
        q = P("POST." + o)
        q.branch({1: "POST.booltest" + o}, "POST.normal" + o, [("CMPI", "vt", 0)])
        P("POST.booltest" + o).branch({BOOL: "POST.bool" + o, (DBL, FLT): "POST.float" + o}, "POST.normal" + o, [("RLD", "vb")])
        qf = P("POST.float" + o)
        emit(qf, "push").call("LOADRAW")
        emit(qf, "push").call("FPSTEP." + o).o("  load64 r1, [r7+8]\n").call("STOREV").o("  load64 r0, [r7+0]\n  .frame -16\n").call("NEXT").ret()
        qb = P("POST.bool" + o)
        emit(qb, "push").call("LOADRAW")
        emit(qb, "push").o("  imm r1, 1\n  %s r0, r0, r1\n" % ("add64" if o == "+" else "sub64")).call("TO.b").o("  load64 r1, [r7+8]\n").call("STOREV").o("  load64 r0, [r7+0]\n  .frame -16\n").call("NEXT").ret()
        q = P("POST.normal" + o)
        emit(q, "push").call("LOADRAW")
        emit(q, "push")
        emit(q, "one")
        emit(q, "pop1").o(E.optext(o))
        emit(q, "pop1").call("STOREV")
        emit(q, fix).call("NEXT").ret()
        P("PX." + nm[2:]).call("CSTEP").goto("POST." + o)
        P("MB." + nm[2:]).branch({1: "PX." + nm[2:]}, bad("increment of array member"), [("CMPI", "marr", 0)])
    p = P("X.var")      # an identifier operand, then the rest of the ladder with it as the left operand
    p.a(("INTERN", "v", "ips", "ipe")).branch({1:"X.func"},"X.enumcheck",[("CMP","v","funcid")])
    P("X.enumcheck").a(("LDX","t","v",END_)).branch({1:"X.enum"},"X.var1",[("CMPI","t",1)])
    P("X.func").call("UF").call("POSTIX").call("C%d" % LEVELS[0]).call("QTAIL").ret()
    P("X.enum").a(("LDX", "nv", "v", ENV), ("LDI", "nx", 1)).o("  imm r0, ").call("NUMOUT").o("\n").a(("LDI", "vt", 0), ("LDI", "vb", 4)).call("C%d" % LEVELS[0]).call("QTAIL").ret()
    p = P("X.var1")
    p.a(("LDI", "isfn", 0)).call("FNVAL").branch({1: "X.fnv"}, "X.v2", [("CMPI", "isfn", 1)])
    P("X.fnv").a(("LDI", "rkok", 0)).call("POSTIX").call("C%d" % LEVELS[0]).call("QTAIL").ret()
    p = P("X.v2")
    p.call("LOOKUP")
    addr(p)
    p.tok({".": "X.mb", ",": "X.cm", ";": "X.sm"}, "X.vl")
    P("X.cm").branch({1: "RET"}, "X.vl", [("CMPI", "cv1", 1)])
    P("X.sm").branch({1: "RET"}, "X.vl", [("CMPI", "st1", 1)])
    P("X.vl").call("VLOAD").call("POSTIX").call("C%d" % LEVELS[0]).call("QTAIL").ret()
    P("X.mb").call("MEMB").call("C%d" % LEVELS[0]).call("QTAIL").ret()
    P("X.cpf").a(("INTERN", "v", "ips", "ipe")).branch({1: "X.pf"}, "X.call", [("CMP", "v", "pfid")])
    P("X.pf").call("PF").call("C%d" % LEVELS[0]).call("QTAIL").ret()
    P("X.call").call("U.call").call("C%d" % LEVELS[0]).call("QTAIL").ret()
    ladder("E", "UNARY")
    ladder("C", None)
    P("U.str").o("  .lea r0, S").num("sk").o("\n").a(("ALUI", "add", "sk", "sk", 1), ("ALUI", "add", "lab", "lab", 1), ("LDI", "vt", 1), ("LDI", "vb", 1), ("LDI", "rkok", 0)).call("NEXT").tok({E.TK_STR: "DEAD.adj"}, "POSTIX")
    g.on("DEAD.adj", range(257), "DEAD", E.rej("not covered: adjacent string literals"), "r")
    q = P("U.cpl")       # ~x: imm r1, -1; xor64 (measured); the operand's type is kept
    q.call("NEXT").call("UNARY").call("NODBL0").o("  imm r1, -1\n  xor64 r0, r0, r1\n").branch({1: "NARU"}, "RET", [("CMPI", "vb", UNS + 4)])
    P("NODBL0").branch({1: "NODBL0.1"}, "NODBL0.p", [("CMPI", "vb", DBL)])
    P("NODBL0.1").branch({1: "DEAD.dbl"}, "NODBL0.p", [("CMPI", "vt", 0)])
    P("NODBL0.p").branch({1: "RET"}, "DEAD.pa", [("CMPI", "vt", 0)])
    # sizeof: a constant, `imm r0, N`; the operand emits nothing (measured). A type, a variable,
    # or a variable with subscripts (each drops one dimension); anything else is not covered
    P("U.szof").call("NEXT").a(("COPYW","szpos","tpos"),("LDI","szparen",0)).tok({"(": "SZ.p", "*": "SZ.star", TK_ID: "SZ.id"}, "SZ.expr")
    P("SZ.p").a(("LDI","szparen",1)).call("NEXT").tok({**{w: "SZ.t" for w in TWORDS}, "struct": "SZ.t", "union": "SZ.t", "enum": "SZ.t", "*": "SZ.star", TK_ID: "SZ.pid"}, "SZ.expr")
    P("SZ.t").call("TSPEC").expect(")").call("ELSZ").a(("COPYW", "sz", "es")).goto("SZ.out")
    P("SZ.id").a(("COPYW", "ips", "ps"), ("COPYW", "ipe", "pe"), ("LDI", "drop", 0)).call("NEXT").tok(
        {"(": "SZ.expr", ".": "SZ.memberstart", "->": "SZ.memberstart", "++": "SZ.expr", "--": "SZ.expr"}, "SZ.idvalue")
    P("SZ.idvalue").call("LOOKUP").goto("SZ.sub")
    P("SZ.pid").call("ISTD").branch({1: "SZ.t"}, "SZ.pvalue")
    P("SZ.pvalue").a(("COPYW", "ips", "ps"), ("COPYW", "ipe", "pe"), ("LDI", "drop", 0)).call("NEXT").tok({")":"SZ.pidvar", "[":"SZ.pidvar", ".":"SZ.memberstart", "->":"SZ.memberstart"}, "SZ.expr")
    P("SZ.pidvar").call("LOOKUP").goto("SZ.sub")
    P("SZ.memberstart").call("LOOKUP").goto("SZ.member")
    P("SZ.member").tok({".": "SZ.memberdot", "->": "SZ.memberptr"}, "SZ.memberend")
    P("SZ.memberdot").branch({1: "SZ.memberstruct"}, "DEAD.mb", [("CMPI", "vt", 0)])
    P("SZ.memberptr").branch({1: "SZ.memberstruct"}, "DEAD.mb", [("CMPI", "vt", 1)])
    P("SZ.memberstruct").branch({(1, 2): "SZ.membername"}, "DEAD.mb", [("CMPI", "vb", SBB)])
    P("SZ.membername").a(("ALUI", "sub", "sid", "vb", SBB)).call("NEXT").tok({TK_ID: "SZ.memberinfo"}, "DEAD.mb")
    P("SZ.memberinfo").call("MB.INFO").branch({1: "DEAD.mb"}, "SZ.membernext", [("CMPI", "ms", 0)])
    P("SZ.membernext").a(("COPYW", "sz", "ms")).branch({1: "SZ.memberadvance"}, "SZ.memberarray", [("CMPI", "marr", 0)])
    P("SZ.memberarray").a(("ALUI", "add", "vt", "vt", 1)).goto("SZ.memberadvance")
    P("SZ.memberadvance").call("NEXT").goto("SZ.member")
    P("SZ.memberend").tok({"[": "SZ.expr", "(": "SZ.expr", "++": "SZ.expr", "--": "SZ.expr"}, "SZ.memberclose")
    P("SZ.memberclose").branch({1: "SZ.memberparen"}, "SZ.exprout", [("CMPI", "szparen", 1)])
    P("SZ.memberparen").tok({")": "SZ.memberdone"}, "SZ.expr")
    P("SZ.memberdone").call("NEXT").goto("SZ.exprout")
    p = P("SZ.sub")
    p.tok({"[": "SZ.sk"}, "SZ.subend")
    P("SZ.subend").branch({1: "SZ.pend"}, "SZ.nend", [("CMPI", "szparen", 1)])
    P("SZ.pend").tok({")": "SZ.cl"}, "SZ.expr")
    P("SZ.nend").call("VL.sizecheck").branch({2:"VL.sizeout"},"SZ.nfixed",[("CMPI","vl_bytes",0)])
    P("SZ.nfixed").call("SZ.calc").goto("SZ.exprout")
    P("SZ.cl").expect(")").goto("SZ.var")
    p = P("SZ.sk")       # skip to the matching ']'
    p.a(("LDI", "dep", 1)).call("NEXT").label("SZ.sl")
    p.tok({"[": "SZ.so", "]": "SZ.sx"}, "SZ.sn")
    P("SZ.sn").call("NEXT").goto("SZ.sl")
    P("SZ.so").a(("ALUI", "add", "dep", "dep", 1)).goto("SZ.sn")
    P("SZ.sx").a(("ALUI", "sub", "dep", "dep", 1)).branch({1: "SZ.sd"}, "SZ.sn", [("CMPI", "dep", 0)])
    P("SZ.sd").a(("ALUI", "add", "drop", "drop", 1)).call("NEXT").goto("SZ.sub")
    p = P("SZ.var")      # vt vb ar vid from LOOKUP
    p.call("VL.sizecheck").branch({2:"VL.sizeclose"},"SZ.fixedvar",[("CMPI","vl_bytes",0)])
    P("SZ.fixedvar").call("SZ.calc").goto("SZ.out")
    p = P("SZ.calc")
    p.a(("COPYW", "td", "vt"), ("COPYW", "tb", "vb")).branch({1: "SZ.sc"}, "SZ.ar", [("CMPI", "ar", 0)])
    P("SZ.sc").branch({1: "SZ.sc1"}, "SZ.pd", [("CMPI", "drop", 0)])
    P("SZ.pd").a(("ALU", "sub", "td", "td", "drop")).branch({0: "DEAD.szx"}, "SZ.sc1", [("CMPI", "td", 0)])
    P("SZ.sc1").call("ELSZ").a(("COPYW", "sz", "es")).ret()
    q = P("SZ.ar")
    q.branch({2: "DEAD.szx"}, "SZ.ar1", [("CMP", "drop", "ar")])
    q = P("SZ.ar1")
    q.a(("ALUI", "sub", "td", "td", 1)).call("ELSZ").a(("COPYW", "sz", "es"), ("COPYW", "k2", "drop")).label("SZ.al")
    q.branch({0: "SZ.a1"}, "RET", [("CMP", "k2", "ar")])
    P("SZ.a1").a(("ALUI", "mul", "u", "vid", 8), ("ALU", "add", "u", "u", "k2"), ("LDX", "u", "u", DIM), ("ALU", "mul", "sz", "sz", "u"), ("ALUI", "add", "k2", "k2", 1)).goto("SZ.al")
    # sizeof *name / **name: inspect the same descriptor as named arrays,
    # without decaying a remaining array dimension or evaluating a load.
    P("SZ.star").a(("LDI", "drop", 0)).label("SZ.stars").tok({"*": "SZ.stars.next", TK_ID: "SZ.starid"}, "SZ.expr")
    P("SZ.stars.next").a(("ALUI", "add", "drop", "drop", 1)).call("NEXT").goto("SZ.stars")
    P("SZ.starid").a(("COPYW", "ips", "ps"), ("COPYW", "ipe", "pe")).call("NEXT").tok(
        {"[": "SZ.expr", "(": "SZ.expr", ".": "SZ.expr", "->": "SZ.expr", "++": "SZ.expr", "--": "SZ.expr"}, "SZ.starcalc")
    P("SZ.starcalc").call("LOOKUP").call("SZ.calc").branch({1: "SZ.starclose"}, "SZ.exprout", [("CMPI", "szparen", 1)])
    P("SZ.starclose").tok({")": "SZ.stardone"}, "SZ.expr")
    P("SZ.stardone").call("NEXT").goto("SZ.exprout")
    # For a general operand, reuse unary/expression parsing and discard its
    # emitted instructions. Save marks on the value stack for nested sizeof.
    # The existing named-array route above retains full dimension sizes.
    # Preserve a literal array's decoded extent under sizeof. Recognise only
    # the complete unary operand (with redundant parentheses); other syntax
    # returns to the common expression/type path without changing pool ids.
    P("SZ.expr").a(("JUMP","szpos"),("LDI","szlp",0)).call("NEXT").goto("SZ.literal0")
    P("SZ.literal0").tok({"(":"SZ.literalpar",E.TK_STR:"SZ.literal"},"SZ.general")
    P("SZ.literalpar").a(("ALUI","add","szlp","szlp",1)).call("NEXT").goto("SZ.literal0")
    P("SZ.literal").a(("LDI","szlit",1)).goto("SZ.lwalk")
    strwalk("SZ.lwalk","SZ.lbyte","SZ.lend")
    P("SZ.lbyte").a(("ALUI","add","szlit","szlit",1)).goto("SZ.lwalk.w")
    P("SZ.lend").call("NEXT").goto("SZ.lclose")
    P("SZ.lclose").branch({1:"SZ.ltail"},"SZ.lparen",[("CMPI","szlp",0)])
    P("SZ.lparen").tok({")":"SZ.lpop"},"SZ.general")
    P("SZ.lpop").a(("ALUI","sub","szlp","szlp",1)).call("NEXT").goto("SZ.lclose")
    P("SZ.ltail").tok({k:"SZ.general" for k in ("[","(",".","->","++","--")},"SZ.lvalue")
    P("SZ.lvalue").a(("COPYW","sz","szlit"),("ALUI","add","sk","sk",1),("ALUI","add","lab","lab",1)).goto("SZ.exprout")
    P("SZ.general").a(("JUMP","szpos"),("OLEN","szmark")).vpush("szmark","si_active").a(("LDI","si_active",0)).call("NEXT").call("UNARY").vpop("szmark","si_active").a(("OCUT","szdiscard","szmark")).branch({1:"SZ.exprsize"},bad("sizeof non-scalar expression"),[("CMPI","vt",0)])
    P("SZ.exprsize").a(("COPYW","td","vt"),("COPYW","tb","vb")).call("ELSZ").a(("COPYW","sz","es")).goto("SZ.exprout")
    P("SZ.exprout").o("  imm r0, ").num("sz").o("\n").a(("LDI","vt",0),("LDI","vb",UNS + 8)).ret()
    g.on("DEAD.szx", range(257), "DEAD", E.rej("not covered: sizeof operand"), "r")
    P("SZ.out").o("  imm r0, ").num("sz").o("\n").a(("LDI", "vt", 0), ("LDI", "vb", UNS + 8)).call("NEXT").ret()
    P("U.fnum").o("  imm r0, ").a(("LDI", "nx", 1)).call("NUMOUT").o("\n").a(("LDI", "vt", 0), ("LDI", "vb", DBL)).branch({1: "U.f32"}, "U.fnext", [("CMPI", "df_mbits", 23)])
    P("U.f32").a(("LDI", "vb", FLT)).goto("U.fnext")
    P("U.fnext").call("NEXT").ret()
    p = P("UNARY")
    p.tok({"sizeof": "U.szof", E.TK_FNUM: "U.fnum", E.TK_STR: "U.str", "~": "U.cpl", "-": "U.neg", "+": "U.pos", "!": "U.not", "(": "U.par", TK_NUM: "U.num", TK_ID: "U.id", "++": "U.pinc", "--": "U.pdec", "&": "U.amp", "*": "U.deref"}, bad("expression"))
    P("U.amp").call("NEXT").tok({TK_ID: "U.amp1"}, bad("address of"))
    q = P("U.amp1")
    q.a(("COPYW", "ips", "ps"), ("COPYW", "ipe", "pe"), ("LDI", "isfn", 0)).call("FNVAL").branch({1:"U.amfn"}, "U.amobj", [("CMPI", "isfn", 1)])
    P("U.amfn").call("NEXT").ret()
    P("U.amobj").call("LOOKUP").call("NEXT").tok({"[": "U.ams", ".": "U.amm", "->": "U.amptr"}, "U.amv")
    q = P("U.amptr")
    addr(q).call("LOADV").goto("U.amm0")
    q = P("U.amm")
    addr(q).goto("U.amm0")
    P("U.amm0").a(("LDI", "amp", 1)).call("MEMB").a(("LDI", "amp", 0)).ret()
    q = P("U.amv")
    addr(q).a(("ALUI", "add", "vt", "vt", 1)).ret()
    q = P("U.ams")       # &a[i]...: the element's address, no load (measured, p53)
    addr(q).call("VLOAD").a(("LDI", "amp", 1)).call("POSTIX").a(("LDI", "amp", 0)).ret()
    q = P("U.deref")     # * operand: its value is the address; one level down, then a load at the new width
    q.call("NEXT").tok({TK_ID: "UD.id"}, "UD.gen")
    P("UD.gen").call("UNARY").goto("UD.dn")
    P("UD.dn").call("DOWN").call("ISFP").branch({1: "RET"}, "LOADV", [])
    P("UD.id").a(("COPYW", "ips", "ps"), ("COPYW", "ipe", "pe")).call("NEXT").tok({"(": "UD.call", "++": "UD.inc", "--": "UD.dec"}, "UD.v")
    P("UD.call").call("U.call").goto("UD.dn")
    for tag, op in (("inc", "+"), ("dec", "-")):
        q = P("ID." + tag).call("LOOKUP").call("NOARR").call("CSTEP")
        addr(q).call("POST." + op).ret()
        P("UD." + tag).call("ID." + tag).goto("UD.dn")
    P("UD.v").a(("INTERN","v","ips","ipe")).branch({1:"UD.func"},"UD.namedvalue",[("CMP","v","funcid")])
    P("UD.func").call("UF").call("POSTIX").goto("UD.dn")
    p = P("UD.namedvalue")     # *f with f a local function pointer: load the callee value
    p.a(("LDI", "isfn", 0)).call("FNVAL").branch({1: "RET"}, "UD.v2", [("CMPI", "isfn", 1)])
    p = P("UD.v2")
    p.call("LOOKUP").call("ISFP").branch({1: "UD.f1"}, "UD.gv", [])
    P("UD.f1").branch({1: "UD.gv"}, "UD.fs", [("CMPI", "s", E.GMARK)])
    P("UD.fs").branch({0: "UD.gv"}, "UD.f2", [("CMPI", "s", 0)])
    P("UD.f2").branch({1: "UD.gv"}, "UD.f3", [("CMPI", "ar", 1)])
    P("UD.f3").o("  load64 r0, [r6-").num("s").o("]\n").ret()
    p = P("UD.gv")
    addr(p)
    p.call("VLOAD").call("POSTIX").goto("UD.dn")
    P("DOWN").call("ISFP").branch({1: "DOWN.fp"}, "DOWN.0", [])
    P("DOWN.fp").ret()
    P("ISFP").branch({1: "RET"}, "ISFP.var", [("CMPI", "vb", FPB)])
    P("ISFP.var").a(("CMPI", "vb", FPV)).ret()
    P("DOWN.0").branch({(1, 2): "DOWN.1"}, bad("dereference of a non-pointer"), [("CMPI", "vt", 1)])
    P("DOWN.1").a(("ALUI", "sub", "vt", "vt", 1)).branch({1: "DOWN.2"}, "RET", [("CMPI", "vt", 0)])
    P("DOWN.2").branch({1: "DEAD.void"}, "RET", [("CMPI", "vb", 0)])
    g.on("DEAD.void", range(257), "DEAD", E.rej("not covered: dereference of void"), "r")
    # Prefix updates share the existing member/subscript address walk. The
    # address is evaluated once; its value kind then selects step/load/store.
    P("PRE.addr").tok({TK_ID: "PRE.id"}, bad("prefix increment operand"))
    q = P("PRE.id")
    q.a(("COPYW", "ips", "ps"), ("COPYW", "ipe", "pe")).call("LOOKUP").call("NEXT")
    addr(q).tok({".": "PRE.member", "->": "PRE.ptr", "[": "PRE.index"}, "PRE.bare")
    P("PRE.bare").call("NOARR").ret()
    P("PRE.ptr").call("LOADV").goto("PRE.member")
    P("PRE.member").vpush("amp").a(("LDI", "amp", 1)).call("MEMB").goto("PRE.done")
    P("PRE.index").call("VLOAD").vpush("amp").a(("LDI", "amp", 1)).call("POSTIX").goto("PRE.done")
    P("PRE.done").branch({1: "PRE.value"}, bad("prefix increment operand"), [("CMPI", "amp", 0)])
    P("PRE.value").vpop("amp").call("DOWN").ret()
    for nm, fix in (("U.pinc", "pre_inc"), ("U.pdec", "pre_dec")):
        q = P(nm)           # addr; push; load; +-step; pop; store
        q.call("NEXT").call("PRE.addr").call("CSTEP")
        emit(q, "push").call("LOADV").branch({1: nm + ".scalar"}, nm + ".integer", [("CMPI", "vt", 0)])
        P(nm + ".scalar").branch({(DBL, FLT): nm + ".float"}, nm + ".integer", [("RLD", "vb")])
        P(nm + ".float").call("FPSTEP." + ("+" if nm == "U.pinc" else "-")).goto(nm + ".store")
        emit(P(nm + ".integer"), fix).call("NARU").goto(nm + ".store")
        emit(P(nm + ".store"), "pop1").call("STOREV").ret()
    P("U.pos").call("NEXT").call("UNARY").call("NODBL0").ret()     # +x: no code (the old E3, p7)
    q = P("U.neg")
    q.call("NEXT").call("UNARY").branch({1: "U.negscalar"}, "U.negint", [("CMPI", "vt", 0)])
    P("U.negscalar").branch({1: "U.negdouble"}, "U.negsingle", [("CMPI", "vb", DBL)])
    P("U.negsingle").branch({1: "U.negfloat"}, "U.negint", [("CMPI", "vb", FLT)])
    P("U.negdouble").o("  imm r1, -9223372036854775808\n  xor64 r0, r0, r1\n").ret()
    P("U.negfloat").o("  imm r1, 2147483648\n  xor64 r0, r0, r1\n").ret()
    P("U.negint").branch({1: "U.ngu32"}, "U.ng1", [("CMPI", "vb", UNS + 4)])
    emit(P("U.ngu32"), "neg").call("NARU").ret()
    q = P("U.ng1")       # -x on any integer but unsigned int: imm r1, 0; sub64 (measured); a narrow operand gives an int
    emit(q, "neg").branch({1: "U.ng4"}, "U.ngwidth", [("CMPI", "vb", BOOL)])
    P("U.ngwidth").branch({(0, 1): "U.ng4"}, "U.ng2", [("CMPI", "vb", 2)])
    P("U.ng2").branch({(1, 2): "U.ng3"}, "RET", [("CMPI", "vb", UNS + 1)])
    P("U.ng3").branch({0: "U.ng4"}, "RET", [("CMPI", "vb", UNS + 3)])
    P("U.ng4").a(("LDI", "vb", 4)).ret()
    q = P("U.not")
    q.call("NEXT").call("UNARY")
    q.call("FNOT").ret()
    P("U.par").call("NEXT").tok({**{w: "U.cast" for w in TWORDS}, TK_ID: "U.pq", "struct": "U.cast", "union": "U.cast"}, "U.pe")
    P("U.pq").call("ISTD").branch({1: "U.cast"}, "U.pe")
    P("U.pe").call("CEXPR").expect(")").call("NEXT").a(("LDI", "rkok", 0)).call("POSTIX").ret()
    q = P("U.cast")      # (T) e: narrowed through the stack to T; long and pointers: no code (measured)
    q.call("TSPEC").tok({"(": "UC.fp"}, "UC.type")
    P("UC.fp").branch({1: "UC.fp0"}, bad("function pointer cast result type"), [("CMPI", "td", 0)])
    P("UC.fp0").branch({1: "UC.fp1"}, "UC.fp8", [("CMPI", "tb", 4)])
    P("UC.fp8").branch({1: "UC.fp1"}, "UC.fpvoid", [("CMPI", "tb", 8)])
    P("UC.fpvoid").branch({1: "UC.fp1"}, bad("function pointer cast result type"), [("CMPI", "tb", 0)])
    P("UC.fp1").call("NEXT").expect("*").call("NEXT").expect(")").call("FPD.c").goto("UC.type")
    P("UC.type").a(("LDI","cp_arr",0),("LDI","cp_n",1)).tok({"[":"CP.array"},"CP.close")
    P("CP.array").a(("LDI","cp_arr",1),("LDI","cp_n",0)).call("NEXT").tok({"]":"CP.boundend"},"CP.bound")
    P("CP.bound").vpush("td","tb").call("CE").vpop("td","tb").a(("COPYW","cp_n","cv")).goto("CP.boundend")
    P("CP.boundend").expect("]").call("NEXT").goto("CP.close")
    P("CP.close").expect(")").vpush("td","tb","cp_arr","cp_n").call("NEXT").vpop("td","tb","cp_arr","cp_n").tok({"{":"CP.literal"},"CP.cast")
    P("CP.cast").branch({1:"CP.cast1"},bad("array cast without literal"),[("CMPI","cp_arr",0)])
    q = P("CP.cast1")
    q.vpush("td", "tb").call("UNARY").vpop("cast_td", "cast_tb")
    q.branch({1: "UC.scalar"}, "UC.to_u", [("CMPI", "cast_td", 0)])
    # Anonymous local objects use the same aggregate walker as declarations.
    P("CP.literal").branch({1:"CP.local"},bad("file-scope compound literal"),[("CMPI","infunc",1)])
    P("CP.local").branch({1:"CP.infer"},"CP.size",[("CMPI","cp_n",0)])
    P("CP.infer").call("TYPECOUNT").a(("COPYW","dm_per","flat"),("COPYW","cp_back","tpos")).vpush("td","tb","cp_arr","cp_back").call("INITCOUNT").vpop("td","tb","cp_arr","cp_back").a(("COPYW","cp_n","dm_n"),("JUMP","cp_back")).call("NEXT").goto("CP.size")
    P("CP.size").branch({2:"CP.alloc"},bad("compound literal extent"),[("CMPI","cp_n",0)])
    P("CP.alloc").call("ELSZ").a(("ALU","mul","ibytes","es","cp_n"),("ALU","add","cur","cur","ibytes")).call("MAXF").a(
        ("COPYW","cp_off","cur"),("COPYW","ivt","td"),("ALU","add","ivt","ivt","cp_arr"),("COPYW","ivb","tb"),("COPYW","iar","cp_arr"),
        ("LDI","ivv",0),("COPYW","isl","cur"),("LDI","imode",0)).vpush("td","tb","cp_arr","cp_off").call("INITVALUE").vpop("td","tb","cp_arr","cp_off").o("  imm r0, ").num("cp_off").o("\n  sub64 r0, r6, r0\n").a(
        ("COPYW","vt","td"),("COPYW","vb","tb"),("LDI","rkok",0)).branch({1:"CP.scalar"},"CP.decay",[("CMPI","cp_arr",0)])
    P("CP.decay").a(("ALUI","add","vt","vt",1)).goto("POSTIX")
    P("CP.scalar").call("LOADV").goto("POSTIX")
    P("UC.scalar").branch({1: "UC.to_d"}, "UC.float", [("CMPI", "cast_tb", DBL)])
    P("UC.float").branch({1: "UC.to_s"}, "UC.integer", [("CMPI", "cast_tb", FLT)])
    P("UC.integer").branch({1: "UC.bool"}, "UC.integer0", [("CMPI", "cast_tb", BOOL)])
    P("UC.bool").call("TO.b").a(("LDI", "vt", 0), ("LDI", "vb", BOOL)).ret()
    P("UC.integer0").branch({1: "UC.to_u"}, "UC.to_i", [("CMPI", "cast_tb", UNS + 8)])
    for kind in ("d", "s", "i", "u"):
        q = P("UC.to_" + kind).call("TO." + kind).a(("COPYW", "vt", "cast_td"), ("COPYW", "vb", "cast_tb"))
        if kind in ("i", "u"):
            q.call("NARROW")
        q.ret()
    # the suffix (the dump's text from pe to the line end): u -> unsigned, l/ll -> long (measured)
    q = P("U.num")
    q.a(("LDI", "nu", 0), ("LDI", "nl", 0), ("INPUSHX", "pe")).goto("SFX.w")
    g.on("SFX.w", [ord("u"), ord("U")], "SFX.w", [("ADV",), ("LDI", "nu", 1)])
    g.on("SFX.w", [ord("l"), ord("L")], "SFX.w", [("ADV",), ("LDI", "nl", 1)])
    g.els("SFX.w", "SFX.x", [("INPOP",)])
    P("SFX.x").branch({1: "SFX.u"}, "SFX.l", [("CMPI", "nu", 1)])
    P("SFX.l").branch({1: "SFX.lo"}, "U.num0", [("CMPI", "nl", 1)])
    P("SFX.lo").a(("LDI", "t", 0), ("C64", "nv", "t")).branch({0: "DEAD.big"}, "U.long")
    P("SFX.u").branch({1: "U.ulong"}, "SFX.u4", [("CMPI", "nl", 1)])
    P("SFX.u4").a(("LDI", "t", U32M), ("C64U", "nv", "t")).branch({2: "U.ulong"}, "U.ui")
    q = P("U.num0")      # an int constant; beyond int: long when it fits (printed as NUMOUT does)
    q.a(("LDI", "t", 2147483647), ("C64U", "nv", "t")).branch({2: "U.big"}, "U.num1")
    q = P("U.big")       # decimal beyond int: long; hex/octal: unsigned int up to 0xffffffff (not covered), then long up to LONG_MAX
    q.branch({1: "U.bh"}, "U.bl", [("CMPI", "nx", 1)])
    P("U.bh").a(("LDI", "t", U32M), ("C64U", "nv", "t")).branch({2: "U.bl"}, "U.ui")
    P("U.ui").o("  imm r0, ").call("NUMOUT").o("\n").a(("LDI", "vt", 0), ("LDI", "vb", UNS + 4)).call("NEXT").ret()
    P("U.bl").a(("LDI", "t", 0)).a(("C64", "nv", "t")).branch({0: "U.bu"}, "U.long")
    P("U.bu").branch({1: "U.ulong"}, "DEAD.big", [("CMPI", "nx", 1)])     # hex beyond LONG_MAX: unsigned long, printed signed (p43)
    P("U.ulong").o("  imm r0, ").a(("LDI", "nx", 1)).call("NUMOUT").o("\n").a(("LDI", "vt", 0), ("LDI", "vb", UNS + 8)).call("NEXT").ret()
    P("U.long").o("  imm r0, ").call("NUMOUT").o("\n").a(("LDI", "vt", 0), ("LDI", "vb", 8)).call("NEXT").ret()
    g.on("DEAD.big", range(257), "DEAD", E.rej("not covered: constant beyond int"), "r")
    q = P("U.num1")
    emit(q, "imm").a(("LDI", "vt", 0), ("LDI", "vb", 4)).call("NEXT").ret()
    P("U.id").a(("COPYW", "ips", "ps"), ("COPYW", "ipe", "pe")).call("NEXT").tok({"(": "U.call", "++": "ID.inc", "--": "ID.dec"}, "U.var0")
    P("U.call").call("CALL").a(("LDI", "rkok", 0)).call("POSTIX").ret()
    printf(warnings)
    q = P("U.var")
    q.call("LOOKUP")
    addr(q)
    q.tok({".": "MEMB"}, "U.vl")
    q = P("U.var0")
    q.a(("INTERN", "v", "ips", "ipe")).branch({1:"U.func"},"U.enumcheck",[("CMP","v","funcid")])
    P("U.enumcheck").a(("LDX","t","v",END_)).branch({1:"U.enum"},"U.var0b",[("CMPI","t",1)])
    P("U.func").call("UF").call("POSTIX").ret()
    P("UF").branch({1:"UF.name"},bad("__func__ outside a function"),[("CMPI","infunc",1)])
    P("UF.name").a(("ALUI","add","ufend","fns",120)).branch({2:"UF.pool"},"UF.short",[("CMP","fne","ufend")])
    P("UF.short").a(("COPYW","ufend","fne")).goto("UF.pool")
    P("UF.pool").a(("BLOBSAVE","ufblob","fns","ufend"),("STX","ips",FNSTR,"ufblob")).o("  .lea r0, S").num("sk").o("\n").a(("ALUI","add","sk","sk",1),("LDI","vt",1),("LDI","vb",1),("LDI","rkok",0)).ret()
    P("U.enum").a(("LDX", "nv", "v", ENV), ("LDI", "nx", 1)).o("  imm r0, ").call("NUMOUT").o("\n").a(("LDI", "vt", 0), ("LDI", "vb", 4)).call("POSTIX").ret()
    q = P("U.var0b")
    q.a(("LDI", "isfn", 0)).call("FNVAL").branch({1: "U.fnp"}, "U.var", [("CMPI", "isfn", 1)])
    P("U.fnp").a(("LDI", "rkok", 0)).goto("POSTIX")
    P("U.vl").call("VLOAD").call("POSTIX").ret()
    P("VLOAD").a(("LDI", "rkok", 0)).branch({0: "LOADV"}, "VL.a", [("CMPI", "ar", 1)])
    P("VL.a").a(("LDI", "rkok", 1), ("COPYW", "rk", "ar")).ret()
    P("NOARR").branch({0: "RET"}, "DEAD.arr", [("CMPI", "ar", 1)])
    g.on("DEAD.arr", range(257), "DEAD", E.rej("not covered: assignment to an array"), "r")
    p = P("MEMB")         # current '.' (r0 = a struct's address) or '->' (r0 = a pointer to one)
    p.tok({".": "MB.dot", "->": "MB.arw"}, "RET")
    P("MB.dot").branch({1: "MB.d1"}, "DEAD.mb", [("CMPI", "vt", 0)])
    P("MB.arw").branch({1: "MB.d1"}, "DEAD.mb", [("CMPI", "vt", 1)])
    P("MB.d1").branch({(1, 2): "MB.ok"}, "DEAD.mb", [("CMPI", "vb", SBB)])
    g.on("DEAD.mb", range(257), "DEAD", E.rej("not covered: member access"), "r")
    p = P("MB.ok")
    p.a(("ALUI", "sub", "sid", "vb", SBB)).call("NEXT").tok({TK_ID: "MB.nm"}, bad("member access"))
    P("MB.nm").call("MB.INFO").branch({1: "MB.zero"}, "MB.has", [("CMPI", "ms", 0)])
    P("MB.INFO").a(("INTERN", "v", "ps", "pe"), ("A64I", "mul", "k", "v", MEMBER_STRIDE), ("A64", "add", "k", "k", "sid"),
        ("LDX", "mo", "k", MOF), ("LDX", "ms", "k", MSZ), ("LDX", "vt", "k", MPT), ("LDX", "vb", "k", MBS), ("LDX", "marr", "k", MAR)).ret()
    P("MB.zero").branch({0: "MB.has"}, "DEAD.mb", [("CMPI", "marr", 0)])
    P("MB.has").branch({1: "MB.z"}, "MB.off", [("CMPI", "mo", 0)])
    P("MB.off").o("  imm r2, ").num("mo").o("\n  add64 r0, r0, r2\n").goto("MB.z")
    P("MB.z").call("NEXT").tok({"=": "PX.as", "++": "MB.inc", "--": "MB.dec", **{o+"=": "LV.c"+o for o in E.CASOPS}, "->": "MB.ptr", ".": "MB.dot2"}, "MB.ld")
    P("MB.dot2").goto("MEMB")          # s.inner.m: the inner struct's address, then its member
    P("MB.ptr").call("LOADV").goto("MEMB")
    P("MB.ld").branch({1: "MB.ld1"}, "MB.arr", [("CMPI", "marr", 0)])
    # &s.ptr[i] needs the pointer value before taking the final element address.
    P("MB.ld1").tok({"[": "MB.value"}, "MB.addrtest")
    P("MB.addrtest").branch({1: "MB.address"}, "MB.value", [("CMPI", "amp", 1)])
    P("MB.address").a(("ALUI", "add", "vt", "vt", 1), ("LDI", "amp", 0)).ret()
    P("MB.value").call("LOADV").a(("LDI", "rkok", 0)).goto("POSTIX")
    P("MB.arr").a(("ALUI", "add", "vt", "vt", 1), ("LDI", "rkok", 0)).branch({1: "MB.arrayaddr"}, "POSTIX", [("CMPI", "amp", 1)])   # s.arr: the address, decayed
    P("MB.arrayaddr").tok({"[": "POSTIX"}, bad("address of array member"))
    p = P("POSTIX")
    p.tok({"[": "PX.i", ".": "MEMB", "->": "MEMB", "(": "PX.fc"}, "RET")
    P("PX.fc").call("ISFP").branch({1: "PX.fc1"}, "RET", [])
    P("PX.fc1").call("FPCALL").a(("LDI", "rkok", 0)).goto("POSTIX")
    q = P("PX.i")
    q.branch({(1, 2): "PX.ok"}, bad("subscript of a non-pointer"), [("CMPI", "vt", 1)])
    q = P("PX.ok")
    q.branch({1: "PX.r0"}, "PX.one", [("CMPI", "rkok", 1)])
    P("PX.r0").branch({2: "PX.md"}, "PX.one", [("CMPI", "rk", 1)])
    q = P("PX.md")       # stride = element size * DIM[rank-rk+1 .. rank-1]
    q.a(("COPYW", "td", "vt"), ("ALUI", "sub", "td", "td", 1), ("COPYW", "tb", "vb")).call("ELSZ").a(("COPYW", "str", "es"),
        ("LDX", "t", "vid", E.ARR), ("ALU", "sub", "k2", "t", "rk"), ("ALUI", "add", "k2", "k2", 1)).label("PX.sl")
    q.branch({0: "PX.s1"}, "PX.sd", [("CMP", "k2", "t")])
    P("PX.s1").a(("ALUI", "mul", "u", "vid", 8), ("ALU", "add", "u", "u", "k2"), ("LDX", "u", "u", DIM), ("ALU", "mul", "str", "str", "u"), ("ALUI", "add", "k2", "k2", 1)).goto("PX.sl")
    q = P("PX.sd")
    emit(q, "push").vpush("vt", "vb", "st1", "rk", "vid", "str", "amp").a(("LDI", "amp", 0)).call("NEXT").call("EXPR").expect("]").vpop("vt", "vb", "st1", "rk", "vid", "str", "amp")
    q.branch({1: "PX.m1"}, "PX.mm", [("CMPI", "str", 1)])
    P("PX.mm").o("  imm r2, ").num("str").o("\n  mul64 r0, r0, r2\n").goto("PX.m1")
    q = P("PX.m1")
    emit(q, "pop1").o("  add64 r0, r1, r0\n").a(("ALUI", "sub", "rk", "rk", 1), ("LDI", "rkok", 1)).call("NEXT").goto("POSTIX")
    q = P("PX.one")
    emit(q, "push").vpush("vt", "vb", "st1", "amp").a(("LDI", "amp", 0)).call("NEXT").call("EXPR").expect("]").vpop("lt", "lb", "st1", "amp").call("SCALE")
    emit(q, "pop1").o("  add64 r0, r1, r0\n").a(("COPYW", "vt", "lt"), ("COPYW", "vb", "lb")).call("DOWN").call("NEXT").tok({"=": "PX.as", "++": "PX.inc", "--": "PX.dec", **{o+"=": "LV.c"+o for o in E.CASOPS}, ".": "MEMB"}, "PX.ld")   # a[i].m: the element's address, then the member
    # a statement that is only `p[i];` computes the address and stops (measured, probe p39)
    P("PX.ld").branch({1: "PX.am"}, "PX.ld1", [("CMPI", "amp", 1)])
    P("PX.am").a(("ALUI", "add", "vt", "vt", 1), ("LDI", "amp", 0)).ret()
    P("PX.ld1").branch({1: "PX.st"}, "PX.l2", [("CMPI", "st1", 1)])
    P("PX.st").tok({";": "RET"}, "PX.l2")
    P("PX.l2").call("LOADV").a(("LDI", "rkok", 0)).goto("POSTIX")
    q = P("PX.as")
    emit(q, "push").vpush("vt", "vb").call("NEXT")
    if warnings: q.a(("COPYW", "wi_target", "vt")).call("WI.expr")
    else: q.call("EXPR")
    q.vpop("lt", "lb").branch({1: "AS.kind"}, "AS.scalar", [("CMPI", "lt", 0)])
    P("AS.kind").branch({(1, 2): "AS.struct"}, "AS.scalar", [("CMPI", "lb", SBB)])
    q = P("AS.scalar")
    q.a(("COPYW", "rvb", "vb"), ("COPYW", "rvt", "vt"), ("COPYW", "vt", "lt"), ("COPYW", "vb", "lb")).call("ASSIGNCV")
    emit(q, "pop1").call("STOREV").call("AS.result").ret()
    # Reference assignment keeps the RHS facts except a floating target's
    # explicit setkind. Store width still comes from the target above.
    P("AS.result").branch({1: "AS.result0"}, "AS.rhs", [("CMPI", "vt", 0)])
    P("AS.result0").branch({1: "RET"}, "AS.result1", [("CMPI", "vb", DBL)])
    P("AS.result1").branch({1: "RET"}, "AS.resultbool", [("CMPI", "vb", FLT)])
    P("AS.resultbool").branch({1: "RET"}, "AS.rhs", [("CMPI", "vb", BOOL)])
    P("AS.rhs").a(("COPYW", "vt", "rvt"), ("COPYW", "vb", "rvb")).ret()
    P("AS.struct").branch({1: "AS.same"}, bad("struct assignment"), [("CMPI", "vt", 0)])
    P("AS.same").branch({1: "AS.copy"}, bad("struct assignment"), [("CMP", "vb", "lb")])
    q = P("AS.copy")
    q.a(("ALUI", "sub", "t", "vb", SBB), ("LDX", "sz", "t", SSZ)).o("  mov r1, r0\n  load64 r0, [r7+0]\n  .frame -8\n").goto("COPYSTRUCT")
    q = P("COPYSTRUCT")
    q.a(("LDI", "k2", 0)).label("AS.loop")
    q.branch({0: "AS.width"}, "RET", [("CMP", "k2", "sz")])
    P("AS.width").a(("LDI", "copyw", 8)).label("AS.fit").branch({2: "AS.half"}, "AS.emit", [("ALU", "add", "t", "k2", "copyw"), ("CMP", "t", "sz")])
    P("AS.half").a(("ALUI", "div", "copyw", "copyw", 2)).goto("AS.fit")
    P("AS.emit").branch({1: "AS.word"}, "AS.part", [("CMPI", "copyw", 8)])
    P("AS.word").o("  load64 r2, [r1+").num("k2").o("]\n  store64 [r0+").num("k2").o("], r2\n").goto("AS.next")
    P("AS.part").o("  .ld r2, [r1+").num("k2").o("], ").num("copyw").o("\n  .st [r0+").num("k2").o("], r2, ").num("copyw").o("\n").goto("AS.next")
    P("AS.next").a(("ALU", "add", "k2", "k2", "copyw")).goto("AS.loop")
    # statement `*E = e` / `*E ...;`: E's value is the address
    q = P("S.star")
    q.call("NEXT").call("UNARY").call("DOWN").tok({"=": "SS.as"}, "SS.rv")
    q = P("SS.as")
    emit(q, "push").vpush("vt", "vb").call("NEXT")
    if warnings: q.a(("COPYW", "wi_target", "vt")).call("WI.expr")
    else: q.call("EXPR")
    q.a(("COPYW", "rvt", "vt"), ("COPYW", "rvb", "vb")).vpop("vt", "vb").call("BOOLTARGET")
    emit(q, "pop1").call("STOREV").expect(";").call("NEXT").ret()
    P("SS.rv").tok({";": "SS.x"}, "SS.rv1")     # `*p;` alone: the address only (measured, p20)
    P("SS.x").call("NEXT").ret()
    P("SS.rv1").call("LOADV").call("C%d" % LEVELS[0]).expect(";").call("NEXT").ret()
    p = P("FNVAL")
    p.a(("INTERN", "v", "ips", "ipe"), ("LDX", "t", "v", LOC)).branch({1: "FNV.f"}, "RET", [("CMPI", "t", 0)])
    P("FNV.f").a(("LDX", "t", "v", E.FND)).branch({1: "FNV.y"}, "RET", [("CMPI", "t", 1)])
    P("FNV.y").o("  .lea r0, ").a(("SPAN2", "ips", "ipe")).o("\n").a(("LDI", "vt", 1), ("LDI", "vb", FPB), ("LDI", "isfn", 1), ("LDX", "t", "v", E.VAR)).branch({1: "FNV.stacked"}, "RET", [("CMPI", "t", 1)])
    P("FNV.stacked").a(("LDI", "vb", FPV)).ret()
    p = P("LOOKUP")     # s := the slot of ips..ipe (0: not a local of this slice)
    if warnings: p.call("WU.use")
    p.a(("INTERN", "v", "ips", "ipe"), ("LDX", "s", "v", LOC), ("LDX", "vt", "v", E.PTR), ("LDX", "vb", "v", E.BASE), ("LDX", "ar", "v", E.ARR), ("COPYW", "vid", "v")).branch({1: "DEAD.nl"}, "RET", [("CMPI", "s", 0)])
    g.on("DEAD.nl", range(257), "DEAD", E.rej("not covered: identifier is not a local"), "r")
    # CALL: at '(' after ips..ipe: arguments pushed left to right, popped into r(n-1)..r0, call
    p = P("CALL")
    # Ordinary callees are resolved after all function bodies have been read.
    # syscall builtins (the old E3's declared table SYSCALLS), __argc(), __argv(k) -- measured there
    p.a(("INTERN", "v", "ips", "ipe"), ("LDI", "sys", 0), ("LDX", "t", "v", LOC)).branch({1: "CL.va0"}, "CL.fpv", [("CMPI", "t", 0)])
    p = P("CL.fpv")      # the callee's value: a local's `load64 r0, [r6-N]`, a global's .lea + load64 (measured)
    p.call("LOOKUP").call("ISFP").branch({1: "CL.fpv1"}, bad("call through a non-function"), [])
    q = P("CL.fpv1")
    q.branch({1: "CL.fpg"}, "CL.fps", [("CMPI", "s", E.GMARK)])
    P("CL.fps").branch({0: "CL.fpg"}, "CL.fpl", [("CMPI", "s", 0)])
    P("CL.fpl").o("  load64 r0, [r6-").num("s").o("]\n").goto("FPCALL")
    q = P("CL.fpg")
    addr(q).o("  load64 r0, [r0+0]\n").goto("FPCALL")
    p = P("FPCALL")      # r0 = the callee; current '(' -- pushed first, then the arguments; callr r5 (measured)
    if warnings: p.a(("LDI","wf_format",0))
    emit(p, "push").vpush("cls", "cle").a(("LDI", "sys", 100), ("LDI", "fid", 0)).branch({1: "FC.stacked"}, "FC.args", [("CMPI", "vb", FPV)])
    P("FC.stacked").a(("LDI", "sys", 101)).goto("FC.args")
    P("FC.args").vpush("sys").a(("LDI", "na", 0)).call("NEXT").tok({")": "CL.done"}, "CL.arg")
    for k, nx in ((0, "CL.va1"), (1, "CL.va2"), (2, "CL.b1")):
        q = P("CL.va%d" % k)
        if warnings and k == 0: q.a(("LDI", "wi_called", 1)).call("WF.entry")
        q.branch({1: "VA%d" % k}, nx, [("CMP", "v", "va%d" % k)])
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
    for k in range(1, len(SYSCALLS) + 1):
        P("CL.b%d" % k).branch({1: "CL.s%d" % k}, "CL.b%d" % (k + 1), [("CMP", "v", "sy%d" % k)])
        P("CL.s%d" % k).a(("LDI", "sys", k)).goto("CL.ok")
    P("CL.b%d" % (len(SYSCALLS) + 1)).branch({1: "CL.ac"}, "CL.av0", [("CMP", "v", "acid")])
    P("CL.ac").call("NEXT").expect(")").o("  .argc r0\n").a(("LDI", "vt", 0), ("LDI", "vb", 4)).call("NEXT").ret()
    P("CL.av0").branch({1: "CL.av"}, "CL.sqrt0", [("CMP", "v", "avid")])
    P("CL.av").call("NEXT").call("EXPR").expect(")").o("  .argv r0, r0\n").a(("LDI", "vt", 1), ("LDI", "vb", 1)).call("NEXT").ret()
    # Hardware sqrt builtins, as in product fkind/fconv. Conversion selection
    # belongs to the parser delta; opcode spellings come from irsel.
    fpu = {row[1]: row[2] for row in E.gold("irsel") if row[0] == "fpu"}
    for k, (base, suffix) in enumerate(((DBL, "d"), (FLT, "s"))):
        tag = "SQ%d" % k
        P("CL.sqrt%d" % k).branch({1: tag}, "CL.sqrt1" if k == 0 else "CL.def", [("CMP", "v", "sqrt%d" % k)])
        P(tag).call("NEXT").call("EXPR").expect(")").call("TO." + suffix).o("  %s r0, r0\n" % fpu[suffix + "sqrt"]).a(("LDI", "vt", 0), ("LDI", "vb", base)).call("NEXT").ret()
    for suffix in ("d", "s", "i", "u"):
        cv = "TO." + suffix  # shared fkind/fconv for casts, sqrt and typed arguments
        P(cv).branch({1: cv + ".base"}, cv + ".u", [("CMPI", "vt", 0)])
        P(cv + ".base").branch({1: cv + ".d"}, cv + ".float", [("CMPI", "vb", DBL)])
        P(cv + ".float").branch({1: cv + ".s"}, cv + ".int", [("CMPI", "vb", FLT)])
        P(cv + ".int").branch({1: cv + ".u"}, cv + ".i", [("CMPI", "vb", UNS + 8)])
        for source in ("d", "s", "i", "u"):
            q = P(cv + "." + source)
            if source != suffix and not (source in ("i", "u") and suffix in ("i", "u")):
                if source == "s" and suffix in ("i", "u"):
                    q.o("  %s r0, r0\n" % fpu["s2d"])
                    source = "d"
                q.o("  %s r0, r0\n" % fpu[source + "2" + suffix])
            q.ret()
    P("CL.def").a(("LDX", "t", "v", E.FND)).branch({1: "CL.def1"}, "CL.implicit", [("CMPI", "t", 1)])
    # Match the reference's unknown-call scalar result; final definition
    # resolution happens after all units, not while reading this call.
    P("CL.implicit").a(("LDI","t",8),("STX","v",E.FRB,"t")).goto("CL.def1")
    P("CL.def1").a(("LDX", "t", "v", E.VAR)).branch({1: "CL.vok"}, "CL.ok", [("CMPI", "t", 1)])
    P("CL.vok").a(("LDI", "sys", 200)).goto("CL.ok")
    g.on("DEAD.vc", range(257), "DEAD", E.rej("not covered: call to a variadic function"), "r")
    p = P("CL.ok")
    if warnings: p.a(("LDX","wf_format","tpos",51<<40))
    p.a(("COPYW", "cls", "ips"), ("COPYW", "cle", "ipe"), ("INTERN", "fid", "ips", "ipe")).vpush("cls", "cle", "sys").a(("LDI", "na", 0)).call("NEXT").tok({")": "CL.done"}, "CL.arg")
    P("ARGCOPY").branch({1: "AC.base"}, "RET", [("CMPI", "vt", 0)])
    P("AC.base").branch({(1, 2): "AC.copy"}, "RET", [("CMPI", "vb", SBB)])
    q = P("AC.copy")
    q.a(("ALUI", "sub", "t", "vb", SBB), ("LDX", "sz", "t", SSZ), ("ALU", "add", "cur", "cur", "sz")).call("MAXF")
    q.o("  mov r1, r0\n  imm r0, ").num("cur").o("\n  sub64 r0, r6, r0\n").goto("COPYSTRUCT")
    p = P("CL.arg")
    p.vpush("na", "fid")
    if warnings: p.a(("COPYW","wf_at","tpos"),("COPYW","wf_index","na")).call("WF.reset").vpush("wf_format","wf_at")
    p.call("EXPR")
    if warnings: p.vpop("wf_format","wf_at")
    p.vpop("na", "fid")
    if warnings: p.a(("COPYW","wf_index","na")).call("WF.argcheck")
    p.call("ARGCOPY")
    p.a(("ALUI", "mul", "t", "fid", 16), ("ALU", "add", "t", "t", "na"), ("LDX", "t", "t", PDB)).branch({1: "CL.ad"}, "CL.af", [("CMPI", "t", DBL)])
    P("CL.ad").call("TO.d").goto("CL.a2")
    P("CL.af").branch({1: "CL.af1"}, "CL.ordinary", [("CMPI", "t", FLT)])
    # No declared parameter kind: the default promotion of scalar float.
    P("CL.ordinary").branch({1: "CL.ab"}, "CL.ordinary0", [("CMPI", "t", BOOL)])
    P("CL.ab").call("TO.b").goto("CL.a2")
    P("CL.ordinary0").branch({1: "CL.default"}, "CL.a2", [("CMPI", "t", 0)])
    P("CL.default").branch({1: "CL.defaultbase"}, "CL.a2", [("CMPI", "vt", 0)])
    P("CL.defaultbase").branch({1: "CL.ad"}, "CL.a2", [("CMPI", "vb", FLT)])
    P("CL.af1").call("TO.s").goto("CL.a2")
    p = P("CL.a2")
    emit(p, "push").a(("ALUI", "add", "na", "na", 1)).tok({",": "CL.more", ")": "CL.done"}, bad("argument list"))
    P("CL.more").call("NEXT").goto("CL.arg")
    p = P("CL.done")
    p.a(("LDX", "t", "vsp", E.VS - 1)).branch({1: "CL.vdone"}, "CL.indirect", [("CMPI", "t", 200)])
    P("CL.indirect").branch({1: "CL.vdone"}, "CL.indirect0", [("CMPI", "t", 101)])
    P("CL.indirect0").branch({1: "CL.indirectn"}, "CL.done1", [("CMPI", "t", 100)])
    P("CL.indirectn").branch({2: "CL.vdone", 1: "DEAD.indirect6"}, "CL.done1", [("CMPI", "na", 6)])
    g.on("DEAD.indirect6", range(257), "DEAD", E.rej("not covered: six indirect register arguments leave no callee register"), "r")
    p = P("CL.vdone")    # the pushed block reversed in place: swap [r7+8i] and [r7+8j]
    p.a(("LDI", "vi", 0), ("ALUI", "sub", "vj", "na", 1)).label("CL.vsw")
    p.branch({0: "CL.vs1"}, "CL.vend", [("CMP", "vi", "vj")])
    q = P("CL.vs1")
    q.a(("ALUI", "mul", "oi", "vi", 8), ("ALUI", "mul", "oj", "vj", 8))
    q.o("  load64 r2, [r7+").num("oi").o("]\n  load64 r1, [r7+").num("oj").o("]\n  store64 [r7+").num("oi").o("], r1\n  store64 [r7+").num("oj").o("], r2\n")
    q.a(("ALUI", "add", "vi", "vi", 1), ("ALUI", "sub", "vj", "vj", 1)).goto("CL.vsw")
    q = P("CL.vend")
    q.vpop("cls", "cle", "sys").a(("ALUI", "mul", "t", "na", 8)).branch({1: "CL.vdirect"}, "CL.vindirect", [("CMPI", "sys", 200)])
    P("CL.vindirect").o("  load64 r5, [r7+").num("t").o("]\n  callr r5\n  .frame -").a(("ALUI", "add", "t", "na", 1), ("ALUI", "mul", "t", "t", 8)).num("t").o("\n").a(("LDI", "vt", 0), ("LDI", "vb", 8)).call("NEXT").ret()
    q = P("CL.vdirect")
    q.o("  call ").a(("SPAN2", "cls", "cle")).o("\n  .frame -").num("t").o("\n")
    if warnings: q.a(("LDI", "wi_called", 1))
    q.a(("INTERN", "v", "cls", "cle"), ("LDX", "vt", "v", E.FRD), ("LDX", "vb", "v", E.FRB)).call("NEXT").ret()
    p = P("CL.done1")
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
    P("CL.callr").o("  load64 r5, [r7+0]\n  .frame -8\n  callr r5\n").a(("LDI", "vt", 0), ("LDI", "vb", 8)).call("NEXT").ret()
    # a syscall: r(n)..r(w-1) zeroed, then `.sys NAME, r0, r1, r2` (w 3) or `.sys6 NAME, r0..r5`
    for k, (_, sc, w) in enumerate(SYSCALLS, 1):
        nx = "CL.w%d" % (k + 1) if k < len(SYSCALLS) else "DEAD"
        P("CL.sysz" if k == 1 else "CL.w%d" % k).branch({1: "CL.y%d" % k}, nx, [("CMPI", "sys", k)])
        q = P("CL.y%d" % k)
        q.a(("LDI", "w", w)).label("CL.z%d" % k)
        q.branch({0: "CL.zz%d" % k}, "CL.x%d" % k, [("CMP", "nar", "w")])
        P("CL.zz%d" % k).o("  imm r").num("nar").o(", 0\n").a(("ALUI", "add", "nar", "nar", 1)).goto("CL.z%d" % k)
        regs = ", ".join("r%d" % i for i in range(w))
        P("CL.x%d" % k).o("  .sys%s %s, %s\n" % ("6" if w == 6 else "", sc, regs)).a(("LDI", "vt", 0), ("LDI", "vb", 8)).call("NEXT").ret()   # a call's value is an i64 (pf_call)
    p = P("CL.call")
    if warnings: p.a(("LDI", "wi_called", 1))
    emit(p, "call").a(("INTERN", "v", "cls", "cle"), ("LDX", "vt", "v", E.FRD), ("LDX", "vb", "v", E.FRB)).call("NEXT").ret()
    ud_install(P)
    start = "START"
    if locations:
        from tokenlocations import install as location_install
        start = location_install(E, P, TIX, "ER.token" if errors else "WU.token" if warnings else None)
        from diagnostics import install as diagnostic_install
        diagnostic_install(E, P)
    if warnings:
        assert locations
        from returnwarnings import install as return_warning_install
        return_warning_install(E, P, SBB)
        from intwarnings import install as int_warning_install
        int_warning_install(E, P, DBL, FLT, FPB, SBB)
        from unusedwarnings import install as unused_warning_install
        unused_warning_install(E, P, TIX, UNDO_SIZE)
        from formatwarnings import install as format_warning_install
        format_warning_install(E, P, DBL, FLT, FPB, SBB)
    if errors:
        from errors import install as error_install
        error_install(E, P, warnings)
    g.finish()
    states = {n: [m, {str(k): v for k, v in row.items()}] for n, (m, row) in g.st.items()}
    return {"start": start, "states": states, "seqs": [list(map(list, s)) for s in g.seqs]}


if __name__ == "__main__":
    warnings = "--warnings" in sys.argv
    if warnings:
        sys.argv.remove("--warnings")
    errors = "--errors" in sys.argv
    if errors: sys.argv.remove("--errors")
    locations = "--locations" in sys.argv or warnings or errors
    if "--locations" in sys.argv:
        sys.argv.remove("--locations")
    d = build(locations=locations, warnings=warnings, errors=errors)
    twice = sorted(k for k, n in DEFS.items() if n > 1)
    assert not twice, "defined twice: %r" % twice
    s = json.dumps(d, separators=(",", ":"))
    open(sys.argv[1], "w").write(s)
    st, ent, live, ns, na = E.sizes(d)
    sys.stderr.write("states %d  entries %d (not 'unreachable' %d)  action seqs %d (%d actions)  json %d B\n"
                     % (st, ent, live, ns, na, len(s)))
