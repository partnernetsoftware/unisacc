"""Generic parse2 executor base (K2): the build/parsebase.py assembler and parse token facts, extended for parse2.

No stage rules here: only executor behaviour shared by every parse2 manifest row.
  P          E.P whose vpush/vpop append each value slot's rank companion (facts valueranks, kind=rank)
             and which counts every procedure/label definition in DEFS (a name defined twice merges two
             states silently; the driver refuses it).
  tokens(E)  the token codes parse2 appends before any row installs (type=extern, type=_Bool) and the
             qualifier tokenizer.
"""
import importlib.util
import pathlib
import sys

_ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(_ROOT / "facts"))
from load import facts as _facts

_RANKOF = {r["name"]: r["value"] for r in _facts("valueranks") if r["kind"] == "rank"}
QUALIFIERS = ("type=const", "type=volatile", "type=restrict", "type=inline")
DEFS = {}   # (name, how) -> count


def slots(items):   # each value slot is followed by its rank companion (valueranks facts)
    result = []
    for item in items:
        result.append(item)
        companion = _RANKOF.get(item)
        if companion and companion not in items:
            result.append(companion)
    return result


def install(E):
    """Install the parse2 P on executor module E (returns it)."""
    class P(E.P):
        def vpush(self, *items):
            return super().vpush(*slots(items))

        def vpop(self, *items):
            return super().vpop(*slots(items))

        def __init__(self, name):
            DEFS[name, "P"] = DEFS.get((name, "P"), 0) + 1
            super().__init__(name)

        def label(self, lab):
            DEFS[lab, "L"] = DEFS.get((lab, "L"), 0) + 1
            return super().label(lab)
    E.P = P
    return P


def tokens(E):
    for w in ("type=extern", "type=_Bool"):
        E.WORDS.append(w)
        E.TK[w] = max(E.TK.values()) + 1
    import assemble   # token reader: exec/parse/tokens2-manifest.tsv (facts k2-gen2-tokens)
    assemble.run(pathlib.Path(__file__).resolve().parents[1] / 'parse' / 'tokens2-manifest.tsv', E, E.P, {}, {})


def twice():
    return sorted(k for k, n in DEFS.items() if n > 1)


def executor():
    """exec/build/gen.py base hook: build/parsebase.py (assembler, parse token facts) with the parse2 P and tokens."""
    spec = importlib.util.spec_from_file_location("e3gen", str(_ROOT / "build" / "parsebase.py"))
    E = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(E)
    install(E)
    tokens(E)
    assert "TN.raw" not in E.g.st
    return E


def check(E):
    """exec/build/gen.py base hook after the run: a procedure/label defined twice merges two states silently."""
    t = twice()
    assert not t, "defined twice: %r" % t
