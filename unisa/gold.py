"""Gold tables. [G]

Every stage declares [(field, vocab)] plus a total label function.  The gold
corpus is the FULL cartesian product of the field vocabs -- that is what makes
stage equivalence decidable by enumeration rather than by testing. [G-0] [P-3]
"""
from itertools import product
from . import catalog as C

# ---------------------------------------------------------------- parse [G-1]
NT = ("top", "stmt", "unary", "postfix", "after_name")
TOKS = ("eof", "type", "id", "num", "str", "if", "else", "while", "for", "do",
        "switch", "case", "default", "return", "break", "continue", "sizeof",
        "struct", "typedef", "enum", "{", "}", "(", ")", "[", "]", ";", ",",
        "=", "+=", "-=", "*=", "/=", "?", ":", "+", "-", "*", "/", "%",
        "==", "!=", "<", ">", "<=", ">=", "&&", "||", "!", "&", "++", "--",
        ".", "->", "goto", "|", "^", "<<", ">>", "union",
        "~", "%=", "&=", "|=", "^=", "<<=", ">>=", "...")
PRODS = ("end", "fn", "global", "typedef", "struct", "enum", "decl", "if",
         "while", "for", "do", "switch", "case", "default", "return", "break",
         "continue", "block", "expr", "neg", "not", "deref", "addr", "sizeof",
         "prim", "index", "call", "inc", "field", "done", "fn_sig", "var_def",
         "goto", "bnot", "preinc", "uplus")
PARSE_DEFAULT = {"top": "global", "stmt": "expr", "unary": "prim",
                 "postfix": "done", "after_name": "var_def"}
SAME_NAME = ("if", "while", "for", "do", "switch", "case", "default",
             "return", "break", "continue", "goto")


def parse_label(nt, tok):
    if nt == "top":
        if tok == "eof":
            return "end"
        if tok in ("typedef", "struct", "enum"):
            return tok
        if tok == "union":
            return "struct"        # a union declaration is the same production
    elif nt == "after_name":
        if tok == "(":
            return "fn_sig"
        if tok == "{":
            # the declarator already took the parameter list, as in
            # `int (*f(int, int))(int, int) { ... }`
            return "fn_sig"
    elif nt == "stmt":
        # [G-1 corrected] struct/enum/typedef at statement start always begin a
        # declaration in C99 -- there is no expression form.  v1's stmt row only
        # had `type`, which sent `struct P p;` to the `expr` default.
        if tok in ("type", "struct", "enum", "typedef", "union"):
            return "decl"
        if tok == "{":
            return "block"
        if tok in SAME_NAME:
            return tok
    elif nt == "unary":
        return {"-": "neg", "+": "uplus", "!": "not", "~": "bnot",
                "*": "deref", "&": "addr", "sizeof": "sizeof",
                "++": "preinc", "--": "preinc"}.get(tok, "prim")
    elif nt == "postfix":
        if tok == "[":
            return "index"
        if tok == "(":
            return "call"
        if tok in ("++", "--"):
            return "inc"
        if tok in (".", "->"):
            return "field"
    return PARSE_DEFAULT[nt]


# ----------------------------------------------------------------- type [G-2]
TYS = ("void", "i8", "i16", "i32", "i64",
       "u8", "u16", "u32", "u64", "ptr", "arr", "struct", "fn",
       "f32", "f64")
TOPS = ("+", "-", "*", "/", "%", "<", "==", "=", "&", "[]", ".", "call",
        "sizeof", ",", "un*", "|", "^", "<<", ">>")
TYOUT = TYS + ("illegal",)
NUM = ("i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64")
FLT = ("f32", "f64")                  # float, double (long double is double)


def _farith(t1, t2):
    """C99 6.3.1.8 for a floating operand: the wider floating type wins, and
    an integer meets a float as that float.  FLT_EVAL_METHOD is 0 on both
    our ISAs (SSE, AArch64 FP), so float OP float stays float."""
    return "f64" if "f64" in (t1, t2) else "f32"
