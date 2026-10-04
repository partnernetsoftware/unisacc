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


def _autonames(E):
    """The reference's autoinc (src/front_pp.c): for each of these headers, a
    raw line opening `static` with `NAME(` and `{` on it names a function;
    if NAME is followed by `(` somewhere in the source and never by
    `( ... ) {`, the whole header is prepended -- tokens the dump does not
    show.  printf is the walker's own (exempt).  Read from include/, as the
    reference reads it."""
    import os
    out = []
    for h in E.AUTOINC_HEADERS:
        for ln in open(os.path.join(E.ROOT, "include", h), encoding="utf-8", errors="replace"):
            ln = ln.rstrip("\n")
            if len(ln) <= 7 or not ln.startswith("static") or "(" not in ln or "{" not in ln:
                continue
            b = ln[:ln.index("(")].rstrip(" ")
            a = len(b)
            while a > 0 and (b[a - 1].isalnum() or b[a - 1] == "_"):
                a -= 1
            if a < len(b) and b[a:] != "printf" and b[a:] not in out:
                out.append(b[a:])
    return out


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


def parsetokens():
    """parse token words and codes (was exec/parse/gen.py module level): words = pre + compound-assign
    (casops + "=") + sorted(gold prec operators) + post; TK = 1-based index, "type" = the "type=int" code."""
    import json
    fw = {}
    for ln in (ROOT / "exec/facts/parse-words.tsv").read_text().splitlines()[1:]:
        if ln and not ln.startswith("#"):
            k, v = ln.split("\t")
            fw.setdefault(k, []).append(json.loads(v))
    prec = {f[0]: int(f[1]) for f in _gold("prec") if f[1].isdigit()}
    words = fw["words_pre"] + [o + "=" for o in fw["casops"]] + sorted(prec) + fw["words_post"]
    tk = {w: k + 1 for k, w in enumerate(words)}
    tk["type"] = tk["type=int"]
    return ["=WORDS\tjson\t" + json.dumps(words, separators=(",", ":")),
            "=TK\tjson\t" + json.dumps(tk, separators=(",", ":"))]


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


    return SimpleNamespace(**locals())


