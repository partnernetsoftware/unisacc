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
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "build"))
import parse2base as _base   # exec/build/parse2base.py: the parse2 executor (rank-slot P, DEFS, token additions)
P = _base.install(E)
DEFS = _base.DEFS
g = E.g
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


def width_dispatch(name, tape=None, masks=False):
    """Value width dispatch: exec/parse2/width-manifest.tsv (K2 sub-manifest; tape/masks are facts widthd.NAME)."""
    import assemble
    assemble.run(Path(__file__).resolve().parent / 'width-manifest.tsv', E, P, {}, dict(width_name=name, width_load=int(name == "LOADV")))


def shape_control(section):
    """Shape descriptors: exec/parse2/shape-manifest.tsv (K2 sub-manifest; membercontrol still calls this)."""
    import assemble
    assemble.run(Path(__file__).resolve().parent / 'shape-manifest.tsv', E, P, {}, dict(shape_section=section))


def structured_control(section, warnings, extra=None, sequence_bindings=None, export=False):
    """Structured control: exec/parse2/control-manifest.tsv (K2 sub-manifest)."""
    import assemble
    section += "-warnings" if warnings and section in ("block", "if") else ""
    return assemble.run(Path(__file__).resolve().parent / 'control-manifest.tsv', E, P, {},
                 dict(control_export=export, control_section=section, statement="STMT.body" if warnings else "STMT",
                      extra=dict(extra or {}), seqb=dict(sequence_bindings or {})))


def segment(name, warnings=False, env=None):
    """One transitional segment of exec/parse2/gen2-manifest.tsv (rows gated by env fact seg_NAME)."""
    import assemble
    return assemble.run(Path(__file__).resolve().parent / 'gen2-manifest.tsv', E, P, dict(warnings=warnings), dict(env or {}, **{"seg_" + name: 1}))


def ordinary_control(section, warnings, extra=None):
    """Ordinary control: gen2-manifest segment ord-SECTION (fresh lets, control calls); extra values arrive as env."""
    return segment("ord-" + section, warnings, extra)


def namespace_constants():
    """Layout constants the function/global/local control sections bind (facts k2-gen2 nsconst)."""
    f = dict(PIDS=PIDS, PDB=PDB, LOC=LOC, FND=E.FND, FRD=E.FRD, FRB=E.FRB, VAR=E.VAR)
    f.update(("FN_PDB" + str(i), PDB + i) for i in range(16))
    g = {name: globals()[name] for name in ("LOC", "GIBLOB", "GIEND", "GINPS", "GINPE", "GSZ", "GUNIT", "SINIT", "SKIPS")}
    g.update((name, getattr(E, name)) for name in ("FND", "GMARK", "BASE", "ARR", "PTR"))
    return dict(function=f, globals=g, local=dict(SKIPS=SKIPS, PTR=E.PTR, BASE=E.BASE))


def _namespace_control(namespace, section, warnings):
    """function/global/local control: gen2-manifest segment ns-NAMESPACE-SECTION (fresh lets + control calls)."""
    env = segment("ns-%s-%s" % (namespace, section), warnings)
    if namespace == "global":   # named results for later stages (librarymodule): global-results.tsv
        for part, key, name in tape_rows("global-results.tsv"):
            if part == section:
                E.__dict__.setdefault("results", {})[name] = env[key]


def function_control(section, warnings):
    _namespace_control("function", section, warnings)


def global_control(section, warnings):
    _namespace_control("global", section, warnings)


def local_control(section, warnings):
    _namespace_control("local", section, warnings)


def return_control(section, extra=None):
    """Return/expression control: exec/parse2/return-manifest.tsv (K2 sub-manifest); returns its bindings."""
    import assemble
    return assemble.run(Path(__file__).resolve().parent / 'return-manifest.tsv', E, P, {},
                        dict(ret_section=section, ret_dispatch=int(section == "expr0"), extra=dict(extra or {})))["rb"]


def update_control(section, extra=None):
    """Lvalue update control: exec/parse2/update-manifest.tsv (K2 sub-manifest); returns its bindings."""
    import assemble
    x = dict(extra or {})
    env = dict(upd_section=section, upd_dispatch=int(section == "id0"), extra=x, extra2={})
    env.update((k, str(x.get(k, ""))) for k in ("op", "name", "suffix", "postfix", "prefixfix", "integer", "floating", "bits"))
    return assemble.run(Path(__file__).resolve().parent / 'update-manifest.tsv', E, P, {}, env)["ub"]


