"""E1 feasibility: generate the whole lexer as one finite delta (option A).

    python3 exec/lex/gen.py [out.json]      -> writes the table, prints sizes

The machine is the one in research/delta-framework.md s2 and the design is in
research/e1-lexer-delta.md.  delta maps an observation to (next state, action
sequence).  Each state declares the ONE observation component it reads:

    'b'  the byte x[i] (0..255) or EOF (256)
    't'  the stack top (a symbol, or BOT)
    'r'  the last result code

and is total over that component (no wildcard hides a missing case; the
entries no input can reach are filled with REJECT 'unreachable' and counted).

Where the data comes from -- derived, not typed in:
  * dispatch (what a byte starts, given the next one): weights/gold/lex.tsv,
    the shipped lex table itself;
  * token kinds, their names and order, the punctuators for maximal munch,
    the keywords: unisa.gold.TOKS (== TOKV in kernel/unisa_model.inc, checked);
  * the words that lex as `type`: unisa.front.lex.TYPEKW (== TYPEV, checked).
  * the byte -> class split and the attribute look-ahead's white space:
    weights/gold/lexcls.tsv; the GCC words that are skipped with their
    parenthesised argument or dropped, and the string/char prefixes:
    weights/gold/lexword.tsv.  Both are declared data read with the gold-table
    reader (unisa/tsvgold.py), and _check_decl() asserts they agree with
    src/front_pp.c charclass()/lex() and src/front_parse.c's OPCH.
Still transcribed: the number scanning rules (design doc s8).
"""
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, ROOT)

from unisa.gold import TOKS                      # noqa: E402
from unisa.front.lex import TYPEKW               # noqa: E402

EOF = 256

# ---- cross-check the Python declarations against what the C lexer uses ----
_inc = open(os.path.join(ROOT, "kernel", "unisa_model.inc"), encoding="latin-1").read()


def _cvocab(name):
    m = re.search(r'char \*%s = "((?:[^"\\]|\\.)*)";' % name, _inc)
    return tuple(m.group(1).split("\\0")[:-1])


assert _cvocab("TOKV") == tuple(TOKS), "TOKS != TOKV"
assert _cvocab("TYPEV") == tuple(TYPEKW), "TYPEKW != TYPEV"

# ---- declared data: weights/gold/lexcls.tsv, weights/gold/lexword.tsv ----------
# Read with the gold-table reader (same input contract as every stage), then
# checked against what src/front_pp.c actually does.
from unisa.tsvgold import load_stage             # noqa: E402

_G = os.path.join(ROOT, "weights", "gold")
_LC = load_stage(os.path.join(_G, "lexcls.tsv"))
_LW = load_stage(os.path.join(_G, "lexword.tsv"))
_CLS = {}
WS_SKIP = []                                             # the attribute look-ahead
for (bv,) in _LC.keys():
    c = EOF if bv == "eof" else int(bv)
    lab = _LC.label(bv)
    _CLS[c] = lab["c"]
    if lab["attws"] == "yes":
        WS_SKIP.append(c)
WS_SKIP = tuple(WS_SKIP)
_W = [(w, _LW.label(w)) for (w,) in _LW.keys()]
SKIPPAREN = tuple(w for w, l in _W if l["gcc"] == "skipparen")   # word [ws] ( ... )
DROP = tuple(w for w, l in _W if l["gcc"] == "drop")
CHARPFX = tuple(w for w, l in _W if l["pfxch"] == "yes")         # before ' : dropped
STRPFX = tuple(w for w, l in _W if l["pfxstr"] == "yes")         # before " : kept


def isal(c):
    return c != EOF and _CLS[c] == "A"


def isdi(c):
    return c != EOF and _CLS[c] == "d"


def ishex(c):
    return isdi(c) or 97 <= c <= 102 or 65 <= c <= 70


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
    assert list(_LC.heads[0][1]) == cfield, "lexcls classes != lex.tsv c field"
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

_check_decl()


