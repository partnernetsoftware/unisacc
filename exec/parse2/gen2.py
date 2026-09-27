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
UNDO_SIZE = 41
FPS_FIRST = 128  # disjoint primitive/signature/structure base-code ranges
FPS_RD, FPS_RB, FPS_VAR, FPS_PARAM, FPS_RSH, FPS_FN, FPS_COUNT, FPS_PSH = (i << 40 for i in range(55, 63))
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
    """Bind variable address selection to shared templates and fresh continuations."""
    bindings = dict(zip(('global','local','done','static','frame','auto'),
                        (p.fresh(k) for k in ('ga','la','ad','sa','fa','auto'))))
    bindings.update(entry=p.cur, entry_test=p.fresh('b'), GMARK=E.GMARK)
    global_out = emit(P(bindings['global']), 'gaddr')
    bindings['local_test'] = P(bindings['local']).fresh('b')
    bindings['static_return'] = P(bindings['static']).fresh('r')
    bindings['frame_test'] = P(bindings['frame']).fresh('b')
    auto_out = emit(P(bindings['auto']), 'addr')
    bindings.update(global_end=global_out.cur, auto_end=auto_out.cur)
    install_rules(g, os.path.dirname(__file__), 'helpers', bindings=bindings,
                  sequences=dict(pending=p.acts, global_tail=global_out.acts, auto_tail=auto_out.acts,
                                 static_prefix=O('  .lea r0, ls'), newline=O('\n')), section='address')
    p.cur, p.acts = bindings['done'], []
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
    """Instantiate the shared precedence ladder from current operator facts."""
    modes = dict(tape_rows("ladder-modes.tsv"))
    for i, lv in enumerate(LEVELS):
        nm = "%s%d" % (prefix, lv)
        up = "%s%d" % (prefix, LEVELS[i + 1]) if i + 1 < len(LEVELS) else bottom
        bindings = dict(ladder_owner=nm, ladder_loop=nm + ".l", ladder_up=up,
                        ladder_next="E%d" % LEVELS[i + 1] if i + 1 < len(LEVELS) else "UNARY")
        structured_control("ladder-up" if up else "ladder-empty", False, dict(bindings, word_state=nm))
        dispatch = P(nm).fresh("b")
        bindings["ladder_dispatch"] = dispatch
        structured_control("ladder-read", False, dict(bindings, word_state=nm))
        targets = {TK[o]: nm + "." + o for o in OPS[lv]}
        for key, target in targets.items():
            g.on(dispatch, [key], target, [], "r")
        for state, row in load_rules(Path(__file__).with_name("ladder-default.tsv"), {},
                domain=set(range(257)) - targets.keys(), bindings=bindings).items():
            for key, (target, actions) in row.items(): g.on(state, [key], target, actions, "r")
        for o in OPS[lv]:
            structured_control("ladder-" + modes.get(o, modes["*"]), False,
                               dict(bindings, ladder_operator=targets[TK[o]], ladder_tail="OPX." + o, word_state=targets[TK[o]]))
    if prefix == "E":
        for lv in LEVELS:
            for o in OPS[lv]:
                if o not in SHORT:
                    optail(o)
        tytail()
    for owner, state, message in tape_rows("ladder-reject.tsv"):
        if owner in ("all", prefix):
            g.on(state, range(257), "DEAD", E.rej(message), "r")


from printfallback import KINDS as PFKINDS, install as pf_install
PFCONV = {ord(k): PFKINDS.index(v) for k,v in E.gold("pfconv") if k != "conv"}

HEX = "0123456789abcdef"
ESC = {"n": 10, "t": 9, "r": 13, "a": 7, "b": 8, "f": 12, "v": 11, "\\": 92, "'": 39, '"': 34, "?": 63, "0": 0}


def fmtwalk(pre, on_byte, on_d, on_end):
    """Bind decoded format scanning to the current conversion catalog."""
    bindings = {key:pre+'.'+suffix for key,suffix in
                [('walk','w'),('percent','pc'),('width','width'),('precision','precision'),('length','length')]}
    bindings.update(on_byte=on_byte, on_d=on_d, on_end=on_end)
    classes = {name:json.loads(value) for name,value in tape_rows('helpers-classes.tsv')}
    sequences = dict(reject=E.rej('not covered: printf conversion'))
    install_rules(g, os.path.dirname(__file__), 'helpers', bindings=bindings, classes=classes,
                  sequences=sequences, section='format')
    rows = [('length', set(range(257))-PFCONV.keys(), bindings)] + [
        ('conversion', [byte], dict(bindings, kind=kind)) for byte,kind in PFCONV.items()]
    for section,domain,facts in rows:
        for state,row in load_rules(Path(__file__).with_name('helpers-byte.tsv'), sequences,
                                    domain=domain, bindings=facts, classes=classes, section=section).items():
            for byte,(target,actions) in row.items(): g.on(state, [byte], target, actions)
    return bindings['walk']


def printf(warnings=False):
    from printfcontrol import install as printf_control
    pf_install(E, P)
    printf_control(E, P, "part0", warnings, TEMPL, dict(SKIPS=SKIPS, FNSTR=FNSTR), HEX)
    strwalk("FMT.walk","FMT.byte","FMT.end")
    printf_control(E, P, "part1", warnings, TEMPL, dict(SKIPS=SKIPS, FNSTR=FNSTR), HEX)
    fmtwalk("PF", "PF.b", "PF.d", "PF.end")
    printf_control(E, P, "part2", warnings, TEMPL, dict(SKIPS=SKIPS, FNSTR=FNSTR), HEX)
    strwalk("PL", "PL.cp", "PL.end")
    from strings import rules as string_rules
    string_rules(E, P, "wide_hooks")
    printf_control(E, P, "part3", warnings, TEMPL, dict(SKIPS=SKIPS, FNSTR=FNSTR), HEX)
    fmtwalk("PO", "PO.b", "PO.d", "PO.end")
    printf_control(E, P, "part4", warnings, TEMPL, dict(SKIPS=SKIPS, FNSTR=FNSTR), HEX)

