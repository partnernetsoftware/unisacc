#!/usr/bin/env python3
"""Compile every enumerated tape with both back ends; the bytes must match.

The Python side stays in ONE process: importing unisa and building the
oracle per tape would cost more than the whole enumeration.  The C side is a
process per tape, because what is under test is the shipped binary, not a
library.

usage: layout_check.py <unisacc> <tapedir> <target> [<target> ...]
"""
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def main(argv):
    ua, tdir = argv[1], argv[2]
    targets = argv[3:] or ["lnx/x86_64"]

    from unisa import image
    from unisa.__main__ import _oracle
    from unisa.assemble import assemble
    from unisa.lower import lower
    from unisa.tape import DATA_BASE, parse as tparse

    o = _oracle("built")

    tapes = sorted(f for f in os.listdir(tdir) if f.endswith(".tape"))
    bad = 0
    n = 0
    for name in tapes:
        path = os.path.join(tdir, name)
        text = open(path, encoding="latin-1").read()
        for t in targets:
            n += 1
            try:
                tp = lower(tparse(text), t, o, drive="built")
                code, st = assemble(tp)
                data = image.relocate(tp, tp.data, st["data_va"] - DATA_BASE)
                want = image.build(tp, code, data, st["entry"])
            except Exception as e:
                bad += 1
                print("  FAIL %-24s %s: python back end: %s: %s"
                      % (name, t, type(e).__name__, e))
                continue
            r = subprocess.run([ua, path, "-b", t], stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE, timeout=60)
            got = r.stdout
            if got == want:
                continue
            bad += 1
            why = r.stderr.decode("latin-1", "replace").strip().split("\n")[0]
            if not got:
                print("  FAIL %-24s %s: the C back end wrote nothing (%s)"
                      % (name, t, why or "no message"))
            elif len(got) != len(want):
                print("  FAIL %-24s %s: %d B, python says %d B"
                      % (name, t, len(got), len(want)))
            else:
                at = next(i for i in range(len(got)) if got[i] != want[i])
                print("  FAIL %-24s %s: differs at byte %d (%02x, python %02x)"
                      % (name, t, at, got[at], want[at]))
    print()
    print("layout  tapes %d   compiles %d   wrong %d   targets %s"
          % (len(tapes), n, bad, ",".join(targets)))
    return 1 if (bad or not n) else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
