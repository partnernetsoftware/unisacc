"""E3 delta generator: shared expression/type helpers and handwritten
state/action rules, with selected attributes read from the gold tables.

The declarative grammar compiler described in archive/docs/exec/e3-structured.md is
an unfulfilled design, not the implementation of this file. Runtime model
construction does not by itself remove the handwritten compilation rules.

    python3 exec/parse2/gen2.py OUT.json
    python3 exec/parse2/gen2.py --locations OUT.json
    python3 exec/parse2/gen2.py --warnings OUT.json
    python3 exec/parse2/gen2.py --errors OUT.json

Warnings imply locations. Errors add located diagnostics and recovery.
Unsupported forms are rejected explicitly. Coverage is recorded in the
fixed probe lists and prd.md, not inferred from the presence of a rule.
"""
import json
import sys
from pathlib import Path
import importlib.util
_spec = importlib.util.spec_from_file_location("e3gen", str(Path(__file__).resolve().parents[1] / "parse" / "gen.py"))
E = importlib.util.module_from_spec(_spec)   # the token reader, the assembler P, the gold tables, the tape constants
_spec.loader.exec_module(E)
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "build"))
import parse2base as _base   # exec/build/parse2base.py: the parse2 executor (rank-slot P, DEFS, token additions)
P = _base.install(E)
g = E.g


def build(locations=False, warnings=False, errors=False):
    import assemble
    # Unit markers are emitted only by the model framing pass. Each scan's
    # first marker resets the epoch; single-unit token dumps keep epoch zero.
    _base.tokens(E)
    # The location/static readers replace NEXT later.  Keep the plain token
    # decoder for lookahead; ordinary qualifier recursion must still pass
    # through NEXT so each source token gets its ordinal.
    assert "TN.raw" not in g.st
    env = assemble.run(Path(__file__).resolve().parent / 'gen-manifest.tsv', E, P, dict(locations=locations, warnings=warnings, errors=errors), {})
    start = env["ex_ret"]
    g.finish()
    states = {n: [m, {str(k): v for k, v in row.items()}] for n, (m, row) in g.st.items()}
    return {"start": start, "states": states, "seqs": [list(map(list, s)) for s in g.seqs]}


if __name__ == "__main__":
    warnings = "--warnings" in sys.argv
    if warnings:
        sys.argv.remove("--warnings")
    errors = "--errors" in sys.argv
    if errors: sys.argv.remove("--errors")
    locations = "--locations" in sys.argv or warnings or errors
    if "--locations" in sys.argv:
        sys.argv.remove("--locations")
    d = build(locations=locations, warnings=warnings, errors=errors)
    twice = _base.twice()
    assert not twice, "defined twice: %r" % twice
    s = json.dumps(d, separators=(",", ":"))
    open(sys.argv[1], "w").write(s)
    st, ent, live, ns, na = E.sizes(d)
    sys.stderr.write("states %d  entries %d (not 'unreachable' %d)  action seqs %d (%d actions)  json %d B\n"
                     % (st, ent, live, ns, na, len(s)))
