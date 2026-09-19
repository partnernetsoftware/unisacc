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
        ".", "->", "goto", "|", "^", "<<", ">>", "union")
PRODS = ("end", "fn", "global", "typedef", "struct", "enum", "decl", "if",
         "while", "for", "do", "switch", "case", "default", "return", "break",
         "continue", "block", "expr", "neg", "not", "deref", "addr", "sizeof",
         "prim", "index", "call", "inc", "field", "done", "fn_sig", "var_def",
         "goto")
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
        return {"-": "neg", "!": "not", "*": "deref", "&": "addr",
                "sizeof": "sizeof"}.get(tok, "prim")
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
TYS = ("void", "i8", "i16", "i32", "i64", "ptr", "arr", "struct", "fn")
TOPS = ("+", "-", "*", "/", "%", "<", "==", "=", "&", "[]", ".", "call",
        "sizeof", ",", "un*", "|", "^", "<<", ">>")
TYOUT = TYS + ("illegal",)
NUM = ("i8", "i16", "i32", "i64")
TY_SIZE = {"void": 1, "i8": 1, "i16": 2, "i32": 4}


def _narrow(t1, t2):
    """char and short promote to int; the walker keeps wider
    arithmetic in i64. [G-2]"""
    return t1 in ("i8", "i16") or t2 in ("i8", "i16")


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
            return "i32" if _narrow(t1, t2) else "i64"
        return "ptr" if (t1 == "i64" and t2 == "i64") else "illegal"
    if op in ("|", "^", "<<", ">>"):
        if t1 in NUM and t2 in NUM:
            return "i32" if _narrow(t1, t2) else "i64"
        return "illegal"
    if op == ",":
        return t2                      # [G-2 corrected] the comma operator
                                       # yields its right operand; v1 had no
                                       # rule so it fell through to illegal
    if op in ("<", "=="):
        return "i64"
    if op in ("+", "-", "*", "/", "%"):
        if op in ("+", "-") and t1 in ("ptr", "arr") and t2 in NUM:
            # [G-2 corrected] v1 had `ptr|arr + num -> ptr` but no rule for
            # `ptr - num`, so `p - 1` fell through to illegal -> i64 and the
            # deref then loaded 8 bytes instead of the element width.  Caught by
            # differential testing (tests/c/b_negidx.c); acc read 1.000 throughout.
            return "ptr"
        if op == "-" and t1 == "ptr" and t2 == "ptr":
            return "i64"
        if t1 in NUM and t2 in NUM:
            return "i32" if _narrow(t1, t2) else "i64"
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
         "other")
LEXACT = ("skip", "nl", "ident", "num", "str", "charlit", "cmt", "linecmt",
          "op", "bad")


def lex_label(c, peek):
    if c == "slash":
        if peek == "slash":
            return "linecmt"
        if peek == "star":
            return "cmt"
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
FAMILY = ("alu", "mem", "ctrl", "call", "lit")
IRSEL_MAP = {
    "alu": {"add": "add64", "sub": "sub64", "mul": "mul64", "lt": "slt64",
            "le": "sle64", "gt": "slt64", "ge": "sle64", "eq": "eq",
            "ne": "ne", "neg": "sub64", "and": "and64", "or": "or64",
            "xor": "xor64", "shl": "shl64", "shr": "shr64"},
    "mem": {"load": "load64", "store": "store64", "lea": "lea", "ld": "ld",
            "st": "st", "zero": "zero"},
    "ctrl": {"jump": "jump", "jumpz": "jumpz", "ret": "ret"},
    "call": {"call": "call", "push": "callpush", "arg": "arg",
             "frame": "frame", "callr": "callr"},
    "lit": {"imm": "imm", "print": "print", "write": "write", "exit": "exit"},
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
                      dict(d=8, hidden=[20], seed=17), weight=_type_weight)
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

    SYMS = C.vocab("symbol")
    SYSNOS = C.vocab("sysno")
    S["isel"] = Stage("isel", [("op", C.OPS), ("arch", C.ARCH)],
                      [("form", C.FORMS, None), ("symbol", SYMS, None)],
                      C.isel_two,
                      dict(dims=(12, 8), hidden=[24, 16], seed=3))
    # [S-2 corrected] gate is an OS fact, so it lives here, not on isel.
    abi_heads = [("sysno", SYSNOS, None), ("arg0", C.REGS, "reg"),
                 ("arg1", C.REGS, "reg"), ("arg2", C.REGS, "reg"),
                 ("ret", C.REGS, "reg"), ("tls", C.TLS, None),
                 ("gate", C.GATES, None)]
    S["abi"] = Stage("abi", [("op", C.OPS), ("os", C.OS), ("arch", C.ARCH)],
                     abi_heads,
                     lambda op, o, a: {k: v for k, v in C.nine(op, o, a).items()
                                       if k in ("sysno", "arg0", "arg1", "arg2",
                                                "ret", "tls", "gate")},
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

TABLES = ("pp", "lex", "parse", "type", "scope", "irsel", "enc", "reloc")
STAGE_NETS = ("isel", "abi")
ALL = TABLES + STAGE_NETS + ("combo",)
