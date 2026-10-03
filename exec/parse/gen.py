"""Shared transition assembler and token-reader support for the model pipeline.

The retired E3 grammar/code-walker was removed after parse2 became the sole
parser entry point. Historical versions remain in git; this module supplies
P, the token reader, formatting helpers and shared data to current generators.
It is not a second parser. Some retained helpers still encode language rules;
they are not claimed to be a fully language-independent constructor.
"""
import os
import sys
import importlib.util

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "exec", "facts"))
from load import facts as _facts  # noqa: E402
# Loaded by path, not by name.  There are nine modules called `gen.py` under
# exec/, and `from gen import G` after a sys.path insert resolves against
# sys.modules first: measured, if another one has already been imported under
# that name this line raises
#     ImportError: cannot import name 'G' from 'gen' (.../exec/lex/gen.py)
# It works today only because this module happens to reach the name first.
# exec/build/gen.py (prune, lower) already loads its dependency by explicit path for the
# same reason; this is that same pattern.
_spec = importlib.util.spec_from_file_location(
    "k2_build_graph_parse", os.path.join(ROOT, "exec", "build", "graph.py"))
_bg = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_bg)
G = _bg.G
sys.path.insert(0, ROOT)
sys.path.insert(0, os.path.join(ROOT, "exec"))   # importers rely on these (were set by exec/pp/gen.py)


def gold(name):
    rows = []
    for ln in open(os.path.join(ROOT, "weights", "gold", name + ".tsv"), encoding="utf-8"):
        if ln.startswith("#"):
            continue
        f = ln.rstrip("\n").split("\t")
        if "=>" in f:
            continue
        rows.append(f)
    return rows


PREC = {f[0]: int(f[1]) for f in gold("prec") if f[1].isdigit()}
BINSEL = {(f[0], f[1]): f[2] for f in gold("binsel") if f[1] in ("s", "u")}
IRSEL = {f[1]: f[2] for f in gold("irsel") if f[0] == "alu"}


def optext(op, u=False):
    sp = IRSEL[BINSEL[(op, "u" if u else "s")]]
    rev = sp.endswith("_rev")
    sp = sp[:-4] if rev else sp
    if sp in ("div", "mod", "udiv", "umod"):
        sp = "." + sp
    return "  %s r0, %s\n" % (sp, "r0, r1" if rev else "r1, r0")


_FC = {r["name"]: r["value"] for r in _facts("parse-constants")}
HEADER = _FC["HEADER"]
FOOTER = _FC["FOOTER"]

_FW = {}
for _r in _facts("parse-words"):
    _FW.setdefault(_r["group"], []).append(_r["value"])
WORDS = _FW["words_pre"] + [o + "=" for o in _FW["casops"]] + sorted(PREC) + _FW["words_post"]
TK = {w: k + 1 for k, w in enumerate(WORDS)}
TK["type"] = TK["type=int"]   # x is the UA_TYPESPELL dump: every other spelling is TK_OTHER
TK_ID = _FC["TK_ID"]
TK_NUM = _FC["TK_NUM"]
TK_BADNUM = _FC["TK_BADNUM"]
TK_OTHER = _FC["TK_OTHER"]
TK_STR = _FC["TK_STR"]
TK_FNUM = _FC["TK_FNUM"]
CASOPS = tuple(_FW["casops"])
GMARK = _FC["GMARK"]
LOC = _FC["LOC"]
FND = _FC["FND"]
UNDO = _FC["UNDO"]
FR = _FC["FR"]
DIG = _FC["DIG"]
VS = _FC["VS"]
TDD = _FC["TDD"]
TDB = _FC["TDB"]
TDN = _FC["TDN"]
ARR = _FC["ARR"]
PTR = _FC["PTR"]
BASE = _FC["BASE"]
CUNK = _FC["CUNK"]
FRD = _FC["FRD"]
FRB = _FC["FRB"]
DPR = _FC["DPR"]
VAR = _FC["VAR"]
AUT = _FC["AUT"]
AUD = _FC["AUD"]
VANAMES = tuple(_FW["vanames"])   # the reference's builtins (va_copy is undefined there: measured)
# struct layouts: STAG[tag] = sid (1..63); SSZ[sid] = size; member key v*64 + sid ->
# MOF offset, MSZ size, MPT pointer depth, MBS base size.  Measured: each member is
# aligned to its own size, the struct's size is rounded up to its largest member
# (struct { char c; long x; short s; int *p; int i; }: c@0 x@8 s@16 p@24 i@32, size 40).
SBB = _FC["SBB"]
STAG = _FC["STAG"]
SSZ = _FC["SSZ"]
MOF = _FC["MOF"]
MSZ = _FC["MSZ"]
MPT = _FC["MPT"]
MBS = _FC["MBS"]
TWORDS = tuple(_FW["twords"])

g = G()


def O(s):
    return [("OUT", c) for c in s.encode()]


def rej(k):
    return [("REJECT", k)]