UNS = E.UNS   # unsigned char/short/int/long: UNS + size
SBB = E.SBB   # a struct's base code: SBB + sid; layouts in the old E3's tables (measured rules)
STAG, SSZ = E.STAG, E.SSZ
STRUCT_MAX, MEMBER_STRIDE = 128, 256
MOF, MSZ, MPT, MBS, MAR, MFLAT = (i << 40 for i in range(1, 7))
BFW, BFO, BFS, TDE = (i << 40 for i in (8, 9, 10, 11))
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
SHAPE = 7 << 40  # pointer object -> dimensions descriptor; separate from object ARR
SHAPE_IDS = 1 << 32  # fresh descriptor pool and member-link namespace; never interned IDs
# START limits token bytes to POSSPAN: interned source names are fewer than
# POSSPAN (each typed identifier costs >1 byte). sid is bounded by STRUCT_MAX.
# SH.NEW checks its monotone count < POSSPAN; DIMS caps rank at eight.
# Pool ARR lies near 2^32, DIM near 2^35; both remain below MOF=2^40.
# SHAPE links use v, or SHAPE_IDS + v*MEMBER_STRIDE + sid, in disjoint ranges.
DIM, TDIM = 28 * 10 ** 6, 29 * 10 ** 6   # DIM[v * 8 + k]: an array's k-th dimension; TDIM[k]: while declaring
PDB = 27 * 10 ** 6   # PDB[f * 16 + k] = base of f's parameter k (a double parameter converts an int argument)
FOPS = {"+":"add", "-":"sub", "*":"mul", "/":"div", "<":"lt", ">":"gt", "<=":"le", ">=":"ge", "==":"eq", "!=":"eq"}
FPU = {row[1]: row[2] for row in E.gold("irsel") if row[0] == "fpu"}
FLT = E.FLT   # f32 value descriptor; pointer depth keeps pointee types distinct
BOOL = 66  # distinct value kind; arithmetic maps to tyinfo u8
assert len({BOOL, DBL, FLT, FPB, FPV}) == 5 and FPV < SBB
TYPEW = {"type=_Bool": BOOL, "type=float": FLT, "type=double": DBL, "type": E.SZ["int"], "type=char": E.SZ["char"], "type=short": E.SZ["short"], "type=long": E.SZ["long"], "type=void": 0}
TWORDS = tuple(TYPEW) + ("type=unsigned", "type=signed")
ENUM_FIRST = 69
ENUM_CAPACITY = FPS_FIRST - ENUM_FIRST
ENUM_STATE, TAG_EPOCH = 63 << 40, 64 << 40
assert FPS_PSH + SBB * 16 < ENUM_STATE < TAG_EPOCH
assert ENUM_CAPACITY > 0 and max([BOOL, DBL, FLT, FPB, FPV] + [code for _, code, *_ in TYINT]) < ENUM_FIRST
assert FPS_FIRST < SBB and SBB + STRUCT_MAX < 4096


def width_dispatch(name, tape, masks=False):
    bindings = {key: name + suffix for key, suffix in
                (("entry", ""), ("base", ".b"), ("double", ".d"), ("float", ".f"),
                 ("f32", ".f32"), ("wide", ".8"), ("number", ".n"), ("bool", ".bool"), ("integer", ".integer"))}
    bindings.update((key + "_test", P(bindings[key]).fresh("b")) for key in ("entry", "base", "double", "float", "number"))
    bindings.update(DBL=DBL, FLT=FLT, BOOL=BOOL, SBB=SBB, entry=name + ".enumraw")
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
    from bitfields import install as bitfield_install
    bitfield_install(E, P, TYINFO, TYPE_TAPE, TYINT, dict(BFW=BFW, BFO=BFO, BFS=BFS, ETAG=ETAG, ENV=ENV, END_=END_, MOF=MOF, MSZ=MSZ, MPT=MPT, MBS=MBS, MAR=MAR, MEMBER_STRIDE=MEMBER_STRIDE, BOOL=BOOL, INTBITS=8 * TYINFO["i32"][0], INTCODE=TYINFO["i32"][0]))
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
    shape_control("dimensions")

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
    shape_control("member-shape")
    structured_control("type-typedef", False)
    follows = dict(tape_rows("type-follow.tsv"))
    for word, value in TYPEW.items():
        p = P("TS." + word)
        structured_control("type-word", False, dict(word_state=p.cur, word_return=p.fresh("r"),
                           type_value=value, word_follow=follows.get(word, follows["*"])))
    structured_control("type-tail", False)
    install_rules(g, os.path.dirname(__file__), "scalar-prefix", section="long-double",
                  bindings=dict(DBL=DBL), classes=dict(double=[TK["type=double"]]))


def shape_control(section):
    bindings = {name: globals()[name] for name in
                ("POSSPAN", "SHAPE", "SHAPE_IDS", "DIM", "TDIM", "MEMBER_STRIDE")}
    bindings.update(ARR=E.ARR, UNSIGNED_CHAR=UNS + 1)
    p = P("shape." + section)
    for part, owner, kind, name in tape_rows("shape-fresh.tsv"):
        if part == section:
            p.cur = owner
            bindings[name] = p.fresh(kind)
    sequences = {name: E.rej(reason) for name, reason in tape_rows("shape-reject.tsv")}
    for name, method, slots in tape_rows("shape-stack.tsv"):
        p.acts = []
        sequences[name] = getattr(p, method)(*slots.split(",")).acts
    classes = {name: [TK[token] for token in tokens.split(",")]
               for name, tokens in tape_rows("shape-tokens.tsv")}
    install_rules(g, os.path.dirname(__file__), "shape", bindings=bindings,
                  sequences=sequences, classes=classes, section=section)


