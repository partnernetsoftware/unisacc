"""UJS gold tables. [G]

Every stage is a total function on a finite closed domain.  FULL gold = the
cartesian product of the field vocabs — equivalence is decided by enumeration.
"""
from itertools import product

from . import catalog as C


# ----------------------------------------------------------------- lex [G]
CHARC = ("eof", "nl", "ws", "A", "D", "Q", "S", "P", "O", "other")
# A=alpha/_  D=digit  Q="  S='  P=punct-single  O=operator-char
LEXACT = ("eof", "ws", "nl", "id", "num", "str", "punct", "op", "bad")


def lex_label(c, peek):
    if c == "eof":
        return "eof"
    if c == "ws":
        return "ws"
    if c == "nl":
        return "nl"
    if c == "A":
        return "id"
    if c == "D":
        return "num"
    if c in ("Q", "S"):
        return "str"
    if c == "P":
        return "punct"
    if c == "O":
        # `/` could start a comment; peek decides.  UJS has // only.
        if peek == "O":
            return "op"
        return "op"
    return "bad"


# --------------------------------------------------------------- parse [G]
NT = ("top", "stmt", "unary", "postfix", "primary", "pswitch")
TOKS = (
    "eof", "id", "num", "str", "null", "true", "false",
    "let", "function", "if", "else", "while", "for", "return",
    "break", "continue", "switch", "case", "default",
    "typeof", "of", "in",
    "{", "}", "(", ")", "[", "]", ";", ",", ":", ".", "?",
    "=", "==", "!=", "<", ">", "<=", ">=",
    "+", "-", "*", "/", "%", "&&", "||", "!", "??",
    "=>", "+=", "-=", "*=", "/=",
    "...",
)
PRODS = (
    "end", "fn", "let", "if", "while", "for", "switch", "case", "default",
    "return", "break", "continue", "block", "expr",
    "neg", "not", "spread", "typeof", "prim",
    "index", "call", "dot", "done",
    "ident", "number", "string", "null", "true", "false",
    "list", "dict", "paren", "arrow", "bad",
)
PARSE_DEFAULT = {
    "top": "expr", "stmt": "expr",
    "unary": "prim", "postfix": "done", "primary": "bad", "pswitch": "bad",
}


def parse_label(nt, tok):
    if nt == "top":
        if tok == "eof":
            return "end"
        if tok == "function":
            return "fn"
        if tok == "let":
            return "let"
    if nt == "stmt":
        if tok == "{":
            return "block"
        if tok == "function":
            return "fn"
        if tok in ("if", "while", "for", "switch", "return",
                   "break", "continue", "let"):
            return tok
        if tok in ("case", "default"):
            return tok
    if nt == "unary":
        return {"-": "neg", "!": "not", "...": "spread",
                "typeof": "typeof"}.get(tok, "prim")
    if nt == "postfix":
        if tok == "[":
            return "index"
        if tok == "(":
            return "call"
        if tok == ".":
            return "dot"
        return "done"
    if nt == "primary":
        if tok == "id":
            return "ident"
        if tok == "num":
            return "number"
        if tok == "str":
            return "string"
        if tok in ("null", "true", "false"):
            return tok
        if tok == "[":
            return "list"
        if tok == "{":
            return "dict"
        if tok == "(":
            return "paren"
        return "bad"
    if nt == "pswitch":
        if tok in ("case", "default", "}"):
            return tok if tok != "}" else "done"
        return "bad"
    return PARSE_DEFAULT[nt]


# ---------------------------------------------------------------- type [G]
TYS = C.TYS
TOPS = (
    "+", "-", "*", "/", "%", "<", "<=", ">", ">=", "==", "!=",
    "&&", "||", "!", "=", "[]", ".", "call", "len", "keys", "in",
    "spread", ",",
)
TYOUT = TYS + ("illegal",)
NUM = ("i64", "f64")


