"""Generic executor base (K2): the graph, the procedure assembler and the parse token/constant facts.

Was the generic part of exec/parse/gen.py (deleted).  No stage rules and no domain derivation here:
  g          the graph under construction (exec/build/graph.py G)
  O, rej     OUT-byte and REJECT sequences
  P          exec/build/procs.py make_P plus token/decimal/value-stack helpers (slot names are facts)
  constants  every row of facts parse-constants as a module attribute, WORDS/TK from facts parse-tokens
             (written by exec/facts/export.py), the tuple groups of facts parse-words.
"""
import os
import sys
import importlib.util

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "exec", "facts"))
from load import facts as _facts  # noqa: E402
# Loaded by explicit path: several modules under exec/ are called gen.py / graph.py.
_spec = importlib.util.spec_from_file_location(
    "k2_build_graph_parse", os.path.join(ROOT, "exec", "build", "graph.py"))
_bg = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_bg)
G = _bg.G
sys.path.insert(0, ROOT)
sys.path.insert(0, os.path.join(ROOT, "exec"))   # importers rely on these

globals().update({r["name"]: r["value"] for r in _facts("parse-constants")})
from assemble import load_facts as _load_facts  # noqa: E402  (=NAME header-form facts)
_FT = _load_facts("parse-tokens")
WORDS = list(_FT["WORDS"])
TK = dict(_FT["TK"])
_FW = {}
for _r in _facts("parse-words"):
    _FW.setdefault(_r["group"], []).append(_r["value"])
CASOPS = tuple(_FW["casops"])
VANAMES = tuple(_FW["vanames"])
TWORDS = tuple(_FW["twords"])

g = G()


def O(s):
    return [("OUT", c) for c in s.encode()]


def rej(k):
    return [("REJECT", k)]


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
