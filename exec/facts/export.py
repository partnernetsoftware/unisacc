#!/usr/bin/env python3
"""Export domain facts to exec/facts/*.tsv (K2; format in exec/assemble.py).

TABLES declares (fact stem, input files, producer).  A producer returns a list
of lines in the facts format; the written file starts with one
`# input PATH sha256:PREFIX` line per input, like weights/gold/*.tsv.

  python3 exec/facts/export.py            write every declared table
  python3 exec/facts/export.py --check    exit 1 when a committed table differs
"""
import hashlib
import json
import importlib.util
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent


# _autonames went with k2-gen2 to tools/facts-gen.cx on 2026-10-30.


def _module(rel, name):
    spec = importlib.util.spec_from_file_location(name, ROOT / rel)
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m


_SKIP = ("type=const", "type=volatile")   # the token reader's default skipped qualifiers (was parse/gen.py tokenizer)


def _gold(name):   # weights/gold/NAME.tsv rows, `#` comments and `=>` rows dropped (was exec/parse/gen.py gold)
    rows = []
    for ln in open(ROOT / "weights" / "gold" / (name + ".tsv"), encoding="utf-8"):
        if ln.startswith("#"):
            continue
        f = ln.rstrip("\n").split("\t")
        if "=>" in f:
            continue
        rows.append(f)
    return rows


# parse-tokens moved to tools/facts-gen.cx on 2026-10-30.


def _parse_e(tag):
    """exec/build/parsebase.py plus the gold-derived tables export.py reads (was exec/parse/gen.py)."""
    E = _module("exec/build/parsebase.py", tag)
    E.gold = _gold
    E.PREC = {f[0]: int(f[1]) for f in _gold("prec") if f[1].isdigit()}
    E.BINSEL = {(f[0], f[1]): f[2] for f in _gold("binsel") if f[1] in ("s", "u")}
    E.IRSEL = {f[1]: f[2] for f in _gold("irsel") if f[0] == "alu"}

    def optext(op, u=False):
        sp = E.IRSEL[E.BINSEL[(op, "u" if u else "s")]]
        rev = sp.endswith("_rev")
        sp = sp[:-4] if rev else sp
        if sp in ("div", "mod", "udiv", "umod"):
            sp = "." + sp
        return "  %s r0, %s\n" % (sp, "r0, r1" if rev else "r1, r0")
    E.optext = optext
    E.TY = {f[0]: (int(f[1]), int(f[2])) for f in _gold("tyinfo") if len(f) >= 3 and f[1].isdigit()}
    E.SZ = {c: E.TY[k][0] for c, k in E.CTY.items()}
    E.PSZ = E.TY["ptr"][0]
    assert E.SZ == {"char": 1, "short": 2, "int": 4, "long": 8} and E.PSZ == 8, (E.SZ, E.PSZ)   # measured widths
    assert not any(E.TY[k][1] for k in E.CTY.values())
    return E


def _gen2ns(tag):
    """The parse2 domain constants and helpers export.py derives facts from (was the module level of
    exec/parse2/gen2.py (deleted); E = exec/parse/gen.py, P = parse2base.install(E), paths under exec/parse2)."""
    from types import SimpleNamespace
    import json
    import sys as _s, pathlib as _p
    _s.path.insert(0, str(ROOT / "exec/facts"))
    from load import facts as _facts
    _LX = {r["name"]: r["value"] for r in _facts("libraryexports")}
    _VR = {r["name"]: r["value"] for r in _facts("valueranks") if r["kind"] == "bank"}   # exec/facts/valueranks.tsv


    TYPERANK, MEMBERRANK, RETURNRANK, PARAMRANK = (_LX[k] for k in ("TYPERANK", "MEMBERRANK", "RETURNRANK", "PARAMRANK"))
    import os
    import re
    import sys
    from pathlib import Path

    import importlib.util
    E = _parse_e(tag + "_e3gen")

    # Intrinsic names come from the product declaration, not a second hand list.
    sys.path.insert(0, E.ROOT)
    from unisa.front.parse import INTRINSIC, INTRINSIC6
    SYSCALLS = ([(name, op, 3) for name, op in INTRINSIC.items()]
                + [(name, op, 6) for name, op in INTRINSIC6.items()]
                + [("__hostcall", "hostcall", 2)]
                + [("__hostaddr%d" % i, "hostaddr%d" % i, 0) for i in range(4)])

    O, TK, TK_ID, TK_NUM, LOC = E.O, E.TK, E.TK_ID, E.TK_NUM, E.LOC
    VLSIZE, VLFRAME, VLDEP = 52 << 40, 53 << 40, 54 << 40
    REGISTERBANK = _VR["REGISTERBANK"]
    UNDO_SIZE = 44
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
    sys.path.insert(0, str(ROOT / "exec/build"))
    import parse2base as _base   # exec/build/parse2base.py: the parse2 executor (rank-slot P, DEFS, token additions)
    P = _base.install(E)
    DEFS = _base.DEFS
    g = E.g
    from finite_rules import install as install_rules, install_rows, install_template, load as load_rules

    # Tape text uses JSON string escaping; PUSH/POP1 retain their shared E bindings.
    # {name} prints W[name] in decimal; declared spans print input slices.
    def tape_rows(filename):
        with open(str(ROOT / "exec/parse2" / filename), encoding="utf-8") as source:
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
        env = assemble.run((ROOT / "exec/parse2") / 'addr-manifest.tsv', E, P, {}, dict(entry=p.cur, pending=p.acts))
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


    def strwalk(pre, body, done):
        import assemble
        assemble.run((ROOT / "exec/parse2") / 'strwalk-manifest.tsv', E, P, {}, dict(pre=pre, body=body, done=done))


    sys.path.insert(0, str(ROOT / 'exec/facts')); from load import facts as _pffacts
    PFKINDS = {r['name']: r['value'] for r in _pffacts('printfallback')}['KINDS']   # printfallback-manifest.tsv (K2 trace translation)
    PFCONV = {ord(k): PFKINDS.index(v) for k,v in E.gold("pfconv") if k != "conv"}

    HEX = "0123456789abcdef"
    ESC = {"n": 10, "t": 9, "r": 13, "a": 7, "b": 8, "f": 12, "v": 11, "\\": 92, "'": 39, '"': 34, "?": 63, "0": 0}


    def fmtwalk(pre, on_byte, on_d, on_end):
        """Decoded format scanning: exec/parse2/fmtwalk-manifest.tsv (K2 sub-manifest)."""
        import assemble
        assemble.run((ROOT / "exec/parse2") / 'fmtwalk-manifest.tsv', E, P, {},
                     dict(pre=pre, on_byte=on_byte, on_d=on_d, on_end=on_end))
        return pre + '.w'


    def printf(warnings=False):
        """printf family: exec/parse2/printf-manifest.tsv (K2 sub-manifest)."""
        import assemble
        assemble.run((ROOT / "exec/parse2") / 'printf-manifest.tsv', E, P, dict(warnings=warnings), dict(warnings=warnings))


    UNS = E.UNS   # unsigned char/short/int/long: UNS + size
    SBB = E.SBB   # a struct's base code: SBB + sid; layouts in the old E3's tables (measured rules)
    STAG, SSZ = E.STAG, E.SSZ
    STRUCT_MAX, MEMBER_STRIDE = E.STRUCT_MAX, 256
    SIG_RADIX = E.SIG_RADIX
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
    GHL, GHE = 21 * POSSPAN, 22 * POSSPAN  # effective file linkage; declaration unit epoch + 1
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
    assert FPS_FIRST < SBB and SBB + STRUCT_MAX < SIG_RADIX
    assert 8 * SIG_RADIX + SBB + STRUCT_MAX < 2 ** 31  # parameter depth packing fits signed 32-bit


    return SimpleNamespace(**locals())