def type_label(t1, op, t2):
    if op == "!":
        return "bool" if t1 != "fn" else "illegal"
    if op == "len":
        return "i64" if t1 in ("str", "list", "dict", "tup") else "illegal"
    if op == "keys":
        return "list" if t1 == "dict" else "illegal"
    if op == "spread":
        return t1 if t1 in ("list", "tup") else "illegal"
    if op == ",":
        return "tup"
    if op == "call":
        return "illegal" if t1 != "fn" else "null"  # return type erased at table
    if op == ".":
        return "null" if t1 == "dict" else "illegal"
    if op == "[]":
        if t1 == "list" and t2 in ("i64",):
            return "null"          # element type erased
        if t1 == "dict" and t2 == "str":
            return "null"
        if t1 == "str" and t2 == "i64":
            return "str"
        return "illegal"
    if op == "in":
        return "bool" if t1 == "str" and t2 == "dict" else "illegal"
    if op == "=":
        return t2 if t1 == t2 or t1 == "null" else "illegal"
    if op in ("&&", "||"):
        return "bool" if t1 == "bool" and t2 == "bool" else "illegal"
    if op in ("==", "!="):
        if t1 == t2:
            return "bool"
        if set((t1, t2)) <= set(NUM):
            return "bool"
        return "illegal"
    if op in ("<", "<=", ">", ">="):
        if t1 in NUM and t2 in NUM:
            return "bool"
        if t1 == "str" and t2 == "str":
            return "bool"
        return "illegal"
    if op in ("+", "-", "*", "/", "%"):
        if t1 == "i64" and t2 == "i64":
            return "f64" if op == "/" else "i64"
        if t1 in NUM and t2 in NUM:
            return "f64"
        if op == "+" and t1 == "str" and t2 == "str":
            return "str"
        return "illegal"
    return "illegal"


# --------------------------------------------------------------- scope [G]
CTX = ("mod", "fn", "block", "switch", "for", "expr")
KIND = ("bind", "load", "store", "param", "rest", "label", "break", "continue")
ACTS = ("ok", "shadow", "unbound", "readonly", "illegal")


def scope_label(ctx, kind):
    if kind == "bind":
        return "ok" if ctx in ("mod", "fn", "block", "for") else "illegal"
    if kind == "param" or kind == "rest":
        return "ok" if ctx == "fn" else "illegal"
    if kind == "load":
        return "ok"
    if kind == "store":
        return "ok" if ctx != "expr" else "illegal"
    if kind == "label":
        return "ok" if ctx == "switch" else "illegal"
    if kind in ("break", "continue"):
        return "ok" if ctx in ("for", "switch", "block") else "illegal"
    return "illegal"


# --------------------------------------------------------------- shape [G]
def shape_label(ty, keysig):
    return C.shape_of(ty, keysig)


# --------------------------------------------------------------- irsel [G]
FAMILY = (
    "arith", "cmp", "logic", "mem", "ctrl", "call", "data", "ic", "host",
)
FLAVOR = (
    "i64", "f64", "bool", "str", "list", "dict", "any",
    "br", "br_if", "ret", "switch", "enter", "print",
)
RECIPE = (
    "op_add", "op_sub", "op_mul", "op_div", "op_mod",
    "op_cmp", "op_logic", "op_load", "op_store", "op_idx",
    "op_jump", "op_call", "op_ret", "op_mk", "op_ic", "op_host", "bad",
)
_IR = {
    ("arith", "i64"): "op_add", ("arith", "f64"): "op_div",
    ("arith", "str"): "op_add", ("arith", "any"): "op_add",
    ("cmp", "i64"): "op_cmp", ("cmp", "f64"): "op_cmp",
    ("cmp", "str"): "op_cmp", ("cmp", "bool"): "op_cmp", ("cmp", "any"): "op_cmp",
    ("logic", "bool"): "op_logic", ("logic", "any"): "op_logic",
    ("mem", "any"): "op_load", ("mem", "list"): "op_idx", ("mem", "dict"): "op_idx",
    ("mem", "str"): "op_idx",
    ("ctrl", "br"): "op_jump", ("ctrl", "br_if"): "op_jump",
    ("ctrl", "ret"): "op_ret", ("ctrl", "switch"): "op_jump",
    ("call", "any"): "op_call",
    ("data", "list"): "op_mk", ("data", "dict"): "op_mk", ("data", "any"): "op_mk",
    ("ic", "enter"): "op_ic",
    ("host", "print"): "op_host",
}