RANK = {"i8": 1, "u8": 1, "i16": 2, "u16": 2,
        "i32": 3, "u32": 3, "i64": 4, "u64": 4}
TY_SIZE = {"void": 1, "i8": 1, "i16": 2, "i32": 4,
           "u8": 1, "u16": 2, "u32": 4}


def _narrow(t1, t2):
    """char and short promote to int; the walker keeps wider
    arithmetic in i64. [G-2]"""
    return RANK.get(t1, 9) < 3 or RANK.get(t2, 9) < 3


def _promote(t):
    """C99 6.3.1.1: anything of lower rank than int becomes int -- signed,
    because int can represent every value of unsigned char and short."""
    return "i32" if RANK.get(t, 9) < 3 else t


def _uns(t1, t2):
    """C99 6.3.1.8, the usual arithmetic conversions, reduced to the one
    question this table asks: is the RESULT unsigned?"""
    a, b = _promote(t1), _promote(t2)
    ua, ub = a[0] == "u", b[0] == "u"
    if not (ua or ub):
        return False
    if ua and ub:
        return True
    u, sg = (a, b) if ua else (b, a)
    # the signed type wins only when it can represent every unsigned value
    return RANK.get(u, 9) >= RANK.get(sg, 9)


def _arith(t1, t2):
    """The result kind of an arithmetic or bitwise operation: C99 6.3.1.8,
    the usual arithmetic conversions.  After the integer promotions the
    result has the rank of the WIDER operand, signed or unsigned.

    [G-2 retired, 2026-09-24] The signed rows used to say "a narrow operand
    gives i32, otherwise i64" -- so `int + int` was a long and `short +
    long` an int, both wrong.  It was a shortcut from before unsigned
    existed, and every table test passed while it stood, because the table
    agreed with ITSELF: enumeration proves the net equals the gold, never
    that the gold is C.  It surfaced as `sizeof(1 + 0) == 8` in corpus
    00200, a test about exactly this rule."""
    r = max(RANK.get(_promote(t1), 9), RANK.get(_promote(t2), 9))
    if _uns(t1, t2):
        return "u64" if r >= 4 else "u32"
    return "i64" if r >= 4 else "i32"


def type_label(t1, op, t2):
    if op == "sizeof":
        return "i64"
    if op == "un*":
        return "i64" if t1 == "ptr" else "illegal"
    if op == "call":
        return "i64" if t1 == "fn" else "illegal"
    if op == "[]":
        return "i64" if t1 in ("ptr", "arr") else "illegal"
    if op == ".":
        return "i64" if t1 == "struct" else "illegal"
    if op == "=":
        return t1
    if op == "&":
        # `&` is overloaded in C: bitwise when both sides are numeric,
        # address-of otherwise (the i64&i64 -> ptr row the walker uses).
        if t1 in NUM and t2 in NUM and not (t1 == "i64" and t2 == "i64"):
            return _arith(t1, t2)
        return "ptr" if (t1 == "i64" and t2 == "i64") else "illegal"
    if op in ("<<", ">>"):
        # [G-2 corrected] C99 6.5.7p3: the integer promotions are performed on
        # EACH operand and the result is the type of the PROMOTED LEFT one.
        # The usual arithmetic conversions do NOT apply here, so
        # `(short)1 << 1L` is an int, not a long -- which is the whole of
        # corpus 00200.  acc read 1.000 while this was wrong.
        if t1 in NUM and t2 in NUM:
            return _promote(t1)
        return "illegal"
    if op in ("|", "^"):
        if t1 in NUM and t2 in NUM:
            return _arith(t1, t2)
        return "illegal"
    if op == ",":
        return t2                      # [G-2 corrected] the comma operator
                                       # yields its right operand; v1 had no
                                       # rule so it fell through to illegal
    if op in ("<", "=="):
        if (t1 in FLT or t2 in FLT) and not (t1 in NUM + FLT and t2 in NUM + FLT):
            return "illegal"
        return "i64"
    if op in ("+", "-", "*", "/") and (t1 in FLT or t2 in FLT):
        # arithmetic with a floating operand; % and the bitwise operators
        # have no floating rows (6.5.5p2, 6.5.10-12) and stay illegal below
        if t1 in NUM + FLT and t2 in NUM + FLT:
            return _farith(t1, t2)
        return "illegal"
    if op in ("+", "-", "*", "/", "%"):
        if op in ("+", "-") and t1 in ("ptr", "arr") and t2 in NUM:
            # [G-2 corrected] v1 had `ptr|arr + num -> ptr` but no rule for
            # `ptr - num`, so `p - 1` fell through to illegal -> i64 and the
            # deref then loaded 8 bytes instead of the element width.  Caught by
            # differential testing (tests/c/b_negidx.c); acc read 1.000 throughout.
            return "ptr"
        if op == "+" and t1 in NUM and t2 in ("ptr", "arr"):
            # C99 6.5.6p2: either operand of + may be the pointer.  With no
            # row, `1 + (int *)p` was an integer sum that moved ONE byte --
            # which is how <math.h>'s high word came out of the wrong place
            return "ptr"
        if op == "-" and t1 == "ptr" and t2 == "ptr":
            return "i64"
        if t1 in NUM and t2 in NUM:
            return _arith(t1, t2)
    return "illegal"