# _ppsrc (E2 source facts for pp-gen/pp-autoinc-gen) went with them to tools/facts-gen.cx on 2026-10-30.


# lex-gen moved to tools/facts-gen.cx on 2026-10-30.


def _esc(s):
    return s.replace("\\", "\\\\").replace("\t", "\\t").replace("\n", "\\n")


# k2-constexpr and structreturnexpr moved to tools/facts-gen.cx on 2026-10-30.


def _gold(name):
    lines = (ROOT / "weights" / "gold" / (name + ".tsv")).read_text().splitlines()
    return [x.split("\t") for x in lines if x and not x.startswith("#") and "=>" not in x]


def lowercode():
    """Lower code-route target facts: tape-word interning per os/arch (TAPE SHAPE words,
    tape registers, fixed spellings), the machine register / encoding form each word
    carries, and the side-table key bases (was exec/lower/code.py)."""
    import json
    sys.path.insert(0, str(ROOT))
    from unisa.tape import SHAPE
    out = ["=%s\tint\t%d" % (n, (105 + i) << 40)
           for i, n in enumerate(("OP", "KIND", "AC", "ARG", "TXT", "REG", "FORM", "ARGREG", "CCW"))]
    words = list(dict.fromkeys(list(SHAPE) + ["r" + str(i) for i in range(8)] + [
        "0", "1", "2", "3", "4", "8", "-8", "_start:", "write", "exit",
        ".hostcall", ".hostaddr", ".librarycall", ".libraryaddr"]
        # H3d: the cc-interop inbound label definitions `__ccw_N:` (front_parse.c ccw_emit, N < 256) carry a `mark`
        # (the CCW side table, set when they are interned): the scan has no prefix primitive, and an id range
        # is not enough -- source labels are interned between these words at run time
        + ["__ccw_" + str(i) + ":" for i in range(256)]))
    from unisa import lower as L
    for n in ("WIN_HSTD", "WIN_WRITTEN", "WIN_SAVE", "WIN_ARGVA", "SYSA", "SYSFP", "SYSSP"):
        out.append("=%s\tint\t%d" % (n, getattr(L, n)))
    out.append("=WIN_STACK_END\tint\t%d" % (L.WIN_EXTRA + L.WIN_STACK))
    out += ["=SYSA%d\tint\t%d" % (i, L.SYSA + 8 * i) for i in range(6)]
    out += ["=key_argc\tjson\t" + json.dumps("\0process/argc"), "=key_argv\tjson\t" + json.dumps("\0process/argv")]
    out.append("@words\ttarget:str\ti:int\tword:str\trslots:json\treg:json\tform:json\tmark:json")
    for os_ in ("lnx", "osx", "win"):
        for arch in ("x86_64", "arm64"):
            regmap = {r[0]: r[2] for r in _gold("regmap") if r[1] == arch}
            enc = {r[0]: r[3] for r in _gold("enc") if r[1:3] == [os_, arch]}
            for i, w in enumerate(words):
                out.append("\t%s/%s\t%d\t%s\t%s\t%s\t%s\t%s" % (
                    os_, arch, i, _esc(w), json.dumps([j for j, k in enumerate(SHAPE.get(w, ())) if k == "r"]),
                    json.dumps([regmap[w]] if w in regmap else []),
                    json.dumps([" form=" + enc[w]] if w in enc else []),
                    json.dumps([1] if w.startswith("__ccw_") else [])))
    # Per-target env of the code route (was exec/lower/code.py env prep): code-manifest.tsv
    # merges codeenv.<target> with bindmap.  Scratch registers are checked here, at export.
    from unisa.emit_x86 import SCR, SCR2
    from unisa.emit_arm import IP0, IP1
    from exec.facts.load import facts
    banks = {r["name"]: r["value"] for r in facts("top-modelbindings-banks")}
    stride = {r["name"]: r["value"] for r in facts("top-modelbindings-const")}["STRIDE"]
    env = {}
    for os_ in ("lnx", "osx", "win"):
        for arch in ("x86_64", "arm64"):
            regmap = {r[0]: r[2] for r in _gold("regmap") if r[1] == arch}
            enc = {r[0]: r[3] for r in _gold("enc") if r[1:3] == [os_, arch]}
            reloc = {r[0]: r[2] for r in _gold("reloc") if r[1] == arch}
            ids = {w: "idc" + str(i) for i, w in enumerate(words)}
            scratch = "x16" if arch == "arm64" else "r11"
            assert scratch not in set(regmap.values()), "callm scratch aliases tape register"
            s0, s1 = ("x" + str(IP0), "x" + str(IP1)) if regmap["r0"].startswith("x") else (SCR, SCR2)
            assert not {s0, s1} & set(regmap.values()), "process scratches alias tape values"
            e = dict(target=os_ + "/" + arch, arch=arch, ids=ids, process_flag="true" if os_ == "lnx" else "false",
                     idmapu={"id_" + n: v for n, v in ids.items()}, idmap={"id:" + n: v for n, v in ids.items()},
                     scratch=scratch, form_load64=enc["load64"], form_callr=enc["callr"],
                     s0=s0, s1=s1, hosted=True,
                     IDS=banks["IDS"], ADDRESS=banks["ADDRESS"], DESC=banks["DESC"], BKIND=banks["KIND"],
                     SUPPORTED=banks["SUPPORTED"], WRITABLE=banks["WRITABLE"], EXTENT=banks["EXTENT"],
                     STRIDE=stride)
            e.update({"reloc_" + k: v for k, v in reloc.items()})
            e.update({"reg_" + k: v for k, v in regmap.items()})
            env[os_ + "/" + arch] = e
    out.append("=codeenv\tjson\t" + json.dumps(env, sort_keys=True))
    return out