def irsel_label(fam, flav):
    return _IR.get((fam, flav), "bad")


# ------------------------------------------------------------------ ic [G]
def ic_label(shape, op, guard):
    return C.ic_stub(shape, op, guard)


# ---------------------------------------------------------------- isel [G]
def isel_label(op):
    return C.isel_form(op)


# ----------------------------------------------------------------- enc [G]
def enc_label(form, imm):
    return C.enc_template(form, imm)


# --------------------------------------------------------------- reloc [G]
def reloc_label(kind):
    return C.reloc_kind(kind)


# ================================================================ registry
class Stage:
    def __init__(self, name, fields, heads, label, cfg, weight=None):
        self.name = name
        self.fields = fields
        self.heads = heads
        self.label = label
        self.cfg = cfg
        self.weight = weight
        self._idx = [{v: i for i, v in enumerate(vo)} for (_, vo) in fields]
        self._hidx = [{c: i for i, c in enumerate(cl)} for (_, cl, _) in heads]

    def keys(self):
        return list(product(*[vo for (_, vo) in self.fields]))

    def corpus(self):
        out = []
        for kv in self.keys():
            ki = tuple(self._idx[i][v] for i, v in enumerate(kv))
            lab = self.label(*kv)
            li = {hn: self._hidx[j][lab[hn]]
                  for j, (hn, _, _) in enumerate(self.heads)}
            out.append((ki, li))
        return out

    def rows(self):
        n = 1
        for (_, vo) in self.fields:
            n *= len(vo)
        return n


def _one(fn):
    return lambda *k: {"y": fn(*k)}


def build():
    S = {}
    S["lex"] = Stage("lex", [("c", CHARC), ("peek", CHARC)],
                     [("y", LEXACT, None)], _one(lex_label),
                     dict(d=6, hidden=[12], seed=31))
    S["parse"] = Stage("parse", [("nt", NT), ("tok", TOKS)],
                       [("y", PRODS, None)], _one(parse_label),
                       dict(d=8, hidden=[16], seed=13))
    S["type"] = Stage("type", [("t1", TYS), ("op", TOPS), ("t2", TYS)],
                      [("y", TYOUT, None)], _one(type_label),
                      dict(d=8, hidden=[32], seed=17))
    S["scope"] = Stage("scope", [("ctx", CTX), ("kind", KIND)],
                       [("y", ACTS, None)], _one(scope_label),
                       dict(d=8, hidden=[16], seed=19))
    S["shape"] = Stage("shape", [("ty", TYS), ("keysig", C.KEYSIG)],
                       [("y", C.SHAPES, None)], _one(shape_label),
                       dict(d=8, hidden=[16], seed=23))
    S["irsel"] = Stage("irsel", [("family", FAMILY), ("flavor", FLAVOR)],
                       [("y", RECIPE, None)], _one(irsel_label),
                       dict(d=8, hidden=[16], seed=11))
    S["ic"] = Stage("ic", [("shape", C.SHAPES), ("op", C.IC_OPS),
                           ("guard", C.GUARDS)],
                    [("y", C.STUBS, None)], _one(ic_label),
                    dict(d=8, hidden=[24], seed=41))
    S["isel"] = Stage("isel", [("op", C.JOPS)],
                      [("y", C.WASM_FORMS, None)], _one(isel_label),
                      dict(d=8, hidden=[16], seed=3))
    S["enc"] = Stage("enc", [("form", C.WASM_FORMS), ("imm", C.IMMCLASS)],
                     [("y", C.ENCTMPL, None)], _one(enc_label),
                     dict(d=8, hidden=[16], seed=29))
    S["reloc"] = Stage("reloc", [("kind", C.JMPKIND)],
                       [("y", C.RELKIND, None)], _one(reloc_label),
                       dict(d=6, hidden=[8], seed=37))
    return S


STAGES = build()
ALL = tuple(STAGES.keys())
TABLES = ALL
