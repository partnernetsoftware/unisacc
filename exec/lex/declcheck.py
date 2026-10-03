#!/usr/bin/env python3
"""E1 declaration check (was exec/lex/gen.py --check-declarations): the declared lexer data
(weights/gold/lexcls.tsv, lexword.tsv, parse.tsv tok, iterate/kernel/typekw.tsv) agrees with src/front_pp.c
charclass()/lex(), src/front_parse.c OPCH and kernel/unisa_model.inc TOKV/TYPEV.  Exit 0 when it agrees."""
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, ROOT)

from pathlib import Path

from exec.facts.load import facts  # noqa: E402

_C = {r['name']: r['value'] for r in facts('lex-consts')}
EOF = _C['EOF']
_G = os.path.join(ROOT, "weights", "gold")
# The token-ID contract is the declared parse/tok field order.
_token_rows = [line.split("\t")[2:] for line in
    (Path(_G) / "parse.tsv").read_text().splitlines()
    if line.startswith("#field\ttok\t")]
assert len(_token_rows) == 1 and _token_rows[0], "missing or repeated token schema"
TOKS = tuple(_token_rows[0])
assert len(TOKS) == len(set(TOKS)) and all(TOKS), "invalid token schema"
_kw = [line.split("\t") for line in
    (Path(ROOT) / "iterate/kernel/typekw.tsv").read_text().splitlines()
    if line and not line.startswith("#")]
assert _kw and all(len(row) == 2 and row[0] == "kw" and row[1] for row in _kw)
TYPEKW = tuple(row[1] for row in _kw)
assert len(TYPEKW) == len(set(TYPEKW)), "duplicate type keyword"

# Reference-source checks are a test mode, never a generation dependency.


# ---- declared data: weights/gold/lexcls.tsv, weights/gold/lexword.tsv ----------
# Read with the gold-table reader (same input contract as every stage), then
# checked against what src/front_pp.c actually does.
from unisa.tsvgold import load_table             # noqa: E402

_G = os.path.join(ROOT, "weights", "gold")
_, _, _LCheads, _LCrows = load_table(os.path.join(_G, "lexcls.tsv"))
_, _, _, _LWrows = load_table(os.path.join(_G, "lexword.tsv"))
_CLS = {}
WS_SKIP = []                                             # the attribute look-ahead
for (bv,), lab in _LCrows.items():
    c = EOF if bv == "eof" else int(bv)
    _CLS[c] = lab["c"]
    if lab["attws"] == "yes":
        WS_SKIP.append(c)
WS_SKIP = tuple(WS_SKIP)
_W = [(w, lab) for (w,), lab in _LWrows.items()]
SKIPPAREN = tuple(w for w, l in _W if l["gcc"] == "skipparen")   # word [ws] ( ... )
DROP = tuple(w for w, l in _W if l["gcc"] == "drop")
CHARPFX = tuple(w for w, l in _W if l["pfxch"] == "yes")         # before ' : dropped
STRPFX = tuple(w for w, l in _W if l["pfxstr"] == "yes")         # before " : kept


def isal(c):
    return c != EOF and _CLS[c] == "A"


def isdi(c):
    return c != EOF and _CLS[c] == "d"


def charclass(c):                     # the declared split
    return _CLS[c]


PUNCTS = [t for t in TOKS if t and not isal(ord(t[0]))]