# lower-armfuse moved to tools/facts-gen.cx (facts: exec/facts/tape-shape.tsv) on 2026-10-30.


def lowerabi():
    """Lower syscall ABI per os/arch (domain data, no labels): gold abi rows (sysno, argument
    registers, return register, gate, number register, argument shape, return convention,
    Windows import), encoding form, Windows API name, and whether the target lowers the op
    (native syscall number, or a Windows API with an import)."""
    import json
    sys.path.insert(0, str(ROOT))
    from unisa.catalog import WINAPI
    out = ["@abiops\ttarget:str\top:str",
           ]
    rows = ["@abi\ttarget:str\top:str\tsysno:str\targs:json\tret:str\tgate:str\tnrreg:str\targshape:str\tretconv:str\twinimp:str\twinapi:str\tform:str\tselected:int"]
    for os_ in ("lnx", "osx", "win"):
        for arch in ("x86_64", "arm64"):
            t = os_ + "/" + arch
            abi = {r[0]: r[3:] for r in _gold("abi") if r[1:3] == [os_, arch]}
            enc = {r[0]: r[3] for r in _gold("enc") if r[1:3] == [os_, arch]}
            out += ["\t%s\t%s" % (t, _esc(o)) for o in dict.fromkeys([*abi, "exit_group"])]
            for o, f in abi.items():
                sel = f[0] != "none" or (os_ == "win" and WINAPI.get(o) is not None and f[12] != "none")
                rows.append("\t" + "\t".join([t, _esc(o), f[0], json.dumps(f[1:7]), f[7], f[8], f[9], f[10], f[11], f[12],
                                                 "none" if WINAPI.get(o) is None else WINAPI[o], enc.get(o, ""), str(int(sel))]))
    # (op, mode) pairs of the selected ops, flattened (decision A): argument sources per
    # exec/lower/code-abi-sources.tsv; mem arguments carry the immediates written before them.
    from unisa import lower as L
    src = [x.split("\t") for x in (ROOT / "exec/lower/code-abi-sources.tsv").read_text().splitlines()
           if x and not x.startswith("#")]
    sysa = {"SYSA%d" % i: L.SYSA + 8 * i for i in range(6)}
    sel = ["@abisel\ttarget:str\top:str"]
    om = ["@opmodes\ttarget:str\top:str\tmode:int\tprev:int\tfirst:json\tlast:json\tsyscall:json\thassysno:json\t"
          "sysnoint:str\tnrreg:str\tret:str\tgate:str\tform:str\tcarry:str\twinapi:str\tsysnoval:str\tretconv:str\t"
          "winimp:str\tmems:json\ttprev:json\ttrail:json"]
    val = lambda v: "'" + v if v == "none" or v.startswith(tuple("0123456789")) else v
    for os_ in ("lnx", "osx", "win"):
        for arch in ("x86_64", "arm64"):
            t = os_ + "/" + arch
            abi = {r[0]: r[3:] for r in _gold("abi") if r[1:3] == [os_, arch]}
            enc = {r[0]: r[3] for r in _gold("enc") if r[1:3] == [os_, arch]}
            for o, f in abi.items():
                if not (f[0] != "none" or (os_ == "win" and WINAPI.get(o) is not None and f[12] != "none")):
                    continue
                sel.append("\t%s\t%s" % (t, _esc(o)))
                for mode in range(5):
                    sm = 5 if o == "syscall" and mode == 1 else mode
                    sources = [(k, sysa[v] if v in sysa else int(v)) for m, sh, k, v in src if int(m) == sm and sh in ("*", f[10])]
                    if not sources:
                        raise ValueError("new Linux " + arch + " argument shape requires migration: " + f[10])
                    mems, pend, prev = [], [], []
                    for i, (k, v) in enumerate(sources):
                        if f[1 + i] == "none":
                            if os_ == "win":
                                break
                            raise ValueError("unsupported stack syscall argument")
                        a = dict(idx=i, kind=k, value=v, reg=f[1 + i])
                        if mode == 1 and os_ != "win":
                            a["stackoffset"] = v - L.SYSA
                        if k == "imm":
                            pend.append(a)
                        else:
                            mems.append(dict(a, imms=pend, prev=prev))
                            pend, prev = [], [dict(idx=i)]
                    om.append("\t" + "\t".join(_esc(x) for x in [
                        t, o, str(mode), str(mode - 1), json.dumps([1] if mode == 0 else []), json.dumps([1] if mode == 4 else []),
                        json.dumps([1] if o == "syscall" else []), json.dumps([1] if f[0] != "none" and o != "syscall" else []),
                        str(int(f[0], 0)) if f[0] != "none" else "", f[9], f[7], f[8], enc.get(o, ""),
                        "true" if os_ == "osx" else "false", "none" if WINAPI.get(o) is None else WINAPI[o],
                        val(f[0]), val(f[11]), val(f[12]), json.dumps(mems),
                        json.dumps(prev), json.dumps(pend)]))
    private_modes = [om[0].replace("@opmodes", "@private_modes", 1)] + [
        row for row in om[1:] if row.split("\t")[3] == "1" and not row.split("\t")[1].startswith("win/")]
    return out + rows + sel + om + private_modes + ["=none\tjson\t[]", "=zero\tint\t0", "=eight\tint\t8"]