def structured_control(section, warnings, extra=None, sequence_bindings=None):
    section += "-warnings" if warnings and section in ("block", "if") else ""
    p = P("control." + section + (extra or {}).get("word_state", ""))
    bindings = dict(SHAPE=SHAPE, VLDEP=VLDEP, CSV=CSV, CSL=CSL, U32M=U32M, DIM=DIM, TDIM=TDIM, FPB=FPB, FPV=FPV,
                    UNSIGNED_INT=UNS + 4, UNSIGNED_LONG=UNS + 8,
                    statement="STMT.body" if warnings else "STMT")
    bindings.update((name, globals()[name]) for name in
                    ("STAG", "TAGLEVEL", "TAGUNDO", "ETAG", "SBB", "SSZ", "SAL", "SMN", "SMEM", "SFLAT",
                     "MOF", "MSZ", "MPT", "MBS", "MAR", "MFLAT", "MEMBER_STRIDE", "BFW", "BFO", "BFS", "TDE"))
    bindings.update(STRUCT_LIMIT=STRUCT_MAX + 1, MEMBER_MASK=-MEMBER_STRIDE,
                    TAGUNDO1=TAGUNDO + 1, TAGUNDO2=TAGUNDO + 2, TAGUNDO3=TAGUNDO + 3)
    bindings.update(TDN=E.TDN, TDB=E.TDB, TDD=E.TDD, UNS=UNS, UNSIGNED_CHAR=UNS + 1, UNSIGNED_SHORT=UNS + 2)
    bindings.update(extra or {})
    for part, prefix, kind, key in tape_rows("control-fresh.tsv"):
        if part == section:
            p.cur = bindings.get(prefix, prefix)
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


def function_control(section, warnings):
    bindings = dict(PIDS=PIDS, PDB=PDB, LOC=LOC, FND=E.FND, FRD=E.FRD, FRB=E.FRB, VAR=E.VAR)
    bindings.update(("FN_PDB" + str(i), PDB + i) for i in range(16))
    for part, mode, prefix, kind, key in tape_rows("function-fresh.tsv"):
        if part == section and mode in ("common", "warnings" if warnings else "plain"):
            bindings[key] = P(prefix + ".function_" + key).fresh(kind)
    for owner, mode, rules in tape_rows("function-sections.tsv"):
        if owner == section and mode in ("common", "warnings" if warnings else "plain"):
            structured_control(rules, False, bindings)


