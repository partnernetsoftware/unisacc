"""E3 delta generator: shared expression/type helpers and handwritten
state/action rules, with selected attributes read from the gold tables.

The declarative grammar compiler described in archive/docs/exec/e3-structured.md is
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
import sys as _s, pathlib as _p
_s.path.insert(0, str(_p.Path(__file__).resolve().parents[1] / "facts"))
from load import facts as _facts
_LX = {r["name"]: r["value"] for r in _facts("libraryexports")}
_VR = {r["name"]: r["value"] for r in _facts("valueranks") if r["kind"] == "bank"}   # exec/facts/valueranks.tsv
_RANKOF = {r["name"]: r["value"] for r in _facts("valueranks") if r["kind"] == "rank"}


def _slots(items):   # each value slot is followed by its rank companion (valueranks facts)
    result = []
    for item in items:
        result.append(item)
        companion = _RANKOF.get(item)
        if companion and companion not in items: result.append(companion)
    return result
TYPERANK, MEMBERRANK, RETURNRANK, PARAMRANK = (_LX[k] for k in ("TYPERANK", "MEMBERRANK", "RETURNRANK", "PARAMRANK"))
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
SYSCALLS = ([(name, op, 3) for name, op in INTRINSIC.items()]
            + [(name, op, 6) for name, op in INTRINSIC6.items()]
            + [("__hostcall", "hostcall", 2)]
            + [("__hostaddr%d" % i, "hostaddr%d" % i, 0) for i in range(4)])

O, TK, TK_ID, TK_NUM, LOC = E.O, E.TK, E.TK_ID, E.TK_NUM, E.LOC
VLSIZE, VLFRAME, VLDEP = 52 << 40, 53 << 40, 54 << 40
UNDO_SIZE = 43
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
    def vpush(self,*items):
        return super().vpush(*_slots(items))
    def vpop(self,*items):
        return super().vpop(*_slots(items))
    def __init__(self, name):
        DEFS[name, "P"] = DEFS.get((name, "P"), 0) + 1
        super().__init__(name)

    def label(self, lab):
        DEFS[lab, "L"] = DEFS.get((lab, "L"), 0) + 1
        return super().label(lab)


E.P = P
g = E.g
from unresolved import install as ud_install
from finite_rules import install as install_rules, install_rows, install_template, load as load_rules

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
    """Variable address selection: exec/parse2/addr-manifest.tsv (K2 sub-manifest)."""
    import assemble
    env = assemble.run(Path(__file__).resolve().parent / 'addr-manifest.tsv', E, P, {}, dict(entry=p.cur, pending=p.acts))
    p.cur, p.acts = env['done'], []
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


def optail_facts(o):
    """Operator domain data for one operator tail (facts k2-gen2 oprows via exec/facts/export.py):
    offset in RST, selected mnemonics, pointer mode, comparison/invert flags, float opcodes."""
    modes = {name: (pointer, int(compare), int(invert)) for name, pointer, compare, invert in tape_rows("operator-modes.tsv")}
    pointer, compare, invert = modes.get(o, modes["*"])
    split = lambda t: re.fullmatch(r"  (\S+) r0, (.*)\n", t).groups()
    sop, sregs = split(E.optext(o))
    uop, uregs = split(E.optext(o, True))
    floats = []
    for suffix, base in (("d", DBL), ("s", FLT)) if o in FOPS else ():
        opcode = FPU[suffix + FOPS[o]]
        floats.append(dict(sfx=suffix, fop=opcode.removesuffix("_rev"),
                           fregs="r0, r1" if opcode.endswith("_rev") else "r1, r0", result=4 if compare else base))
    one = lambda b: [{}] if b else []
    return dict(op=o, offset=[x for lv in LEVELS for x in OPS[lv] if x not in SHORT].index(o) * 256,
                sop=sop, sregs=sregs, uop=uop, uregs=uregs, ptr=pointer, ptrn=one(pointer in ("add", "sub")),
                cmpl=one(compare), invl=one(invert), fl=one(floats), nf=one(not floats), floats=floats)


def tytail():
    segment("tytail")   # gen2-manifest rows over facts k2-gen2 ckmrows/resdrows


def strwalk(pre, body, done):
    import assemble
    assemble.run(Path(__file__).resolve().parent / 'strwalk-manifest.tsv', E, P, {}, dict(pre=pre, body=body, done=done))


def ladder(prefix, bottom):
    """Precedence ladder: gen2-manifest segments (levels, then E's operator tails, then rejects)."""
    segment("ladder-" + prefix)
    if prefix == "E":
        segment("optail")
        tytail()
    segment("ladder-%s-reject" % prefix)


sys.path.insert(0, str(Path(__file__).resolve().parent.parent / 'facts')); from load import facts as _pffacts
PFKINDS = {r['name']: r['value'] for r in _pffacts('printfallback')}['KINDS']   # printfallback-manifest.tsv (K2 trace translation)
PFCONV = {ord(k): PFKINDS.index(v) for k,v in E.gold("pfconv") if k != "conv"}

HEX = "0123456789abcdef"
ESC = {"n": 10, "t": 9, "r": 13, "a": 7, "b": 8, "f": 12, "v": 11, "\\": 92, "'": 39, '"': 34, "?": 63, "0": 0}


def fmtwalk(pre, on_byte, on_d, on_end):
    """Decoded format scanning: exec/parse2/fmtwalk-manifest.tsv (K2 sub-manifest)."""
    import assemble
    assemble.run(Path(__file__).resolve().parent / 'fmtwalk-manifest.tsv', E, P, {},
                 dict(pre=pre, on_byte=on_byte, on_d=on_d, on_end=on_end))
    return pre + '.w'


def printf(warnings=False):
    """printf family: exec/parse2/printf-manifest.tsv (K2 sub-manifest)."""
    import assemble
    assemble.run(Path(__file__).resolve().parent / 'printf-manifest.tsv', E, P, dict(warnings=warnings), dict(warnings=warnings))


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
FOPS = dict(tape_rows("operator-float.tsv"))   # arithmetic operator -> float mnemonic (declared, not a second hand list)
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
    import assemble   # K2 trace translation: bitfields-manifest.tsv
    assemble.run(Path(__file__).parent / 'bitfields-manifest.tsv', E, P, {}, {})   # no mode rows
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
    segment("types-dimensions")
    strwalk("DM.s", "DM.sb", "DM.se")
    segment("types-dimensions-tail")
    # The bounded lookahead for a tentative incomplete array is a declared
    # transition graph.  Token codes are stable parser facts; the source row
    # set is exec/parse2/tentative-result.tsv.
    E.g.labels.update(("DM.tent.scan", "DM.tent.after"))
    install_rules(g, os.path.dirname(__file__), "tentative",
                  bindings=dict(TENTATIVE=764 << 40), section="main")
    shape_control("dimensions")

    # TSPEC: type words then stars -> tb (base size, 0 void), td (depth); current token after
    dispatch = segment("types-entry")["td"]
    targets = {TK[word]: "TS." + word for word in TWORDS}
    targets.update((TK_ID if word == "identifier" else TK[word], target)
                   for word, target in tape_rows("type-entry.tsv"))
    ctx = dict(dispatch=dispatch, operators=[dict(key=key, target=target)
                                            for key, target in targets.items()])
    install_template(g, os.path.dirname(__file__), "dispatch", dict(ctx=[ctx], op=ctx["operators"]),
                     P(dispatch).fresh, sequences=dict(reject=E.rej("not covered: type")),
                     section="type")
    segment("types-prefix")
    shape_control("member-shape")
    segment("types-typedef")
    segment("types-word")   # foreach facts k2-gen2 typewords
    segment("types-tail")
    install_rules(g, os.path.dirname(__file__), "scalar-prefix", section="long-double",
                  bindings=dict(DBL=DBL), classes=dict(double=[TK["type=double"]]))


def shape_control(section):
    bindings = {name: globals()[name] for name in
                ("POSSPAN", "SHAPE", "SHAPE_IDS", "DIM", "TDIM", "MEMBER_STRIDE", "TYPERANK", "MEMBERRANK", "RETURNRANK", "PARAMRANK")}
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


def structured_control(section, warnings, extra=None, sequence_bindings=None, export=False):
    """Structured control: exec/parse2/control-manifest.tsv (K2 sub-manifest)."""
    import assemble
    section += "-warnings" if warnings and section in ("block", "if") else ""
    return assemble.run(Path(__file__).resolve().parent / 'control-manifest.tsv', E, P, {},
                 dict(control_export=export, control_section=section, statement="STMT.body" if warnings else "STMT",
                      extra=dict(extra or {}), seqb=dict(sequence_bindings or {})))


def segment(name, warnings=False):
    """One transitional segment of exec/parse2/gen2-manifest.tsv (rows gated by env fact seg_NAME)."""
    import assemble
    return assemble.run(Path(__file__).resolve().parent / 'gen2-manifest.tsv', E, P, dict(warnings=warnings), {"seg_" + name: 1})


def ordinary_control(section, warnings, extra=None):
    bindings=dict(DBL=DBL, FLT=FLT, GMARK=E.GMARK, bottom="C%d" % LEVELS[0])
    bindings.update(extra or {})
    p=P("ordinary.bindings."+section+bindings.get("word_state", ""))
    for part, mode, owner, kind, key in tape_rows("ordinary-fresh.tsv"):
        if part == section and mode in ("common", "warnings" if warnings else "plain"):
            p.cur=owner;bindings[key]=p.fresh(kind)
    sequences={name:O(json.loads(value)) for name,value in tape_rows("ordinary-text.tsv")}
    for part, mode, rules in tape_rows("ordinary-sections.tsv"):
        if part == section and mode in ("common", "warnings" if warnings else "plain"):
            structured_control(rules, False, bindings, sequences)
    return bindings


def _namespace_control(namespace, section, warnings, bindings):
    """Shared dispatch loop: function_control/global_control/local_control differ
    only in their tsv prefix and starting bindings, not in this iteration shape."""
    mode_ok = "warnings" if warnings else "plain"
    for part, mode, prefix, kind, key in tape_rows(namespace + "-fresh.tsv"):
        if part == section and mode in ("common", mode_ok):
            bindings[key] = P(prefix + "." + namespace + "_" + key).fresh(kind)
    if namespace == "global":   # named results for later stages (librarymodule): global-results.tsv
        for part, key, name in tape_rows("global-results.tsv"):
            if part == section:
                E.__dict__.setdefault("results", {})[name] = bindings[key]
    for owner, mode, rules in tape_rows(namespace + "-sections.tsv"):
        if owner == section and mode in ("common", mode_ok):
            structured_control(rules, False, bindings)


def function_control(section, warnings):
    bindings = dict(PIDS=PIDS, PDB=PDB, LOC=LOC, FND=E.FND, FRD=E.FRD, FRB=E.FRB, VAR=E.VAR)
    bindings.update(("FN_PDB" + str(i), PDB + i) for i in range(16))
    _namespace_control("function", section, warnings, bindings)


def global_control(section, warnings):
    bindings = {name: globals()[name] for name in
                ("LOC", "GIBLOB", "GIEND", "GINPS", "GINPE", "GSZ", "GUNIT", "SINIT", "SKIPS")}
    bindings.update((name, getattr(E, name)) for name in ("FND", "GMARK", "BASE", "ARR", "PTR"))
    _namespace_control("global", section, warnings, bindings)


def local_control(section, warnings):
    bindings = dict(SKIPS=SKIPS, PTR=E.PTR, BASE=E.BASE)
    _namespace_control("local", section, warnings, bindings)


def return_control(section, extra=None):
    b = dict(SBB=SBB, SSZ=SSZ, CKT=CKT, expr_entry="E%d" % LEVELS[0], tail_entry="C%d" % LEVELS[0])
    b.update(extra or {})
    p = P("return.bindings." + section + "." + str(P.n))
    for part, owner, kind, key in tape_rows("return-fresh.tsv"):
        if part == section:
            p.cur = owner
            b[key] = p.fresh(kind)
    sequences = {name: O(json.loads(value)) for name,value in tape_rows("return-text.tsv")}
    sequences.update((name, O(re.split(r"(\{[^}]*\})", TEMPL[template])[int(index)]))
                     for name,template,index in tape_rows("return-template.tsv"))
    sequences.update((name, E.rej(message)) for name,message in tape_rows("return-reject.tsv"))
    for name,method,slots in tape_rows("return-stack.tsv"):
        p.acts = []
        sequences[name] = getattr(p, method)(*slots.split(",")).acts
    tokens = dict(TK, identifier=TK_ID)
    classes = {name:[tokens[token]] for name,token in tape_rows("return-tokens.tsv")}
    classes.update(typewords=[TK[w] for w in TWORDS], scalar_types=[BOOL,DBL,FLT], float_types=[DBL,FLT],
                   axis_f64=[AX.index("f64")], axis_f32=[AX.index("f32")],
                   operators=[TK[o] for o in ("=","++","--")] + [TK[o+"="] for o in E.CASOPS])
    install_rules(g, os.path.dirname(__file__), "return", bindings=b, sequences=sequences, classes=classes, section=section)
    if section == "expr0":
        compound = {TK[o+"="]:"LV.c"+o for o in E.CASOPS}
        dispatch = b["f48"]
        for domain, selected, additions in [(set(range(257))-compound.keys(), "expr0", {})] + [
                ([key], "compound", dict(lp_dispatch=dispatch,operation=target)) for key,target in compound.items()]:
            install_rows(g, Path(__file__).with_name("return-dispatch.tsv"), sequences,
                         domain=domain, bindings=dict(b,**additions), classes=classes,
                         section=selected)
    return b


def update_control(section, extra=None):
    b = {name: globals()[name] for name in ("SBB", "UNS", "DBL", "FLT", "BOOL", "FPB", "FPV", "ENV", "END_", "LOC", "FPS_FN", "FPS_VAR")}
    b.update((name, getattr(E, name)) for name in ("FND", "VAR", "PTR", "BASE", "ARR"))
    b.update(("U"+str(size), UNS+size) for size in (1,2,4,8))
    b.update(tail_entry="C%d" % LEVELS[0], axis_ptr=AX.index("ptr"), axis_struct=AX.index("struct"))
    b.update(extra or {})
    for key,template in tape_rows("update-states.tsv"):
        b[key] = template.format(op=b.get("op",""), name=b.get("name",""), suffix=b.get("suffix",""))
    p = P("update.bindings."+section+"."+str(P.n)+"."+b.get("op","")+"."+b.get("suffix",""))
    for part,owner,kind,key in tape_rows("update-fresh.tsv"):
        if part == section:
            p.cur = b[owner[1:]] if owner.startswith("$") else owner
            b[key] = p.fresh(kind)
    texts = {name:json.loads(value) for name,value in tape_rows("update-text.tsv")}
    sequences = {name:O(value) for name,value in texts.items()}
    sequences.update((name,E.rej(message)) for name,message in tape_rows("update-reject.tsv"))
    for name,template,index in tape_rows("update-template.tsv"):
        template = b.get(template[1:]) if template.startswith("$") else template
        if template is not None: sequences[name]=O(re.split(r"(\{[^}]*\})",TEMPL[template])[int(index)])
    for name,method,slots in tape_rows("update-stack.tsv"):
        p.acts = []
        sequences[name] = getattr(p,method)(*slots.split(",")).acts
    if "op" in b:
        sequences["operator"] = O(E.optext(b["op"]))
    if "integer" in b:
        sequences["bool_step"] = O(texts["bool_step"] % b["integer"])
    if "bits" in b:
        sequences["fp_step"] = O(texts["fp_step"] % (b["bits"],FPU[b["suffix"]+b["floating"]]))
    tokens=dict(TK,identifier=TK_ID)
    classes={name:[tokens[token]] for name,token in tape_rows("update-tokens.tsv")}
    classes.update(BOOL=[BOOL],float_types=[DBL,FLT],float_axes=[AX.index("f32"),AX.index("f64")],
                   signed_narrow_codes=[code for _,code,size,uns,_ in TYINT if not uns and size < 8])
    install_rules(g,os.path.dirname(__file__),"update",bindings=b,sequences=sequences,classes=classes,section=section)
    if section == "id0":
        compound={TK[o+"="]:"X.c"+o for o in E.CASOPS}
        for domain,selected,additions in [(set(range(257))-compound.keys(),"id0",{})]+[
                ([key],"compound",dict(id_dispatch=b["f1"],operation=target)) for key,target in compound.items()]:
            install_rows(g, Path(__file__).with_name("update-dispatch.tsv"), sequences,
                         domain=domain, bindings=dict(b,**additions), classes=classes,
                         section=selected)
    return b


def build(locations=False, warnings=False, errors=False):
    # Unit markers are emitted only by the model framing pass. Each scan's
    # first marker resets the epoch; single-unit token dumps keep epoch zero.
    E.WORDS.append("type=extern"); E.TK["type=extern"] = max(E.TK.values()) + 1
    E.WORDS.append("type=_Bool"); E.TK["type=_Bool"] = max(E.TK.values()) + 1
    qualifiers = ("type=const", "type=volatile", "type=restrict", "type=inline")
    E.tokenizer(qualifiers)
    # The location/static readers replace NEXT later.  Keep the plain token
    # decoder for lookahead; ordinary qualifier recursion must still pass
    # through NEXT so each source token gets its ordinal.
    assert "TN.raw" not in g.st
    install_template(g, os.path.dirname(__file__), "stage-edits", {},
                     P("NX").fresh, section="startup")
    install_template(g, os.path.dirname(__file__), "stage-edits", {},
                     lambda kind: None, section="startup-entry", mode="b", domain=[64])
    segment("startup-marker")
    from strings import token_span
    token_span(E, P)
    from strings import initializer as string_initializer
    string_initializer(E, P, ESC)
    E.prn()
    E.numout()
    import assemble   # K2 trace translation: floatconst-manifest.tsv
    from types import SimpleNamespace
    assemble.run(Path(__file__).resolve().parent / 'floatconst-manifest.tsv', E, P, dict(locations=locations, warnings=warnings, errors=errors), dict())
    E.autoscan()
    types()
    import assemble   # K2 trace translation: functiontypes-manifest.tsv
    from types import SimpleNamespace
    assemble.run(Path(__file__).resolve().parent / 'functiontypes-manifest.tsv', E, P, dict(locations=locations, warnings=warnings, errors=errors), dict())
    import assemble   # K2 trace translation: truth-manifest.tsv
    assemble.run(Path(__file__).parent / 'truth-manifest.tsv', E, P,
                 dict(locations=locations, warnings=warnings, errors=errors), dict(DBL=DBL, FLT=FLT))
    import assemble   # K2 trace translation: booleans-manifest.tsv
    assemble.run(Path(__file__).parent / 'booleans-manifest.tsv', E, P,
                 dict(locations=locations, warnings=warnings, errors=errors), dict(BOOL=BOOL, DBL=DBL, FLT=FLT))
    from constexpr import install as const_install
    const_install(E, P, LEVELS, OPS, ENV, END_)
    import assemble   # K2 trace translation: statics-manifest.tsv
    from types import SimpleNamespace
    assemble.run(Path(__file__).resolve().parent / 'statics-manifest.tsv', E, P, dict(locations=locations, warnings=warnings, errors=errors), dict(BOOL=BOOL, LOC=LOC, SIEND=SIEND, SINIT=SINIT, SKIPS=SKIPS, TIX=TIX))
    assemble.run(Path(__file__).parent / "initializers-manifest.tsv", E, P, {}, dict(
        SBB=SBB, LOC=LOC, SSZ=SSZ, SMN=SMN, SMEM=SMEM, MOF=MOF, MPT=MPT, MBS=MBS, MAR=MAR,
        SFLAT=SFLAT, MFLAT=MFLAT, MEMBER_STRIDE=MEMBER_STRIDE, SKIPS=SKIPS, UCHAR=E.UNS + 1,
        BFW=BFW, BFO=BFO, BFS=BFS, SHAPE=SHAPE, SHAPE_IDS=SHAPE_IDS, MSZ=MSZ,
        PTR=E.PTR, BASE=E.BASE, ARR=E.ARR, DIM=DIM, DIM1=DIM + 1, DIM2=DIM + 2))
    strwalk("IC.string", "IC.string_byte", "IC.string_end")
    segment("staticauto")
    # ---- declared data 3: the grammar, compiled to procedures ---------------------------
    segment("startup-guard")   # POSSPAN from facts k2-gen2
    env = segment("startup-run")   # startup data from facts k2-gen2 (tyrows, syscalls, autonames)
    E.__dict__.setdefault("results", {}).update(lm_hstate=env["lm_hstate"], lm_hnext=env["lm_hnext"], lm_header=O(E.HEADER), lm_errors=errors)
    global_control("global0", warnings)
    import assemble   # K2 trace translation: enumtypes-manifest.tsv
    assemble.run(Path(__file__).parent / 'enumtypes-manifest.tsv', E, P,
                 dict(locations=locations, warnings=warnings, errors=errors), {})
    # enum is a type specifier in both declarations and typedefs.
    global_control("global1", warnings)
    shape_control("typedef-shape")
    global_control("global3", warnings)
    shape_control("global-type")
    global_control("global5", warnings)
    shape_control("global-binding")
    size_bindings = {key:P(state).fresh('b') for key,state in
                     [('enum_test','ELSZ.enumraw'),('base_test','ELSZ.b0'),('struct_test','ELSZ.st'),
                      ('void_test','ELSZ.b'),('double_test','ELSZ.s'),('float_test','ELSZ.sf'),
                      ('bool_test','ELSZ.bool'),('unsigned_test','ELSZ.s2')]}
    install_rules(g, os.path.dirname(__file__), 'width', section='element-size',
                  bindings=dict(size_bindings, SBB=SBB, SSZ=SSZ, DBL=DBL, FLT=FLT, BOOL=BOOL, UNS=UNS),
                  sequences=dict(incomplete=E.rej('not covered: incomplete struct')))
    function_control("function0", warnings)
    shape_control("parameter-type")
    function_control("function2", warnings)
    shape_control("parameter-dimensions")
    function_control("function4", warnings)
    shape_control("descriptor-storage")
    function_control("function6", warnings)
    segment("parameter-declarators")
    function_control("function8", warnings)
    # DECL: the identifier ps..pe becomes the next 8-byte slot (measured: params and int locals)
    # DECLN: the name was saved in ips..ipe (the current token is after it)
    install_rules(g, os.path.dirname(__file__), "declaration", section="name")
    # Scope record fields are declared once; bind/unwind share their layout bindings.
    scope_bindings = {name: getattr(E, name) for name in
                      ("UNDO", "PTR", "BASE", "ARR", "TDN", "TDB", "TDD", "FND", "FRD", "FRB", "VAR")}
    scope_bindings.update(VALUEBANK=_VR["VALUEBANK"], TYPERANK=TYPERANK, LOC=LOC, END_=END_, ENV=ENV, VLSIZE=VLSIZE, UNDO_SIZE=UNDO_SIZE, SHAPE=SHAPE, TDE=TDE)
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
    import assemble   # K2 trace translation: vla-manifest.tsv
    from types import SimpleNamespace
    assemble.run(Path(__file__).resolve().parent / 'vla-manifest.tsv', E, P, dict(locations=locations, warnings=warnings, errors=errors), dict(DEP=VLDEP, ENUM=END_, FRAME=VLFRAME, SIZE=VLSIZE, UNS=UNS))
    # statements
    segment("dispatch-block", warnings)
    scope_bindings["scope_compare"] = P("S.uw").fresh("b")
    if warnings: scope_bindings["unbind_return"] = P("S.uw1").fresh("r")
    install_rules(g, os.path.dirname(__file__), "scope", bindings=scope_bindings,
                  sequences=scope_sequences, section="unwind-warnings" if warnings else "unwind")

    local_control("local0", warnings)
    shape_control("local-type")
    local_control("local2", warnings)
    # Same parameter parser, no frame or parameter bindings for a prototype.
    function_control("function9", warnings)
    local_control("local4", warnings)
    shape_control("local-binding")
    local_control("local6", warnings)
    install_rules(g, os.path.dirname(__file__), "local-declarators",
                  classes=dict(identifier=[TK_ID], paren=[TK["("]]),
                  sequences=dict(reject=E.rej("not covered: declarator")), section="main")
    local_control("local8", warnings)
    return_control("ret0")
    return_control("ret1", dict(addr_end=addr(P("S.rs3")).cur))
    segment("if-loops", warnings)
    return_control("expr0")
    for name,op in (("inc","+"),("dec","-")):
        return_control("update", dict(update_entry="LP."+name,update_target="POST."+op))
    return_control("qt0")
    import assemble   # K2 trace translation: conditional-manifest.tsv
    assemble.run(Path(__file__).parent / 'conditional-manifest.tsv', E, P,
                 dict(locations=locations, warnings=warnings, errors=errors), dict(enum_values=ENV, enum_defined=END_))
    return_control("qt1")
    for label,cv,base in (("double","d",DBL),("single","s",FLT)):
        return_control("floating", dict(float_entry="QT."+label,float_convert="TO."+cv,result_base=base))
    return_control("qt2")
    current="QT.scalar"
    for _,code,*_ in TYINT:
        b=return_control("integer", dict(integer_current=current,integer_code=code))
        current=b["f92"]
    return_control("qt3", dict(integer_end=current))
    b=update_control("id0")
    update_control("id1",dict(address_end=addr(P(b["f3"])).cur))
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
    update_control("step-entry")
    pointer_ops={row[0] for row in tape_rows("update-pointer.tsv")}
    for op in E.CASOPS:
        b=update_control("compound0",dict(op=op))
        b=update_control("compound1",dict(b,address_end=addr(P(b["f6"])).cur))
        body=b["f7"]
        if op not in pointer_ops:
            b=update_control("compound-check",b)
            body="X.c"+op+".i"
        b=update_control("compound-body",dict(b,compound_body=body,compound_owner="LV" if op in pointer_ops else "X"))
        install_rules(g,os.path.dirname(__file__),"functiontypes",section="compound",
                      bindings=dict(wide="BC.wide"+op,test="BC.widetest"+op,narrow="BC.narrow"+op,store="BC.store"+op))
        update_control("compound-tail",b)
    update_control("taxonomy")
    current="TAX.0"
    for code,name in ((1,"i8"),(2,"i16"),(4,"i32"),(8,"i64"),(UNS+1,"u8"),(UNS+2,"u16"),(UNS+4,"u32"),(UNS+8,"u64"),
                      (BOOL,"u8"),(0,"void"),(DBL,"f64"),(FLT,"f32"),(FPB,"ptr"),(FPV,"ptr")):
        b=update_control("type-row",dict(type_current=current,type_code=code,type_axis=AX.index(name)))
        current=b["f28"]
    update_control("type-tail",dict(type_end=current))
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
    update_control("step0")
    shape_control("pointee-width")
    update_control("step1")
    for name,op,postfix,prefixname,prefixfix,integer,floating in tape_rows("update-modes.tsv"):
        update_control("fp-test",dict(op=op))
        for suffix,bits in tape_rows("update-float.tsv"):
            update_control("fp-output",dict(op=op,suffix=suffix,bits=int(bits),floating=floating))
    for name,op,postfix,prefixname,prefixfix,integer,floating in tape_rows("update-modes.tsv"):
        b=update_control("post0",dict(name=name,op=op,postfix=postfix,integer=integer))
        b=update_control("post1",dict(b,address_end=addr(P(b["f44"])).cur))
        update_control("post-body",b)
    b=update_control("variable0")
    update_control("variable1",dict(address_end=addr(P(b["f81"])).cur))
    ladder("E", "UNARY")
    ladder("C", None)
    ordinary_control('string', warnings)
    # sizeof: a constant, `imm r0, N`; the operand emits nothing (measured). A type, a variable,
    # or a variable with subscripts (each drops one dimension); anything else is not covered
    segment("sizeof0")
    shape_control("sizeof-type")
    segment("sizeof1")
    shape_control("sizeof-object")
    segment("sizeof2")
    strwalk("SZ.lwalk", "SZ.lbyte", "SZ.lend")
    strwalk("CE.lwalk", "CE.lbyte", "CE.lend")   # sizeof("...") inside a constant expression
    segment("sizeof3")
    ordinary_control('dispatch', warnings)
    segment("address")
    object_address = addr(P("ADR.object"))
    install_template(g, os.path.dirname(__file__), "stage-edits",
                     dict(address=[dict(source=object_address.cur, target="ADR.object.next")]),
                     object_address.fresh, section="address")
    ordinary_control('deref', warnings)
    shape_control("dereference")
    ops = [dict(key=E.TK[op + '='], target='LV.c' + op) for op in E.CASOPS]
    install_template(g, os.path.dirname(__file__), "stage-edits", dict(op=ops),
                     P("UD.load.b").fresh, section="update")
    ordinary_control('id', warnings)
    for tag, op in (("inc", "+"), ("dec", "-")):
        update=dict(word_state=tag, update_entry="ID."+tag, update_deref="UD."+tag, update_post="POST."+op)
        bound=ordinary_control("update-address", warnings, update)
        q=addr(P(bound["ordinary_update_address3"]))
        ordinary_control("update-result", warnings, dict(update, resume=q.cur))
    ordinary_control('value', warnings)
    p = P("UD.gv")
    addr(p)
    ordinary_control('down', warnings, dict(resume=p.cur))
    # Prefix updates share the existing member/subscript address walk. The
    # address is evaluated once; its value kind then selects step/load/store.
    for name,op,postfix,prefixname,prefixfix,integer,floating in tape_rows("update-modes.tsv"):
        update_control("prefix",dict(name=prefixname,op=op,prefixfix=prefixfix))
    install_rules(g, os.path.dirname(__file__), "scalar-prefix", section="positive",
                  bindings=dict(INT=TYINFO["i32"][0]),
                  classes=dict(promote=[BOOL] + [code for _, code, size, _, _ in TYINT if size < TYINFO["i32"][0]],
                               arithmetic=[DBL, FLT] + [code for _, code, size, _, _ in TYINT if size >= TYINFO["i32"][0]]),
                  sequences=dict(reject=E.rej("not covered: unary + requires arithmetic operand")))
    import assemble
    unit_span = int(re.search(r"^#define MAXTOK ([0-9]+)\b", Path(E.ROOT, "src/front_pp.c").read_text(), re.M).group(1))
    assemble.run(Path(__file__).resolve().parent / 'unarycontrol-manifest.tsv', E, P, dict(warnings=warnings),
                 dict(warnings=warnings, ucx=dict(TIX=TIX, MAXTOK=unit_span),
                      ufacts=dict(DBL=DBL, FLT=FLT, BOOL=BOOL, UNS1=UNS+1, UNS3=UNS+3, UNS4=UNS+4, UNS8=UNS+8,
                                  U32M=U32M, ENV=ENV, END_=END_, FNSTR=FNSTR, TIX=TIX, MAXTOK=unit_span)))
    shape_control("value-load")
    ordinary_control('array', warnings)
    from membercontrol import install as member_control
    member_control(E, P, warnings, TEMPL,
                   dict(SBB=SBB, MEMBER_STRIDE=MEMBER_STRIDE, MOF=MOF, MSZ=MSZ, MPT=MPT, MBS=MBS,
                        MAR=MAR, BFW=BFW, BFO=BFO, BFS=BFS, SHAPE_IDS=SHAPE_IDS, SHAPE=SHAPE,
                        ARR=E.ARR, SSZ=SSZ, DBL=DBL, FLT=FLT, BOOL=BOOL,
                        TYINT=TYINT), shape_control)
    # statement `*E = e` / `*E ...;`: E's value is the address
    ordinary_control('star', warnings)
    update_control("fnvalue")
    lookup_entry=update_control("lookup-warn")["f112"] if warnings else "LOOKUP"
    update_control("lookup-body",dict(lookup_entry=lookup_entry))
    import assemble
    call_env = assemble.run(Path(__file__).resolve().parent / 'callcontrol-begin-manifest.tsv', E, P,
                            dict(warnings=warnings), dict(warnings=warnings))
    E.__dict__.setdefault("results", {})["fpcont"] = call_env["fpcont"]   # FS.CALLTYPE continuation (librarycallables)
    fpu = {row[1]: row[2] for row in E.gold("irsel") if row[0] == "fpu"}
    from truth import conversions as scalar_conversions
    scalar_conversions(E, P, DBL, FLT, UNS + 8, fpu)
    assemble.run(Path(__file__).resolve().parent / 'callcontrol-finish-manifest.tsv', E, P,
                 dict(warnings=warnings), dict(warnings=warnings, cb=call_env['cb']))
    import assemble
    _flags = dict(locations=locations, warnings=warnings, errors=errors)
    assemble.run(Path(__file__).parent / 'offsetofcontrol-manifest.tsv', E, P, _flags,
                 dict(SBB=SBB, MEMBER_STRIDE=MEMBER_STRIDE, MOF=MOF, MSZ=MSZ, MBS=MBS, MAR=MAR))
    assemble.run(Path(__file__).parent / 'setjmpcontrol-manifest.tsv', E, P, _flags)
    ud_install(E, P)
    start = "START"
    if locations:
        from tokenlocations import install as location_install
        start = location_install(E, P, TIX, "ER.token" if errors else "WU.token" if warnings else None)
        import assemble, tokenlocations as _tl
        assemble.run(Path(__file__).parent / 'diagnostics-manifest.tsv', E, P, {},
                     {k: getattr(_tl, k) for k in ('SPLICES', 'INCLUDE_LINE', 'INCLUDE_LINES', 'INCLUDE_NAME')})
    if warnings:
        assert locations
        _tokens = dict(TK=E.TK, TK_ID=E.TK_ID, TK_NUM=E.TK_NUM, TK_FNUM=E.TK_FNUM)
        assemble.run(Path(__file__).parent / 'returnwarnings-manifest.tsv', E, P, _flags, dict(_tokens, SBB=SBB))
        assemble.run(Path(__file__).parent / "intwarnings-manifest.tsv", E, P, {},
                     dict(TOKEN_POS=_tl.TOKEN_POS, DBL=DBL, FLT=FLT, FPB=FPB, SBB=SBB))
        assemble.run(Path(__file__).parent / 'unusedwarnings-manifest.tsv', E, P, _flags,
                     dict(_tokens, TIX=TIX, UNDO_SIZE=UNDO_SIZE))
        import tokenlocations as _tl
        assemble.run(Path(__file__).parent / 'formatwarnings-manifest.tsv', E, P, {},
                     dict(TOKEN_POS=_tl.TOKEN_POS, NAME_TOKEN=assemble.load_facts('unusedwarnings')['NAME_TOKEN'],
                          DBL=DBL, FLT=FLT, FPB=FPB, SBB=SBB))
    if errors:
        _err = assemble.run(Path(__file__).parent / 'errors-manifest.tsv', E, P, _flags, {})   # K2: errors
        E.__dict__.setdefault("results", {})["lm_nomain_msg"] = _err["lm_nomain_msg"]   # librarymodule's no-main message state
    from libraryexports import install as libraryexports_install
    start = libraryexports_install(E, P, {name: globals()[name] for name in
        ('FPS_FN','FPS_RD','FPS_RB','FPS_RSH','FPS_COUNT','FPS_PARAM','FPS_PSH','FPS_VAR',
         'SBB','FPB','FPV','FPS_FIRST','BOOL','DBL','FLT','ENUM_FIRST','GSZ','GUNIT',
         'SSZ','SAL','SMN','SMEM','MOF','MSZ','MPT','MBS','MAR','BFW','BFO','BFS')}, start, TYINT)
    assemble.run(Path(__file__).parent / 'layoutfacts-manifest.tsv', E, P, {}, dict(start=start, **{'b_' + name: globals()[name] for name in
        ('SBB','MBS','MPT','MAR','MOF','BFW','MSZ','BFO','BFS','SHAPE_IDS','SHAPE')}))   # K2: layoutfacts
    start = 'LF.start'
    assemble.run(Path(__file__).parent / 'valueranks-manifest.tsv', E, P, _flags, dict())   # K2: valueranks
    from layoutprovenance import parser as source_provenance
    start = source_provenance(E, P, start)
    assemble.run(Path(__file__).parent / 'parenfold-manifest.tsv', E, P, _flags, dict(ordinal_table=TIX))   # K2: parenfold
    assemble.run(Path(__file__).parent / 'unitmode-manifest.tsv', E, P, _flags,   # K2: unitmode
                 dict(start=start, FND=E.FND, TK_extern=E.TK['type=extern'], TK_assign=E.TK['='],
                      header_json=json.dumps([list(a) for a in E.O(E.HEADER)]), lm_initret=E.results['lm_initret']))
    start = 'UM.start'
    assemble.run(Path(__file__).parent / 'objectdefinitions-manifest.tsv', E, P, _flags, dict(TKEQ=E.TK["="]))
    import assemble
    assemble.run(Path(__file__).parent / 'structreturnexpr-manifest.tsv', E, P,
                 dict(locations=locations, warnings=warnings, errors=errors))
    import assemble   # K2 trace translation: forward-manifest.tsv
    from types import SimpleNamespace
    start = assemble.run(Path(__file__).resolve().parent / 'forward-manifest.tsv', E, P, dict(locations=locations, warnings=warnings, errors=errors), dict(fps_fn=FPS_FN, start=start))['ret']
    # The compound-literal output splice walks a saved byte blob.  Its
    # EOF branch observes the reader byte, not the previous arithmetic result.
    mode, row = g.st['CP.restore']
    assert mode == 'r' and len(row) == 257
    install_template(g, os.path.dirname(__file__), "stage-edits", {},
                     P("CP.restore").fresh, section="final")
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