def load_lex_table():
    head = None
    T = {}
    for ln in open(os.path.join(ROOT, "weights", "gold", "lex.tsv"), encoding="utf-8"):
        f = ln.rstrip("\n").split("\t")
        if f[0] == "#head":
            head = f[3:]
            continue
        if ln.startswith("#") or len(f) < 3 or f[0] == "c":
            continue
        T[(f[0], f[1])] = f[2]
    return head, T


# ---- the machine under construction ----------------------------------------
class Delta:
    def __init__(self):
        self.states = {}          # name -> (mode, {value: (next, seqid)})
        self.seqs = []            # action sequences (tuples)
        self.seqix = {}
        self.unreach = 0

    def seq(self, acts):
        acts = tuple(tuple(a) if isinstance(a, list) else a for a in acts)
        if acts not in self.seqix:
            self.seqix[acts] = len(self.seqs)
            self.seqs.append(acts)
        return self.seqix[acts]

    def state(self, name, mode):
        if name not in self.states:
            self.states[name] = (mode, {})
        return self.states[name][1]

    def put(self, name, key, nxt, acts):
        row = self.state(name, "b")
        row[key] = (nxt, self.seq(acts))


D = Delta()
A = ("ADV",)


def OUT(s):
    return [("OUT", ord(ch)) for ch in s]


def emit_kind(k, span=("S", None)):
    """The -dump-tokens line of a token of kind k: name, and for id/num/str
    '=' and the spelling.  kind 1 (`type`) prints no spelling."""
    name = TOKS[k]
    acts = OUT(name)
    if k in (2, 3, 4):
        acts += OUT("=")
        acts += [("SPAN2", span[0], span[1])] if span[1] else [("SPAN", span[0])]
    acts += OUT("\n") + [("INC", "NT")]
    return acts


KID, KNUM, KSTR = 2, 3, 4
ALLB = list(range(257))

head, LT = load_lex_table()
CLASSES = sorted(set(c for c, _ in LT))


def lexact(c, p):
    return LT[(charclass(c), charclass(p))]


# Which classes need the peek: the table row is not constant over peek.
rowconst = {}
for cl in CLASSES:
    vals = set(v for (a, b), v in LT.items() if a == cl)
    rowconst[cl] = vals.pop() if len(vals) == 1 else None


# ---- handler entries: what each lex action does on its first byte c ---------
def handler(a, c):
    """(next, acts) for lex action `a` at byte c, i at c, c not consumed."""
    if a == "skip":
        if c == EOF:
            return "CNT0", OUT("eof\n") + [("INC", "NT")]
        return "DISPATCH", [A]
    if a == "nl":
        return "DISPATCH", [A]
    if a == "linecmt":
        return "LC", [A, A]
    if a == "cmt":
        return "BC", [A, A]
    if a == "ident":
        return idnext("", c)
    if a == "num":
        return num_start(c)
    if a == "str":
        return "STR", [("MARK", "S"), A]
    if a == "charlit":
        return "CH", [("MARK", "S"), A]
    if a == "op":
        return op_step("", c, [("MARK", "S")])
    if a == "bad":
        return "HALT", [("REJECT", "unexpected character")]
    raise KeyError(a)


# ---- identifiers: a trie over every word the lexer treats specially --------
KWKIND = {}
for k, t in enumerate(TOKS):
    if isal(ord(t[0])):
        KWKIND[t] = KID if k in (0, 1) else k     # `eof`, `type` are not keywords
for t in TYPEKW:
    KWKIND[t] = 1
WORDS = set(KWKIND) | set(SKIPPAREN) | set(DROP) | set(CHARPFX) | set(STRPFX)
PREFIXES = set(w[:n] for w in WORDS for n in range(1, len(w) + 1))


def idstate(w):
    return "ID:" + w if w in PREFIXES else "ID*"


def idnext(w, c):
    """ident scan: in node w, the next byte c continues the name."""
    acts = [("MARK", "S")] if w == "" else []
    nw = w + chr(c)
    return (idstate(nw) if w != "*" else "ID*"), acts + [A]