# opt-gen (E4 START data and peep indices) moved to tools/facts-gen.cx on 2026-10-30.


# top-modelbindings-template, parse-tokens and k2-tokenlocations-map moved to tools/facts-gen.cx on 2026-10-30.


# nativeabi moved to tools/facts-gen.cx (trie choice order recorded in exec/nativeabi/trie-order.tsv) on 2026-10-30.


# pp-autoinc-gen moved to tools/facts-gen.cx (closure from seed/unisa_libneed.c) on 2026-10-30.


# pp-gen moved to tools/facts-gen.cx (escapes: exec/facts/pp-escapes.tsv) on 2026-10-30.


# (fact stem, inputs whose sha prefixes head the file, producer)
# k2-gen2 moved to tools/facts-gen.cx (literals: exec/facts/gen2-constants.tsv, gen2-literals.tsv) on 2026-10-30.


# k2-gen2-tokens moved to tools/facts-gen.cx on 2026-10-30.


# k2-units-tokens moved to tools/facts-gen.cx on 2026-10-30.


TABLES = [
    ("lower-abi", ["unisa/catalog.py", "unisa/lower.py", "exec/lower/code-abi-sources.tsv", "weights/gold/abi.tsv", "weights/gold/enc.tsv", "exec/facts/export.py"], lowerabi),
    ("lower-code", ["unisa/tape.py", "unisa/lower.py", "weights/gold/regmap.tsv", "weights/gold/enc.tsv", "weights/gold/reloc.tsv", "unisa/emit_x86.py", "unisa/emit_arm.py", "exec/facts/top-modelbindings-banks.tsv", "exec/facts/top-modelbindings-const.tsv", "exec/facts/export.py"], lowercode),
]


def render(stem, inputs, producer):
    head = ["# exec/facts/%s.tsv: written by exec/facts/export.py, do not edit" % stem]
    head += ["# input %s sha256:%s" % (p, hashlib.sha256((ROOT / p).read_bytes()).hexdigest()[:16])
             for p in inputs]
    return "\n".join(head + producer()) + "\n"


def main(argv):
    if argv not in ([], ["--check"]):
        print("usage: python3 exec/facts/export.py [--check]", file=sys.stderr)
        return 2
    check = "--check" in argv
    bad = 0
    for stem, inputs, producer in TABLES:
        path = HERE / (stem + ".tsv")
        text = render(stem, inputs, producer)
        if check:
            try:
                same = path.read_text() == text
            except FileNotFoundError:
                same = False
            if not same:
                bad += 1
                print("facts differ: " + str(path.relative_to(ROOT)))
        else:
            path.write_text(text)
    if check:
        bad += _bank_owners()
    print("facts: %d tables, %d differ" % (len(TABLES), bad) if check else "facts: wrote %d tables" % len(TABLES))
    return 1 if bad else 0


# Bank ownership (was runtime asserts in layoutfacts.py / libraryexports.py): an owner's bank
# list must not be reused by any other stage's constant table (`# source: literal constants
# formerly in ...` name/value tables, the stage constants).
BANK_OWNERS = (("layoutfacts", "RESERVED_BANKS"), ("libraryexports", "RANK_BANKS"))


def _bank_owners():
    import json
    consts = {}
    for path in sorted(HERE.glob("*.tsv")):
        lines = path.read_text().split("\n")
        if len(lines) < 2 or not lines[1].startswith("# source: literal constants formerly in"):
            continue
        consts[path.stem] = {}
        for ln in lines[2:]:
            f = ln.split("\t")
            if len(f) == 2 and not ln.startswith("#"):
                consts[path.stem][f[0]] = json.loads(f[1]) if f[1][:1] in "[-0123456789" else f[1]
    bad = 0
    for owner, name in BANK_OWNERS:
        owned = set(consts[owner][name])
        for stem, d in consts.items():
            hit = sorted(k for k, v in d.items() if stem != owner and type(v) is int and v in owned)
            if hit:
                bad += 1
                print("bank owned by %s.%s reused in %s: %s" % (owner, name, stem, ", ".join(hit)))
    return bad


