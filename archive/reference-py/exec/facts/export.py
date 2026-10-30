#!/usr/bin/env python3
"""Export domain facts to exec/facts/*.tsv (K2; format in exec/assemble.py).

TABLES declares (fact stem, input files, producer).  A producer returns a list
of lines in the facts format; the written file starts with one
`# input PATH sha256:PREFIX` line per input, like weights/gold/*.tsv.

  python3 exec/facts/export.py            write every declared table
  python3 exec/facts/export.py --check    exit 1 when a committed table differs
"""
import hashlib
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent


# _autonames went with k2-gen2 to tools/facts-gen.cx on 2026-10-30.


# _module, _parse_e and _gen2ns (the parse2 namespace) went with the k2 tables to tools/facts-gen.cx on 2026-10-30.


# _ppsrc (E2 source facts for pp-gen/pp-autoinc-gen) went with them to tools/facts-gen.cx on 2026-10-30.


# lex-gen moved to tools/facts-gen.cx on 2026-10-30.


def _esc(s):
    return s.replace("\\", "\\\\").replace("\t", "\\t").replace("\n", "\\n")


# k2-constexpr and structreturnexpr moved to tools/facts-gen.cx on 2026-10-30.


def _gold(name):
    lines = (ROOT / "weights" / "gold" / (name + ".tsv")).read_text().splitlines()
    return [x.split("\t") for x in lines if x and not x.startswith("#") and "=>" not in x]


# lower-code moved to tools/facts-gen.cx (literals: exec/facts/enc-literals.tsv) on 2026-10-30.



# lower-armfuse moved to tools/facts-gen.cx (facts: exec/facts/tape-shape.tsv) on 2026-10-30.


# lower-abi moved to tools/facts-gen.cx (winapi names and SYSA: exec/facts/enc-literals.tsv) on 2026-10-30.


# opt-gen (E4 START data and peep indices) moved to tools/facts-gen.cx on 2026-10-30.


# top-modelbindings-template, parse-tokens and k2-tokenlocations-map moved to tools/facts-gen.cx on 2026-10-30.


# nativeabi moved to tools/facts-gen.cx (trie choice order recorded in exec/nativeabi/trie-order.tsv) on 2026-10-30.


# pp-autoinc-gen moved to tools/facts-gen.cx (closure from seed/unisa_libneed.c) on 2026-10-30.


# pp-gen moved to tools/facts-gen.cx (escapes: exec/facts/pp-escapes.tsv) on 2026-10-30.


# (fact stem, inputs whose sha prefixes head the file, producer)
# k2-gen2 moved to tools/facts-gen.cx (literals: exec/facts/gen2-constants.tsv, gen2-literals.tsv) on 2026-10-30.


# k2-gen2-tokens moved to tools/facts-gen.cx on 2026-10-30.


# k2-units-tokens moved to tools/facts-gen.cx on 2026-10-30.


TABLES = [
]


def render(stem, inputs, producer):
    head = ["# exec/facts/%s.tsv: written by exec/facts/export.py, do not edit" % stem]
    head += ["# input %s sha256:%s" % (p, hashlib.sha256((ROOT / p).read_bytes()).hexdigest()[:16])
             for p in inputs]
    return "\n".join(head + producer()) + "\n"


def main(argv):
    if argv not in ([], ["--check"]):
        print("usage: python3 exec/facts/export.py [--check]", file=sys.stderr)
        return 2
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
    if check:
        bad += _bank_owners()
    print("facts: %d tables, %d differ" % (len(TABLES), bad) if check else "facts: wrote %d tables" % len(TABLES))
    return 1 if bad else 0


# Bank ownership (was runtime asserts in layoutfacts.py / libraryexports.py): an owner's bank
# list must not be reused by any other stage's constant table (`# source: literal constants
# formerly in ...` name/value tables, the stage constants).
BANK_OWNERS = (("layoutfacts", "RESERVED_BANKS"), ("libraryexports", "RANK_BANKS"))


def _bank_owners():
    import json
    consts = {}
    for path in sorted(HERE.glob("*.tsv")):
        lines = path.read_text().split("\n")
        if len(lines) < 2 or not lines[1].startswith("# source: literal constants formerly in"):
            continue
        consts[path.stem] = {}
        for ln in lines[2:]:
            f = ln.split("\t")
            if len(f) == 2 and not ln.startswith("#"):
                consts[path.stem][f[0]] = json.loads(f[1]) if f[1][:1] in "[-0123456789" else f[1]
    bad = 0
    for owner, name in BANK_OWNERS:
        owned = set(consts[owner][name])
        for stem, d in consts.items():
            hit = sorted(k for k, v in d.items() if stem != owner and type(v) is int and v in owned)
            if hit:
                bad += 1
                print("bank owned by %s.%s reused in %s: %s" % (owner, name, stem, ", ".join(hit)))
    return bad


def _fieldchain():
    """Field-chain recorder for image headers: domain items only.  Each field row is
    (width, value int|register name, lead = the literal-byte / computation item
    names that precede it); `tail` is the lead after the last field."""
    class C:
        def __init__(self):
            self.fields, self.lead = [], []
        def lit(self, bs):
            self.lead += ["byte%d" % b for b in bs]
        def comp(self, name):
            self.lead.append(name)
        def field(self, w, v, endian="little"):
            self.fields.append(dict(width=w, value=v, src="const" if isinstance(v, int) else "reg",
                                  endian=endian, lead=self.lead))
            self.lead = []
    return C()


def _rows(name, rows):
    import json
    out = ["@%s\ti:int\twidth:int\tsrc:str\tvalue:json\tendian:str\tlead:json" % name]
    for i, r in enumerate(rows):
        out.append("\t%d\t%d\t%s\t%s\t%s\t%s" % (i, r["width"], r["src"], json.dumps(r["value"]), r["endian"],
                                                json.dumps(r["lead"], separators=(",", ":"))))
    return out


# enc-pe and enc-pe-forward moved to tools/facts-gen.cx (facts: exec/facts/image-pe.tsv) on 2026-10-30.


# enc-macho and enc-elf moved to tools/facts-gen.cx (facts: exec/facts/image-macho.tsv, image-elf.tsv) on 2026-10-30.


# enc-x86 and enc-arm moved to tools/facts-gen.cx (literals: exec/facts/enc-literals.tsv) on 2026-10-30.

# k2-strings and k2-unitlocations moved to tools/facts-gen.cx on 2026-10-30.


# k2-membercontrol moved to tools/facts-gen.cx on 2026-10-30.


# k2-truth, k2-stack and k2-librarymodule-map moved to tools/facts-gen.cx on 2026-10-30.



# k2-libraryenv moved to tools/facts-gen.cx on 2026-10-30.

# signature-layout moved to tools/facts-gen.cx on 2026-10-30.

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