def id_end(w, c):
    """the name (trie node w; '*' = none) ended; c is the byte after it"""
    if w in SKIPPAREN:
        return "SKW:" + w, [("MARK", "E")]
    if w in DROP:
        return "DISPATCH", []
    if w in CHARPFX and c == 39:
        return "CH", [("MARK", "S"), A]           # L'x': the prefix is dropped
    if w in STRPFX and c == 34:
        return "STR", [A]                          # L"x": the prefix is kept
    k = KWKIND.get(w, KID)
    return "DISPATCH", emit_kind(k)


def build_ident():
    nodes = sorted(PREFIXES) + ["*"]
    for w in nodes:
        st = "ID:" + w if w != "*" else "ID*"
        for c in ALLB:
            if c != EOF and (isal(c) or isdi(c)):
                if w == "*":
                    D.put(st, c, "ID*", [A])
                else:
                    D.put(st, c, idstate(w + chr(c)), [A])
            elif c == 92:                          # a UCN continues the name
                D.put(st, c, "UCN", [("MARK", "B"), A])
            else:
                nx, acts = id_end(w, c)
                D.put(st, c, nx, acts)
    # \uXXXX / \UXXXXXXXX; a malformed one ends the name at the backslash,
    # which then lexes as `other` -> the same reject, at the backslash
    bad = [("JUMP", "B"), ("REJECT", "unexpected character")]
    for c in ALLB:
        if c == 117:
            D.put("UCN", c, "UCNH4.0", [A])
        elif c == 85:
            D.put("UCN", c, "UCNH8.0", [A])
        else:
            D.put("UCN", c, "HALT", bad)
    for n in (4, 8):
        for k in range(n):
            st = "UCNH%d.%d" % (n, k)
            nx = "ID*" if k == n - 1 else "UCNH%d.%d" % (n, k + 1)
            for c in ALLB:
                if c != EOF and ishex(c):
                    D.put(st, c, nx, [A])
                else:
                    D.put(st, c, "HALT", bad)
    # the GCC words: skip white space; `(` starts a balanced skip, anything
    # else means it was an ordinary identifier after all
    for w in SKIPPAREN:
        st = "SKW:" + w
        for c in ALLB:
            if c in WS_SKIP:
                D.put(st, c, st, [A])
            elif c == 40:
                D.put(st, c, "ATT", [("PUSH", "P"), A])
            else:
                D.put(st, c, "DISPATCH", emit_kind(KWKIND.get(w, KID), ("S", "E")))
    for c in ALLB:
        if c == 40:
            D.put("ATT", c, "ATT", [("PUSH", "P"), A])
        elif c == 41:
            D.put("ATT", c, "ATTCHK", [("POP",), A])
        elif c == EOF:
            D.put("ATT", c, "ATTDRAIN", [])
        else:
            D.put("ATT", c, "ATT", [A])
    D.state("ATTCHK", "t").update({"BOT": ("DISPATCH", D.seq([])),
                                   "P": ("ATT", D.seq([]))})
    D.state("ATTDRAIN", "t").update({"BOT": ("DISPATCH", D.seq([])),
                                     "P": ("ATTDRAIN", D.seq([("POP",)]))})


# ---- numbers ---------------------------------------------------------------
def num_start(c):
    if c == 48:
        return "NZ", [("MARK", "S"), A]
    if isdi(c):
        return "DEC", [("MARK", "S"), A]
    if c == 46:
        return "DFRAC", [("MARK", "S"), A]
    return "HALT", [("REJECT", "unreachable")]


NUMEMIT = emit_kind(KNUM)