# ---- agreement with the C lexer (src/front_pp.c, src/front_parse.c) ----------
def _check_decl():
    pp = open(os.path.join(ROOT, "src", "front_pp.c"), encoding="latin-1").read()
    fp = open(os.path.join(ROOT, "src", "front_parse.c"), encoding="latin-1").read()
    # the class names in charclass()'s return order are lex.tsv's `c` field order
    cfield = [ln.rstrip("\n").split("\t")[2:] for ln in
              open(os.path.join(_G, "lex.tsv"), encoding="utf-8") if ln.startswith("#field\tc\t")][0]
    assert list(_LCheads[0][1]) == cfield, "lexcls classes != lex.tsv c field"
    body = re.search(r"int charclass\(int c\) \{(.*?)\n\}", pp, re.S).group(1)
    # isal/isdi: the C ranges
    def ranges(fn):
        b = re.search(r"int %s\(int c\) \{(.*?)return 0;" % fn, pp, re.S).group(1)
        rs = [(int(x), int(y)) for x, y in re.findall(r"c >= (\d+)\) \{ if \(c <= (\d+)\)", b)]
        rs += [(int(x), int(x)) for x in re.findall(r"c == (\d+)\) return 1", b)]
        return set(c for x, y in rs for c in range(x, y + 1))
    AL, DI = ranges("isal"), ranges("isdi")
    # OPCH: first bytes of TOKV, plus the ones front_parse.c adds by hand
    opch = set(ord(t[0]) for t in TOKS if t) | set(int(x) for x in re.findall(r"OPCH\[(\d+)\] = 1", fp))
    def cref(c):                                  # charclass(), statement by statement
        if c == EOF:
            k = int(re.search(r"c < 0\) return (\d+)", body).group(1))
            return cfield[k]
        for v, k in re.findall(r"if \(c == (\d+)\) return (\d+);", body):
            if c == int(v):
                return cfield[int(k)]
        if c in AL:
            return cfield[int(re.search(r"isal\(c\)\) return (\d+)", body).group(1))]
        if c in DI:
            return cfield[int(re.search(r"isdi\(c\)\) return (\d+)", body).group(1))]
        if c < 128 and c in opch:
            return cfield[int(re.search(r"OPCH\[c\]\) return (\d+)", body).group(1))]
        return cfield[int(re.search(r"\n    return (\d+);", body).group(1))]
    for c in range(257):
        assert _CLS[c] == cref(c), "lexcls: byte %d is %s, charclass() says %s" % (c, _CLS[c], cref(c))
    lx = pp[pp.index("/* ident */"):pp.index("/* a wide CHARACTER")]
    sp = lx[:lx.index("k = j;")]
    dr = lx[lx.index("__extension__") - 20:]
    assert set(re.findall(r'srcis\(i, j - i, "(\w+)"\)', sp)) == set(SKIPPAREN), "SKIPPAREN != lex()"
    assert set(re.findall(r'srcis\(i, j - i, "(\w+)"\)', dr)) == set(DROP), "DROP != lex()"
    ws = lx[lx.index("k = j;"):lx.index("if (at(k) == 40)")]
    assert set(int(v) for v in re.findall(r"at\(k\) == (\d+)", ws)) == set(c for c in WS_SKIP), "attws != lex()"
    # prefixes, from the two branches of the ident action: before `'` and before `"`
    def pfx(block):
        one = set(chr(int(v)) for v in re.findall(r"at\(i\) == (\d+)", block[:block.index("j - i == 2")]))
        two = re.findall(r"at\(i\) == (\d+)\) \{ if \(at\(i\+1\) == (\d+)", block)
        return one | set(chr(int(x)) + chr(int(y)) for x, y in two)
    w0 = pp.index("/* a wide CHARACTER")
    w1 = pp.index("if (at(j) == 34) {", w0)
    w2 = pp.index("kind = vfind(TOKV", w1)
    assert pfx(pp[w0:w1]) == set(CHARPFX), "CHARPFX != lex()"
    assert pfx(pp[w1:w2]) == set(STRPFX), "STRPFX != lex()"
    # an adjacent literal may carry the same prefixes
    nxt = pp[pp.index("the next literal may carry"):]
    nxt = nxt[:nxt.index("if (at(q) == 34) { j")]
    one = set(chr(int(v)) for v in re.findall(r"at\(q\) == (\d+)", nxt))
    assert one == set(w for w in STRPFX if len(w) == 1) and "at(q + 1) == 56" in nxt, "STRPFX != adjacent literal"

from unisa.gold import TOKS as reference_tokens
from unisa.front.lex import TYPEKW as reference_types
assert tuple(reference_tokens) == TOKS and tuple(reference_types) == TYPEKW
_inc = (Path(ROOT) / "kernel/unisa_model.inc").read_text(encoding="latin-1")
for name, values in (("TOKV", TOKS), ("TYPEV", TYPEKW)):
    m = re.search(r'char \*%s = "((?:[^"\\]|\\.)*)";' % name, _inc)
    assert m and tuple(m.group(1).split("\\0")[:-1]) == values, name
_check_decl()


print("lex declarations agree")