# ---------------------------------------------------------------- scope [G-3]
CTX = ("top", "param", "local", "expr", "sizeof", "field")
KIND = ("type_kw", "id", "typedef_id", "star", "lparen")
ACTS = ("bind_global", "bind_param", "bind_local", "lookup", "type_name",
        "fn_name", "field")
TYPEKW = ("type_kw", "typedef_id")


def scope_label(ctx, kind):
    if ctx == "top":
        if kind in TYPEKW:
            return "type_name"
        if kind == "id":
            return "bind_global"
        if kind == "lparen":
            return "fn_name"
    elif ctx == "param":
        if kind == "id":
            return "bind_param"
    elif ctx == "local":
        if kind == "id":
            return "bind_local"
        if kind in TYPEKW:
            return "type_name"
    elif ctx == "sizeof":
        if kind in TYPEKW:
            return "type_name"
    elif ctx == "field":
        if kind == "id":
            return "field"
    return "lookup"


# ------------------------------------------------------------------- pp [G-4]
DIRS = ("ifdef", "ifndef", "if", "elif", "else", "endif", "define",
        "include", "undef")
DEFINED = ("0", "1")
PPACT = ("take", "skip", "pop", "macro")


def pp_label(d, f):
    on = (f == "1")
    if d == "endif":
        return "pop"
    if d in ("define", "undef"):
        return "macro"
    if d == "include":
        # `include` mutates translation state and emits no tokens -- the same
        # shape as define/undef, so it belongs to `macro`.  It was `skip` only
        # while the preprocessor had no include mechanism.  [G-4 corrected]
        return "macro"
    if d == "ifdef":
        return "take" if on else "skip"
    if d == "ifndef":
        return "take" if not on else "skip"
    if d in ("if", "elif", "else"):
        return "take" if on else "skip"
    return "skip"


# ------------------------------------------------------------------ lex [G-5]
CHARC = ("ws", "nl", "A", "d", "q", "sq", "slash", "star", "punct", "eof",
         "other", "dot")
LEXACT = ("skip", "nl", "ident", "num", "str", "charlit", "cmt", "linecmt",
          "op", "bad")