def build_num():
    def row(st, f):
        for c in ALLB:
            nx, acts = f(c)
            D.put(st, c, nx, acts)

    def dig(c):
        return c != EOF and isdi(c)

    def hx(c):
        return c != EOF and ishex(c)

    def dec(c):
        if dig(c):
            return "DEC", [A]
        if c == 46:
            return "DFRAC", [A]
        if c in (101, 69):
            return "DEXP0.i", [("MARK", "E"), A]
        return "SUFI", []
    row("DEC", dec)
    row("NZ", lambda c: ("HEX", [A]) if c in (120, 88) else dec(c))

    def dfrac(c):
        if dig(c):
            return "DFRAC", [A]
        if c in (101, 69):
            return "DEXP0.f", [("MARK", "E"), A]
        return "SUFF", []
    row("DFRAC", dfrac)
    for fl in ("i", "f"):
        back = [("JUMP", "E")]
        suf = "SUFI" if fl == "i" else "SUFF"
        row("DEXP0." + fl, lambda c, back=back, suf=suf:
            ("DEXPS." + suf, [A]) if c in (43, 45)
            else (("EXPD", [A]) if dig(c) else (suf, back)))
    for suf in ("SUFI", "SUFF"):
        row("DEXPS." + suf, lambda c, suf=suf:
            ("EXPD", [A]) if dig(c) else (suf, [("JUMP", "E")]))
    row("EXPD", lambda c: ("EXPD", [A]) if dig(c) else ("SUFF", []))
    row("HEX", lambda c: ("HEX", [A]) if hx(c) else
        (("HFRAC", [("MARK", "E"), A]) if c == 46 else
         (("HEXP0", [("MARK", "E"), A]) if c in (112, 80) else ("SUFI", []))))
    row("HFRAC", lambda c: ("HFRAC", [A]) if hx(c) else
        (("HEXP0", [A]) if c in (112, 80) else ("SUFI", [("JUMP", "E")])))
    row("HEXP0", lambda c: ("HEXP1", [A]) if c in (43, 45) else
        (("EXPD", [A]) if dig(c) else ("SUFI", [("JUMP", "E")])))
    row("HEXP1", lambda c: ("EXPD", [A]) if dig(c) else ("SUFI", [("JUMP", "E")]))
    row("SUFF", lambda c: ("DISPATCH", [A] + NUMEMIT) if c in (102, 70, 108, 76)
        else ("DISPATCH", NUMEMIT))
    row("SUFI", lambda c: ("SUFI", [A]) if c in (117, 85, 108, 76)
        else ("DISPATCH", NUMEMIT))


# ---- strings and character constants --------------------------------------
STREMIT = [("JUMP", "E")] + emit_kind(KSTR)


def build_str():
    for c in ALLB:
        if c == 92:
            D.put("STR", c, "STR", [A, A])
        elif c == 34:
            D.put("STR", c, "STRWS", [A, ("MARK", "E")])
        elif c == EOF:          # unterminated: rejected at the literal's start,
                                # as the reference now does (e1-lexer-delta.md s6)
            D.put("STR", c, "HALT", [("JUMP", "S"), ("REJECT", "missing terminating '\"' character")])
        else:
            D.put("STR", c, "STR", [A])
        # after the closing quote: white space, an optional prefix, and
        # another literal continue the same token [W-12]
        if c in WS_SKIP:
            D.put("STRWS", c, "STRWS", [A])
        elif c == 34:
            D.put("STRWS", c, "STR", [A])
        elif c in (76, 85):
            D.put("STRWS", c, "STRP1", [A])
        elif c == 117:
            D.put("STRWS", c, "STRPu", [A])
        else:
            D.put("STRWS", c, "DISPATCH", STREMIT)
        D.put("STRP1", c, *(("STR", [A]) if c == 34 else ("DISPATCH", STREMIT)))
        D.put("STRPu", c, *(("STR", [A]) if c == 34 else
                            (("STRPu8", [A]) if c == 56 else ("DISPATCH", STREMIT))))
        D.put("STRPu8", c, *(("STR", [A]) if c == 34 else ("DISPATCH", STREMIT)))
        # 'x': to the next quote; a backslash takes the byte after it
        if c == 39:
            D.put("CH", c, "DISPATCH", [A] + NUMEMIT)
        elif c == 92:
            D.put("CH", c, "CH", [A, A])
        elif c == EOF:
            D.put("CH", c, "HALT", [("JUMP", "S"), ("REJECT", "missing terminating ' character")])
        else:
            D.put("CH", c, "CH", [A])