# ---- the token reader: a byte trie over the dump's lines -------------------
def tokenizer(qualifiers=("type=const", "type=volatile")):
    from pathlib import Path
    from finite_rules import install_template
    spans = dict(line.split("\t") for line in Path(HERE, "token-prefixes.tsv").read_text().splitlines() if line and not line.startswith("#"))
    # Reader rows (entry, span, trie edges, qualifier/word, tail): token-template.tsv; the
    # facts are the prefix nodes in sorted order with their kind-dependent row lists.
    pre = {""}
    for w in WORDS + list(spans) + list(qualifiers):
        for i in range(1, len(w) + 1):
            pre.add(w[:i])
    N = []
    for p in sorted(pre):
        st = "NX" + p
        if p in spans:
            N.append(dict(st=st, span=[dict(target=spans[p])], edges=[], q=[], w=[], tail=[]))
            continue
        N.append(dict(st=st, span=[], edges=[dict(b=b, to="NX" + p + chr(b)) for b in range(256) if p + chr(b) in pre],
                      q=[1] if p in qualifiers else [], w=[dict(tok=TK[p])] if p not in qualifiers and p in TK else [],
                      tail=[dict(tok=TK_OTHER)]))
    install_template(g, HERE, "token", dict(N=N), None, mode="b")
    from finite_rules import install as install_rules
    bindings = {name: globals()[name] for name in ("TK_ID", "TK_STR", "TK_NUM", "TK_BADNUM")}
    # Derive each pre-multiply bound from the integer domain, never frozen answers.
    bindings.update(("limit" + str(d), (2**64 - 1 - d) // 10) for d in range(10))
    install_rules(g, HERE, "tokenread", bindings=bindings,
                  sequences={"truncated": rej("not covered: truncated token dump")})


# ---- a small structured assembler onto (state, r) rows ---------------------
_pspec = importlib.util.spec_from_file_location("k2_build_procs", os.path.join(ROOT, "exec", "build", "procs.py"))
_procs = importlib.util.module_from_spec(_pspec)
_pspec.loader.exec_module(_procs)


class P(_procs.make_P(g, O, rej)):
    """the generic procedure assembler (exec/build/procs.py) plus this pipeline's token,
    decimal-print and value-stack helpers"""

    def tok(self, cases, other):
        self.branch({(TK[k] if isinstance(k, str) else k): v for k, v in cases.items()}, other, [("RLD", "tk")])

    def expect(self, w):
        ok = self.fresh("e")
        self.tok({w: ok}, ("rej", "not covered: expected " + w))
        self.cur = ok
        return self

    def num(self, slot):          # print W[slot] in decimal
        return self.a(("COPYW", "n", slot)).call("PRN")

    def lab(self, slot):
        return self.o("L").num(slot)

    def vpush(self, *slots):
        for s in slots:
            self.a(("STX", "vsp", VS, s), ("ALUI", "add", "vsp", "vsp", 1))
        return self

    def vpop(self, *slots):
        for s in reversed(slots):
            self.a(("ALUI", "sub", "vsp", "vsp", 1), ("LDX", s, "vsp", VS))
        return self

    def newlab(self, slot):
        return self.a(("ALUI", "add", "lab", "lab", 1), ("COPYW", slot, "lab"))


# numeric_rules/prn/fconv: still called by exec/parse2/units.py:24 (E.prn(); E.fconv()), cdx file
def numeric_rules(section, bindings=None, owner=None):
    # Unique metadata instances also satisfy gen2's duplicate-definition guard.
    from pathlib import Path
    from finite_rules import install as install_rules
    bindings = dict(bindings or {}, DIG=DIG, TK_FNUM=TK_FNUM)
    for line in (Path(HERE).parent / "facts" / "numeric-names.tsv").read_text().splitlines():
        if not line.startswith("#"):
            selected, name, prefix, kind = line.split("\t")
            if selected == section:
                bindings[name] = P((owner or prefix) + ".numeric_" + name).fresh(kind)
    install_rules(g, HERE, "numeric", bindings=bindings, section=section)


def prn():
    # One declared decimal algorithm, instantiated at widths zero and six.
    suffixes = ("", ".loop", ".out0", ".pad", ".sp", ".out", ".done") + tuple(".d%d" % i for i in range(20))
    for name, width in (("PRN", 0), ("PRNW", 6)):
        bindings = {"PRN" + suffix.replace(".", "_"): name + suffix for suffix in suffixes}
        numeric_rules("prn", dict(bindings, width=width), owner=name)


def fconv():
    numeric_rules("fconv")


def tyinfo():                 # stage tyinfo (weights/gold/tyinfo.tsv): type key -> (size, unsigned)
    return {f[0]: (int(f[1]), int(f[2])) for f in gold("tyinfo") if len(f) >= 3 and f[1].isdigit()}


TY = tyinfo()
CTY = _FC["CTY"]
SZ = {c: TY[k][0] for c, k in CTY.items()}
PSZ = TY["ptr"][0]
assert SZ == {"char": 1, "short": 2, "int": 4, "long": 8} and PSZ == 8, (SZ, PSZ)   # measured widths
assert not any(TY[k][1] for k in CTY.values())
# Value descriptor tags shared with the current parser.
UNS = _FC["UNS"]
DBL = _FC["DBL"]
FLT = _FC["FLT"]
FPB = _FC["FPB"]


PUSH = _FC["PUSH"]
POP1 = _FC["POP1"]


def sizes(d):
    unr = [i for i, s in enumerate(d["seqs"]) if s == [["REJECT", "unreachable"]]]
    ent = sum(len(r) for _, r in d["states"].values())
    live = sum(1 for _, r in d["states"].values() for v in r.values() if v[1] not in unr)
    return len(d["states"]), ent, live, len(d["seqs"]), sum(len(a) for a in d["seqs"])


if __name__ == "__main__":
    sys.exit("The retired E3 generator is no longer an entry point; use exec/parse2/gen2.py.")
