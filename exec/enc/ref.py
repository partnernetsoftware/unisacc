#!/usr/bin/env python3
"""The referee for the x86_64 encoder slices (research/e5-slice.md): the TIns text
(exec/enc/tins.py) through unisa/assemble.py -- its relaxation rounds and
unisa/emit_x86.encode; every instruction must encode (encoded == insns: no UD2
placeholder), the bytes on stdout.  A branch or call without reloc gets rel32:
a compatibility convention for the older fixtures, not a reading of the table."""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", ".."))
sys.path.insert(0, HERE)
import tins                             # noqa: E402
from unisa.assemble import assemble     # noqa: E402

tp = tins.parse(open(sys.argv[1]).read())
for ins in tp.code:
    if ins.op in ("jump", "jumpz", "call"):
        ins.meta.setdefault("reloc", "rel32")
code, st = assemble(tp)
if st["encoded"] != st["insns"]:
    sys.exit("ref: %d of %d instructions encode (the rest would be UD2)" % (st["encoded"], st["insns"]))
sys.stdout.buffer.write(code)