def build(locations=False, warnings=False, errors=False):
    # Unit markers are emitted only by the model framing pass. Each scan's
    # first marker resets the epoch; single-unit token dumps keep epoch zero.
    _base.tokens(E)
    # The location/static readers replace NEXT later.  Keep the plain token
    # decoder for lookahead; ordinary qualifier recursion must still pass
    # through NEXT so each source token gets its ordinal.
    assert "TN.raw" not in g.st
    import assemble
    segment("startup-marker")   # + call strings-token-span, strings-initializer (gen2-manifest)
    segment("numeric")
    segment("types")
    segment("typing")
    from constexpr import install as const_install
    const_install(E, P, LEVELS, OPS, ENV, END_)
    segment("statics-init")   # + call strwalk IC.string (gen2-manifest)
    segment("staticauto")
    # ---- declared data 3: the grammar, compiled to procedures ---------------------------
    segment("startup-guard")   # POSSPAN from facts k2-gen2
    env = segment("startup-run")   # startup data from facts k2-gen2 (tyrows, syscalls, autonames)
    E.__dict__.setdefault("results", {}).update(lm_hstate=env["lm_hstate"], lm_hnext=env["lm_hnext"], lm_header=O(E.HEADER), lm_errors=errors)
    global_control("global0", warnings)
    # enum is a type specifier in both declarations and typedefs.
    global_control("global1", warnings)   # + call shape typedef-shape (gen2-manifest)
    global_control("global3", warnings)   # + call shape global-type (gen2-manifest)
    global_control("global5", warnings)   # + call shape global-binding (gen2-manifest)
    function_control("function0", warnings)   # + call shape parameter-type (gen2-manifest)
    function_control("function2", warnings)   # + call shape parameter-dimensions (gen2-manifest)
    function_control("function4", warnings)   # + call shape descriptor-storage (gen2-manifest)
    function_control("function6", warnings)
    segment("parameter-declarators")
    function_control("function8", warnings)
    # DECL: the identifier ps..pe becomes the next 8-byte slot (measured: params and int locals)
    # DECLN: the name was saved in ips..ipe (the current token is after it)
    scope = assemble.run(Path(__file__).parent / 'scope-manifest.tsv', E, P, dict(warnings=warnings), dict(scope_part1=1))['sb']
    # statements
    segment("dispatch-block", warnings)
    assemble.run(Path(__file__).parent / 'scope-manifest.tsv', E, P, dict(warnings=warnings), dict(scope_part2=1, extra=scope))

    local_control("local0", warnings)   # + call shape local-type (gen2-manifest)
    local_control("local2", warnings)
    # Same parameter parser, no frame or parameter bindings for a prototype.
    function_control("function9", warnings)
    local_control("local4", warnings)   # + call shape local-binding (gen2-manifest)
    local_control("local6", warnings)
    local_control("local8", warnings)
    segment("ret0")
    segment("if-loops", warnings)
    segment("ret-expr")
    segment("ret-qt0")
    segment("ret-qt")
    segment("upd-id")   # + call shape update-entry (gen2-manifest)
    segment("upd-compound")   # + calls folded into gen2-manifest
    segment("upd-type")   # + calls folded into gen2-manifest
    segment("upd-modes")
    segment("upd-var")
    segment("ladders")
    ordinary_control('string', warnings)
    # sizeof: a constant, `imm r0, N`; the operand emits nothing (measured). A type, a variable,
    # or a variable with subscripts (each drops one dimension); anything else is not covered
    segment("sizeof0")   # + calls folded into gen2-manifest
    segment("sizeof1")   # + calls folded into gen2-manifest
    segment("sizeof2")   # + calls folded into gen2-manifest
    segment("sizeof3")
    ordinary_control('dispatch', warnings)
    segment("address")
    ordinary_control('deref', warnings)   # + calls folded into gen2-manifest
    ordinary_control('id', warnings)
    segment("ord-update", warnings)
    ordinary_control('value', warnings)
    ordinary_control('down', warnings)
    # Prefix updates share the existing member/subscript address walk. The
    # address is evaluated once; its value kind then selects step/load/store.
    segment("upd-prefix", warnings)   # + calls folded into gen2-manifest
    ordinary_control('array', warnings)
    from membercontrol import install as member_control
    member_control(E, P, warnings, TEMPL,
                   dict(SBB=SBB, MEMBER_STRIDE=MEMBER_STRIDE, MOF=MOF, MSZ=MSZ, MPT=MPT, MBS=MBS,
                        MAR=MAR, BFW=BFW, BFO=BFO, BFS=BFS, SHAPE_IDS=SHAPE_IDS, SHAPE=SHAPE,
                        ARR=E.ARR, SSZ=SSZ, DBL=DBL, FLT=FLT, BOOL=BOOL,
                        TYINT=TYINT), shape_control)
    # statement `*E = e` / `*E ...;`: E's value is the address
    ordinary_control('star', warnings)
    segment("upd-lookup", warnings)
    import assemble
    call_env = assemble.run(Path(__file__).resolve().parent / 'callcontrol-begin-manifest.tsv', E, P,
                            dict(warnings=warnings), dict(warnings=warnings))
    E.__dict__.setdefault("results", {})["fpcont"] = call_env["fpcont"]   # FS.CALLTYPE continuation (librarycallables)
    fpu = {row[1]: row[2] for row in E.gold("irsel") if row[0] == "fpu"}
    import assemble
    assemble.run(Path(__file__).parent / "truth-conversions-manifest.tsv", E, P, {},
                 dict(DBL=DBL, FLT=FLT, UNSIGNED_WIDE=UNS + 8, **fpu))
    assemble.run(Path(__file__).resolve().parent / 'callcontrol-finish-manifest.tsv', E, P,
                 dict(warnings=warnings), dict(warnings=warnings, cb=call_env['cb']))
    import assemble
    _flags = dict(locations=locations, warnings=warnings, errors=errors)
    segment("offsetof")
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
        # librarymodule's no-main continuation: the errors message state that replaced the reject (named at creation,
        # errors-manifest.tsv lm_nomain_msg); the message state itself rejects, so the edge carries no actions.
        E.__dict__.setdefault("results", {}).update(lm_nomain_msg=_err["lm_nomain_msg"], lm_mainnext=_err["lm_nomain_msg"], lm_mainreject=[])
    from libraryexports import install as libraryexports_install
    start = libraryexports_install(E, P, {name: globals()[name] for name in
        ('FPS_FN','FPS_RD','FPS_RB','FPS_RSH','FPS_COUNT','FPS_PARAM','FPS_PSH','FPS_VAR',
         'SBB','FPB','FPV','FPS_FIRST','BOOL','DBL','FLT','ENUM_FIRST','GSZ','GUNIT',
         'SSZ','SAL','SMN','SMEM','MOF','MSZ','MPT','MBS','MAR','BFW','BFO','BFS')}, start, TYINT)
    assemble.run(Path(__file__).parent / 'layoutfacts-manifest.tsv', E, P, {}, dict(start=start, **{'b_' + name: globals()[name] for name in
        ('SBB','MBS','MPT','MAR','MOF','BFW','MSZ','BFO','BFS','SHAPE_IDS','SHAPE')}))   # K2: layoutfacts
    start = 'LF.start'
    assemble.run(Path(__file__).parent / 'valueranks-manifest.tsv', E, P, _flags, dict())   # K2: valueranks
    assemble.run(Path(__file__).parents[1] / 'layoutprovenance-manifest.tsv', E, P, {}, dict(start=start))   # K2: layoutprovenance parser
    start = 'SF3.start'
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
    assemble.run(Path(__file__).parent / 'gen2parts-manifest.tsv', E, P, {}, dict(part_final=1))
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
    twice = _base.twice()
    assert not twice, "defined twice: %r" % twice
    s = json.dumps(d, separators=(",", ":"))
    open(sys.argv[1], "w").write(s)
    st, ent, live, ns, na = E.sizes(d)
    sys.stderr.write("states %d  entries %d (not 'unreachable' %d)  action seqs %d (%d actions)  json %d B\n"
                     % (st, ent, live, ns, na, len(s)))