def _fieldchain():
    """Field-chain recorder for image headers: domain items only.  Each field row is
    (width, value int|register name, lead = the literal-byte / computation item
    names that precede it); `tail` is the lead after the last field."""
    class C:
        def __init__(self):
            self.fields, self.lead = [], []
        def lit(self, bs):
            self.lead += ["byte%d" % b for b in bs]
        def comp(self, name):
            self.lead.append(name)
        def field(self, w, v, endian="little"):
            self.fields.append(dict(width=w, value=v, src="const" if isinstance(v, int) else "reg",
                                  endian=endian, lead=self.lead))
            self.lead = []
    return C()


def _rows(name, rows):
    import json
    out = ["@%s\ti:int\twidth:int\tsrc:str\tvalue:json\tendian:str\tlead:json" % name]
    for i, r in enumerate(rows):
        out.append("\t%d\t%d\t%s\t%s\t%s\t%s" % (i, r["width"], r["src"], json.dumps(r["value"]), r["endian"],
                                                json.dumps(r["lead"], separators=(",", ":"))))
    return out


# enc-pe and enc-pe-forward moved to tools/facts-gen.cx (facts: exec/facts/image-pe.tsv) on 2026-10-30.


# enc-macho and enc-elf moved to tools/facts-gen.cx (facts: exec/facts/image-macho.tsv, image-elf.tsv) on 2026-10-30.


def x86entry():
    """x86_64 encoder entry (was exec/enc/gen.py): table bases and class ids (local
    constants of the encoder), ENCSPEC alu2/setcc/shiftext opcodes, emit_x86.NUM
    register numbers, the words START interns, Win32 import names."""
    import json
    sys.path.insert(0, str(ROOT))
    from unisa.catalog import ENCSPEC, REGMAP
    from unisa.emit_x86 import NUM
    from unisa.image.pe import IMPORTS
    from exec.facts.load import facts as F
    X86 = ENCSPEC["x86_64"]
    const = dict(OPC=70, REGN=71, AOPC=72, ACC=73, LABD=74, KND=75, BLB=76, SZ=77, TGT=78, BRG=79, SHT=80,
                 OFF=81, FIT=82, SHX=83, MSN=84)
    const = {k: v * 10 ** 6 for k, v in const.items()}
    cn = ("C_MOV C_IMM C_ALU C_MUL C_LD8 C_ST8 C_LD C_ST C_SET C_RET C_SHF C_CALLR C_PUSH C_POP C_NOP C_FRAME C_ZERO "
          "C_SETREG C_SPINIT C_DIV C_MOD C_UDIV C_UMOD C_GATE C_LEA C_SETMEM C_ARGSAVE C_ARGVGET C_ITOA").split()
    C = {n: i for i, n in enumerate(cn, 1)}
    C.update(C_HOSTCALL=60, C_HOSTADDR=61)
    const.update(C, SCR=11, SCR2=3, SPREG=NUM[REGMAP["x86_64"][7]], RAX=NUM["rax"], RDX=NUM["rdx"])
    out = ["=%s\tint\t%d" % kv for kv in const.items()]
    out.append("=division_registers\tjson\t%s" % json.dumps([NUM[r] for r in REGMAP["x86_64"][:7]]))
    classes = {".div": C["C_DIV"], ".mod": C["C_MOD"], ".udiv": C["C_UDIV"], ".umod": C["C_UMOD"], "setreg": C["C_SETREG"],
               "spinit": C["C_SPINIT"], ".zero": C["C_ZERO"], "push": C["C_PUSH"], "pop": C["C_POP"], "nop": C["C_NOP"],
               ".frame": C["C_FRAME"], "callr": C["C_CALLR"], "mov": C["C_MOV"], "imm": C["C_IMM"], "mul64": C["C_MUL"],
               "load64": C["C_LD8"], "store64": C["C_ST8"], ".ld": C["C_LD"], ".st": C["C_ST"], "ret": C["C_RET"]}
    classes.update({r["op"]: r["id"] for r in F("enc-fp-ops")})
    classes.update(hostcall=60, hostaddr=61)
    classes.update({r["name"]: r["value"] for r in F("enc-x86win-ids")})
    classes.update({"itoa": C["C_ITOA"], "gate": C["C_GATE"], ".lea": C["C_LEA"], "setmem": C["C_SETMEM"],
                    "argsave": C["C_ARGSAVE"], "argvget": C["C_ARGVGET"]})
    for op in X86["alu2"]:
        classes[op] = C["C_ALU"]
    for op in X86["setcc"]:
        classes[op] = C["C_SET"]
    for op in X86["shiftext"]:
        classes[op] = C["C_SHF"]
    out.append("=unknown\tjson\t%s" % json.dumps(sorted(set(range(257)) - set(classes.values()))))
    out.append("@wi\tword:str")
    out += ["\t" + k for k in tuple(F("enc-x86win-ops")) + tuple(F("enc-x86win-rcs"))]
    out.append("@wimp\tword:str\tn:int")
    out += ["\t%s\t%d" % (name, i + 1) for i, name in enumerate(IMPORTS)]
    out.append("@classes\tword:str\tcls:int\taopc:json\tacc:json\tshx:json")
    for op, c in classes.items():
        out.append("\t%s\t%d\t%s\t%s\t%s" % (op, c, *(json.dumps([X86[t][op]] if op in X86[t] else [])
                                                      for t in ("alu2", "setcc", "shiftext"))))
    out.append("@regs\tword:str\tn:int")
    out += ["\t%s\t%d" % (nm, n + 1) for nm, n in NUM.items()]
    meta = tuple(F("enc-tins-meta"))
    ids = [("imm", "tagimm"), ("reg", "tagreg"), ("role", "role"), ("form", "form"), ("reloc", "reloc"), ("rel32", "rel32")]
    ids += [(w, w) for w in ("true", "false", "winapi", "carry",
                             *[k for k in meta if k not in ("role", "form", "reloc", "carry", "winapi")])]
    ids += [("mem", "tagmem"), ("addr", "tagaddr"), ("lnx/x86_64", "target1"), ("osx/x86_64", "target2"),
            ("win/x86_64", "target3")]
    ids += [("@" + k, "h_" + k) for k in ("target", "data", "sym", "src_os", "data_len", "bss", "threads", "relocs", "argc", "argv")]
    out.append("@ids\tword:str\tnm:str")
    out += ["\t%s\t%s" % w for w in ids]
    win = tuple(F("enc-x86win-meta"))
    out.append("@metakeys\tkey:str\twin:int")
    out += ["\t%s\t%d" % (k, k in win) for k in meta if k not in ("role", "form", "reloc", "carry")]
    out.append("@winmeta\tk:str")
    out += ["\t" + k for k in win]
    out.append("@puts\ti:int")
    out += ["\t%d" % i for i in range(4)]
    return out


