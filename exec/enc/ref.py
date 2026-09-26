#!/usr/bin/env python3
"""The referee for the x86_64 encoder slice (research/e5-slice.md): each line of
a TIns fixture through unisa/emit_x86.encode; every line must encode (a None
would be a UD2 placeholder downstream), the bytes concatenated on stdout."""
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


out = bytearray()
for n, ln in enumerate(open(sys.argv[1]), 1):
    ln = ln.strip()
    if not ln:
        continue
    op, _, rest = ln.partition(" ")
    ins = TIns(op, [arg(a) for a in rest.split(",")] if rest else [])
    b = emit_x86.encode(ins, 0, {}, "x86_64")
    if b is None:
        sys.exit("ref: line %d does not encode: %s" % (n, ln))
    out += b
sys.stdout.buffer.write(bytes(out))
