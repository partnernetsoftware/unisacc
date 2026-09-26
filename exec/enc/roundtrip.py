#!/usr/bin/env python3
"""exec/enc/roundtrip.py FILE.c... -- real TargetPrograms (the Python front end
and unisa/lower.py, lnx/x86_64) dumped as TIns text and parsed back: op, args,
meta and label positions must survive (spinit's None by the stated
normalisation).  A program with a form the text lacks is reported, not cut."""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", ".."))
sys.path.insert(0, HERE)
import tins                                     # noqa: E402
from unisa.__main__ import _oracle              # noqa: E402
from unisa.driver import compile_file           # noqa: E402
from unisa.lower import lower                   # noqa: E402

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
    same = (len(back.code) == len(tp.code) and back.labels == tp.labels and
            all(a.op == b.op and list(a.args) == list(b.args) and a.meta == b.meta for a, b in zip(tp.code, back.code)))
    print("  %s %s  insns %d  labels %d" % ("same" if same else "DIFFER", f, len(tp.code), len(tp.labels)))
    bad += not same
sys.exit(1 if bad else 0)