TABLES.append(("enc-x86", ["unisa/catalog.py", "unisa/emit_x86.py", "unisa/image/pe.py", "exec/facts/enc-fp-ops.tsv",
                           "exec/facts/enc-x86win-ids.tsv", "exec/facts/enc-x86win-ops.tsv", "exec/facts/enc-x86win-rcs.tsv",
                           "exec/facts/enc-x86win-meta.tsv", "exec/facts/enc-tins-meta.tsv", "exec/facts/export.py"], x86entry))

def armentry():
    """ARM64 encoder entry (was exec/enc/arm.py): table bases, op specs (shape string ->
    operand checks), ENCSPEC alu3/invcond values, register names, START words."""
    import json
    sys.path.insert(0, str(ROOT))
    from unisa.catalog import ENCSPEC
    from unisa.image.pe import IMPORTS
    from exec.facts.load import facts as F
    A = ENCSPEC["arm64"]
    specs = {'mov': ('rr', 1), 'imm': ('ri', 2), 'mul64': ('rrr', 3), 'ret': ('', 4), 'nop': ('', 5), 'callr': ('r', 6),
             'hostcall': ('rr', 60), 'hostaddr': ('ri', 61), 'load64': ('rri', 9), 'store64': ('rir', 10),
             '.ld': ('rrii', 11), '.st': ('riri', 12), 'jump': ('l', 13), 'jumpz': ('rl', 14), 'call': ('l', 15),
             'setreg': ('rv', 25), 'spinit': ('r', 26), 'gate': ('', 27), '.lea': ('rl', 28), 'setmem': ('ir', 29),
             'argsave': ('iib', 30), 'argvget': ('rri', 31), '.zero': ('rii', 32), 'winsave': ('i', 33),
             'winrest': ('ir', 34), 'winstdh': ('i', 35), 'winargs': ('iii', 36), 'itoa': ('iii', 37),
             'hentry': ('r', 62), 'hleave': ('', 63)}
    specs.update({r['op']: (r['shape'], r['cls']) for r in F('enc-armint-specs')})
    specs.update({r['op']: (r['shape'], r['cls']) for r in F('enc-armfp-specs')})
    specs.update({k: ('rrr', 7) for k in A['alu3']})
    specs.update({k: ('rrr', 8) for k in A['invcond']})
    out = ["=OP\tint\t70000000", "=REG\tint\t71000000", "=BASE\tint\t72000000", "=LP\tint\t73000000", "=ZERO\tint\t0"]
    out.append("=unknown\tjson\t%s" % json.dumps(sorted(set(range(257)) - set(range(1, len(specs) + 1)))))
    meta = tuple(F("enc-tins-meta"))
    out.append("=meta_classes\tjson\t%s" % json.dumps({"meta_" + k: [i] for i, k in enumerate(meta, 1)}))
    codes = {'r': 1, 'i': 2, 'l': 3, 'b': 4}
    out.append("@specs\ti:int\top:str\tn:int\tcls:int\tlpos:int\tbase:int\tspin:int\tchars:json\tlast:int\tlastsigned:int")
    for i, (op, (shape, cls)) in enumerate(specs.items(), 1):
        chars = []
        for j, k in enumerate(shape):
            chars.append(dict(i=j, p=j - 1, first=int(j == 0), ps=int(j > 0 and chars[-1]['signed']),
                              isv=int(k == 'v'), code=codes.get(k, 0), signed=int(k == 'i' and op != 'imm')))
        out.append("\t%d\t%s\t%d\t%d\t%d\t%d\t%d\t%s\t%d\t%d" % (
            i, op, len(shape), cls, shape.find('l'), A['alu3'].get(op, A['invcond'].get(op, 0)), int(op == 'spinit'),
            json.dumps(chars, separators=(",", ":")), len(shape) - 1, int(bool(chars) and chars[-1]['signed'])))
    out.append("@regs\tword:str\tn:int")
    out += ["\tx%d\t%d" % (i, i + 1) for i in range(31)]
    out.append("@metakeys\tword:str\tn:int")
    out += ["\t%s\t%d" % (k, n) for n, k in enumerate(meta, 1)]
    out.append("@imports\tword:str\tn:int")
    out += ["\t%s\t%d" % (name, i + 1) for i, name in enumerate(IMPORTS)]
    for name, rows in (("inputkeys", F('enc-arminput-keys')), ("headers", F('enc-armlayout-headers')),
                       ("winkeys", F('enc-armwin-keys')), ("winreset", F('enc-armwin-reset'))):
        out.append("@%s\tword:str" % name)
        out += ["\t" + k for k in rows]
    out.append("@targets\tkey:str\tword:str")
    out += ["\t%s\t%s" % (r['key'], r['value']) for r in F('enc-armlayout-targets')]
    out.append("@puts\ti:int\tp:int")
    out += ["\t%d\t%d" % (i, i - 1) for i in range(4)]
    return out


