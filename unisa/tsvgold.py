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


class GoldDataError(ValueError):
    pass


def load_stage(path):
    """The stage in `path`, or GoldDataError.  The input contract, shared
    with any other reader (the C constructor to come): the schema's field
    and head names are unique, each field's values and each head's classes
    are unique; every row has exactly one column per field and per head;
    every key value is in its field, every label in its head's classes; no
    key repeats, and the rows cover the whole product of the fields."""
    def bad(msg):
        raise GoldDataError("%s: %s" % (path, msg))
    name = None
    fields, heads, rows = [], [], {}
    header = None
    with open(path, encoding="utf-8") as f:
        for ln, line in enumerate(f, 1):
            line = line.rstrip("\n")
            if line.startswith("# stage "):
                name = line[len("# stage "):].split(":", 1)[0]
                continue
            parts = line.split("\t")
            if parts[0] == "#field":
                if header is not None:
                    bad("line %d: schema after the header" % ln)
                if len(parts) < 3:
                    bad("line %d: a field with no values" % ln)
                fields.append((parts[1], tuple(parts[2:])))
                continue
            if parts[0] == "#head":
                if header is not None:
                    bad("line %d: schema after the header" % ln)
                if len(parts) < 4:
                    bad("line %d: a head with no classes" % ln)
                heads.append((parts[1], tuple(parts[3:]),
                              None if parts[2] == "-" else parts[2]))
                continue
            if header is None:
                want = [fn for fn, _ in fields] + ["=> " + hn for hn, _, _ in heads]
                if parts != want:
                    bad("line %d: header %r is not the schema's %r" % (ln, parts, want))
                header = parts
                continue
            m, k = len(fields), len(heads)
            if len(parts) != m + k:
                bad("line %d: %d columns, the schema has %d" % (ln, len(parts), m + k))
            key = tuple(parts[:m])
            for i, (fn, vo) in enumerate(fields):
                if key[i] not in vo:
                    bad("line %d: %r is not a value of field %s" % (ln, key[i], fn))
            lab = {}
            for j, (hn, cl, _) in enumerate(heads):
                c = parts[m + j]
                if c not in cl:
                    bad("line %d: %r is not a class of head %s" % (ln, c, hn))
                lab[hn] = c
            if key in rows:
                bad("line %d: key %r repeats" % (ln, key))
            rows[key] = lab
    if name is None or not fields or not heads or header is None:
        bad("not a gold table with a schema")
    for kind, names in (("field", [fn for fn, _ in fields]),
                        ("head", [hn for hn, _, _ in heads])):
        if len(set(names)) != len(names):
            bad("%s names repeat" % kind)
    for fn, vo in fields:
        if len(set(vo)) != len(vo):
            bad("field %s: values repeat" % fn)
    for hn, cl, _ in heads:
        if len(set(cl)) != len(cl):
            bad("head %s: classes repeat" % hn)
    n = 1
    for _, vo in fields:
        n *= len(vo)
    if len(rows) != n:          # unique, in-domain keys: equal count = full cover
        bad("%d keys, the fields' product is %d" % (len(rows), n))

    def label(*kv):
        return rows[tuple(kv)]
    return Stage(name, fields, heads, label, cfg=None)


def load_all(d, names):
    return {n: load_stage(os.path.join(d, n + ".tsv")) for n in names}