def global_control(section, warnings):
    bindings = {name: globals()[name] for name in
                ("LOC", "GIBLOB", "GIEND", "GINPS", "GINPE", "GSZ", "GUNIT", "SINIT", "SKIPS")}
    bindings.update((name, getattr(E, name)) for name in ("FND", "GMARK", "BASE", "ARR", "PTR"))
    for part, mode, prefix, kind, key in tape_rows("global-fresh.tsv"):
        if part == section and mode in ("common", "warnings" if warnings else "plain"):
            bindings[key] = P(prefix + ".global_" + key).fresh(kind)
    for owner, mode, rules in tape_rows("global-sections.tsv"):
        if owner == section and mode in ("common", "warnings" if warnings else "plain"):
            structured_control(rules, False, bindings)


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
    from functiontypes import install as functiontypes_install
    functiontypes_install(E, P, dict(FPS_FIRST=FPS_FIRST, SBB=SBB, FPS_RD=FPS_RD, FPS_RB=FPS_RB, FPS_VAR=FPS_VAR, FPS_PARAM=FPS_PARAM, FPS_RSH=FPS_RSH, FPS_FN=FPS_FN, FPS_COUNT=FPS_COUNT, FPS_PSH=FPS_PSH, SHAPE=SHAPE, ARR=E.ARR, DIM=DIM, PDB=PDB, FPB=FPB, FPV=FPV), TYINT)
    from truth import install as truth_install
    truth_install(P, DBL, FLT)
    from booleans import install as bool_install
    bool_install(P, BOOL, DBL, FLT)
    from constexpr import install as const_install
    const_install(E, P, LEVELS, OPS, ENV, END_)
    from statics import install as static_install
    static_install(E, P, TIX, SINIT, SIEND, LOC, SKIPS, BOOL)
    from initializers import install as init_install
    init_install(E, P, SBB, LOC, DIM, SSZ, SMN, SMEM, MOF, MSZ, MPT, MBS, MAR, SFLAT, MFLAT, MEMBER_STRIDE, SKIPS, dict(BFW=BFW, BFO=BFO, BFS=BFS, SHAPE=SHAPE, SHAPE_IDS=SHAPE_IDS))
    strwalk("IC.string", "IC.string_byte", "IC.string_end")
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
    global_control("global0", warnings)
    from enumtypes import install as enum_install
    enum_install(E, P, dict(ENUM_FIRST=ENUM_FIRST, ENUM_LIMIT=FPS_FIRST, ENUM_STATE=ENUM_STATE, TAG_EPOCH=TAG_EPOCH,
                           ETAG=ETAG, TAGLEVEL=TAGLEVEL, STAG=STAG, INT=TYINFO["i32"][0],
                           FPS_FN=FPS_FN, FPS_PARAM=FPS_PARAM),
                 [TK[w] for w in TWORDS if w != "type=void"])
    # enum is a type specifier in both declarations and typedefs.
    global_control("global1", warnings)
    shape_control("typedef-shape")
    global_control("global3", warnings)
    shape_control("global-type")
    global_control("global5", warnings)
    shape_control("global-binding")
    P("ELSZ.enumraw").branch({1: "ELSZ.b0"}, "ELSZ.8", [("CMPI", "td", 0)])
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
    function_control("function0", warnings)
    shape_control("parameter-type")
    function_control("function2", warnings)
    shape_control("parameter-dimensions")
    function_control("function4", warnings)
    shape_control("descriptor-storage")
    function_control("function6", warnings)
    structured_control("parameter-declarators", False)
    function_control("function8", warnings)
    # DECL: the identifier ps..pe becomes the next 8-byte slot (measured: params and int locals)
    # DECLN: the name was saved in ips..ipe (the current token is after it)
    install_rules(g, os.path.dirname(__file__), "declaration", section="name")
    # Scope record fields are declared once; bind/unwind share their layout bindings.
    scope_bindings = {name: getattr(E, name) for name in
                      ("UNDO", "PTR", "BASE", "ARR", "TDN", "TDB", "TDD", "FND", "FRD", "FRB", "VAR")}
    scope_bindings.update(LOC=LOC, END_=END_, ENV=ENV, VLSIZE=VLSIZE, UNDO_SIZE=UNDO_SIZE, SHAPE=SHAPE, TDE=TDE)
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
    p.call("TSPEC").a(("COPYW", "local_base", "tb")).tok({TK_ID: "S.did0", "(": "S.dfp", ";": "S.empty"}, bad("declaration"))
    p = P("S.dfp")
    p.call("FPDECL").a(("LDI", "dsz", 8), ("LDI", "dar", 0)).branch({1: "S.dd"}, "S.dfa", [("CMPI", "fpn", 0)])
    P("S.dfa").a(("ALUI", "mul", "dsz", "fpn", 8), ("LDI", "dar", 1)).goto("S.dd")
    P("S.did0").a(("COPYW", "ips", "ps"), ("COPYW", "ipe", "pe")).goto("S.did")
    P("S.did").a(("LDI", "dsz", 8), ("LDI", "dar", 0)).branch({1: "S.plaintype"}, "S.aliastype", [("CMPI", "type_shape", 0)])
    shape_control("local-type")
    P("S.dst").branch({(1, 2): "S.dst1"}, "S.dnx", [("CMPI", "tb", SBB)])
    P("S.dst1").call("ELSZ").a(("COPYW", "dsz", "es")).goto("S.dnx")
    P("S.dnx").call("NEXT").tok({"[": "S.darr", "(": "S.prototype"}, "S.dd")
    # Same parameter parser, no frame or parameter bindings for a prototype.
    function_control("function9", warnings)
    P("S.dd.enumraw").call("DECLN").call("S.aliassave").tok({"=": "S.din", ",": "S.dcm"}, "S.dend")
    shape_control("local-binding")
    P("S.dend").expect(";").call("NEXT").ret()
    install_rules(g, os.path.dirname(__file__), "local-declarators",
                  classes=dict(identifier=[TK_ID], paren=[TK["("]]),
                  sequences=dict(reject=E.rej("not covered: declarator")), section="main")
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
    P("IA.named").branch({1: "IA.global"}, "CP.ia.mode", [("CMPI", "imode", 1)])
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
    P("S.fixedarray").call("SH.suffix").call("DIMS").call("ELSZ").a(("ALU", "mul", "dsz", "prd", "es"), ("COPYW", "dar", "drk")).goto("S.dd")
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
    p.a(("LDI", "fp_abi_wide", 0), ("LDI", "bf_value", 0), ("COPYW", "st1", "stl"), ("LDI", "stl", 0), ("COPYW", "cv1", "cv"), ("LDI", "cv", 0)).tok({TK_ID: "X.id", "(": "LP.scan"}, "EX.l")
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
    for name, op in (("inc", "+"), ("dec", "-")):
        P("LP."+name).call("CSTEP").call("POST."+op).call("C%d" % LEVELS[0]).call("QTAIL").ret()
    # c ? a : b -- labels as if/else (measured): jumpz L a; a; jump L b; L a: b; L b:
    P("QTAIL").tok({"?": "QT"}, "RET")
    p = P("QT")
    p.a(("ALUI", "add", "lab", "lab", 1), ("COPYW", "a", "lab"), ("ALUI", "add", "lab", "lab", 1), ("COPYW", "b", "lab"))
    emit(p.call("FTRUTH"), "jumpz").call("NEXT").a(("COPYW", "qtpos", "tpos"), ("OLEN", "qtout")).vpush("a", "b", "qtpos", "qtout").call("EXPR").vpop("a", "b", "qtpos", "qtout").expect(":")
    p.a(("COPYW", "np_start", "qtpos"), ("COPYW", "np_end", "tpos")).call("QN.PROOF")
    emit(p, "jump_b")
    emit(p, "label_a").vpush("a", "b", "qtpos", "qtout", "vt", "vb", "np_zero", "rkok", "vid", "rk").call("NEXT").a(("COPYW", "qt_rhs", "tpos")).vpush("qt_rhs").call("E%d" % LEVELS[0]).call("QTAIL").vpop("qt_rhs")
    p.a(("COPYW", "np_start", "qt_rhs"), ("COPYW", "np_end", "tpos")).call("QN.PROOF").vpop("a", "b", "qtpos", "qtout", "lt", "lb", "qt_lzero", "qt_lshape", "qt_lvid", "qt_lrank")
    p.goto("QN.TYPE")
    from conditional import install as conditional_install
    conditional_install(E, P, ENV, END_)
    P("QT.original").branch({1: "QT.merge"}, "QT.floatcheck", [("CMP", "vb", "lb")])
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
    p = P("QT.merge").a(("LDI", "bf_value", 0))
    emit(p, "label_b").branch({1: "QT.1"}, "DEAD.qt", [("CMP", "vt", "lt")])
    P("QT.1").branch({1: "QT.scalar"}, "QT.same", [("CMPI", "vt", 0)])
    P("QT.same").a(("COPYW", "fs_l", "vb"), ("COPYW", "fs_r", "lb")).call("FS.TYPEEQ").branch({1: "RET"}, "DEAD.qt", [])
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
    # Target dispatch is declared once; four conversion calls share the saved descriptor.
    bindings = dict(BOOL=BOOL, DBL=DBL, FLT=FLT, UNSIGNED_WIDE=UNS + 8)
    bindings.update((key, P(owner).fresh("b")) for key, owner in
                    (("entry_test", "ASSIGNCV"), ("bool_test", "ACV.scalar"),
                     ("double_test", "ACV.double"), ("float_test", "ACV.float"), ("unsigned_test", "ACV.int")))
    install_rules(g, os.path.dirname(__file__), "conversion", bindings=bindings, section="assign")
    for suffix in ("d", "s", "i", "u"):
        q = P("ACV." + suffix)
        bindings.update(entry=q.cur, convert="TO." + suffix, resume=q.fresh("r"))
        save = q.vpush("vt", "vb").acts
        q.acts = []
        restore = q.vpop("vt", "vb").acts
        install_rules(g, os.path.dirname(__file__), "conversion", bindings=bindings,
                      sequences={"save": save, "restore": restore}, section="convert")
    shape_control("update-entry")
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
        q.a(("COPYW", "bc_targetvt", "vt"), ("COPYW", "bc_targetvb", "vb"))
        emit(q, "push").call("BF.READ")
        emit(q, "push").vpush("vt", "vb", "stp", "lshapeok", "lshape", "lrank", "bf_width", "bf_offset", "bf_signed", "bf_unit", "bc_targetvt", "bc_targetvb").call("NEXT").call("EXPR").a(("COPYW", "rvb", "vb"), ("COPYW", "rvt", "vt")).vpop("vt", "vb", "stp", "lshapeok", "lshape", "lrank", "bf_width", "bf_offset", "bf_signed", "bf_unit", "bc_targetvt", "bc_targetvb")
        # Compound assignment uses the ordinary gold-table operation, then
        # converts back to the saved destination without reevaluating its address.
        q.vpush("bc_targetvt", "bc_targetvb").a(("COPYW", "lt", "vt"), ("COPYW", "lb", "vb"),
                              ("COPYW", "vt", "rvt"), ("COPYW", "vb", "rvb")).call("OPX." + o)
        q.call("TAX").a(("COPYW", "bc_axis", "ax"), ("COPYW", "rvt", "vt"), ("COPYW", "rvb", "vb")).vpop("vt", "vb").call("ASSIGNCV")
        # OPX already normalizes its result type, and ASSIGNCV owns bool/float conversion.
        q.branch({(AX.index("f32"), AX.index("f64")): "BC.store" + o}, "BC.target" + o, [("RLD", "bc_axis")])
        P("BC.target" + o).branch({1: "BC.store" + o}, "BC.same" + o, [("CMPI", "vb", BOOL)])
        P("BC.same" + o).branch({1: "BC.base" + o}, "BC.narrow" + o, [("CMP", "vt", "rvt")])
        P("BC.base" + o).branch({1: "BC.wide" + o}, "BC.narrow" + o, [("CMP", "vb", "rvb")])
        install_rules(g, os.path.dirname(__file__), "functiontypes", section="compound",
                      bindings=dict(wide="BC.wide"+o, test="BC.widetest"+o, narrow="BC.narrow"+o, store="BC.store"+o))
        P("BC.narrow" + o).call("NARROW").goto("BC.store" + o)
        emit(P("BC.store" + o), "pop1").call("BF.WRITE").call("SH.RESULT").ret()
    P("NODBL").branch({1: "NODBL.l"}, "NODBL.r", [("CMPI", "lb", DBL)])      # a double VALUE (not a pointer to one)
    P("NODBL.l").branch({1: "DEAD.dbl"}, "NODBL.r", [("CMPI", "lt", 0)])
    P("NODBL.r").branch({1: "NODBL.r2"}, "RET", [("CMPI", "vb", DBL)])
    P("NODBL.r2").branch({1: "DEAD.dbl"}, "RET", [("CMPI", "vt", 0)])
    g.on("DEAD.dbl", range(257), "DEAD", E.rej("not covered: double operand"), "r")
    # NARU: an unsigned char/short result masked back before its store (measured, p46); others as they are
    P("TAX.enumraw").branch({(1, 2): "TAX.p"}, "TAX.0", [("CMPI", "vt", 1)])
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
    bindings = dict(BOOL=BOOL, entry_test=P("NARU").fresh("b"), bool_test=P("NARU.1").fresh("b"))
    install_rules(g, os.path.dirname(__file__), "conversion", bindings=bindings, section="unsigned")
    q = P("NARU.integer")
    for _, code, size, uns, _ in (row for row in TYINT if row[3] and row[2] < 8):
        bindings.update(current=q.cur, hit=q.fresh("h"), next=q.fresh("n"), test=q.fresh("b"), code=code)
        install_rules(g, os.path.dirname(__file__), "width", bindings=bindings,
                      sequences={"row": O(TYPE_TAPE["mask"] % ((1 << (8 * size)) - 1))}, section="row")
        q = P(bindings["next"])
    bindings["current"] = q.cur
    install_rules(g, os.path.dirname(__file__), "conversion", bindings=bindings, section="unsigned-end")
    P("STEPTY.enumraw").a(("LDI", "stp", 1)).branch({1: "STY.s"}, "STY.p", [("CMPI", "vt", 0)])
    P("STY.s").branch({1: "RET"}, "STY.s0", [("CMPI", "vb", BOOL)])
    P("STY.s0").branch({1: "DEAD.nint"}, "STY.s1", [("CMPI", "vb", 0)])
    P("STY.s1").branch({(0, 1): "RET"}, "STY.s2", [("CMPI", "vb", 8)])
    P("STY.s2").branch({(0, 1): "STY.s3"}, "DEAD.nint", [("CMPI", "vb", UNS + 8)])
    P("STY.s3").branch({2: "RET"}, "DEAD.nint", [("CMPI", "vb", UNS)])
    P("STY.p").call("ISFP").branch({1: "DEAD.nint"}, "STY.p1", [])
    shape_control("pointee-width")
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
        q.branch({1: "POST.ordinary" + o}, "BF.post" + o, [("CMPI", "bf_width", 0)])
        q = P("POST.ordinary" + o)
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
        emit(q, fix).call("NEXT").call("SH.RESULT").ret()
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
    P("U.str").o("  .lea r0, S").num("sk").o("\n").a(("ALUI", "add", "sk", "sk", 1), ("ALUI", "add", "lab", "lab", 1), ("LDI", "vt", 1), ("COPYW", "vb", "sw"), ("LDI", "rkok", 0)).call("NEXT").tok({E.TK_STR: "DEAD.adj"}, "POSTIX")
    g.on("DEAD.adj", range(257), "DEAD", E.rej("not covered: adjacent string literals"), "r")
    q = P("U.cpl")       # ~x: imm r1, -1; xor64 (measured); the operand's type is kept
    q.call("NEXT").call("UNARY").call("BF.RVALUE").call("NODBL0").o("  imm r1, -1\n  xor64 r0, r0, r1\n").branch({1: "NARU"}, "RET", [("CMPI", "vb", UNS + 4)])
    P("NODBL0").branch({1: "NODBL0.1"}, "NODBL0.p", [("CMPI", "vb", DBL)])
    P("NODBL0.1").branch({1: "DEAD.dbl"}, "NODBL0.p", [("CMPI", "vt", 0)])
    P("NODBL0.p").branch({1: "RET"}, "DEAD.pa", [("CMPI", "vt", 0)])
    # sizeof: a constant, `imm r0, N`; the operand emits nothing (measured). A type, a variable,
    # or a variable with subscripts (each drops one dimension); anything else is not covered
    P("U.szof").call("NEXT").a(("COPYW","szpos","tpos"),("LDI","szparen",0)).tok({"(": "SZ.p", "*": "SZ.star", TK_ID: "SZ.id"}, "SZ.expr")
    P("SZ.p").a(("LDI","szparen",1)).call("NEXT").tok({**{w: "SZ.t" for w in TWORDS}, "struct": "SZ.t", "union": "SZ.t", "enum": "SZ.t", "*": "SZ.star", TK_ID: "SZ.pid"}, "SZ.expr")
    P("SZ.t").call("TSPEC").expect(")").call("ELSZ").a(("COPYW", "sz", "es")).branch({1: "SZ.out"}, "SZ.aliastype", [("CMPI", "type_shape", 0)])
    shape_control("sizeof-type")
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
    P("SZ.memberclose").branch({1: "SZ.memberparen"}, "BF.size.direct", [("CMPI", "szparen", 1)])
    P("SZ.memberparen").tok({")": "SZ.memberdone"}, "SZ.expr")
    P("SZ.memberdone").call("BF.ADDRESSABLE").call("NEXT").goto("SZ.exprout")
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
    shape_control("sizeof-object")
    P("SZ.pd").a(("ALU", "sub", "td", "td", "drop")).branch({0: "DEAD.szx"}, "SZ.sc1", [("CMPI", "td", 0)])
    P("SZ.sc1").call("ELSZ").a(("COPYW", "sz", "es")).ret()
    q = P("SZ.ar")
    q.branch({2: "DEAD.szx"}, "SZ.ar1", [("CMP", "drop", "ar")])
    q = P("SZ.ar1")
    q.a(("ALUI", "sub", "td", "td", 1)).call("ELSZ").a(("COPYW", "sz", "es"), ("COPYW", "k2", "drop")).label("SZ.al")
    q.branch({0: "SZ.a1"}, "RET", [("CMP", "k2", "ar")])
    P("SZ.a1").a(("A64I", "mul", "u", "vid", 8), ("A64", "add", "u", "u", "k2"), ("LDX", "u", "u", DIM), ("ALU", "mul", "sz", "sz", "u"), ("ALUI", "add", "k2", "k2", 1)).goto("SZ.al")
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
    P("SZ.lend").a(("ALU","mul","szlit","szlit","dw")).call("NEXT").goto("SZ.lclose")
    P("SZ.lclose").branch({1:"SZ.ltail"},"SZ.lparen",[("CMPI","szlp",0)])
    P("SZ.lparen").tok({")":"SZ.lpop"},"SZ.general")
    P("SZ.lpop").a(("ALUI","sub","szlp","szlp",1)).call("NEXT").goto("SZ.lclose")
    P("SZ.ltail").tok({k:"SZ.general" for k in ("[","(",".","->","++","--")},"SZ.lvalue")
    P("SZ.lvalue").a(("COPYW","sz","szlit"),("ALUI","add","sk","sk",1),("ALUI","add","lab","lab",1)).goto("SZ.exprout")
    P("SZ.general").a(("JUMP","szpos"),("OLEN","szmark")).vpush("szmark","si_active").a(("LDI","si_active",0)).call("NEXT").call("UNARY").vpop("szmark","si_active").a(("OCUT","szdiscard","szmark")).call("BF.SIZEVALUE").branch({(1,2):"SZ.exprsize"},bad("sizeof non-scalar expression"),[("CMPI","vt",0)])
    P("SZ.exprsize").a(("COPYW","td","vt"),("COPYW","tb","vb")).call("ELSZ").a(("COPYW","sz","es")).goto("SZ.exprout")
    P("SZ.exprout").o("  imm r0, ").num("sz").o("\n").a(("LDI","vt",0),("LDI","vb",UNS + 8)).ret()
    g.on("DEAD.szx", range(257), "DEAD", E.rej("not covered: sizeof operand"), "r")
    P("SZ.out").o("  imm r0, ").num("sz").o("\n").a(("LDI", "vt", 0), ("LDI", "vb", UNS + 8)).call("NEXT").ret()
    P("U.fnum").o("  imm r0, ").a(("LDI", "nx", 1)).call("NUMOUT").o("\n").a(("LDI", "vt", 0), ("LDI", "vb", DBL)).branch({1: "U.f32"}, "U.fnext", [("CMPI", "df_mbits", 23)])
    P("U.f32").a(("LDI", "vb", FLT)).goto("U.fnext")
    P("U.fnext").call("NEXT").ret()
    p = P("UNARY").a(("LDI", "bf_value", 0))
    p.tok({"sizeof": "U.szof", E.TK_FNUM: "U.fnum", E.TK_STR: "U.str", "~": "U.cpl", "-": "U.neg", "+": "U.pos", "!": "U.not", "(": "U.par", TK_NUM: "U.num", TK_ID: "U.id", "++": "U.pinc", "--": "U.pdec", "&": "U.amp", "*": "U.deref"}, bad("expression"))
    structured_control("address", False)
    addr(P("ADR.object")).goto("ADR.object.next")
    q = P("U.deref")     # * operand: its value is the address; one level down, then a load at the new width
    q.call("NEXT").tok({TK_ID: "UD.id"}, "UD.gen")
    P("UD.gen").call("UNARY").goto("UD.dn")
    shape_control("dereference")
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
    P("DOWN.0").branch({(1, 2): "DOWN.1"}, bad("dereference of a non-pointer"), [("CMPI", "vt", 1)])
    P("DOWN.1").a(("ALUI", "sub", "vt", "vt", 1)).branch({1: "DOWN.2"}, "RET", [("CMPI", "vt", 0)])
    P("DOWN.2").branch({1: "DEAD.void"}, "RET", [("CMPI", "vb", 0)])
    g.on("DEAD.void", range(257), "DEAD", E.rej("not covered: dereference of void"), "r")
    # Prefix updates share the existing member/subscript address walk. The
    # address is evaluated once; its value kind then selects step/load/store.
    for nm, fix in (("U.pinc", "pre_inc"), ("U.pdec", "pre_dec")):
        q = P(nm)           # addr; push; load; +-step; pop; store
        q.call("NEXT").call("PRE.addr").call("CSTEP")
        emit(q, "push").call("BF.READ").branch({1: nm + ".scalar"}, nm + ".integer", [("CMPI", "vt", 0)])
        P(nm + ".scalar").branch({(DBL, FLT): nm + ".float"}, nm + ".integer", [("RLD", "vb")])
        P(nm + ".float").call("FPSTEP." + ("+" if nm == "U.pinc" else "-")).goto(nm + ".store")
        emit(P(nm + ".integer"), fix).call("NARU").goto(nm + ".store")
        emit(P(nm + ".store"), "pop1").call("BF.WRITE").call("SH.RESULT").ret()
    install_rules(g, os.path.dirname(__file__), "scalar-prefix", section="positive",
                  bindings=dict(INT=TYINFO["i32"][0]),
                  classes=dict(promote=[BOOL] + [code for _, code, size, _, _ in TYINT if size < TYINFO["i32"][0]],
                               arithmetic=[DBL, FLT] + [code for _, code, size, _, _ in TYINT if size >= TYINFO["i32"][0]]),
                  sequences=dict(reject=E.rej("not covered: unary + requires arithmetic operand")))
    from unarycontrol import install as unary_control
    unit_span = int(re.search(r"^#define MAXTOK ([0-9]+)\b", Path(E.ROOT, "src/front_pp.c").read_text(), re.M).group(1))
    unary_control(E, P, warnings, TEMPL, addr, printf, structured_control,
                  dict(DBL=DBL, FLT=FLT, BOOL=BOOL, UNS1=UNS+1, UNS3=UNS+3, UNS4=UNS+4, UNS8=UNS+8,
                       U32M=U32M, ENV=ENV, END_=END_, FNSTR=FNSTR, TIX=TIX, MAXTOK=unit_span), TWORDS)
    shape_control("value-load")
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
        ("LDX", "mo", "k", MOF), ("LDX", "ms", "k", MSZ), ("LDX", "vt", "k", MPT), ("LDX", "vb", "k", MBS), ("LDX", "marr", "k", MAR), ("LDX", "bf_width", "k", BFW), ("LDX", "bf_offset", "k", BFO), ("LDX", "bf_signed", "k", BFS), ("COPYW", "bf_unit", "ms"), ("A64I", "add", "member_vid", "k", SHAPE_IDS), ("LDX", "member_vid", "member_vid", SHAPE)).ret()
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
    P("MB.address").a(("ALUI", "add", "vt", "vt", 1), ("LDI", "amp", 0), ("LDI", "adr_kind", 1)).ret()
    P("MB.value").call("BF.READ").a(("COPYW", "bf_value", "bf_width"), ("LDI", "rkok", 0)).goto("POSTIX")
    P("MB.arr").a(("ALUI", "add", "vt", "vt", 1), ("COPYW", "vid", "member_vid"), ("LDX", "rk", "vid", E.ARR), ("LDI", "rkok", 1)).branch({1: "MB.arrayaddr"}, "POSTIX", [("CMPI", "amp", 1)])   # s.arr: the address, decayed
    P("MB.arrayaddr").tok({"[": "POSTIX"}, "ADR.arrayterminal")
    p = P("POSTIX")
    p.tok({"[": "PX.i", ".": "MEMB", "->": "MEMB", "(": "PX.fc"}, "RET")
    P("PX.fc").call("ISFP").branch({1: "PX.fc1"}, "RET", [])
    P("PX.fc1").call("FPCALL").goto("POSTIX")
    q = P("PX.i")
    q.branch({(1, 2): "PX.ok"}, bad("subscript of a non-pointer"), [("CMPI", "vt", 1)])
    q = P("PX.ok")
    q.branch({1: "PX.r0"}, "PX.one", [("CMPI", "rkok", 1)])
    P("PX.r0").branch({2: "PX.md"}, "PX.one", [("CMPI", "rk", 1)])
    shape_control("subscript")
    q = P("PX.sd")
    emit(q, "push").vpush("vt", "vb", "st1", "rk", "vid", "str", "amp").a(("LDI", "amp", 0)).call("NEXT").call("EXPR").expect("]").vpop("vt", "vb", "st1", "rk", "vid", "str", "amp")
    q.branch({1: "PX.m1"}, "PX.mm", [("CMPI", "str", 1)])
    P("PX.mm").o("  imm r2, ").num("str").o("\n  mul64 r0, r0, r2\n").goto("PX.m1")
    q = P("PX.m1").call("BF.OBJECT")
    emit(q, "pop1").o("  add64 r0, r1, r0\n").a(("ALUI", "sub", "rk", "rk", 1), ("LDI", "rkok", 1)).call("NEXT").goto("POSTIX")
    q = P("PX.one")
    emit(q, "push").vpush("vt", "vb", "st1", "amp").a(("LDI", "amp", 0)).call("NEXT").call("EXPR").expect("]").vpop("lt", "lb", "st1", "amp").a(("LDI", "lshapeok", 0)).call("SCALE")
    emit(q, "pop1").o("  add64 r0, r1, r0\n").a(("COPYW", "vt", "lt"), ("COPYW", "vb", "lb")).call("DOWN").call("BF.OBJECT").call("NEXT").tok({"=": "PX.as", "++": "PX.inc", "--": "PX.dec", **{o+"=": "LV.c"+o for o in E.CASOPS}, ".": "MEMB"}, "PX.ld")   # a[i].m: the element's address, then the member
    # a statement that is only `p[i];` computes the address and stops (measured, probe p39)
    P("PX.ld").branch({1: "PX.am"}, "PX.ld1", [("CMPI", "amp", 1)])
    P("PX.am").a(("ALUI", "add", "vt", "vt", 1), ("LDI", "amp", 0), ("LDI", "adr_kind", 1)).ret()
    P("PX.ld1").branch({1: "PX.st"}, "PX.l2", [("CMPI", "st1", 1)])
    P("PX.st").tok({";": "RET"}, "PX.l2")
    P("PX.l2").call("LOADV").a(("LDI", "rkok", 0)).goto("POSTIX")
    q = P("PX.as")
    emit(q, "push").vpush("vt", "vb", "bf_width", "bf_offset", "bf_signed", "bf_unit").call("NEXT")
    if warnings: q.a(("COPYW", "wi_target", "vt")).call("WI.expr")
    else: q.call("EXPR")
    q.vpop("lt", "lb", "bf_width", "bf_offset", "bf_signed", "bf_unit").branch({1: "AS.kind"}, "AS.scalar", [("CMPI", "lt", 0)])
    P("AS.kind").branch({(1, 2): "AS.struct"}, "AS.scalar", [("CMPI", "lb", SBB)])
    q = P("AS.scalar")
    q.a(("COPYW", "rvb", "vb"), ("COPYW", "rvt", "vt"), ("COPYW", "vt", "lt"), ("COPYW", "vb", "lb")).call("ASSIGNCV")
    emit(q, "pop1").call("BF.WRITE").call("BF.assignment").ret()
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
    q.call("NEXT").call("UNARY").call("DOWN").call("BF.OBJECT").goto("SS.dispatch")
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
    P("FNV.y").o("  .lea r0, ").a(("SPAN2", "ips", "ipe")).o("\n").a(("LDI", "vt", 1), ("LDI", "isfn", 1), ("LDX", "vb", "v", FPS_FN), ("LDX", "t", "v", E.VAR), ("STX", "vb", FPS_VAR, "t")).ret()
    p = P("LOOKUP")     # s := the slot of ips..ipe (0: not a local of this slice)
    if warnings: p.call("WU.use")
    p.a(("LDI", "bf_width", 0), ("INTERN", "v", "ips", "ipe"), ("LDX", "s", "v", LOC), ("LDX", "vt", "v", E.PTR), ("LDX", "vb", "v", E.BASE), ("LDX", "ar", "v", E.ARR), ("COPYW", "vid", "v")).branch({1: "DEAD.nl"}, "RET", [("CMPI", "s", 0)])
    g.on("DEAD.nl", range(257), "DEAD", E.rej("not covered: identifier is not a local"), "r")
    from callcontrol import install as call_control
    call_facts = dict(LOC=LOC, SBB=SBB, SSZ=SSZ, FPS_FN=FPS_FN, PDB=PDB, DBL=DBL, FLT=FLT, BOOL=BOOL)
    fpu = {row[1]: row[2] for row in E.gold("irsel") if row[0] == "fpu"}
    call_bindings = call_control(E, P, warnings, TEMPL, addr, call_facts, SYSCALLS, fpu, "begin")
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
    call_control(E, P, warnings, TEMPL, addr, call_facts, SYSCALLS, fpu, "finish", call_bindings)
    ud_install(E, P)
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
