"""Track external and unit-local definitions across a multi-source tape."""

INITIALISED = 760 << 40
STATIC_INITIALISED = 761 << 40
FUNCTION_DEFINED = 762 << 40
STATIC_FUNCTION_DEFINED = 763 << 40


def install(E, P):
    # Stage control lives in objectdefinitions-result.tsv (sections od.entry, od.rest,
    # fd.entry, fd.rest); fresh labels are declared in objectdefinitions-fresh.tsv.
    # Python binds only dynamic facts (the four definition banks and the '=' token
    # code) and keeps the residue: renaming GV.record / FN.def1 to their originals
    # and aliasing the old names to the new entry states (state order preserved).
    from pathlib import Path
    from finite_rules import install as rules
    g = E.g
    root = Path(__file__).parent
    bindings = {"INITIALISED": INITIALISED, "STATIC_INITIALISED": STATIC_INITIALISED,
                "FUNCTION_DEFINED": FUNCTION_DEFINED,
                "STATIC_FUNCTION_DEFINED": STATIC_FUNCTION_DEFINED, "tk.eq": E.TK["="]}

    class _Scope:
        def __init__(self, cur): self.cur = cur
    fresh = [l.split("\t") for l in (root / "objectdefinitions-fresh.tsv").read_text().splitlines()[1:]]

    def section(name):
        for part, key, kind, prefix in fresh:
            if part == name: bindings[key] = E.P.fresh(_Scope(prefix), kind)
        rules(g, root, "objectdefinitions", bindings, None, None, name)

    def rename(state, old):
        g.st[old] = g.st.pop(state)
        g.labels.add(old)

    rename("GV.record", "OD.original.GV.record")
    section("od.entry")
    g.st["GV.record"] = g.st["OD.entry"]
    section("od.rest")
    rename("FN.def1", "FD.original.FN.def1")
    section("fd.entry")
    g.st["FN.def1"] = g.st["FD.entry"]
    section("fd.rest")