TABLES.append(("enc-arm", ["unisa/catalog.py", "unisa/image/pe.py", "exec/facts/enc-armint-specs.tsv",
                           "exec/facts/enc-armfp-specs.tsv", "exec/facts/enc-tins-meta.tsv", "exec/facts/enc-arminput-keys.tsv",
                           "exec/facts/enc-armlayout-headers.tsv", "exec/facts/enc-armlayout-targets.tsv",
                           "exec/facts/enc-armwin-keys.tsv", "exec/facts/enc-armwin-reset.tsv", "exec/facts/export.py"], armentry))

# k2-strings and k2-unitlocations moved to tools/facts-gen.cx on 2026-10-30.


def k2membercontrol():
    """Member/assignment domain values; transitions and fresh labels remain in parse2 TSVs."""
    for d in ("exec", "exec/parse2"):
        if str(ROOT / d) not in sys.path:
            sys.path.insert(0, str(ROOT / d))
    G = _gen2ns("k2membercontrol_gen2")
    textrows = lambda name: [ln.split("\t", 1) for ln in (ROOT / ("exec/parse2/membercontrol-" + name + ".tsv")).read_text().splitlines()[1:]]
    consts = {n: getattr(G, n) for n in ("SBB", "MEMBER_STRIDE", "MOF", "MSZ", "MPT", "MBS", "MAR", "BFW", "BFO", "BFS", "SHAPE_IDS", "SHAPE", "SSZ", "DBL", "FLT", "BOOL")}
    consts["ARR"] = G.E.ARR
    tokens = dict(G.E.TK, identifier=G.E.TK_ID)
    classes = {name: [tokens[token]] for name, token in textrows("tokens")}
    sources = [(code, size, uns) for _, code, size, uns, _ in G.TYINT]
    sources += [(G.FLT, 4, 0), (G.DBL, 8, 0), (G.BOOL, 1, 0)]
    types = [dict(name=name, code=code, wide=[1] if size >= 8 else [],
                  narrow=[] if size >= 8 else [dict(values=sorted({v for v, width, signedness in sources
                                                               if width <= size and signedness == uns}))])
             for name, code, size, uns, _ in G.TYINT]
    operators = [ln.split("\t") for ln in (ROOT / "exec/parse2/membercontrol-operators.tsv").read_text().splitlines()[1:]]
    assert operators == [["part0", "f_part0_1061_MB_b_1", "MB.ld"],
                         ["part2", "f_part2_1090_PX_b_1", "PX.ld"]]
    source_fresh = (ROOT / "exec/parse2/membercontrol-fresh.tsv").read_text().splitlines()
    manifest_fresh = (ROOT / "exec/parse2/membercontrol-manifest-fresh.tsv").read_text().splitlines()
    expected = [source_fresh[0]]
    after_choice = False
    for row in source_fresh[1:]:
        cells = row.split("\t")
        if cells[0] == "part2":
            if cells[1] in ("plain", "warnings"):
                after_choice = True
                cells[0] = "part2.choice"
            else:
                cells[0] = "part2.tail" if after_choice else "part2.head"
        expected.append("\t".join(cells))
    assert manifest_fresh == expected
    casops = [dict(op=op, token=tokens[op + "="]) for op in G.E.CASOPS]
    templates = {name: G.TEMPL[json.loads(spec)[0]] for name, spec in textrows("template")}
    assert set(templates) == {"template0", "template1"}
    names = [name for source in ("text", "stack", "template") for name, _ in textrows(source)]
    compact = lambda value: json.dumps(value, separators=(",", ":"), ensure_ascii=True)
    return ["=consts\tjson\t" + compact(consts),
            "=classes\tjson\t" + compact(classes),
            "=types\tjson\t" + compact(types),
            "=casops\tjson\t" + compact(casops),
            "=seqnames\tjson\t" + compact(names),
            "=push_text\tjson\t" + compact(templates["template0"]),
            "=pop1_text\tjson\t" + compact(templates["template1"])]


TABLES.append(("k2-membercontrol", ["exec/build/parsebase.py", "exec/build/parse2base.py", "exec/facts/parse2-extra.tsv", "exec/parse2/membercontrol-template.tsv",
                                    "exec/parse2/membercontrol-tokens.tsv", "exec/parse2/membercontrol-operators.tsv",
                                    "exec/parse2/membercontrol-fresh.tsv", "exec/parse2/membercontrol-manifest-fresh.tsv",
                                    "exec/parse2/membercontrol-text.tsv", "exec/parse2/membercontrol-stack.tsv",
                                    "exec/parse2/tape-templates.tsv", "exec/facts/export.py"], k2membercontrol))


# k2-truth, k2-stack and k2-librarymodule-map moved to tools/facts-gen.cx on 2026-10-30.



