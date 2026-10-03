"""E1 feasibility: generate the whole lexer as one finite delta (option A).

    python3 exec/lex/gen.py [out.json]      -> writes the table, prints sizes

The machine is the one in research/delta-framework.md s2 and the design is in
docs/exec/e1-lexer-delta.md.  delta maps an observation to (next state, action
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
    the keywords: weights/gold/parse.tsv, field tok;
  * the words that lex as `type`: iterate/kernel/typekw.tsv.
  * the byte -> class split and the attribute look-ahead's white space:
    weights/gold/lexcls.tsv; the GCC words that are skipped with their
    parenthesised argument or dropped, and the string/char prefixes:
    weights/gold/lexword.tsv.  Both are declared data read with the gold-table
    reader (unisa/tsvgold.py); --check-declarations asserts agreement with
    src/front_pp.c charclass()/lex() and src/front_parse.c's OPCH.
Finite scanning/entry/output/framing rules: the TSV files beside this module.
This generator expands finite rules, links declared actions, builds word and
punctuator tries, and checks totality. See rules.md for the input contract
and the remaining construction-side assumptions; no reference files are
read unless --check-declarations is requested.
"""
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, ROOT)

from pathlib import Path

EOF = 256
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
CHECK_DECL = "--check-declarations" in sys.argv
if CHECK_DECL:
    sys.argv.remove("--check-declarations")


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

if CHECK_DECL:
    from unisa.gold import TOKS as reference_tokens
    from unisa.front.lex import TYPEKW as reference_types
    assert tuple(reference_tokens) == TOKS and tuple(reference_types) == TYPEKW
    _inc = (Path(ROOT) / "kernel/unisa_model.inc").read_text(encoding="latin-1")
    for name, values in (("TOKV", TOKS), ("TYPEV", TYPEKW)):
        m = re.search(r'char \*%s = "((?:[^"\\]|\\.)*)";' % name, _inc)
        assert m and tuple(m.group(1).split("\\0")[:-1]) == values, name
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
        self.st = self.states     # the graph view finite_rules edits
        self.labels = set()

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

    # thin adapter for exec/finite_rules.install_template
    def on(self, name, keys, nxt, acts, mode):
        row = self.state(name, mode)
        for key in keys:
            row[key] = (nxt, self.seq(acts))


D = Delta()
A = ("ADV",)


# --typed: write tokens.typed (exec/pipeline/formats.md) -- a `type` token also
# carries its spelling, the same generic SPAN copy as id/num/str.  Default
# output (tokens.plain) is unchanged. --positions implies typed and adds
# fixed-size byte-offset prefixes; see tokens.positions in formats.md.
LOCATIONS = "--locations" in sys.argv
if LOCATIONS:
    sys.argv.remove("--locations")
POSITIONS = "--positions" in sys.argv or LOCATIONS
if "--positions" in sys.argv:
    sys.argv.remove("--positions")
TYPED = "--typed" in sys.argv or POSITIONS
if "--typed" in sys.argv:
    sys.argv.remove("--typed")
SOURCEFACTS = TYPED or "--sourcefacts" in sys.argv
if "--sourcefacts" in sys.argv:
    sys.argv.remove("--sourcefacts")


def OUT(s):
    return [("OUT", ord(ch)) for ch in s]


def declarations(path):
    rows = [line.split("\t") for line in path.read_text().splitlines()
            if line and not line.startswith("#")]
    assert rows and len({row[0] for row in rows}) == len(rows), path
    return rows


OUTPUT = {name: json.loads(actions) for name, actions in declarations(Path(HERE) / "output.tsv")}
SPELLING = {}
for token, plain, typed in declarations(Path(HERE) / "spelling.tsv"):
    assert token in TOKS and plain in ("yes", "no") and typed in ("yes", "no")
    SPELLING[token] = (plain == "yes", typed == "yes")


def output_sequence(sequence, **parameters):
    result = []
    for action in OUTPUT[sequence]:
        values = [parameters[v[1:]] if isinstance(v, str) and v.startswith("$") else v
                  for v in action]
        if values[0] == "@bytes":
            assert len(values) == 2 and isinstance(values[1], str)
            result.extend(OUT(values[1]))
        else:
            result.append(tuple(values))
    return result


def position(reg):
    return output_sequence("position", start=reg) if POSITIONS else []


def emit_kind(k, span=("S", None)):
    name = TOKS[k]
    acts = position(span[0]) + output_sequence("name", name=name)
    if SPELLING.get(name, (False, False))[int(TYPED)]:
        acts += output_sequence("span2" if span[1] else "span", start=span[0], end=span[1])
    return acts + output_sequence("end")


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


# ---- the declared machine pieces: facts for gen-template.tsv -----------------
# The template expresses dispatch, the identifier trie and the punctuator trie;
# Python only exposes domain facts: byte classes, lex.tsv rows, entry/number rows,
# the word lists as trie prefixes, the token kinds and the output sequences.
sys.path.insert(0, os.path.join(ROOT, "exec"))
from finite_rules import load as load_byte_rules, install_template, prefix_facts, rule_facts  # noqa: E402

KWKIND = {}
for k, t in enumerate(TOKS):
    if isal(ord(t[0])):
        KWKIND[t] = KID if k < 5 else k          # eof/type/id/num/str are token-class names
for t in TYPEKW:
    KWKIND[t] = 1
WORDS = set(KWKIND) | set(SKIPPAREN) | set(DROP) | set(CHARPFX) | set(STRPFX)
IDROOT, IDPFX = prefix_facts(sorted(WORDS))
OPROOT, OPPFX = prefix_facts([t if t in PUNCTS else None for t in TOKS])