def lex_label(c, peek):
    if c == "slash":
        if peek == "slash":
            return "linecmt"
        if peek == "star":
            return "cmt"
    if c == "dot":
        # `.5` is a floating constant (C99 6.4.4.2); every other `.` is the
        # member operator or part of `...`
        return "num" if peek == "d" else "op"
    return {"ws": "skip", "nl": "nl", "A": "ident", "d": "num", "q": "str",
            "sq": "charlit", "slash": "op", "star": "op", "punct": "op",
            "eof": "skip", "other": "bad"}[c]


# ---------------------------------------------------------------- reloc [G-7]
JMPKIND = ("jmp", "jz", "call")
RELKIND = ("rel32", "arm26", "arm19")


def reloc_label(k, arch):
    if arch == "x86_64":
        return "rel32"
    return "arm19" if k == "jz" else "arm26"


# ---------------------------------------------------------------- irsel [G-8]
FAMILY = ("alu", "mem", "ctrl", "call", "lit", "fpu")
IRSEL_MAP = {
    # `_rev`: the same op with its two sources swapped -- a > b is b < a.
    # The swap is part of the answer, so no front end decides it in code.
    "alu": {"add": "add64", "sub": "sub64", "mul": "mul64", "lt": "slt64",
            "le": "sle64", "gt": "slt64_rev", "ge": "sle64_rev", "eq": "eq",
            "ne": "ne", "neg": "sub64", "and": "and64", "or": "or64",
            "xor": "xor64", "shl": "shl64", "shr": "shr64",
            "ult": "ult64", "ule": "ule64", "ugt": "ult64_rev",
            "uge": "ule64_rev", "lshr": "lshr64"},
    "mem": {"load": "load64", "store": "store64", "lea": "lea", "ld": "ld",
            "st": "st", "zero": "zero"},
    "ctrl": {"jump": "jump", "jumpz": "jumpz", "ret": "ret"},
    "call": {"call": "call", "push": "callpush", "arg": "arg",
             "frame": "frame", "callr": "callr"},
    "lit": {"imm": "imm", "print": "print", "write": "write", "exit": "exit"},
    # floating point: values travel as IEEE bit patterns in the general
    # registers (the tape convention is ours on both ends of every call), so
    # these are the only ops that look inside them.  d = double, s = float.
    "fpu": {"dadd": "fadd64", "dsub": "fsub64", "dmul": "fmul64",
            "ddiv": "fdiv64", "dlt": "flt64", "dle": "fle64", "deq": "feq64",
            "sadd": "fadd32", "ssub": "fsub32", "smul": "fmul32",
            "sdiv": "fdiv32", "slt": "flt32", "sle": "fle32", "seq": "feq32",
            "i2d": "cvtid", "u2d": "cvtud", "i2s": "cvtis", "u2s": "cvtus",
            "d2i": "cvtdi", "d2u": "cvtdu", "s2d": "cvtsd", "d2s": "cvtds",
            "dsqrt": "fsqrt64", "ssqrt": "fsqrt32",
            "dgt": "flt64_rev", "dge": "fle64_rev",
            "sgt": "flt32_rev", "sge": "fle32_rev"},
}
FLAVOR = tuple(dict.fromkeys(f for m in IRSEL_MAP.values() for f in m))
RECIPE = tuple(sorted({r for m in IRSEL_MAP.values() for r in m.values()})) + ("bad",)


def irsel_label(fam, flav):
    return IRSEL_MAP[fam].get(flav, "bad")


