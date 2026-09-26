#!/usr/bin/env python3
"""The referee for the x86_64 encoder slices (research/e5-slice.md): the TIns
fixture (with `name:` label lines) through unisa/assemble.py -- its relaxation
rounds and unisa/emit_x86.encode; every instruction must encode (encoded ==
insns: no UD2 placeholder), the bytes on stdout."""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
from unisa import emit_x86            # noqa: E402
from unisa.lower import TIns          # noqa: E402


def arg(s):
    s = s.strip()
    try:
        return int(s, 0)
    except ValueError:
        return s


from unisa.lower import TargetProgram   # noqa: E402
from unisa.assemble import assemble     # noqa: E402

tp = TargetProgram("lnx/x86_64", b"", {})
for n, ln in enumerate(open(sys.argv[1]), 1):
    ln = ln.strip()
    if not ln:
        continue
    if ln.endswith(":") and " " not in ln:
        if ln[:-1] in tp.labels:
            sys.exit("ref: label defined twice at line %d" % n)
        tp.labels[ln[:-1]] = len(tp.code)
        continue
    op, _, rest = ln.partition(" ")
    args = [arg(a) for a in rest.split(",")] if rest else []
    tp.emit(op, *args, **({"reloc": "rel32"} if op in ("jump", "jumpz") else {}))
code, st = assemble(tp)       # unisa/assemble.py: the relaxation rounds, then encode
if st["encoded"] != st["insns"]:
    sys.exit("ref: %d of %d instructions encode (the rest would be UD2)" % (st["encoded"], st["insns"]))
sys.stdout.buffer.write(code)