# ---- comments ----------------------------------------------------------------
def build_cmt():
    for c in ALLB:
        D.put("LC", c, *(("DISPATCH", []) if c in (10, EOF) else ("LC", [A])))
        if c == EOF:
            D.put("BC", c, "DISPATCH", [])
            D.put("BCS", c, "DISPATCH", [])
        elif c == 42:
            D.put("BC", c, "BCS", [A])
            D.put("BCS", c, "BCS", [A])
        elif c == 47:
            D.put("BC", c, "BC", [A])
            D.put("BCS", c, "DISPATCH", [A])
        else:
            D.put("BC", c, "BC", [A])
            D.put("BCS", c, "BC", [A])


# ---- punctuators: maximal munch as a trie with a remembered last accept -----
PPREF = set(t[:n] for t in PUNCTS for n in range(1, len(t) + 1))


def munch(s):
    """the reference algorithm on the literal string s: TOKV order, longest"""
    best, bl = -1, 0
    for k, t in enumerate(TOKS):
        if t and not isal(ord(t[0])) and len(t) > bl and s.startswith(t):
            best, bl = k, len(t)
    return best, bl


def opname(node, last):
    return "OP:%s|%s" % (node, last)


def op_step(node, c, pre):
    """in trie node `node` (i past it), byte c"""
    nxt = node + chr(c) if c != EOF else None
    if nxt is not None and nxt in PPREF:
        k, bl = munch(nxt)
        acc = bl == len(nxt)
        last = k
        return opname(nxt, last), pre + [A] + ([("MARK", "E")] if acc else [])
    if node == "":
        return "HALT", [("REJECT", "stray char")]
    k, bl = munch(node)
    if bl == len(node):
        return "DISPATCH", emit_kind(k)
    return "DISPATCH", [("JUMP", "E")] + emit_kind(k)


def build_op():
    for node in sorted(PPREF):
        k, bl = munch(node)
        st = opname(node, k)
        for c in ALLB:
            nx, acts = op_step(node, c, [])
            D.put(st, c, nx, acts)
    for c in ALLB:
        nx, acts = op_step("", c, [("MARK", "S")])
        D.put("OP:", c, nx, acts)


# ---- dispatch, the token count, halting -------------------------------------
def build_dispatch():
    for c in ALLB:
        cl = charclass(c)
        a = rowconst[cl]
        if a is not None:
            nx, acts = handler(a, c)
            D.put("DISPATCH", c, nx, acts)
        else:                              # this class needs the next byte
            D.put("DISPATCH", c, "PEEK:" + cl, [("MARK", "S"), A])
    for cl in CLASSES:
        if rowconst[cl] is not None:
            continue
        st = "PEEK:" + cl
        for p in ALLB:
            a = LT[(cl, charclass(p))]
            D.put(st, p, "H:" + a, [("JUMP", "S")])
            # H:a -- the handler, re-reading its first byte
            for c in ALLB:
                if charclass(c) == cl:
                    nx, acts = handler(a, c)
                    D.put("H:" + a, c, nx, acts)
    # the count line: "%d tokens\n" from W[NT], digits via the stack
    for c in ALLB:
        D.put("CNT0", c, "CNT1", [("COPYW", "T", "NT"), ("DIVMOD10", "T")])
    r = D.state("CNT1", "r")
    for d in range(10):
        r[d] = ("CNT1", D.seq([("PUSH", "D%d" % d), ("DIVMOD10", "T")]))
        r[10 + d] = ("CNTP", D.seq([("PUSH", "D%d" % d)]))
    t = D.state("CNTP", "t")
    for d in range(10):
        t["D%d" % d] = ("CNTP", D.seq([("OUT", 48 + d), ("POP",)]))
    t["BOT"] = ("HALT", D.seq(OUT(" tokens\n") + [("ACCEPT",)]))


build_dispatch()
build_ident()
build_num()
build_str()
build_cmt()
build_op()

