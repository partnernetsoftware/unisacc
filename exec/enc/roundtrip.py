#!/usr/bin/env python3
"""exec/enc/roundtrip.py FILE.c... -- real TargetPrograms (the Python front end
and unisa/lower.py, lnx/x86_64) dumped as TIns text and parsed back: the
instructions' op, args and meta and the label positions must survive, compared
by type as well as value (True is not 1).  This is an instruction/meta/label
round trip, not TargetProgram fidelity: data, syms and the target are not in
the text.  A program with a form the text lacks is reported, not cut.
With ROUNDTRIP_MUTATE=1 one bool meta is turned into an int after the dump's
parse, and the comparison must then fail (a control)."""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", ".."))
sys.path.insert(0, HERE)
import tins                                     # noqa: E402
from unisa.__main__ import _oracle              # noqa: E402
from unisa.driver import compile_file           # noqa: E402
from unisa.lower import lower                   # noqa: E402

def same(a, b):
    """equal by type and value, recursively"""
    if type(a) is not type(b):
        return False
    if isinstance(a, (list, tuple)):
        return len(a) == len(b) and all(same(x, y) for x, y in zip(a, b))
    if isinstance(a, dict):
        return a.keys() == b.keys() and all(same(a[k], b[k]) for k in a)
    return a == b


if len(sys.argv) < 2:
    sys.exit("roundtrip: no input files")
bad = 0
o = _oracle("built")
for f in sys.argv[1:]:
    tape = compile_file([f], o, "lnx/x86_64")
    tp = lower(tape, "lnx/x86_64", o, drive="built")
    try:
        text = tins.dump(tp)
    except ValueError as e:
        print("  NO FORM %s: %s" % (f, e)); bad += 1; continue
    back = tins.parse(text)
    if os.environ.get("ROUNDTRIP_MUTATE"):
        for ins in back.code:
            b = [k for k, v in ins.meta.items() if isinstance(v, bool)]
            if b:
                ins.meta[b[0]] = int(ins.meta[b[0]])
                break
    ok = (len(back.code) == len(tp.code) and same(back.labels, tp.labels) and
          all(a.op == b.op and same(list(a.args), list(b.args)) and same(a.meta, b.meta) for a, b in zip(tp.code, back.code)))
    print("  %s %s  insns %d  labels %d" % ("same" if ok else "DIFFER", f, len(tp.code), len(tp.labels)))
    bad += not ok
sys.exit(1 if bad else 0)