def optional(present):
    return [{}] if present else []


IDFACTS = [dict(p, kind=KWKIND.get(p["name"], KID),
                ifskip=optional(p["name"] in SKIPPAREN), ifdrop=optional(p["name"] in DROP),
                iftoken=optional(p["name"] not in SKIPPAREN and p["name"] not in DROP),
                ifpfxch=optional(p["name"] in CHARPFX), ifpfxstr=optional(p["name"] in STRPFX))
           for p in IDPFX]

ENTRY = rule_facts(Path(HERE) / "entry.tsv")
assert set(ENTRY) == set(head), "entry actions must cover lex.tsv exactly"
LINKS = ("@identifier", "@number", "@punctuator")
assert all(r["actions"] == "[]" for rows in ENTRY.values() for r in rows if r["target"] in LINKS)


def handle(state, cls, act, peek=()):
    rows = ENTRY[act]
    return dict(state=state, cls=cls, act=act, peek=list(peek),
                peekstate=[{"name": cls}] if peek else [],
                plain=[r for r in rows if r["target"] not in LINKS],
                identifier=[r for r in rows if r["target"] == "@identifier"],
                number=[r for r in rows if r["target"] == "@number"],
                punctuator=[r for r in rows if r["target"] == "@punctuator"])


BYTECLASSES = {cl: [c for c in ALLB if charclass(c) == cl] for cl in CLASSES}
HANDLE = [handle("DISPATCH", cl, rowconst[cl]) for cl in CLASSES if rowconst[cl] is not None]
PEEKCLASS = [{"name": cl} for cl in CLASSES if rowconst[cl] is None]
for cl in CLASSES:
    if rowconst[cl] is None:            # lex.tsv row of cl, its columns by first byte
        cols = sorted(CLASSES, key=lambda col: BYTECLASSES[col][0])
        peek = [{"cls": cl, "col": col, "act": LT[(cl, col)]} for col in cols]
        acts = list(dict.fromkeys(r["act"] for r in peek))
        HANDLE += [handle("H:" + a, cl, a, peek if i == 0 else ()) for i, a in enumerate(acts)]

SEQUENCES = {name: output_sequence(name) for name in ("scan.start", "scan.advance", "punct.reject")}
SEQUENCES.update({"accept.yes": output_sequence("scan.accept"), "accept.no": [],
                  "rewind.yes": [], "rewind.no": output_sequence("scan.rewind"),
                  "eof": ([("MARK", "S")] + position("S") if POSITIONS else []) + output_sequence("eof")})
for k in range(len(TOKS)):
    SEQUENCES["tok.%d" % k] = emit_kind(k)
    SEQUENCES["bounded.%d" % k] = emit_kind(k, ("S", "E"))
FACTS = dict(HANDLE=HANDLE, PEEKCLASS=PEEKCLASS, IDROOT=IDROOT["children"],
             NSTART=rule_facts(Path(HERE) / "number.tsv")["NSTART"],
             OPROOT=OPROOT["children"], IDPFX=IDFACTS, OPPFX=OPPFX, KID=[KID])
TEMPLATE_CLASSES = dict(BYTECLASSES, identifier=[c for c in ALLB if c != EOF and (isal(c) or isdi(c))],
                        space=list(WS_SKIP))


def install_section(section):
    install_template(D, HERE, "gen", FACTS, None, sequences=SEQUENCES, classes=TEMPLATE_CLASSES,
                     section=section, mode="b", overlay=True)


def install_rules(filename, mode, domain, sequences=None, classes=None, skip=(), in_domain_order=False):
    rows = load_byte_rules(Path(HERE) / filename, sequences or {}, domain, classes)
    for state, row in rows.items():
        if state in skip:
            continue
        out = D.state(state, mode)
        for observation in (domain if in_domain_order else row):
            target, actions = row[observation]
            out[observation] = target, D.seq(actions)


NUMEMIT = emit_kind(KNUM)  # character constants use the same token format

install_section("dispatch")
for filename, mode, domain in (("count-byte.tsv", "b", ALLB),
                               ("count-result.tsv", "r", range(20)),
                               ("count-stack.tsv", "t", ["BOT"] + ["D%d" % n for n in range(10)])):
    install_rules(filename, mode, domain)
install_section("ident")
for filename, mode, domain in (("ident-byte.tsv", "b", ALLB),
                               ("ident-stack.tsv", "t", ("BOT", "P"))):
    install_rules(filename, mode, domain)
# entry action is inlined by dispatch, so NSTART is not installed
install_rules("number.tsv", "b", ALLB, {"number": emit_kind(KNUM)}, skip=("NSTART",), in_domain_order=True)
install_rules("literal.tsv", "b", ALLB,
              {"number": NUMEMIT, "string": [("JUMP", "E")] + emit_kind(KSTR)},
              {"space": WS_SKIP})
install_section("punct")
START = "DISPATCH"
if LOCATIONS:
    from locations import install
    START = install(D)
if SOURCEFACTS:
    from lexsourcefacts import install as install_sourcefacts
    START = install_sourcefacts(D, START, FACTS)

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
reach, todo = {START}, [START]
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
                ident_trie_nodes=len(IDPFX) + 1, punct_trie_nodes=len(OPPFX) + 1,
                peek_classes=[c for c in CLASSES if rowconst[c] is None],
                ), [cols[k] for k in cols]


if __name__ == "__main__":
    st, classes = stats()
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, "delta.json")
    with open(out, "w") as f:
        json.dump({"start": START, "states": {k: [m, {str(v): list(e) for v, e in row.items()}]
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