def k2libraryenv():
    """parse2 library export chain environment (was gen2._libraryexports/librarymodule.install, Python-built):
    lx (libraryexports phases), types/imports/callables child envs, librarymodule header and no-main reject."""
    G = _gen2ns("k2libraryenv_gen2")
    E = G.E
    sys.path.insert(0, str(HERE)); from load import facts as F
    sys.path.insert(0, str(ROOT / "exec"))
    import assemble
    C = assemble.load_facts("k2-gen2")["buildconst"]
    b = {n: C[n] for n in ('FPS_FN','FPS_RD','FPS_RB','FPS_RSH','FPS_COUNT','FPS_PARAM','FPS_PSH','FPS_VAR',
         'SBB','FPB','FPV','FPS_FIRST','BOOL','DBL','FLT','ENUM_FIRST','GSZ','GUNIT',
         'SSZ','SAL','SMN','SMEM','MOF','MSZ','MPT','MBS','MAR','BFW','BFO','BFS')}
    integers = C["TYINT"]
    LX = {r["name"]: r["value"] for r in F("libraryexports")}
    VR = {r["name"]: r["value"] for r in F("valueranks") if r["kind"] == "bank"}
    lx = {k: v for k, v in LX.items() if type(v) is int and v >= 1 << 40}
    bints = {'b_' + k: v for k, v in b.items() if type(v) is int}
    L = lambda acts: [list(a) for a in acts]
    out_rows = [{'name': 'out ' + text.rstrip('\n'), 'kind': 'out', 'text': text}
                for text in ('USLSIG2\n', 'USLSIG3\n', 'USLTAPE1\n')]
    out_rows.append({'name': 'reject', 'kind': 'reject',
                     'reason': 'not covered: library signature resource or duplicate definition'})
    lxenv = dict(constants=dict(lx, **{k: b[k] for k in ('FPS_FN', 'FPS_COUNT', 'FPS_VAR', 'FPS_RD', 'FPS_RB', 'FPS_RSH')}),
                  out_names=[row['name'] for row in out_rows], out_rows=out_rows,
                  tk_static=E.TK['type=static'], allkeys=list(range(257)))
    types = dict(bints, isize=next(size for name, code, size, uns, narrow in integers if name == 'i32'),
                 ints=[{'code': code, 'size': size, 'uns': int(uns)} for _, code, size, uns, _ in integers], E_ARR=E.ARR, gen2_DIM=G.DIM)
    imports = dict(bints, TK_ID=E.TK_ID, TK_SEMI=E.TK[';'], FPB_FPV=[b['FPB'], b['FPV']], BOOL=[b['BOOL']],
                   ints2=[{'code': code, 'width': width, 'uns': uns} for _, code, width, uns, _ in integers])
    li = assemble.load_facts('libraryimports')['libraryimports!']
    U = {r['name']: r['value'] for r in F('unresolved')}
    lc = dict({r['name']: r['value'] for r in F('librarycallables') if type(r['value']) is int and r['value'] >= 1 << 40},
              RETURNRANK=LX['RETURNRANK'], PARAMRANK=LX['PARAMRANK'], LCSITERANK=VR['LCSITERANK'],
              **{k: li[k] for k in ('BYNAME', 'ADDRESS', 'FORMAT', 'SUPPORTED', 'TYPEDSIG', 'PLAN')},
              REQUESTS=assemble.load_facts('libraryvariadic')['REQUESTS'], DEFINED=U['DEFINED'], VARIADIC=LX['VARIADIC'],
              E_VAR=E.VAR, E_DBL=E.DBL, E_INT=E.SZ['int'], TK_SEMI=E.TK[';'],
              **{'b_' + k: b[k] for k in ('FPS_FN', 'FPS_COUNT', 'FPS_RB', 'FPS_RD', 'FPS_RSH', 'FPS_VAR', 'SSZ', 'SBB')})
    seqs = {}
    for line in (ROOT / 'exec/parse2/librarycallables-result.tsv').read_text().splitlines()[1:]:
        if line:
            for a in json.loads(line.split('\t')[4]):
                if a[0] == '@' and a[1].startswith('out:'):
                    seqs[a[1]] = a[1][4:]
    callables = dict(constants=lc, classes={'uns1': [E.UNS + 1], 'uns2': [E.UNS + 2], 'bool': [b['BOOL']], 'float': [b['FLT']]},
                     text_names=list(seqs), text_rows=[{'name': name, 'text': text} for name, text in seqs.items()])
    mainreason = assemble.load_facts('k2-librarymodule-map')['mainreason']
    dump = lambda x: json.dumps(x, separators=(",", ":"), sort_keys=True)
    return ["=lx\tjson\t" + dump(lxenv), "=types\tjson\t" + dump(types), "=imports\tjson\t" + dump(imports),
            "=callables\tjson\t" + dump(callables), "=lm_header_text\tjson\t" + dump(E.HEADER),
            "=nomain_reason\tjson\t" + dump(mainreason), "=ret_text\tjson\t" + dump('  ret\n')]


TABLES.append(("k2-libraryenv", ["exec/build/parsebase.py", "exec/build/parse2base.py", "exec/facts/parse2-extra.tsv", "exec/build/parsebase.py", "exec/parse2/librarycallables-result.tsv",
               "exec/facts/libraryexports.tsv", "exec/facts/librarycallables.tsv", "exec/facts/libraryimports.tsv",
               "exec/facts/libraryvariadic.tsv", "exec/facts/unresolved.tsv", "exec/facts/valueranks.tsv",
               "exec/facts/k2-gen2.tsv", "exec/facts/k2-librarymodule-map.tsv", "exec/facts/export.py"], k2libraryenv))

# signature-layout moved to tools/facts-gen.cx on 2026-10-30.

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