def _ppsrc():
    """E2 source facts (was the module level of exec/pp/gen.py): layout names, byte classes, targets,
    the gold pp.tsv directive vocabulary, predefines.tsv, operators.tsv, autoinc trigger names."""
    import os, re
    from types import SimpleNamespace
    sys.path.insert(0, str(ROOT))
    from unisa.tsvgold import load_table
    from exec.facts.load import facts
    E = SimpleNamespace(**{r["name"]: r["value"] for r in facts("pp-layout")})
    E.TARGETS = tuple(r["os"] + "/" + r["arch"] for r in facts("pp-targets"))
    by = {r["class"]: {c for a, b in r["ranges"] for c in range(a, b + 1)} for r in facts("pp-bytes")}
    E.ID = by["alpha"] | by["digit"]
    name, fields, heads, rows = load_table(str(ROOT / "weights/gold/pp.tsv"))
    assert name == "pp" and [n for n, _ in fields] == ["dir", "defined"]
    assert fields[1][1] == ("0", "1") and [n for n, _, _ in heads] == ["y"]
    assert list(heads[0][1]) == ["take", "skip", "pop", "macro"]
    E.DIRV = fields[0][1]
    E.PPT = {(d, int(b)): labels["y"] for (d, b), labels in rows.items()}
    path = ROOT / "exec/pp/predefines.tsv"
    pre = {}
    for line, text in enumerate(path.read_text(encoding="utf-8").splitlines(True), 1):
        if text.startswith("#") or not text.strip():
            continue
        f = text.rstrip("\n").split("\t")
        key, names = tuple(f[:2]), f[2:]
        if (len(f) < 3 or key in pre or len(set(names)) != len(names) or
                any(not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", n) for n in names)):
            raise ValueError("%s:%d: invalid or duplicate predefinition row" % (path, line))
        pre[key] = names
    if set(pre) != {("os", x) for x in ("lnx", "osx", "win")} | {("arch", x) for x in ("x86_64", "arm64")} | {("common", "*")}:
        raise ValueError("%s: expected OS, architecture and common declarations" % path)
    E.PREDEF = pre
    E.XOPS = {}
    for line in (ROOT / "exec/pp/operators.tsv").read_text().splitlines():
        if not line or line.startswith("#"):
            continue
        code, spelling, precedence, arity = line.split("\t")
        code, precedence, arity = int(code), int(precedence), int(arity)
        assert code not in E.XOPS and 0 < code < 257 and precedence >= 0 and arity in (0, 1, 2, 3)
        assert spelling and spelling not in {v[0] for v in E.XOPS.values()}
        E.XOPS[code] = spelling, precedence, arity
    assert E.XOPS, "empty expression operator declarations"
    E.AUTOINC_ORDER = tuple(facts("pp-autoinc"))
    def autoinc_map():   # src/front_pp.c hdrneeded's line rule over include/*.h
        m = {}
        for h in E.AUTOINC_ORDER:
            names = []
            for ln in (ROOT / "include" / h).read_text(encoding="latin-1").split("\n"):
                if len(ln) > 7 and ln.startswith("static") and "(" in ln and "{" in ln[ln.index("("):]:
                    mm = re.search(r"([A-Za-z0-9_]+)\s*$", ln[:ln.index("(")])
                    if mm:
                        names.append(mm.group(1))
            m[h] = names
        return m
    E.autoinc_map = autoinc_map
    E.sbconst = lambda s: [("SBCLR",)] + [("SBOUT", c) for c in s.encode()]
    def xe_init():
        a = []
        for c, (_, p, _) in list(E.XOPS.items()) + [(0, (None, -1, 0))]:
            a += [("LDI", "xc", E.XPRB + c), ("LDI", "xq", p), ("STX", "xc", 0, "xq")]
        return a
    E.xe_init = xe_init
    return E


def lexgen():
    """E1 lexer facts (was the module level of exec/lex/gen.py): token schema (weights/gold/parse.tsv tok),
    type keywords, byte classes and GCC words (weights/gold/lexcls.tsv, lexword.tsv), lex.tsv dispatch
    rows as HANDLE/PEEKCLASS, word and punctuator tries, token names and spelling
    properties, template byte classes. Executable output/entry recipes live in lexer templates."""
    import json
    sys.path.insert(0, str(ROOT))
    sys.path.insert(0, str(ROOT / "exec"))
    from unisa.tsvgold import load_table
    from exec.facts.load import facts
    from finite_rules import prefix_facts
    here = ROOT / "exec" / "lex"
    G = ROOT / "weights" / "gold"
    C = {r["name"]: r["value"] for r in facts("lex-consts")}
    EOF = C["EOF"]
    rows = [l.split("\t")[2:] for l in (G / "parse.tsv").read_text().splitlines() if l.startswith("#field\ttok\t")]
    assert len(rows) == 1 and rows[0], "missing or repeated token schema"
    TOKS = tuple(rows[0])
    assert len(TOKS) == len(set(TOKS)) and all(TOKS), "invalid token schema"
    kw = [l.split("\t") for l in (ROOT / "iterate/kernel/typekw.tsv").read_text().splitlines() if l and not l.startswith("#")]
    assert kw and all(len(r) == 2 and r[0] == "kw" and r[1] for r in kw)
    TYPEKW = tuple(r[1] for r in kw)
    assert len(TYPEKW) == len(set(TYPEKW)), "duplicate type keyword"
    _, _, _, lcrows = load_table(str(G / "lexcls.tsv"))
    _, _, _, lwrows = load_table(str(G / "lexword.tsv"))
    CLS, WS = {}, []
    for (bv,), lab in lcrows.items():
        c = EOF if bv == "eof" else int(bv)
        CLS[c] = lab["c"]
        if lab["attws"] == "yes":
            WS.append(c)
    W = [(w, lab) for (w,), lab in lwrows.items()]
    SKIPPAREN = tuple(w for w, l in W if l["gcc"] == "skipparen")
    DROP = tuple(w for w, l in W if l["gcc"] == "drop")
    CHARPFX = tuple(w for w, l in W if l["pfxch"] == "yes")
    STRPFX = tuple(w for w, l in W if l["pfxstr"] == "yes")
    isal = lambda c: c != EOF and CLS[c] == "A"
    isdi = lambda c: c != EOF and CLS[c] == "d"
    PUNCTS = [t for t in TOKS if t and not isal(ord(t[0]))]
    head, LT = None, {}
    for ln in (G / "lex.tsv").read_text(encoding="utf-8").splitlines(True):
        f = ln.rstrip("\n").split("\t")
        if f[0] == "#head":
            head = f[3:]
            continue
        if ln.startswith("#") or len(f) < 3 or f[0] == "c":
            continue
        LT[(f[0], f[1])] = f[2]
    CLASSES = sorted(set(c for c, _ in LT))
    rowconst = {}
    for cl in CLASSES:
        vals = set(v for (a, b), v in LT.items() if a == cl)
        rowconst[cl] = vals.pop() if len(vals) == 1 else None

    def decl(path):
        r = [l.split("\t") for l in path.read_text().splitlines() if l and not l.startswith("#")]
        assert r and len({x[0] for x in r}) == len(r), path
        return r
    SPELLING = {}
    for token, plain, typed in decl(here / "spelling.tsv"):
        assert token in TOKS and plain in ("yes", "no") and typed in ("yes", "no")
        SPELLING[token] = (plain == "yes", typed == "yes")

    KID, KNUM, KSTR = C["KID"], C["KNUM"], C["KSTR"]
    ALLB = list(range(EOF + 1))
    KWKIND = {}
    for k, t in enumerate(TOKS):
        if isal(ord(t[0])):
            KWKIND[t] = KID if k < C["CLASSNAMES"] else k
    for t in TYPEKW:
        KWKIND[t] = C["TYPE"]
    WORDS = set(KWKIND) | set(SKIPPAREN) | set(DROP) | set(CHARPFX) | set(STRPFX)
    IDROOT, IDPFX = prefix_facts(sorted(WORDS))
    OPROOT, OPPFX = prefix_facts([t if t in PUNCTS else None for t in TOKS])
    opt = lambda present: [{}] if present else []
    IDFACTS = [dict(p, kind=KWKIND.get(p["name"], KID),
                    ifskip=opt(p["name"] in SKIPPAREN), ifdrop=opt(p["name"] in DROP),
                    iftoken=opt(p["name"] not in SKIPPAREN and p["name"] not in DROP),
                    ifpfxch=opt(p["name"] in CHARPFX), ifpfxstr=opt(p["name"] in STRPFX)) for p in IDPFX]
    assert set(head) == {"skip", "nl", "linecmt", "cmt", "ident", "num", "op", "str", "charlit", "bad"}

    def handle(state, cls, act, peek=()):
        return dict(state=state, cls=cls, act=act, peek=list(peek),
                    peekstate=[{"name": cls}] if peek else [],
                    **{name: opt(act == name) for name in head})
    BYTECLASSES = {cl: [c for c in ALLB if CLS[c] == cl] for cl in CLASSES}
    HANDLE = [handle("DISPATCH", cl, rowconst[cl]) for cl in CLASSES if rowconst[cl] is not None]
    PEEKCLASS = [{"name": cl} for cl in CLASSES if rowconst[cl] is None]
    for cl in CLASSES:
        if rowconst[cl] is None:
            cols = sorted(CLASSES, key=lambda col: BYTECLASSES[col][0])
            peek = [{"cls": cl, "col": col, "act": LT[(cl, col)]} for col in cols]
            acts = list(dict.fromkeys(r["act"] for r in peek))
            HANDLE += [handle("H:" + a, cl, a, peek if i == 0 else ()) for i, a in enumerate(acts)]

    tokens = [dict(kind=k, name=n, plain_spell=SPELLING.get(n, (False, False))[0],
                   typed_spell=SPELLING.get(n, (False, False))[1]) for k, n in enumerate(TOKS)]
    classes = dict(BYTECLASSES, identifier=[c for c in ALLB if c != EOF and (isal(c) or isdi(c))], space=list(WS))
    out = ["=HANDLE\tjson\t" + json.dumps(HANDLE), "=PEEKCLASS\tjson\t" + json.dumps(PEEKCLASS),
           "=IDROOT\tjson\t" + json.dumps(IDROOT["children"]),
           "=OPROOT\tjson\t" + json.dumps(OPROOT["children"]), "=IDPFX\tjson\t" + json.dumps(IDFACTS),
           "=OPPFX\tjson\t" + json.dumps(OPPFX), "=KID\tjson\t" + json.dumps([KID]),
           "=classes\tjson\t" + json.dumps(classes), "=literalclasses\tjson\t" + json.dumps({"space": list(WS)}),
           "=tok_names\tjson\t" + json.dumps(list(TOKS))]
    out.append("=tokens\tjson\t" + json.dumps(tokens))
    return out


def _esc(s):
    return s.replace("\\", "\\\\").replace("\t", "\\t").replace("\n", "\\n")


def constexprentry():
    """Integer-expression precedence and token classes for the E3 table."""
    import csv
    E = _parse_e("exec_parse_gen_constexpr_facts")
    levels = sorted(set(E.PREC.values()))
    path = ROOT / "exec/parse2/constexpr-operators.tsv"
    with path.open() as f:
        opinfo = {r[0]: r[1:] for r in list(csv.reader(f, delimiter="\t"))[1:]}
    level_rows = []
    for i, level in enumerate(levels):
        operators = []
        for name in sorted(o for o, v in E.PREC.items() if v == level):
            category, action, comparison, zero, nonzero = opinfo[name]
            operators.append(dict(k=E.TK[name], name=name, category=category, action=action,
                                  comparison=[] if comparison == "-" else [int(x) for x in comparison.split(",")],
                                  zero=zero, nonzero=nonzero))
        level_rows.append(dict(level=level,
                               next=levels[i + 1] if i + 1 < len(levels) else ".atom",
                               ops=operators))
    tokens = dict(E.TK, identifier=E.TK_ID, number=E.TK_NUM, string=E.TK_STR)
    # gen2 adds these two type words before it installs constexpr.
    first = max(E.TK.values()) + 1
    tokens.update({"type=extern": first, "type=_Bool": first + 1})
    with (ROOT / "exec/parse2/constexpr-tokens.tsv").open() as f:
        classes = {name: [tokens[token]] for name, token in list(csv.reader(f, delimiter="\t"))[1:]}
    classes["typeword"] = sorted({tokens[w] for w in tokens if w.startswith("type")} |
                                  {tokens[w] for w in ("struct", "union", "enum")})
    return ["=levels\tjson\t" + json.dumps(level_rows, separators=(",", ":")),
            "=classes\tjson\t" + json.dumps(classes, separators=(",", ":")),
            "=SKIPS\tint\t" + str(7 * (1 << 26))]


def structreturnexpr():
    E = _parse_e("exec_parse_gen_facts")
    reason = "not covered: struct return expression outside local lvalue"
    return ["=reason\tstr\t" + _esc(reason),
            "@sr\tentry:str\tloc:int\treason:str",
            "\tLI.original.structreturn\t%d\t%s" % (E.LOC, _esc(reason))]



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
           for i, n in enumerate(("OP", "KIND", "AC", "ARG", "TXT", "REG", "FORM", "ARGREG"))]
    words = list(dict.fromkeys(list(SHAPE) + ["r" + str(i) for i in range(8)] + [
        "0", "1", "2", "3", "4", "8", "-8", "_start:", "write", "exit",
        ".hostcall", ".hostaddr", ".librarycall", ".libraryaddr"]))
    from unisa import lower as L
    for n in ("WIN_HSTD", "WIN_WRITTEN", "WIN_SAVE", "WIN_ARGVA", "SYSA", "SYSFP", "SYSSP"):
        out.append("=%s\tint\t%d" % (n, getattr(L, n)))
    out.append("=WIN_STACK_END\tint\t%d" % (L.WIN_EXTRA + L.WIN_STACK))
    out += ["=SYSA%d\tint\t%d" % (i, L.SYSA + 8 * i) for i in range(6)]
    out += ["=key_argc\tjson\t" + json.dumps("\0process/argc"), "=key_argv\tjson\t" + json.dumps("\0process/argv")]
    out.append("@words\ttarget:str\ti:int\tword:str\trslots:json\treg:json\tform:json")
    for os_ in ("lnx", "osx", "win"):
        for arch in ("x86_64", "arm64"):
            regmap = {r[0]: r[2] for r in _gold("regmap") if r[1] == arch}
            enc = {r[0]: r[3] for r in _gold("enc") if r[1:3] == [os_, arch]}
            for i, w in enumerate(words):
                out.append("\t%s/%s\t%d\t%s\t%s\t%s\t%s" % (
                    os_, arch, i, _esc(w), json.dumps([j for j, k in enumerate(SHAPE.get(w, ())) if k == "r"]),
                    json.dumps([regmap[w]] if w in regmap else []),
                    json.dumps([" form=" + enc[w]] if w in enc else [])))
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



def lowerarmfuse():
    """ARM immediate fusion: TAPE SHAPE ops eligible under exec/lower/armfuse-shapes.tsv
    (shape / exclude / read policy), with the operand positions read (was armfuse.py)."""
    import json
    sys.path.insert(0, str(ROOT))
    from unisa.tape import SHAPE
    pol = [x.split("\t") for x in (ROOT / "exec/lower/armfuse-shapes.tsv").read_text().splitlines() if x and not x.startswith("#")]
    shapes = {tuple(v.split(",")) for k, v in pol if k == "shape"}
    excl = {v for k, v in pol if k == "exclude"}
    reads = {v for k, v in pol if k == "read"}
    out = ["=none\tjson\t[]"]
    out += ["@shapes\ti:int\top:str\treads:json\tfirst:json\trest:json"]
    sel = [(o, sh) for o, sh in SHAPE.items() if sh in shapes and o not in excl]
    for i, (o, sh) in enumerate(sel):
        out.append("\t%d\t%s\t%s\t%s\t%s" % (i, _esc(o), json.dumps([j for j, k in enumerate(sh[1:], 1) if k in reads]),
                                             json.dumps([1] if i == 0 else []), json.dumps([] if i == 0 else [1])))
    return out



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
    return out + rows + sel + om + ["=none\tjson\t[]", "=zero\tint\t0", "=eight\tint\t8"]


def optgen():
    """E4 optimiser START data and peep index facts (was exec/opt/gen.py build/peep_start/peepround):
    start1/start2 = the START action list per level (interned start words, -O2 peep table and
    opinfo classes, opinfo simple set), peepidx = PA_/PB_/PR_ column indices and sizes, Y = one
    answer row per peep head with its target template (answer-targets.tsv)."""
    import json
    sys.path.insert(0, str(ROOT))
    from exec.facts.load import facts
    C = {r["name"]: r["value"] for r in facts("opt-gen-constants")}
    pf = {}
    for ln in (ROOT / "weights/gold/peep.tsv").read_text().splitlines():
        f = ln.split("\t")
        if ln.startswith("#field") or ln.startswith("#head"):
            pf[f[1]] = f[2:]
    PA, PB, PR, PY = pf["a"], pf["b"], pf["rel"], pf["y"]
    def gold(name):   # field-wise `=>` filter, as exec/parse/gen.py gold()
        return [f for f in (ln.split("\t") for ln in (ROOT / "weights/gold" / (name + ".tsv")).read_text(encoding="utf-8").splitlines()
                            if not ln.startswith("#")) if "=>" not in f]
    peep, opinfo = gold("peep"), gold("opinfo")
    def word(w, reg):
        return [["SBCLR"]] + [["SBOUT", c] for c in w.encode()] + [["SBINTERN", reg]]
    out = []
    for level in (1, 2):
        acts = []
        for w in facts("opt-gen-startwords"):
            acts += word(w, "id_" + w.strip("."))
        if level >= 2:
            for f in peep:
                if len(f) == 4 and f[0] in PA and f[1] in PB and f[2] in PR and f[3] in PY:
                    k = (PA.index(f[0]) * len(PB) + PB.index(f[1])) * len(PR) + PR.index(f[2])
                    acts += [["LDI", "q_t", k], ["LDI", "q_u", PY.index(f[3]) + 1], ["STX", "q_t", C["PEEPB"], "q_u"]]
            for f in opinfo:
                if len(f) == 4:
                    acts += word(f[0], "q_t")
                    if f[2] in PA:
                        acts += [["LDI", "q_u", PA.index(f[2]) + 1], ["STX", "q_t", C["ACLSB"], "q_u"]]
                    if f[3] in PB:
                        acts += [["LDI", "q_u", PB.index(f[3]) + 1], ["STX", "q_t", C["BCLSB"], "q_u"]]
        for f in opinfo:
            if len(f) >= 2 and f[1] == "1":
                acts += word(f[0], "t") + [["LDI", "u", 1], ["STX", "t", C["SIMPLE"], "u"]]
        out.append("=start%d\tjson\t%s" % (level, json.dumps(acts)))
    idx = {}
    for prefix, values in (("PA", PA), ("PB", PB), ("PR", PR)):
        idx.update((prefix + "_" + n, i) for i, n in enumerate(values))
    idx.update(PB_size=len(PB), PR_size=len(PR))
    out.append("=peepidx\tjson\t" + json.dumps(idx))
    targets = dict(l.split("\t") for l in (ROOT / "exec/opt/answer-targets.tsv").read_text().splitlines()
                   if l and not l.startswith("#"))
    out.append("=Y\tjson\t" + json.dumps([dict(i=i, target=targets.get(n, targets["*"]).format(name=n)) for i, n in enumerate(PY)]))
    return out


def modelbindingstemplate():
    """USBIND magic chain, identifier byte classes, record kinds (from top-modelbindings-const)."""
    sys.path.insert(0, str(HERE))
    from load import facts
    c = {r["name"]: r["value"] for r in facts("top-modelbindings-const")}
    out = ["@magic\ti:int\tc:int\tnext:int"]
    out += ["\t%d\t%d\t%d" % (i, ch, i + 1) for i, ch in enumerate(c["magic"].encode())]
    L, D = list(c["letters"].encode()), list(c["digits"].encode())
    out += ["@id\tletters:json\tdigits:json\tall:json", "\t%s\t%s\t%s" % tuple(json.dumps(x) for x in (L, D, L + D))]
    out += ["=kind\tjson\t" + json.dumps(c["kind"])]
    return out


_NATIVE_TRIE = r"""
import csv, json, pathlib, sys
lines = pathlib.Path(sys.argv[1]).read_text().splitlines()
rows = list(csv.DictReader(lines[:7], delimiter="\t"))
profiles = {r["profile"] for r in rows}
prefixes = {b""}
for profile in profiles:
    raw = profile.encode(); prefixes.update(raw[:i] for i in range(1, len(raw) + 1))
names = {p: "NC.profile" + ("" if not p else "." + p.hex()) for p in prefixes}
T = []
for prefix in sorted(prefixes):
    if prefix.decode() in profiles:
        r = next(r for r in rows if r["profile"] == prefix.decode())
        T.append(dict(name=names[prefix], leaf=[dict(im=int(r["integer_min_width"]), fp=int(r["homogeneous_fp_carrier_kind"]),
                 ld=int(r["long_double_format"]), family=int(r["family"]))], inner=[]))
    else:
        T.append(dict(name=names[prefix], leaf=[], inner=[dict(choices=[dict(byte=p[len(prefix)], target=names[p])
                 for p in prefixes if len(p) == len(prefix) + 1 and p.startswith(prefix)])]))
print(json.dumps(T))
"""


def nativeabi():
    """FFI carrier facts from exec/nativeabi/rules.tsv (was nativeabi/gen.py + ordered.py):
    T = profile byte trie (state order = sorted prefixes; choice order = the original
    PYTHONHASHSEED=0 iteration order of the prefix set, recorded by a seeded child), A = union16
    recipes, extents/alignments classes, ordseq = the ordered `@` field/extra read sequences."""
    import csv, json, os, subprocess
    sys.path.insert(0, str(ROOT))
    from exec.facts.load import facts
    rules = ROOT / "exec/nativeabi/rules.tsv"
    lines = rules.read_text().splitlines()
    rows = list(csv.DictReader(lines[:7], delimiter="\t"))
    decl = [x.split("\t") for x in lines[7:] if not x.startswith("#")]
    leaf = {int(x[1]): "NSI".index(x[2]) for x in decl if x[0] == "leaf"}
    joins = {(x[1], x[2]): x[3] for x in decl if x[0] == "merge"}
    assert all(joins[a, b] == "NSI"[max("NSI".index(a), "NSI".index(b))] for a in "NSI" for b in "NSI")
    recipes = {(int(x[1]), x[2]): x[3] for x in decl if x[0] == "recipe"}
    assert len(recipes) == 8 and leaf == {1: 2, 2: 2, 3: 1}
    assert {r["profile"] for r in rows} == {f"{o}/{a}" for o in ("osx", "lnx", "win") for a in ("arm64", "x86_64")} and len(rows) == 6
    assert all(r["rule"] == "natural_scalar_union_v2" and int(r["mixed_width"]) == 8 for r in rows)
    assert all(int(r["integer_min_width"]) in (1, 8) and int(r["homogeneous_fp_carrier_kind"]) in (1, 3) for r in rows)
    assert {int(r["long_double_format"]) for r in rows} <= {2, 3, 4}
    T = subprocess.run([sys.executable, "-c", _NATIVE_TRIE, str(rules)], env=dict(os.environ, PYTHONHASHSEED="0"),
                       capture_output=True, check=True, text=True).stdout.strip()
    A = [dict(al=al, recipes=[dict(cls=c, code=3 * "NSI".index(c[0]) + "NSI".index(c[1]), n=len(recipes[al, c]),
         elems=[dict(off=i * al, kind=1 if k == "I" else 3, integer=int(k == "I")) for i, k in enumerate(recipes[al, c])])
         for c in ("II", "IS", "SI", "SS")]) for al in (4, 8)]
    ext = [int(x[1]) for x in decl if x[0] == "ordered_extent"]
    ali = [int(x[1]) for x in decl if x[0] == "ordered_alignment"]
    assert ext == list(range(1, 17)) and ali == [1, 2, 4, 8]
    assert [x[1] for x in decl if x[0] == "ordered_anon_policy"] == ["invariant_integer_lanes"]
    geq = {r["name"]: r["value"] for r in facts("top-modelgraphequality-banks")}
    ordered = {}
    for ln in (ROOT / "exec/nativeabi/ordered-result.tsv").read_text().splitlines()[1:]:
        for a in json.loads(ln.split("\t")[4]):
            if a[0] == "@" and a[1] not in ordered:
                kind, index, reg, node = a[1].split()
                assert kind in ("field", "extra")
                ordered[a[1]] = dict(name=a[1], index=int(index), reg=reg, node=node,
                                     stride=16 if kind == "field" else 8,
                                     bank=geq["FIELDS"] if kind == "field" else geq["EXTRA"])
    gf = [dict(section=a, name=b, prefix=c, kind=d) for a, b, c, d in (l.split("\t") for l in (ROOT / "exec/nativeabi/gen-fresh.tsv").read_text().splitlines()[1:])]
    of = [dict(section=a, name=b, kind=c) for a, b, c in (l.split("\t") for l in (ROOT / "exec/nativeabi/ordered-fresh.tsv").read_text().splitlines()[1:])]
    return ["=T\tjson\t" + T, "=A\tjson\t" + json.dumps(A), "=extents\tjson\t" + json.dumps(ext),
            "=alignments\tjson\t" + json.dumps(ali), "=ordseq_names\tjson\t" + json.dumps(list(ordered)),
             "=ordseq_rows\tjson\t" + json.dumps(list(ordered.values())),
            "=genfresh\tjson\t" + json.dumps(gf), "=ordfresh\tjson\t" + json.dumps(of),
            "=reject\tjson\t" + json.dumps(facts("nativeabi-gen-reject"))]


def ppautoinc():
    """Autoinc domain data: library names, closure slots and header order.

    Ordinal intervals describe source-list order; state spelling and all action
    recipes belong to exec/pp/autoinc-manifest.tsv, not this producer.
    """
    import json
    E = _ppsrc()
    from unisa.libneed import table, roots, PREFIX
    inc = str(ROOT / "include")
    keys, closure, bodies = table(inc)
    index = {b: i for i, b in enumerate(bodies)}
    def slots(names):
        return [dict(offset=E.NEEDB + index[b]) for b in names]
    K = [dict(id=i, end=i + 1, terminal=i + 1 == len(keys),
              name=k, slots=slots(closure[k])) for i, k in enumerate(keys)]
    B = [dict(id=j, end=j + 1, terminal=j + 1 == len(bodies),
              slot=E.NEEDB + j, name=PREFIX + b) for j, b in enumerate(bodies)]
    amap, H = E.autoinc_map(), list(E.AUTOINC_ORDER)
    HD = []
    for h, hn in enumerate(H):
        names = [n for n in amap[hn] if n != "printf"]
        HD.append(dict(id=h, end=h + 1, terminal=h + 1 == len(H), count=len(names),
                       names=[dict(id=k, end=k + 1, name=nm) for k, nm in enumerate(names)]))
    EM = [dict(special=True, id=len(H), end=len(H) - 1, terminal=False, header="stdio.h")]
    EM += [dict(special=False, id=h, end=h - 1, terminal=h == 0, header=H[h])
           for h in range(len(H) - 1, -1, -1)]
    return ["=idclass\tjson\t" + json.dumps(sorted(E.ID)), "=roots\tjson\t" + json.dumps(slots(roots(inc))),
            "=keys\tjson\t" + json.dumps(K), "=bodies\tjson\t" + json.dumps(B),
            "=lndef\tstr\t__UNISA_FTRIM_LIBC",
            "=headers\tjson\t" + json.dumps(HD), "=emits\tjson\t" + json.dumps(EM)]


def ppgen():
    """E2 delta facts (was exec/pp/gen.py build/build_xe): START init actions (directive ids from
    weights/gold/pp.tsv, pp-init spellings, XE precedences), per-target predefine chains, the DSW
    directive switch, directive-action instances (gold pp.tsv answers), simple escapes, PREC_* layout."""
    import json
    E = _ppsrc()
    from exec.facts.load import facts
    # Domain facts only.  The pp body manifest assembles the action recipes.
    dirrows = [dict(word=w, arg=k + 1) for k, w in enumerate(E.DIRV)]
    dirrows += [dict(word=r["word"], arg=r["arg"]) for r in facts("pp-init") if r["kind"] == "dir"]
    internrows = [dict(word=r["word"], arg=r["arg"]) for r in facts("pp-init") if r["kind"] != "dir"]
    xerows = [dict(slot=E.XPRB + c, precedence=p) for c, (_, p, _) in list(E.XOPS.items()) + [(0, (None, -1, 0))]]
    predef = {}
    for t in E.TARGETS:
        o, a = t.split("/")
        names = E.PREDEF["os", o] + E.PREDEF["arch", a] + E.PREDEF["common", "*"]
        assert len(set(names)) == len(names), "overlapping target predefinitions: " + t
        predef[t] = [dict(entry="P3PD%d" % k, resume="P3PDR%d" % k, next="P3PD%d" % (k + 1) if k + 1 < len(names) else "OOBJ.start",
                          name=nm) for k, nm in enumerate(names)]
    # The first four switch rows are fixed in dsw-template.tsv.  Only the
    # directive spelling/ordinal/target are facts.
    cases = [dict(key=k + 1, target="D_" + w) for k, w in enumerate(E.DIRV)]
    acts = [dict(name="D_%s_a%d" % (w, fl), section=w + "/" + E.PPT[(w, fl)]) for w in E.DIRV for fl in (0, 1)]
    from unisa.front.lex import ESC
    esc = [{"code": ord(ch), "value": ord(v)} for ch, v in ESC.items() if ch not in "01234567x"]
    prec = {"PREC_" + str(c): p for c, (_, p, _) in E.XOPS.items()}
    prec["XOB_PREV"] = E.XOB - 1
    return ["=dirrows\tjson\t" + json.dumps(dirrows), "=internrows\tjson\t" + json.dumps(internrows),
            "=xerows\tjson\t" + json.dumps(xerows), "=predef\tjson\t" + json.dumps(predef),
            "=cases\tjson\t" + json.dumps(cases), "=dswkeys\tjson\t" + json.dumps(sorted([0, 100, 101, 102] + [c["key"] for c in cases])), "=dactions\tjson\t" + json.dumps(acts),
            "=esc\tjson\t" + json.dumps(esc), "=esckeys\tjson\t" + json.dumps([e["code"] for e in esc]),
            "=xelayout\tjson\t" + json.dumps(prec),
            "=predefres\tjson\t" + json.dumps({t: "".join(n + "\0" for n in E.PREDEF["os", t.split("/")[0]] + E.PREDEF["arch", t.split("/")[1]] + E.PREDEF["common", "*"])
                                                 for t in sorted(E.TARGETS)})]


def tokenlocationsmap():
    """Located-token record fields; actions remain in the stage template."""
    import csv
    with (ROOT / "exec/facts/tokenlocations.tsv").open() as f:
        rows = {r[0]: r[1] for r in list(csv.reader(f, delimiter="\t"))[1:]}
    names = json.loads(rows["MAP_FIELDS"])
    assert len(names) == len(set(names)) and names
    return ["=mapfields\tjson\t" + json.dumps(
        [dict(i=i, register="diag_" + name) for i, name in enumerate(names)], separators=(",", ":"))]


# (fact stem, inputs whose sha prefixes head the file, producer)
def _namespace_constants(G):
    """Layout constants the function/global/local control sections bind (facts k2-gen2 nsconst; was gen2.py)."""
    E = G.E
    f = dict(PIDS=G.PIDS, PDB=G.PDB, LOC=G.LOC, FND=E.FND, FRD=E.FRD, FRB=E.FRB, VAR=E.VAR)
    f.update(("FN_PDB" + str(i), G.PDB + i) for i in range(16))
    g = {name: getattr(G, name) for name in ("LOC", "GIBLOB", "GIEND", "GINPS", "GINPE", "GSZ", "GUNIT", "SINIT", "SKIPS")}
    g.update((name, getattr(E, name)) for name in ("FND", "GMARK", "BASE", "ARR", "PTR"))
    return dict(function=f, globals=g, local=dict(SKIPS=G.SKIPS, PTR=E.PTR, BASE=E.BASE))


def _tk2(E):
    """parse2's token ids: parse words plus type=extern/_Bool appended after max(TK) (parse2base.tokens)."""
    assert "type=extern" not in E.TK
    tk = dict(E.TK)
    for w in ("type=extern", "type=_Bool"):
        tk[w] = max(tk.values()) + 1
    return tk


def k2gen2():
    """parse2/gen2-manifest.tsv constants: POSSPAN, type words, tytail ckm/resd tables (from gen2.py constants)."""
    for d in ("exec", "exec/parse2"):
        if str(ROOT / d) not in sys.path:
            sys.path.insert(0, str(ROOT / d))
    G = _gen2ns("gen2")
    dump = lambda v: json.dumps(v)
    follow = dict(G.tape_rows("type-follow.tsv"))
    words = [dict(word=w, value=v, rank=(1 if w == "type=float" else 2 if w == "type=double" else 0),
                  follow=follow.get(w, follow["*"])) for w, v in G.TYPEW.items()]
    ck, current, first = [], "CKM", [{}]
    for width, mask in [(sz, (1 << (8 * sz)) - 1) for _, _, sz, un, _ in G.TYINT if un and sz < 8]:
        nxt = "CKM.k%d" % width
        ck.append(dict(current=current, hit="CKM.m%d" % width, next=nxt, axis=G.AX.index("u%d" % (8 * width)),
                       first=first, masktext=G.TYPE_TAPE["mask_pair"] % mask))
        current, first = nxt, []
    ckf = dict(current=current, axis=G.AX.index("u64"), first=first)
    rs, current, first = [], "RESD", [{}]
    for name, code, size, unsigned, _ in G.TYINT:
        hit, nxt = "RESD." + name, "RESD.n" + name
        rs.append(dict(current=current, hit=hit, next=nxt, axis=G.AX.index(name), code=code, first=first,
                       masktext=G.TYPE_TAPE["mask"] % ((1 << (8 * size)) - 1) if unsigned and size < 8 else ""))
        current, first = nxt, []
    rsf = dict(current=current, first=first)
    lm = dict(G.tape_rows("ladder-modes.tsv"))
    ladder = [dict(lv=lv, nxt=G.LEVELS[i + 1] if i + 1 < len(G.LEVELS) else None,
                   mid=[{}] if i + 1 < len(G.LEVELS) else [], last=[] if i + 1 < len(G.LEVELS) else [{}],
                   ops=[dict(op=o, key=G.TK[o], mode=lm.get(o, lm["*"])) for o in G.OPS[lv]])
              for i, lv in enumerate(G.LEVELS)]
    tops = [o for lv in G.LEVELS for o in G.OPS[lv] if o not in G.SHORT]
    tyrows = []
    for l in range(16):
        for r in range(16):
            tyrows.append(dict(t=l * 16 + r, u=G.AX.index(G.TYROW.get((G.AX[l], "+", G.AX[r]), "illegal")), tab=G.CKT))
            for i, o in enumerate(tops):
                tyrows.append(dict(t=i * 256 + l * 16 + r, tab=G.RST,
                                   u=G.AX.index(G.TYROW.get((G.AX[l], G.TYOP.get(o, o), G.AX[r]), "illegal"))))
    syscalls = [dict(i=k, name=nm) for k, (nm, _, _) in enumerate(G.SYSCALLS, 1)]
    autonames = [dict(name=nm) for nm in _autonames(G.E)]
    i32 = G.TYINFO["i32"][0]
    btk = dict(G.TK)   # build() appends these two token codes before types() runs
    for w in ("type=extern", "type=_Bool"):
        btk[w] = max(btk.values()) + 1
    typetargets = {btk[w]: "TS." + w for w in G.TWORDS}
    typetargets.update((G.TK_ID if w == "identifier" else btk[w], t) for w, t in G.tape_rows("type-entry.tsv"))
    gen2parts = dict(
        localdecl=dict(classes=dict(identifier=[G.TK_ID], paren=[G.TK["("]])),
        longdouble=dict(consts=dict(DBL=G.DBL), classes=dict(double=[G.TK["type=double"]])),
        positive=dict(consts=dict(INT=i32),
                      classes=dict(promote=[G.BOOL] + [c for _, c, z, _, _ in G.TYINT if z < i32],
                                   arithmetic=[G.DBL, G.FLT] + [c for _, c, z, _, _ in G.TYINT if z >= i32])),
        tentative=dict(consts=dict(TENTATIVE=764 << 40)),
        typeops=[dict(key=k, target=t) for k, t in typetargets.items()],
        updateops=[dict(key=G.TK[o + "="], target="LV.c" + o) for o in G.E.CASOPS])
    scopeconst = {n: getattr(G.E, n) for n in ("UNDO", "PTR", "BASE", "ARR", "TDN", "TDB", "TDD", "FND", "FRD", "FRB", "VAR")}
    scopeconst.update(VALUEBANK=G._VR["VALUEBANK"], TYPERANK=G.TYPERANK, LOC=G.LOC, END_=G.END_, ENV=G.ENV, VLSIZE=G.VLSIZE,
                      UNDO_SIZE=G.UNDO_SIZE, SHAPE=G.SHAPE, TDE=G.TDE)
    for n, base, size in (("UNDO", G.E.UNDO, G.UNDO_SIZE), ("DIM", G.DIM, 8), ("PDB", G.PDB, 16)):
        scopeconst.update((n + "_" + str(i), base + i) for i in range(size))
    widthparts = dict(
        store=dict(consts=dict(FLT=G.FLT)),
        elsz=dict(consts=dict(SBB=G.SBB, SSZ=G.SSZ, DBL=G.DBL, FLT=G.FLT, BOOL=G.BOOL, UNS=G.UNS)),
        naru=dict(consts=dict(BOOL=G.BOOL), rows=[dict(code=c) for _, c, z, u, _ in G.TYINT if u and z < 8]),
        conv=dict(consts=dict(BOOL=G.BOOL, DBL=G.DBL, FLT=G.FLT, UNSIGNED_WIDE=G.UNS + 8),
                  suffixes=[dict(s=x) for x in ("d", "s", "i", "u")]),
        narrow=dict(consts=dict(UNSIGNED_WIDE=G.UNS + 8, DBL=G.DBL, BOOL=G.BOOL, TDN=G.E.TDN),
                    rows=[dict(code=vb) for _, vb, size, uns, nar in G.TYINT if nar]))
    widthd = {wname: dict(rows=[dict(code=vb) for _, vb, *_ in G.TYINT if vb != 8])
              for wname in ("LOADV", "LOADRAW", "STOREV0")}
    updconst = {n: getattr(G, n) for n in ("SBB", "UNS", "DBL", "FLT", "BOOL", "FPB", "FPV", "ENV", "END_", "LOC", "FPS_FN", "FPS_VAR")}
    updconst.update((n, getattr(G.E, n)) for n in ("FND", "VAR", "PTR", "BASE", "ARR"))
    updconst.update(("U" + str(z), G.UNS + z) for z in (1, 2, 4, 8))
    updconst.update(tail_entry="C%d" % G.LEVELS[0], axis_ptr=G.AX.index("ptr"), axis_struct=G.AX.index("struct"))
    tk = dict(G.TK, identifier=G.TK_ID)
    for w in ("type=extern", "type=_Bool"):   # the token codes gen2 build() appends before any rows install
        tk.setdefault(w, max(v for k, v in tk.items() if k != "identifier") + 1)
    updclasses = {n: [tk[t]] for n, t in G.tape_rows("update-tokens.tsv")}
    updclasses.update(BOOL=[G.BOOL], float_types=[G.DBL, G.FLT], float_axes=[G.AX.index("f32"), G.AX.index("f64")],
                      signed_narrow_codes=[c for _, c, z, u, _ in G.TYINT if not u and z < 8])
    updcompound = [dict(key=G.TK[o + "="], target="X.c" + o) for o in G.E.CASOPS]
    updid0 = sorted(set(range(257)) - {c["key"] for c in updcompound})
    retconst = dict(SBB=G.SBB, SSZ=G.SSZ, CKT=G.CKT, expr_entry="E%d" % G.LEVELS[0], tail_entry="C%d" % G.LEVELS[0])
    tk = dict(G.TK, identifier=G.TK_ID)
    for w in ("type=extern", "type=_Bool"):   # the token codes gen2 build() appends before any rows install
        tk.setdefault(w, max(v for k, v in tk.items() if k != "identifier") + 1)
    retclasses = {n: [tk[t]] for n, t in G.tape_rows("return-tokens.tsv")}
    retclasses.update(typewords=[tk[w] for w in G.TWORDS], scalar_types=[G.BOOL, G.DBL, G.FLT], float_types=[G.DBL, G.FLT],
                      axis_f64=[G.AX.index("f64")], axis_f32=[G.AX.index("f32")],
                      operators=[G.TK[o] for o in ("=", "++", "--")] + [G.TK[o + "="] for o in G.E.CASOPS])
    retcompound = [dict(key=G.TK[o + "="], target="LV.c" + o) for o in G.E.CASOPS]
    retexpr0 = sorted(set(range(257)) - {c["key"] for c in retcompound})
    shapeconst = {n: getattr(G, n) for n in ("POSSPAN", "SHAPE", "SHAPE_IDS", "DIM", "TDIM", "MEMBER_STRIDE", "TYPERANK", "MEMBERRANK", "RETURNRANK", "PARAMRANK")}
    shapeconst.update(ARR=G.E.ARR, UNSIGNED_CHAR=G.UNS + 1)
    shapeclasses = {n: [G.TK[t] for t in ts.split(",")] for n, ts in G.tape_rows("shape-tokens.tsv")}
    return ["=VS\tint\t%d" % G.E.VS, "=POSSPAN\tint\t%d" % G.POSSPAN, "=typewords\tjson\t" + dump(words),
            "=ckmrows\tjson\t" + dump(ck), "=ckmfinal\tjson\t" + dump(ckf),
            "=resdrows\tjson\t" + dump(rs), "=resdfinal\tjson\t" + dump(rsf),
            "=oprows\tjson\t" + dump([G.optail_facts(o) for lv in G.LEVELS for o in G.OPS[lv] if o not in G.SHORT]),
            "=CKT\tint\t%d" % G.CKT, "=RST\tint\t%d" % G.RST,
            "=AXILL\tint\t%d" % G.AX.index("illegal"), "=INVTEXT\tjson\t" + dump(G.TYPE_TAPE["float_invert"]), "=ladder\tjson\t" + dump(ladder), "=nsconst\tjson\t" + dump(_namespace_constants(G)), "=truthfpu\tjson\t" + dump({row[1]: row[2] for row in G.E.gold("irsel") if row[0] == "fpu"}), "=buildconst\tjson\t" + dump({"UNSIGNED_WIDE": G.UNS + 8, "TK": _tk2(G.E), "TK_ID": G.E.TK_ID, "TK_NUM": G.E.TK_NUM, "TK_FNUM": G.E.TK_FNUM, "FND": G.E.FND, **{n: getattr(G, n) for n in ['FPS_FN', 'FPS_RD', 'FPS_RB', 'FPS_RSH', 'FPS_COUNT', 'FPS_PARAM', 'FPS_PSH', 'FPS_VAR', 'SBB', 'FPB', 'FPV', 'FPS_FIRST', 'BOOL', 'DBL', 'FLT', 'ENUM_FIRST', 'GSZ', 'GUNIT', 'SSZ', 'SAL', 'SMN', 'SMEM', 'MOF', 'MSZ', 'MPT', 'MBS', 'MAR', 'BFW', 'BFO', 'BFS', 'SHAPE_IDS', 'SHAPE', 'UNS', 'TIX', 'UNDO_SIZE', 'ENV', 'END_', 'TYINT']}}), "=ordconst\tjson\t" + dump(dict(DBL=G.DBL, FLT=G.FLT, GMARK=G.E.GMARK, bottom="C%d" % G.LEVELS[0])), "=tyrows\tjson\t" + dump(tyrows), "=syscalls\tjson\t" + dump(syscalls),
            "=shapeconst\tjson\t" + dump(shapeconst), "=shapeclasses\tjson\t" + dump(shapeclasses),
            "=retconst\tjson\t" + dump(retconst), "=retclasses\tjson\t" + dump(retclasses),
            "=retexpr0\tjson\t" + dump(retexpr0), "=retint\tjson\t" + dump([dict(code=c) for _, c, *_ in G.TYINT]), "=retfloat\tjson\t" + dump([dict(label=l, cv=v, base=b) for l, v, b in (("double", "d", G.DBL), ("single", "s", G.FLT))]), "=retcompound\tjson\t" + dump(retcompound),
            "=updconst\tjson\t" + dump(updconst), "=updclasses\tjson\t" + dump(updclasses),
           
            "=updid0\tjson\t" + dump(updid0), "=updcompound\tjson\t" + dump(updcompound), "=ordupd\tjson\t" + dump([dict(tag="inc", op="+"), dict(tag="dec", op="-")]), "=staticsenv\tjson\t" + dump({n: getattr(G, n) for n in ('BOOL', 'LOC', 'SIEND', 'SINIT', 'SKIPS', 'TIX')}), "=initenv\tjson\t" + dump(dict({n: getattr(G, n) for n in ('SBB', 'LOC', 'SSZ', 'SMN', 'SMEM', 'MOF', 'MPT', 'MBS', 'MAR', 'SFLAT', 'MFLAT', 'MEMBER_STRIDE', 'SKIPS', 'BFW', 'BFO', 'BFS', 'SHAPE', 'SHAPE_IDS', 'MSZ', 'DIM')}, UCHAR=G.E.UNS + 1, PTR=G.E.PTR, BASE=G.E.BASE, ARR=G.E.ARR, DIM1=G.DIM + 1, DIM2=G.DIM + 2)), "=k2env\tjson\t" + dump(dict(DBL=G.DBL, FLT=G.FLT, BOOL=G.BOOL, VLDEP=G.VLDEP, END_=G.END_, VLFRAME=G.VLFRAME, VLSIZE=G.VLSIZE, UNS=G.UNS, ENV=G.ENV, SBB=G.SBB, MEMBER_STRIDE=G.MEMBER_STRIDE, MOF=G.MOF, MSZ=G.MSZ, MBS=G.MBS, MAR=G.MAR)), "=unaryenv\tjson\t" + dump((lambda mt: dict(ucx=dict(TIX=G.TIX, MAXTOK=mt), ufacts=dict(DBL=G.DBL, FLT=G.FLT, BOOL=G.BOOL, UNS1=G.UNS + 1, UNS3=G.UNS + 3, UNS4=G.UNS + 4, UNS8=G.UNS + 8, U32M=G.U32M, ENV=G.ENV, END_=G.END_, FNSTR=G.FNSTR, TIX=G.TIX, MAXTOK=mt)))(int(G.re.search(r"^#define MAXTOK ([0-9]+)\b", G.Path(G.E.ROOT, "src/front_pp.c").read_text(), G.re.M).group(1)))), "=updcas\tjson\t" + dump([dict(op=o, nptr=int(o not in {r[0] for r in G.tape_rows("update-pointer.tsv")}), owner="X" if o not in {r[0] for r in G.tape_rows("update-pointer.tsv")} else "LV") for o in G.E.CASOPS]), "=updtype\tjson\t" + dump([dict(code=c, axis=G.AX.index(n)) for c, n in ((1,"i8"),(2,"i16"),(4,"i32"),(8,"i64"),(G.UNS+1,"u8"),(G.UNS+2,"u16"),(G.UNS+4,"u32"),(G.UNS+8,"u64"),(G.BOOL,"u8"),(0,"void"),(G.DBL,"f64"),(G.FLT,"f32"),(G.FPB,"ptr"),(G.FPV,"ptr"))]), "=updmodes\tjson\t" + dump([dict(zip(("name","op","postfix","prefixname","prefixfix","integer","floating"), r)) for r in G.tape_rows("update-modes.tsv")]), "=updfloat\tjson\t" + dump([dict(suffix=s, bits=int(b)) for s, b in G.tape_rows("update-float.tsv")]),
            "=widthd\tjson\t" + dump(widthd), "=widthconst\tjson\t" + dump(dict(DBL=G.DBL, FLT=G.FLT, BOOL=G.BOOL, SBB=G.SBB)),
            "=widthparts\tjson\t" + dump(widthparts),
            "=scopeconst\tjson\t" + dump(scopeconst),
            "=gen2parts\tjson\t" + dump(gen2parts),
            "=autonames\tjson\t" + dump(autonames), "=HEADER\tjson\t" + dump(G.E.HEADER), "=AUT\tint\t%d" % G.E.AUT, "=AXF64\tint\t%d" % G.AX.index("f64")]


def k2gen2tokens():
    """parse2 (gen2) token reader facts (was parse2base.tokens -> E.tokenizer(QUALIFIERS)): the parse words plus
    type=extern/_Bool appended after max(TK), parse2's skipped qualifiers, span readers, limits."""
    if str(ROOT / "exec") not in sys.path:
        sys.path.insert(0, str(ROOT / "exec"))
    from finite_rules import prefix_facts
    E = _parse_e("k2gen2tok_parse")
    B = _module("exec/build/parse2base.py", "k2gen2tok_base")
    tk = dict(E.TK)
    words = list(E.WORDS)
    for w in ("type=extern", "type=_Bool"):
        words.append(w); tk[w] = max(tk.values()) + 1
    skip = B.QUALIFIERS
    spans = dict(ln.split("\t") for ln in (ROOT / "exec/parse/token-prefixes.tsv").read_text().splitlines()
                 if ln and not ln.startswith("#"))
    root, pre = prefix_facts(words + list(spans) + list(skip))
    def node(n):
        p = n["name"]
        if p in spans:
            return dict(name=p, span=[dict(reader=spans[p])], children=[], q=[], w=[], tail=[])
        return dict(name=p, span=[], children=[dict(name=c["name"], last=c["last"]) for c in n["children"]],
                    q=[1] if p in skip else [], w=[dict(tok=tk[p])] if p not in skip and p in tk else [],
                    tail=[dict(tok=E.TK_OTHER)])
    out = ["=N\tjson\t" + json.dumps([node(root)] + [node(n) for n in pre])]
    out += ["=limit%d\tint\t%d" % (d, (2**64 - 1 - d) // 10) for d in range(10)]
    return out


def k2unitstokens():
    """parse2/units-manifest.tsv tokenizer facts (was units.py E.WORDS/E.TK edits + E.tokenizer): word list
    with token codes (units qualifiers appended after max(TK)), span prefixes with their reader, prefix-expanded
    words (finite_rules.prefix_facts form; edges come from token-template.tsv rows), builtin codes, limits."""
    if str(ROOT / "exec") not in sys.path:
        sys.path.insert(0, str(ROOT / "exec"))
    from finite_rules import prefix_facts
    E = _parse_e("k2units_parse")
    rows = lambda f: [ln.split("\t") for ln in (ROOT / "exec/parse2" / f).read_text().splitlines()[1:]]
    quals = [r[0] for r in rows("units-qualifiers.tsv")]
    tk = dict(E.TK)
    for q in quals:
        tk[q] = max(tk.values()) + 1
    words = E.WORDS + quals
    spans = dict(ln.split("\t") for ln in (ROOT / "exec/parse/token-prefixes.tsv").read_text().splitlines()
                 if ln and not ln.startswith("#"))
    skip = _SKIP   # the reader's own skipped qualifiers; units qualifiers are words
    root, pre = prefix_facts(words + list(spans))
    def node(n):
        p = n["name"]
        if p in spans:
            return dict(name=p, span=[dict(reader=spans[p])], children=[], q=[], w=[], tail=[])
        return dict(name=p, span=[], children=[dict(name=c["name"], last=c["last"]) for c in n["children"]],
                    q=[1] if p in skip else [], w=[dict(tok=tk[p])] if p not in skip and p in tk else [],
                    tail=[dict(tok=E.TK_OTHER)])
    sel = rows("units-builtin.tsv")
    builtin = [tk[w] for w in words if any(w.startswith(a) and w != b for a, b in sel)]
    out = ["=N\tjson\t" + json.dumps([node(root)] + [node(n) for n in pre]),
           "=builtin\tjson\t" + json.dumps(builtin),
           "@qualifiers\tname:str\ttok:int"]
    out += ["\t%s\t%d" % (q, tk[q]) for q in quals]
    out.append("@tokens\tclass:str\ttok:int")
    out += ["\t%s\t%d" % (c, E.TK_ID if t == "identifier" else tk[t]) for c, t in rows("units-tokens.tsv")]
    out += ["=limit%d\tint\t%d" % (d, (2**64 - 1 - d) // 10) for d in range(10)]
    classes = {c: [E.TK_ID if t == "identifier" else tk[t]] for c, t in rows("units-tokens.tsv")}
    classes["builtin"] = builtin
    out.append("=classes\tjson\t" + json.dumps(classes))
    out += ["=%s\tstr\t%s" % (n, m) for n, m in rows("units-reject.tsv")]
    out.append("@lengths\tbyte:str\tstep:str\tnext:str\tshift:int")
    out += ["\tL%d\tL%db\t%s\t%d" % (i, i, "L%d" % (i + 1) if i < 3 else "EXTENT", 8 * i) for i in range(4)]
    out.append("@counters\tsection:str\ttoken:str\tslot:str")
    out += ["\t" + "\t".join(r) for r in rows("units-counters.tsv")]
    out.append("@separators\tentry:str\tnext:str")
    out += ["\t" + "\t".join(r) for r in rows("units-separators.tsv")]
    out.append("@trailer\tstate:str\tnext:str\tbyte:int")
    out += ["\tTRAIL%d\tTRAIL%d\t%d" % (i, i + 1, c) for i, c in enumerate(json.loads(rows("units-trailer.tsv")[0][0]))]
    out.append("@keys\tkey:int")
    out += ["\t%d" % k for k in range(257)]
    out.append("=STATIC\tint\t%d" % (1 << 40))
    out += ["=tk_goto\tint\t%d" % tk["goto"], "=tk_colon\tint\t%d" % tk[":"]]
    return out


TABLES = [
    ("parse-tokens", ["exec/facts/parse-words.tsv", "weights/gold/prec.tsv", "exec/facts/export.py"], parsetokens),
    ("k2-gen2-tokens", ["exec/build/parsebase.py", "exec/build/parse2base.py", "exec/parse/token-prefixes.tsv", "exec/finite_rules.py", "exec/facts/export.py"], k2gen2tokens),
    ("k2-units-tokens", ["exec/build/parsebase.py", "exec/parse/token-prefixes.tsv", "exec/parse2/units-qualifiers.tsv", "exec/parse2/units-builtin.tsv", "exec/parse2/units-tokens.tsv", "exec/parse2/units-reject.tsv", "exec/parse2/units-counters.tsv", "exec/parse2/units-separators.tsv", "exec/parse2/units-trailer.tsv", "exec/finite_rules.py", "exec/facts/export.py"], k2unitstokens),
    ("k2-gen2", ["exec/build/parsebase.py", "exec/build/parse2base.py", "src/front_pp.c", "exec/parse2/operator-actions.tsv", "exec/parse2/type-follow.tsv", "exec/parse2/ladder-modes.tsv", "exec/parse2/shape-reject.tsv", "exec/parse2/shape-stack.tsv", "exec/parse2/shape-tokens.tsv", "exec/parse2/return-text.tsv", "exec/parse2/return-template.tsv", "exec/parse2/return-reject.tsv", "exec/parse2/return-stack.tsv", "exec/parse2/return-tokens.tsv", "exec/parse2/tape-templates.tsv", "exec/parse2/update-text.tsv", "exec/parse2/update-reject.tsv", "exec/parse2/update-template.tsv", "exec/parse2/update-stack.tsv", "exec/parse2/update-modes.tsv", "exec/parse2/update-pointer.tsv", "exec/parse2/update-float.tsv", "exec/parse2/update-tokens.tsv", "exec/parse2/type-tape.tsv", "exec/parse2/scope-actions.tsv", "exec/parse2/type-entry.tsv", "exec/facts/export.py"], k2gen2),
    ("lex-gen", ["weights/gold/parse.tsv", "iterate/kernel/typekw.tsv", "weights/gold/lexcls.tsv", "weights/gold/lexword.tsv", "weights/gold/lex.tsv", "exec/lex/spelling.tsv", "exec/facts/lex-consts.tsv", "exec/finite_rules.py", "exec/facts/export.py"], lexgen),
    ("pp-gen", ["exec/facts/pp-targets.tsv", "exec/facts/pp-bytes.tsv", "exec/facts/pp-autoinc.tsv", "exec/pp/operators.tsv", "exec/pp/predefines.tsv", "weights/gold/pp.tsv", "exec/facts/pp-init.tsv", "exec/facts/pp-layout.tsv", "unisa/front/lex.py", "exec/facts/export.py"], ppgen),
    ("pp-autoinc-gen", ["exec/facts/pp-bytes.tsv", "unisa/libneed.py", "exec/facts/pp-autoinc.tsv", "exec/facts/pp-layout.tsv", "exec/facts/export.py"], ppautoinc),
    ("nativeabi", ["exec/nativeabi/rules.tsv", "exec/nativeabi/ordered-result.tsv", "exec/nativeabi/gen-fresh.tsv", "exec/nativeabi/ordered-fresh.tsv", "exec/facts/nativeabi-gen-reject.tsv", "exec/facts/top-modelgraphequality-banks.tsv", "exec/facts/export.py"], nativeabi),
    ("opt-gen", ["weights/gold/peep.tsv", "weights/gold/opinfo.tsv", "exec/facts/opt-gen-constants.tsv", "exec/facts/opt-gen-startwords.tsv", "exec/opt/answer-targets.tsv", "exec/facts/export.py"], optgen),
    ("top-modelbindings-template", ["exec/facts/top-modelbindings-const.tsv", "exec/facts/export.py"], modelbindingstemplate),
    ("structreturnexpr", ["exec/build/parsebase.py", "exec/facts/export.py"], structreturnexpr),
    ("k2-tokenlocations-map", ["exec/facts/tokenlocations.tsv", "exec/facts/export.py"], tokenlocationsmap),
    ("k2-constexpr", ["exec/build/parsebase.py", "exec/parse2/constexpr-operators.tsv", "exec/parse2/constexpr-tokens.tsv", "weights/gold/prec.tsv", "exec/facts/export.py"], constexprentry),
    ("lower-armfuse", ["unisa/tape.py", "exec/lower/armfuse-shapes.tsv", "exec/facts/export.py"], lowerarmfuse),
    ("lower-abi", ["unisa/catalog.py", "unisa/lower.py", "exec/lower/code-abi-sources.tsv", "weights/gold/abi.tsv", "weights/gold/enc.tsv", "exec/facts/export.py"], lowerabi),
    ("lower-code", ["unisa/tape.py", "unisa/lower.py", "weights/gold/regmap.tsv", "weights/gold/enc.tsv", "weights/gold/reloc.tsv", "unisa/emit_x86.py", "unisa/emit_arm.py", "exec/facts/top-modelbindings-banks.tsv", "exec/facts/top-modelbindings-const.tsv", "exec/facts/export.py"], lowercode),
]


def render(stem, inputs, producer):
    head = ["# exec/facts/%s.tsv: written by exec/facts/export.py, do not edit" % stem]
    head += ["# input %s sha256:%s" % (p, hashlib.sha256((ROOT / p).read_bytes()).hexdigest()[:16])
             for p in inputs]
    return "\n".join(head + producer()) + "\n"


def main(argv):
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


def pefields():
    """PE32+ header and import-directory field schema (unisa/image/pe.py)."""
    import json
    sys.path.insert(0, str(ROOT))
    from unisa.image import pe
    n = len(pe.IMPORTS); iat = 40 + 8 * (n + 1); off = iat + 8 * (n + 1)
    names = []
    for name in pe.IMPORTS:
        raw = b"\0\0" + name.encode() + b"\0"; raw += b"\0" * (len(raw) % 2)
        names.append((off, raw)); off += len(raw)
    dlloff = off; off += len(pe.DLL) + 1; cfg = (off + 7) & -8; idlen = cfg + pe.LOADCFG
    consts = dict(DATA=1 << 40, RELOCS=4 << 40, SORTED=5 << 40, TEXT_RVA=pe.TEXT_RVA, IMAGEBASE=pe.IMAGEBASE,
                  HDR_FILE=pe.HDR_FILE, section_minus_one=pe.SECT_ALIGN - 1, section_mask=-pe.SECT_ALIGN,
                  file_minus_one=pe.FILE_ALIGN - 1, file_mask=-pe.FILE_ALIGN,
                  import_vsize=(idlen + pe.SECT_ALIGN - 1) & -pe.SECT_ALIGN,
                  import_fsize=(idlen + pe.FILE_ALIGN - 1) & -pe.FILE_ALIGN,
                  cookie_at=cfg + pe.COOKIE_FIELD, cfg=cfg, iat=iat, dlloff=dlloff)
    out = ["=%s\tint\t%d" % kv for kv in consts.items()]
    used = set()
    for arch in sorted(pe.MACHINE):
        c = _fieldchain()
        c.lit(b"MZ" + bytes(58)); c.field(4, 64); c.lit(b"PE\0\0")
        for w, v in [(2, pe.MACHINE[arch]), (2, 4), (4, 0), (4, 0), (4, 0), (2, 240), (2, 0x22)]: c.field(w, v)
        c.comp("entry-value")
        for w, v in [(2, 0x20B), (1, 14), (1, 0), (4, "pe_tf"), (4, 0), (4, 0), (4, "pe_entry"), (4, pe.TEXT_RVA), (8, pe.IMAGEBASE), (4, pe.SECT_ALIGN), (4, pe.FILE_ALIGN),
                     (2, 4), (2, 0), (2, 0), (2, 0), (2, 4), (2, 0), (4, 0), (4, "pe_img"), (4, pe.HDR_FILE), (4, 0), (2, 3), (2, 0x8160), (8, 0x100000), (8, 0x1000), (8, 0x100000), (8, 0x1000), (4, 0), (4, 16)]:
            c.field(w, v)
        c.comp("directory-values")
        dirs = {1: ("pe_rd", 40), 5: ("pe_rr", "pe_rlsize"), 10: ("pe_cfg", pe.LOADCFG), 12: ("pe_iat", (n + 1) * 8)}
        for i in range(16):
            c.field(4, dirs.get(i, (0, 0))[0]); c.field(4, dirs.get(i, (0, 0))[1])
        for name, rva, vs, fo, fs, flags in [(b".text", pe.TEXT_RVA, "endo", pe.HDR_FILE, "pe_tf", 0x60000020), (b".rdata", "pe_rd", idlen, "pe_rf", (idlen + 511) & -512, 0x40000040),
                                             (b".data", "pe_dt", "pe_dvs", "pe_df", "pe_datafile", 0xC0000040), (b".reloc", "pe_rr", "pe_rlsize", "pe_relf", None, 0x42000040)]:
            if fs is None: c.comp("reloc-align"); fs = "pe_rsize"
            c.lit(name.ljust(8, b"\0"))
            for w, v in [(4, vs), (4, rva), (4, fs), (4, fo), (4, 0), (4, 0), (2, 0), (2, 0), (4, flags)]: c.field(w, v)
        out += _rows("header_" + arch, c.fields) + _rows("header_tail_" + arch, [dict(width=0, value=0, src="none", endian="-", lead=c.lead)])
        used |= {x for r in c.fields + [dict(lead=c.lead)] for x in r["lead"]}
    c = _fieldchain()
    c.comp("import-values")
    for w, v in [(4, "pe_int"), (4, 0), (4, 0), (4, "pe_dll"), (4, "pe_iat")]: c.field(w, v)
    c.lit(bytes(20))
    for _ in range(2):
        for offset, raw in names:
            c.comp("name-value%d" % offset); c.field(8, "pe_name")
        c.field(8, 0)
    for offset, raw in names: c.lit(raw)
    c.lit(pe.DLL + b"\0" + bytes(cfg - (dlloff + len(pe.DLL) + 1)))
    c.field(4, pe.LOADCFG); c.lit(bytes(pe.COOKIE_FIELD - 4)); c.field(8, "pe_cookie"); c.lit(bytes(pe.LOADCFG - pe.COOKIE_FIELD - 8))
    out += _rows("imports", c.fields) + _rows("imports_tail", [dict(width=0, value=0, src="none", endian="-", lead=c.lead)])
    used |= {x for r in c.fields + [dict(lead=c.lead)] for x in r["lead"]}
    out += ["@bytes\tv:int"] + ["\t%d" % int(x[4:]) for x in sorted(used) if x.startswith("byte")]
    out += ["@names\toffset:int"] + ["\t%d" % o for o, _ in names]
    return out


TABLES.append(("enc-pe", ["unisa/image/pe.py", "exec/facts/export.py"], pefields))


def machofields():
    """Mach-O load commands and ad-hoc code-signature field schema (unisa/image/macho.py)."""
    sys.path.insert(0, str(ROOT))
    from unisa.image import macho as M
    out = ["=%s\tint\t%d" % kv for kv in dict(DATA=1 << 40, VMADDR=M.VMADDR, STRTAB=M.STRTAB, page_minus_one=M.PAGE - 1,
                                               page_mask=-M.PAGE, page4_minus_one=M.PAGE4 - 1, PAGE4=M.PAGE4,
                                               signature_base=20 + 88 + len(M.IDENT), DLPREFIX=M.DLPREFIX,
                                               DLBINDLEN=len(M.DLBIND)).items()]
    used = set()
    for arch in sorted(M.CPU):
        out.append("=HDRS_%s\tint\t%d" % (arch, M.HDRS(arch)))
        c = _fieldchain()
        def fields(items, big=False):
            for w, v in items: c.field(w, v, "big" if big else "little")
        def name(v): c.lit(v.ljust(16, b"\0"))
        def seg(n, va, vm, fo, fs, prot, ns):
            fields([(4, M.LC_SEGMENT_64), (4, M.SEG + M.SECT * ns)]); name(n)
            fields([(8, va), (8, vm), (8, fo), (8, fs), (4, prot), (4, prot), (4, ns), (4, 0)])
        def sect(n, s, va, sz, off, flags):
            name(n); name(s); fields([(8, va), (8, sz), (4, off), (4, 2), (4, 0), (4, 0), (4, flags), (4, 0), (4, 0), (4, 0)])
        fields([(4, 0xFEEDFACF), (4, M.CPU[arch][0]), (4, M.CPU[arch][1]), (4, 2), (4, M.NCMDS), (4, M._cmdsz(arch)), (4, 0x200085), (4, 0)])
        seg(b"__PAGEZERO", 0, M.VMADDR, 0, 0, 0, 0)
        seg(b"__TEXT", M.VMADDR, "mh_text", 0, "mh_text", 5, 1)
        sect(b"__text", b"__TEXT", M.VMADDR + M.HDRS(arch), "endo", M.HDRS(arch), 0x80000400)
        seg(b"__DATA", "mh_datava", "mh_vm", "mh_text", "mh_data", 3, 2)
        sect(b"__data", b"__DATA", "mh_datava", "mh_stored", "mh_text", 0)
        sect(b"__bss", b"__DATA", "mh_bssva", "mh_bsslen", 0, 1)
        seg(b"__LINKEDIT", "mh_linkva", "mh_linkvm", "mh_link", "mh_linksz", 1, 0)
        fields([(4, M.LC_LOAD_DYLINKER), (4, 32), (4, 12)])
        c.lit(M.DYLD.ljust(20, b"\0"))
        lib = (M.LIBSYS + b"\0"); lib += bytes((-len(lib)) % 8)
        fields([(4, M.LC_LOAD_DYLIB), (4, 24 + len(lib)), (4, 24), (4, 0), (4, 0x10000), (4, 0x10000)])
        c.lit(lib)
        fields([(4, M.LC_MAIN), (4, 24), (8, "mh_entry"), (8, 0), (4, M.LC_BUILD_VERSION), (4, 24), (4, 1), (4, 13 << 16), (4, 13 << 16), (4, 0)])
        fields([(4, M.LC_DYLD_INFO_ONLY), (4, 48), (4, 0), (4, 0), (4, "mh_bindoff"), (4, len(M.DLBIND))] + [(4, 0)] * 6)
        fields([(4, M.LC_SYMTAB), (4, 24), (4, "mh_link"), (4, 0), (4, "mh_link"), (4, M.STRTAB)])
        fields([(4, M.LC_DYSYMTAB), (4, 80)] + [(4, 0)] * 18)
        fields([(4, M.LC_CODE_SIGNATURE), (4, 16), (4, "mh_sigoff"), (4, "mh_siglen")])
        c.lit(bytes(M.SLACK))
        out += _rows("header_" + arch, c.fields) + _rows("header_tail_" + arch, [dict(width=0, value=0, src="none", endian="-", lead=c.lead)])
        used |= {x for r in c.fields + [dict(lead=c.lead)] for x in r["lead"]}
        c = _fieldchain()
        fields([(4, M.CS_MAGIC_EMBEDDED), (4, "mh_siglen"), (4, 1), (4, 0), (4, 20)], True)
        c.comp("signature-length")
        fields([(4, M.CS_MAGIC_CODEDIRECTORY), (4, "mh_cdlen"), (4, 0x20400), (4, M.CS_ADHOC), (4, 88 + len(M.IDENT)), (4, 88), (4, 0), (4, "mh_slots"), (4, "mh_sigoff"), (1, 32), (1, 2), (1, 0), (1, 12), (4, 0),
                (4, 0), (4, 0), (4, 0), (8, 0), (8, 0), (8, "mh_text"), (8, M.CS_EXECSEG_MAIN_BINARY)], True)
        c.lit(M.IDENT)
        out += _rows("signature_" + arch, c.fields) + _rows("signature_tail_" + arch, [dict(width=0, value=0, src="none", endian="-", lead=c.lead)])
        used |= {x for r in c.fields + [dict(lead=c.lead)] for x in r["lead"]}
    c = _fieldchain(); c.lit(bytes(M.DLPREFIX))
    pre = c.lead
    c = _fieldchain(); c.lit(M.DLBIND)
    out += _rows("prefix", [dict(width=0, value=0, src="none", endian="-", lead=pre)])
    out += _rows("binddata", [dict(width=0, value=0, src="none", endian="-", lead=c.lead)])
    used |= set(pre) | set(c.lead)
    out += ["@bytes\tv:int"] + ["\t%d" % int(x[4:]) for x in sorted(used) if x.startswith("byte")]
    return out


TABLES.append(("enc-macho", ["unisa/image/macho.py", "exec/facts/export.py"], machofields))


def elffields():
    """ELF program-header / dynamic-section field schema and the import/loader slot tables (unisa/image/elf.py, pe.py)."""
    sys.path.insert(0, str(ROOT))
    from unisa.image import elf
    from unisa.image.pe import IMPORTS
    out = ["=%s\tint\t%d" % kv for kv in dict(DATA=1 << 40, RELOCS=4 << 40, iat_size=8 * len(IMPORTS), VADDR=elf.VADDR,
                                               dyn_doff=elf.VADDR + 32, fmt_elf=1, fmt_macho=2, fmt_pe=3).items()]
    used = set()
    def emit(name, c):
        nonlocal used
        out.extend(_rows(name, c.fields) + _rows(name + "_tail", [dict(width=0, value=0, src="none", endian="-", lead=c.lead)]))
        used |= {x for r in c.fields + [dict(lead=c.lead)] for x in r["lead"]}
    c = _fieldchain()
    for i in range(len(IMPORTS)): c.field(8, "ml_imp_%d" % i)
    emit("iat", c)
    c = _fieldchain()
    for i in range(4): c.field(8, "ml_dl_%d" % i)
    emit("dlslots", c)
    c = _fieldchain(); c.lit(bytes(32)); emit("dynslots", c)
    for arch in sorted(elf.MACHINE):
        out.append("=HDRS_%s\tint\t%d" % (arch, elf.HDRS(arch)))
        def preamble(c, phnum):
            c.lit(b"\x7fELF" + bytes([2, 1, 1, 0]) + bytes(8))
            for w, v in [(2, 2), (2, elf.MACHINE[arch]), (4, 1), (8, "entryva"), (8, elf.EHDR), (8, 0),
                         (4, 0), (2, elf.EHDR), (2, elf.PHDR), (2, phnum), (2, 0), (2, 0), (2, 0)]: c.field(w, v)
        def phdr(c, typ, flags, off, va, filesz, memsz, align):
            for w, v in [(4, typ), (4, flags), (8, off), (8, va), (8, va), (8, filesz), (8, memsz), (8, align)]: c.field(w, v)
        c = _fieldchain(); c.comp("header-init")
        preamble(c, 2)
        phdr(c, 1, 5, 0, elf.VADDR, "tend", "tend", elf.PAGE)
        phdr(c, 1, 6, "doff", "data_va", "stored", "memlen", elf.PAGE)
        emit("static_" + arch, c)
        c = _fieldchain(); c.comp("dyn-init")
        preamble(c, 4)
        interp = b"/lib/ld-linux-aarch64.so.1" if arch == "arm64" else b"/lib64/ld-linux-x86-64.so.2"
        phdr(c, 3, 4, 288, elf.VADDR + 288, len(interp) + 1, len(interp) + 1, 1)
        phdr(c, 1, 5, 0, elf.VADDR, "tend", "tend", elf.PAGE)
        phdr(c, 1, 6, "doff", "dyn_slots", "dyn_filesz", "dyn_memsz", elf.PAGE)
        phdr(c, 2, 4, 608, elf.VADDR + 608, 176, 176, 8)
        c.lit(interp + b"\0" + bytes(32 - len(interp) - 1))
        c.lit(b"\0libc.so.6\0dlopen\0dlsym\0dlclose\0dlerror\0")
        c.lit(bytes(24))
        for nameoff in (11, 18, 24, 32):
            c.field(4, nameoff); c.lit(bytes((0x12, 0, 0, 0))); c.field(8, 0); c.field(8, 0)
        c.field(4, 1); c.field(4, 5)
        c.lit(bytes(24))
        for i in range(4):
            c.comp("dynreloc%d" % (8 * i))
            c.field(8, "dyn_reloc"); c.field(8, ((i + 1) << 32) | (1025 if arch == "arm64" else 6)); c.field(8, 0)
        for tag, val in ((1, 1), (4, elf.VADDR + 480), (5, elf.VADDR + 320), (6, elf.VADDR + 360),
                         (10, 40), (11, 24), (7, elf.VADDR + 512), (8, 96), (9, 24), (30, 8), (0, 0)):
            c.field(8, tag); c.field(8, val)
        emit("dyn_" + arch, c)
    out += ["@relocs\toff:int"] + ["\t%d" % (8 * i) for i in range(4)]
    out += ["@bytes\tv:int"] + ["\t%d" % int(x[4:]) for x in sorted(used) if x.startswith("byte")]
    return out


TABLES.append(("enc-elf", ["unisa/image/elf.py", "unisa/image/pe.py", "exec/facts/export.py"], elffields))


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
    ids += [("@" + k, "h_" + k) for k in ("target", "data", "sym", "src_os", "data_len", "bss", "relocs", "argc", "argv")]
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
             'winrest': ('ir', 34), 'winstdh': ('i', 35), 'winargs': ('iii', 36), 'itoa': ('iii', 37)}
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

def k2strings():
    """String rejection text and escape data from the existing domain declarations."""
    records = [ln.split("\t", 2) for ln in (ROOT / "exec/facts/strings.tsv").read_text().splitlines()
               if ln and not ln.startswith("#")]
    reject = {name: json.loads(value) for kind, name, value in records if kind == "reject"}
    escape = {json.loads(name): int(value) for kind, name, value in records if kind == "escape"}
    reserved = {ord(c) for c in "01234567x"}
    entries = [{"byte": ord(k), "value": v} for k, v in escape.items() if ord(k) not in reserved]
    parse_constants = dict(ln.split("\t", 1) for ln in (ROOT / "exec/facts/parse-constants.tsv").read_text().splitlines()
                           if ln and not ln.startswith("#"))
    compact = lambda value: json.dumps(value, separators=(",", ":"))
    return ["=rej\tjson\t" + compact(reject), "=escape\tjson\t" + compact(entries),
            "=escbytes\tjson\t" + compact([row["byte"] for row in entries]),
            "=TK_STR\tint\t" + parse_constants["TK_STR"]]


TABLES.append(("k2-strings", ["exec/facts/strings.tsv",
                              "exec/facts/parse-constants.tsv", "exec/facts/export.py"], k2strings))


def k2unitlocations():
    """Storage key for located unit framing; transition actions stay in the manifest."""
    return ["# Located unit map storage key; actions live in unitlocations-manifest.tsv.",
            "=MAPS\tint\t" + str(49 << 40)]


TABLES.append(("k2-unitlocations", ["exec/facts/export.py"], k2unitlocations))


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


TABLES.append(("k2-membercontrol", ["exec/build/parsebase.py", "exec/build/parse2base.py", "exec/parse2/membercontrol-template.tsv",
                                    "exec/parse2/membercontrol-tokens.tsv", "exec/parse2/membercontrol-operators.tsv",
                                    "exec/parse2/membercontrol-fresh.tsv", "exec/parse2/membercontrol-manifest-fresh.tsv",
                                    "exec/parse2/membercontrol-text.tsv", "exec/parse2/membercontrol-stack.tsv",
                                    "exec/parse2/tape-templates.tsv", "exec/facts/export.py"], k2membercontrol))


def k2truth():
    """Scalar truth opcodes from the declared output variants; actions remain in scalar TSVs."""
    rows = [ln.split("\t") for ln in (ROOT / "exec/parse2/scalar-outputs.tsv").read_text().splitlines()[1:]]
    variants = [(group, entry, argument) for group, entry, _, argument, _ in rows
                if group in ("truth", "not")]
    assert variants == [("truth", "FT.double", "64"), ("truth", "FT.float", "32"),
                        ("not", "FN.double", "feq64"), ("not", "FN.float", "feq32"),
                        ("not", "FN.integer", "eq")]
    names = ("TRUTH64_OP", "TRUTH32_OP", "NOT64_OP", "NOT32_OP", "NOTINT_OP")
    return ["=" + name + "\tstr\t" + ("feq" + argument if group == "truth" else argument)
            for name, (group, _, argument) in zip(names, variants)]


def k2stack():
    """Value-stack layout: ordered value/rank slots. Actions are declared by consumers."""
    sys.path.insert(0, str(HERE))
    from load import facts as F
    vs = {r["name"]: r["value"] for r in F("parse-constants")}["VS"]
    rank = {r["name"]: r["value"] for r in F("valueranks") if r["kind"] == "rank"}
    out = ["=VS\tint\t%d" % vs]
    for stem, file in STACKS:
        for ln in (ROOT / file).read_text().splitlines()[1:]:
            name, value = ln.split("\t")
            method, items = json.loads(value)
            assert method in ("vpush", "vpop")
            slots = [slot for item in items for slot in
                     ([item, rank[item]] if rank.get(item) and rank[item] not in items else [item])]
            if method == "vpop":
                slots.reverse()
            out.append("=stack_%s_%s\tjson\t%s" %
                       (stem, name, json.dumps([dict(register=x) for x in slots])))
    return out


STACKS = [("initializers", "exec/parse2/initializers-stack.tsv"), ("intwarnings", "exec/parse2/intwarnings-stack.tsv"),
          ("returnwarnings", "exec/parse2/returnwarnings-stack.tsv"), ("formatwarnings", "exec/parse2/formatwarnings-stack.tsv"),
          ("membercontrol", "exec/parse2/membercontrol-stack.tsv")]
TABLES.append(("k2-stack", ["exec/facts/parse-constants.tsv", "exec/facts/valueranks.tsv"] + [f for _, f in STACKS]
               + ["exec/facts/export.py"], k2stack))


TABLES.append(("k2-truth", ["exec/parse2/scalar-outputs.tsv", "exec/facts/export.py"], k2truth))


def k2librarymodule():
    """Finite header and main-key domains; continuation labels stay in the manifest."""
    source = {}
    for line in (HERE / "k2-librarymodule.tsv").read_text().splitlines():
        if line.startswith("="):
            name, kind, value = line[1:].split("\t", 2)
            source[name] = json.loads(value) if kind == "json" else value
    lo, hi = source["hdomain"]
    mlo, mhi = source["maindomain"]
    ok = set(source["mainok"])
    compact = lambda x: json.dumps(x, separators=(",", ":"))
    return ["=hkeys\tjson\t" + compact(list(range(lo, hi))),
            "=mainbad\tjson\t" + compact([k for k in range(mlo, mhi) if k not in ok]),
            "=mainreason\tstr\t" + source["mainreason"]]


TABLES.append(("k2-librarymodule-map", ["exec/facts/k2-librarymodule.tsv", "exec/facts/export.py"],
               k2librarymodule))



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


TABLES.append(("k2-libraryenv", ["exec/build/parsebase.py", "exec/build/parse2base.py", "exec/build/parsebase.py", "exec/parse2/librarycallables-result.tsv",
               "exec/facts/libraryexports.tsv", "exec/facts/librarycallables.tsv", "exec/facts/libraryimports.tsv",
               "exec/facts/libraryvariadic.tsv", "exec/facts/unresolved.tsv", "exec/facts/valueranks.tsv",
               "exec/facts/k2-gen2.tsv", "exec/facts/k2-librarymodule-map.tsv", "exec/facts/export.py"], k2libraryenv))

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
