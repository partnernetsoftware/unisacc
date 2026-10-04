#!/usr/bin/env python3
"""Small independent contracts for the nine manifest operations.

Each case assembles a private manifest, checks its observable graph effect,
then changes one source operand and requires the same contract to fail.
Neither a graph hash nor a previously generated delta is the oracle.
"""
from pathlib import Path
import json
import sys
import tempfile
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT), str(ROOT / "exec"), str(ROOT / "exec/build")]
import assemble
import graph
import procs


def row(*cells):
    assert len(cells) == 9
    return "\t".join(cells) + "\n"


def write(base, name, contents):
    (base / name).write_text(contents)


def machine(base, manifest):
    g = graph.G()
    e = SimpleNamespace(g=g, O=lambda s: [("OUT", c) for c in s.encode()],
                        rej=lambda s: [("REJECT", s)])
    e.P = procs.make_P(g, e.O, e.rej)
    old_facts, old_cache = assemble.FACTS, assemble._cache
    assemble.FACTS, assemble._cache = base / "facts", {}
    try:
        env = assemble.run(base / manifest, e, e.P, {}, {})
    finally:
        assemble.FACTS, assemble._cache = old_facts, old_cache
    return g, env


def edge(g, state, key):
    target, seq = g.st[state][1][key]
    return target, list(g.seqs[seq])


def fixture(op, base, mutate):
    """Return a contract closure; only the named op's source operand changes."""
    (base / "facts").mkdir(exist_ok=True)
    action = '[["OUT",65]]'
    end = "BAD" if mutate else "END"
    rule = f"S\t0\t{end}\t{action}\nS\t*\tEND\t[]\n"
    check = lambda g, env: edge(g, "S", 0) == ("END", [("OUT", 65)])
    if op == "rows":
        write(base, "r-byte.tsv", rule)
        write(base, "r-result.tsv", "R\t*\tEND\t[]\n")
        manifest = row("rows", "r", "-", "-", "-", "-", "-", "-", '{"domain":[0,2]}')
    elif op == "table":
        write(base, "r.tsv", rule)
        manifest = row("table", "r.tsv", "-", "-", "-", "-", "-", "-", '{"mode":"b","domain":[0,2]}')
    elif op == "template":
        write(base, "r-template.tsv", f"main\tblock\t-\t-\trule\tS\t0\t{end}\t{action}\n")
        manifest = row("template", "r", "main", "-", "-", "-", "-", "-", '{"mode":"b","overlay":true,"domain":[0,2]}')
    elif op == "call":
        write(base, "r.tsv", rule)
        write(base, "sub-manifest.tsv", row("table", "r.tsv", "-", "-", "-", "-", "-", "-", '{"mode":"b","domain":[0,2]}'))
        manifest = row("call", "sub", "-", "-", "-", "-", "-", "-", "-")
    elif op == "foreach":
        write(base / "facts", "items.tsv", "@items\tname:str\ttarget:str\n\tS\t" + end + "\n")
        write(base, "r.tsv", f"$NAME\t0\t$TARGET\t{action}\n$NAME\t*\tEND\t[]\n")
        manifest = row("foreach", "-", "-", "-", "items", "-", "-", "-", '{"over":"items","as":"it"}')
        manifest += "." + row("table", "r.tsv", "-", "-", "-", "-", "-", "NAME=it.name,TARGET=it.target", '{"mode":"b","domain":[0,2]}')
    elif op == "holder":
        write(base, "r.tsv", f"S\t0\t$TARGET\t{action}\nS\t*\tEND\t[]\n")
        manifest = row("holder", "H", "-", "-", "-", "U:ROOT", "-", "-", "-")
        manifest += row("table", "r.tsv", "-", "-", "-", "-", "-", "TARGET=fresh:=H:" + ("bad" if mutate else "k"), '{"mode":"b","domain":[0,2]}')
        check = lambda g, env: edge(g, "S", 0) == ("ROOT.k1", [("OUT", 65)])
    elif op == "label":
        manifest = row("label", end, "-", "-", "-", "-", "-", "-", "-")
        check = lambda g, env: g.labels == {"END"}
    elif op == "assert-absent":
        manifest = row("assert-absent", "S", "-", "-", "-", "-", "-", "-", json.dumps({"present": mutate}))
        check = lambda g, env: "S" not in g.st
    elif op == "let":
        write(base, "r.tsv", f"S\t0\t$TARGET\t{action}\nS\t*\tEND\t[]\n")
        manifest = row("let", "-", "-", "-", "-", "-", "-", "TARGET=@str:" + end, "-")
        manifest += row("table", "r.tsv", "-", "-", "-", "-", "-", "TARGET=$TARGET", '{"mode":"b","domain":[0,2]}')
    else:
        raise AssertionError(op)
    write(base, "main.tsv", manifest)
    return check


def main():
    ops = ("rows", "table", "template", "call", "foreach", "holder", "label", "assert-absent", "let")
    for op in ops:
        with tempfile.TemporaryDirectory(prefix="dsl-op-") as tmp:
            base = Path(tmp)
            check = fixture(op, base, False)
            g, env = machine(base, "main.tsv")
            assert check(g, env), f"{op}: expected graph effect missing"
            mutated = fixture(op, base, True)
            try:
                bad_g, bad_env = machine(base, "main.tsv")
            except (AssertionError, ValueError):
                caught = True
            else:
                caught = not mutated(bad_g, bad_env)
            assert caught, f"{op}: source mutation escaped"
        print(f"dsl op {op}: contract and mutation caught")
    print("dsl ops: 9/9 contracts, 9/9 mutations caught")


if __name__ == "__main__":
    main()