# ================================================================== registry
class Stage:
    def __init__(self, name, fields, heads, label, cfg, weight=None):
        self.name = name
        self.fields = fields          # [(fname, vocab tuple)]
        self.heads = heads            # [(hname, classes tuple, share|None)]
        self.label = label            # fn(*key_values) -> {head: class}
        self.cfg = cfg                # net hyperparameters
        self.weight = weight          # fn(labels) -> float, training only
        self._idx = [{v: i for i, v in enumerate(vo)} for (_, vo) in fields]
        self._hidx = [{c: i for i, c in enumerate(cl)} for (_, cl, _) in heads]

    def keys(self):
        return list(product(*[vo for (_, vo) in self.fields]))

    def corpus(self):
        """FULL gold: every key, with class indices. [G-0]"""
        out = []
        for kv in self.keys():
            ki = tuple(self._idx[i][v] for i, v in enumerate(kv))
            lab = self.label(*kv)
            li = {hn: self._hidx[j][lab[hn]]
                  for j, (hn, _, _) in enumerate(self.heads)}
            out.append((ki, li))
        return out

    def train_corpus(self):
        base = self.corpus()
        if self.weight is None:
            return [(k, l, 1.0) for (k, l) in base]
        out = []
        for (ki, li), kv in zip(base, self.keys()):
            out.append((ki, li, self.weight(self.label(*kv))))
        return out

    def rows(self):
        n = 1
        for (_, vo) in self.fields:
            n *= len(vo)
        return n


def _one(fn):
    return lambda *k: {"y": fn(*k)}


# [G-2a] v1 said "keep 1/19 illegal in train" to balance the 611/960 illegal
# majority.  MEASURED: that downweighting is what pins type at 0.999 -- the one
# row it loses is (i64,&,i32)->illegal, a direct neighbour of the single
# positive (i64,&,i64)->ptr, and the tightest margins in the whole table are all
# on '&'.  Class balance protects generalization; there is no generalization
# here to protect, only memorization, so starving 64% of the domain only blinds
# the net next to the sparse positives.  1.0 => type reaches 1.000.  [F-3]
TYPE_ILLEGAL_WEIGHT = 1.0        # set to 1/19 to reproduce the v1 behaviour


def _type_weight(lab):
    return TYPE_ILLEGAL_WEIGHT if lab["y"] == "illegal" else 1.0


