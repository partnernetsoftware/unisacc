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



def _gold(name):
    lines = (ROOT / "weights" / "gold" / (name + ".tsv")).read_text().splitlines()
    return [x.split("\t") for x in lines if x and not x.startswith("#") and "=>" not in x]


def lowercode():
    """Lower code-route target facts: tape-word interning per os/arch (TAPE SHAPE words,
    tape registers, fixed spellings), the machine register / encoding form each word
    carries, and the side-table key bases (was exec/lower/code.py)."""
    import json
    sys.path.insert(0, str(ROOT))
    from unisa.tape import SHAPE
    out = ["=%s\tint\t%d" % (n, (105 + i) << 40)
           for i, n in enumerate(("OP", "KIND", "AC", "ARG", "TXT", "REG", "FORM", "ARGREG"))]
    words = list(dict.fromkeys(list(SHAPE) + ["r" + str(i) for i in range(8)] + [
        "0", "1", "2", "3", "4", "8", "-8", "_start:", "write", "exit",
        ".hostcall", ".hostaddr", ".librarycall", ".libraryaddr"]))
    from unisa import lower as L
    for n in ("WIN_HSTD", "WIN_WRITTEN", "WIN_SAVE", "WIN_ARGVA", "SYSA", "SYSFP", "SYSSP"):
        out.append("=%s\tint\t%d" % (n, getattr(L, n)))
    out.append("=WIN_STACK_END\tint\t%d" % (L.WIN_EXTRA + L.WIN_STACK))
    out += ["=SYSA%d\tint\t%d" % (i, L.SYSA + 8 * i) for i in range(6)]
    out += ["=key_argc\tjson\t" + json.dumps("\0process/argc"), "=key_argv\tjson\t" + json.dumps("\0process/argv")]
    out.append("@words\ttarget:str\ti:int\tword:str\trslots:json\treg:json\tform:json")
    for os_ in ("lnx", "osx", "win"):
        for arch in ("x86_64", "arm64"):
            regmap = {r[0]: r[2] for r in _gold("regmap") if r[1] == arch}
            enc = {r[0]: r[3] for r in _gold("enc") if r[1:3] == [os_, arch]}
            for i, w in enumerate(words):
                out.append("\t%s/%s\t%d\t%s\t%s\t%s\t%s" % (
                    os_, arch, i, _esc(w), json.dumps([j for j, k in enumerate(SHAPE.get(w, ())) if k == "r"]),
                    json.dumps([regmap[w]] if w in regmap else []),
                    json.dumps([" form=" + enc[w]] if w in enc else [])))
    return out



def lowerarmfuse():
    """ARM immediate fusion: TAPE SHAPE ops eligible under exec/lower/armfuse-shapes.tsv
    (shape / exclude / read policy), with the operand positions read (was armfuse.py)."""
    import json
    sys.path.insert(0, str(ROOT))
    from unisa.tape import SHAPE
    pol = [x.split("\t") for x in (ROOT / "exec/lower/armfuse-shapes.tsv").read_text().splitlines() if x and not x.startswith("#")]
    shapes = {tuple(v.split(",")) for k, v in pol if k == "shape"}
    excl = {v for k, v in pol if k == "exclude"}
    reads = {v for k, v in pol if k == "read"}
    out = ["=none\tjson\t[]"]
    out += ["@shapes\ti:int\top:str\treads:json\tfirst:json\trest:json"]
    sel = [(o, sh) for o, sh in SHAPE.items() if sh in shapes and o not in excl]
    for i, (o, sh) in enumerate(sel):
        out.append("\t%d\t%s\t%s\t%s\t%s" % (i, _esc(o), json.dumps([j for j, k in enumerate(sh[1:], 1) if k in reads]),
                                             json.dumps([1] if i == 0 else []), json.dumps([] if i == 0 else [1])))
    return out



def lowerabi():
    """Lower syscall ABI per os/arch (domain data, no labels): gold abi rows (sysno, argument
    registers, return register, gate, number register, argument shape, return convention,
    Windows import), encoding form, Windows API name, and whether the target lowers the op
    (native syscall number, or a Windows API with an import)."""
    import json
    sys.path.insert(0, str(ROOT))
    from unisa.catalog import WINAPI
    out = ["@abiops\ttarget:str\top:str",
           ]
    rows = ["@abi\ttarget:str\top:str\tsysno:str\targs:json\tret:str\tgate:str\tnrreg:str\targshape:str\tretconv:str\twinimp:str\twinapi:str\tform:str\tselected:int"]
    for os_ in ("lnx", "osx", "win"):
        for arch in ("x86_64", "arm64"):
            t = os_ + "/" + arch
            abi = {r[0]: r[3:] for r in _gold("abi") if r[1:3] == [os_, arch]}
            enc = {r[0]: r[3] for r in _gold("enc") if r[1:3] == [os_, arch]}
            out += ["\t%s\t%s" % (t, _esc(o)) for o in dict.fromkeys([*abi, "exit_group"])]
            for o, f in abi.items():
                sel = f[0] != "none" or (os_ == "win" and WINAPI.get(o) is not None and f[12] != "none")
                rows.append("\t" + "\t".join([t, _esc(o), f[0], json.dumps(f[1:7]), f[7], f[8], f[9], f[10], f[11], f[12],
                                                 "none" if WINAPI.get(o) is None else WINAPI[o], enc.get(o, ""), str(int(sel))]))
    return out + rows


# (fact stem, inputs whose sha prefixes head the file, producer)
TABLES = [
    ("structreturnexpr", ["exec/parse/gen.py", "exec/facts/export.py"], structreturnexpr),
    ("lower-armfuse", ["unisa/tape.py", "exec/lower/armfuse-shapes.tsv", "exec/facts/export.py"], lowerarmfuse),
    ("lower-abi", ["unisa/catalog.py", "weights/gold/abi.tsv", "weights/gold/enc.tsv", "exec/facts/export.py"], lowerabi),
    ("lower-code", ["unisa/tape.py", "unisa/lower.py", "weights/gold/regmap.tsv", "weights/gold/enc.tsv", "exec/facts/export.py"], lowercode),
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
