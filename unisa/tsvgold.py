"""The gold tables read back from their data form, weights/gold/*.tsv. [J10]

gold.py computes each stage's truth table from rules; `gold-export` writes
the enumerated table, with its schema (field values and head classes, in
order).  This module rebuilds a Stage from that file ALONE, so the weights
can be constructed without gold.py -- the first step of moving construction
out of the seed.  `python3 -m unisa build-weights --from-tsv --check` builds
every stage from the files and requires the UNS2 blob to equal the shipped
weights/built.uns2 byte for byte.
"""
import os

from .gold import Stage


def load_stage(path):
    name = None
    fields, heads, rows = [], [], {}
    header = None
    with open(path, encoding="utf-8") as f:
        for line in f:
            line = line.rstrip("\n")
            if line.startswith("# stage "):
                name = line[len("# stage "):].split(":", 1)[0]
                continue
            parts = line.split("\t")
            if parts[0] == "#field":
                fields.append((parts[1], tuple(parts[2:])))
                continue
            if parts[0] == "#head":
                heads.append((parts[1], tuple(parts[3:]),
                              None if parts[2] == "-" else parts[2]))
                continue
            if header is None:
                header = parts
                continue
            m = len(fields)
            rows[tuple(parts[:m])] = {h: c for (h, _, _), c
                                      in zip(heads, parts[m:])}
    if name is None or not fields or not heads or header is None:
        raise ValueError("%s: not a gold table with a schema" % path)
    n = 1
    for _, vo in fields:
        n *= len(vo)
    if len(rows) != n:
        raise ValueError("%s: %d rows, the fields' product is %d"
                         % (path, len(rows), n))

    def label(*kv):
        return rows[tuple(kv)]
    return Stage(name, fields, heads, label, cfg=None)


def load_all(d, names):
    return {n: load_stage(os.path.join(d, n + ".tsv")) for n in names}