# ---- totality: every state total over what it reads -------------------------
GAMMA = ["BOT", "P"] + ["D%d" % d for d in range(10)]
RDOM = list(range(20))
DOM = {"b": ALLB, "t": GAMMA, "r": RDOM}
# a state some transition names must exist
named = set(nx for _, (m, row) in D.states.items() for nx, _ in row.values())
named.discard("HALT")
missing = named - set(D.states)
assert not missing, missing
for name, (mode, row) in D.states.items():
    for v in DOM[mode]:
        if v not in row:
            row[v] = ("HALT", D.seq([("REJECT", "unreachable")]))
            D.unreach += 1
# forward reachability over the state graph
reach, todo = {"DISPATCH"}, ["DISPATCH"]
while todo:
    s = todo.pop()
    for nx, _ in D.states[s][1].values():
        if nx != "HALT" and nx not in reach:
            reach.add(nx)
            todo.append(nx)
dead = set(D.states) - reach
for s in dead:
    del D.states[s]


# ---- sizes ---------------------------------------------------------------------
def stats():
    Q = len(D.states)
    ent = sum(len(DOM[m]) for m, _ in D.states.values())
    nseq = len(D.seqs)
    nact = sum(len(s) for s in D.seqs)
    ENT = 4                 # next state u16 + sequence id u16
    pool = nact * 2 + nseq  # opcode+operand per action, a length byte per sequence
    dense = ent * ENT + pool + Q          # + one mode byte per state
    # sparse: per state a default entry + (key, entry) for each exception
    sp = 0
    for m, row in D.states.values():
        cnt = {}
        for v in row.values():
            cnt[v] = cnt.get(v, 0) + 1
        dflt = max(cnt.values())
        sp += 1 + ENT + 2 + (len(row) - dflt) * (1 + ENT)
    sparse = sp + pool
    # the byte classes delta itself induces: bytes no b-state tells apart
    bstates = [row for m, row in D.states.values() if m == "b"]
    cols = {}
    for c in ALLB:
        sig = tuple(row[c] for row in bstates)
        cols.setdefault(sig, []).append(c)
    K = len(cols)
    classed = (len(bstates) * K + sum(len(DOM[m]) for m, _ in D.states.values() if m != "b")) * ENT + 257 + pool + Q
    # the literal Obs of s2.2: |Q| x |R| x 257 x |Gamma+BOT|
    full = Q * len(RDOM) * 257 * len(GAMMA)
    return dict(states=Q, b_states=len(bstates), t_states=sum(1 for m, _ in D.states.values() if m == "t"),
                r_states=sum(1 for m, _ in D.states.values() if m == "r"),
                entries=ent, unreachable_filled=D.unreach, dead_states_dropped=sorted(dead),
                action_sequences=nseq, actions_in_pool=nact, pool_bytes=pool,
                dense_bytes=dense, sparse_bytes=sparse,
                byte_classes_induced=K, classed_dense_bytes=classed,
                full_obs_product_entries=full,
                ident_trie_nodes=len(PREFIXES) + 1, punct_trie_nodes=len(PPREF) + 1,
                peek_classes=[c for c in CLASSES if rowconst[c] is None],
                ), [cols[k] for k in cols]


if __name__ == "__main__":
    st, classes = stats()
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, "delta.json")
    with open(out, "w") as f:
        json.dump({"states": {k: [m, {str(v): list(e) for v, e in row.items()}]
                              for k, (m, row) in D.states.items()},
                   "seqs": [list(map(list, s)) for s in D.seqs],
                   "tok_names": list(TOKS)}, f, separators=(",", ":"))
    for k, v in st.items():
        print("%-26s %s" % (k, v))

    def rng(cs):
        return ",".join(str(c) if c < 256 else "EOF" for c in cs[:6]) + ("..(%d)" % len(cs) if len(cs) > 6 else "")
    print("induced byte classes:")
    for cs in sorted(classes, key=lambda c: c[0]):
        print("   ", rng(cs))
