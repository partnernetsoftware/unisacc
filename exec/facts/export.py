#!/usr/bin/env python3
"""Export domain facts to exec/facts/*.tsv (K2; format in exec/assemble.py).

TABLES declares (fact stem, input files, producer).  A producer returns a list
of lines in the facts format; the written file starts with one
`# input PATH sha256:PREFIX` line per input, like weights/gold/*.tsv.

  python3 exec/facts/export.py            write every declared table
  python3 exec/facts/export.py --check    exit 1 when a committed table differs
"""
import hashlib
import importlib.util
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent


def _module(rel, name):
    spec = importlib.util.spec_from_file_location(name, ROOT / rel)
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m


def _esc(s):
    return s.replace("\\", "\\\\").replace("\t", "\\t").replace("\n", "\\n")


def structreturnexpr():
    E = _module("exec/parse/gen.py", "exec_parse_gen_facts")
    reason = "not covered: struct return expression outside local lvalue"
    return ["=reason\tstr\t" + _esc(reason),
            "@sr\tentry:str\tloc:int\treason:str",
            "\tLI.original.structreturn\t%d\t%s" % (E.LOC, _esc(reason))]


# (fact stem, inputs whose sha prefixes head the file, producer)
TABLES = [
    ("structreturnexpr", ["exec/parse/gen.py", "exec/facts/export.py"], structreturnexpr),
]


def render(stem, inputs, producer):
    head = ["# exec/facts/%s.tsv: written by exec/facts/export.py, do not edit" % stem]
    head += ["# input %s sha256:%s" % (p, hashlib.sha256((ROOT / p).read_bytes()).hexdigest()[:16])
             for p in inputs]
    return "\n".join(head + producer()) + "\n"


def main(argv):
    check = "--check" in argv
    bad = 0
    for stem, inputs, producer in TABLES:
        path = HERE / (stem + ".tsv")
        text = render(stem, inputs, producer)
        if check:
            try:
                same = path.read_text() == text
            except FileNotFoundError:
                same = False
            if not same:
                bad += 1
                print("facts differ: " + str(path.relative_to(ROOT)))
        else:
            path.write_text(text)
    print("facts: %d tables, %d differ" % (len(TABLES), bad) if check else "facts: wrote %d tables" % len(TABLES))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