def build():
    S = {}
    S["pp"] = Stage("pp", [("dir", DIRS), ("defined", DEFINED)],
                    [("y", PPACT, None)], _one(pp_label),
                    dict(d=6, hidden=[12], seed=23))
    S["lex"] = Stage("lex", [("c", CHARC), ("peek", CHARC)],
                     [("y", LEXACT, None)], _one(lex_label),
                     dict(d=6, hidden=[12], seed=31))
    S["parse"] = Stage("parse", [("nt", NT), ("tok", TOKS)],
                       [("y", PRODS, None)], _one(parse_label),
                       dict(d=8, hidden=[16], seed=13))
    S["type"] = Stage("type", [("t1", TYS), ("op", TOPS), ("t2", TYS)],
                      [("y", TYOUT, None)], _one(type_label),
                      dict(d=8, hidden=[32], seed=17), weight=_type_weight)
    S["scope"] = Stage("scope", [("ctx", CTX), ("kind", KIND)],
                       [("y", ACTS, None)], _one(scope_label),
                       dict(d=8, hidden=[16], seed=19))
    S["irsel"] = Stage("irsel", [("family", FAMILY), ("flavor", FLAVOR)],
                       [("y", RECIPE, None)], _one(irsel_label),
                       dict(d=8, hidden=[16], seed=11))
    S["enc"] = Stage("enc", [("op", C.OPS), ("os", C.OS), ("arch", C.ARCH)],
                     [("y", C.FORMS, None)], _one(C.enc_form),
                     dict(d=8, hidden=[16], seed=29))
    S["reloc"] = Stage("reloc", [("kind", JMPKIND), ("arch", C.ARCH)],
                       [("y", RELKIND, None)], _one(reloc_label),
                       dict(d=6, hidden=[8], seed=37))

    # what a scalar type IS, for code generation: its size, whether it is
    # unsigned, whether it is narrower than a register.  These were three
    # dicts the front ends read (TY_SIZE, UNSIGNED, NARROW). [C-9]
    _SZ = {"void": 1, "i8": 1, "i16": 2, "i32": 4, "u8": 1, "u16": 2,
           "u32": 4, "f32": 4}
    S["tyinfo"] = Stage(
        "tyinfo", [("t", TYOUT)],
        [("size", ("1", "2", "4", "8"), None), ("uns", ("0", "1"), None),
         ("narrow", ("0", "1"), None)],
        lambda t: {"size": str(_SZ.get(t, 8)),
                   "uns": "1" if t in ("u8", "u16", "u32", "u64") else "0",
                   "narrow": "1" if t in ("i8", "i16", "i32", "u8", "u16",
                                          "u32") else "0"},
        dict(d=6, hidden=[12], seed=43))

    # which routine prints a printf conversion -- the letter was an if-chain
    # in both front ends [C-10]
    _PF = {"d": "int", "i": "int", "u": "u32", "x": "hex", "X": "HEX",
           "o": "oct", "p": "hex", "c": "chr", "s": "str"}
    S["pfconv"] = Stage("pfconv", [("conv", tuple(_PF))],
                        [("y", ("int", "u32", "hex", "HEX", "oct", "chr",
                                "str"), None)],
                        _one(lambda c: _PF[c]), dict(d=6, hidden=[8], seed=47))

    # which machine register holds tape register rN -- a table, so a stage
    # (it was a dict both back ends read directly) [C-8]
    TREGS = ("r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7")
    MREGS = tuple(sorted(set(C.REGMAP["x86_64"]) | set(C.REGMAP["arm64"])))
    S["regmap"] = Stage("regmap", [("treg", TREGS), ("arch", C.ARCH)],
                        [("y", MREGS, None)],
                        _one(lambda t, a: C.REGMAP[a][int(t[1])]),
                        dict(d=6, hidden=[12], seed=41))

    SYMS = C.vocab("symbol")
    SYSNOS = C.vocab("sysno")
    S["isel"] = Stage("isel", [("op", C.OPS), ("arch", C.ARCH)],
                      [("form", C.FORMS, None), ("symbol", SYMS, None)],
                      C.isel_two,
                      dict(dims=(12, 8), hidden=[24, 16], seed=3))
    # [S-2 corrected] gate is an OS fact, so it lives here, not on isel.
    abi_heads = [("sysno", SYSNOS, None), ("arg0", C.REGS, "reg"),
                 ("arg1", C.REGS, "reg"), ("arg2", C.REGS, "reg"),
                 ("arg3", C.REGS, "reg"), ("arg4", C.REGS, "reg"),
                 ("arg5", C.REGS, "reg"), ("ret", C.REGS, "reg"),
                 ("gate", C.GATES, None),
                 ("nrreg", C.vocab("nrreg"), None)]
    S["abi"] = Stage("abi", [("op", C.OPS), ("os", C.OS), ("arch", C.ARCH)],
                     abi_heads,
                     lambda op, o, a: {k: v for k, v in C.nine(op, o, a).items()
                                       if k in ("sysno", "arg0", "arg1", "arg2",
                                                "arg3", "arg4", "arg5",
                                                "ret", "gate",
                                                "nrreg")},
                     dict(dims=(12, 8, 8), hidden=[32, 20], seed=5,
                          bilinear=8))
    S["combo"] = Stage("combo", [("op", C.OPS), ("os", C.OS), ("arch", C.ARCH)],
                       [("form", C.FORMS, None), ("symbol", SYMS, None)]
                       + abi_heads,
                       C.nine,
                       dict(dims=(16, 8, 8), hidden=[48, 32], seed=7,
                            factor=12, bilinear=8))
    return S


STAGES = build()
# the keyword set the C lexer needs, kept next to TOKS so they stay in step
KEYWORDS_C = tuple(t for t in TOKS if t[0].isalpha() and t != "eof")

TABLES = ("pp", "lex", "parse", "type", "scope", "irsel", "enc", "reloc",
          "regmap", "tyinfo", "pfconv")
STAGE_NETS = ("isel", "abi")
ALL = TABLES + STAGE_NETS + ("combo",)
