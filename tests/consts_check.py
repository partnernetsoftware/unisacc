#!/usr/bin/env python3
"""The numbers the two back ends must agree on, checked. [A-43]

`unisa/lower.py` derives the scratch-area layout from named constants that
build on each other:

    SCRATCH = 144 ... PRINTMAX = 24
    WIN_HSTD = SCRATCH + PRINTMAX ... WIN_EXTRA = WIN_ARGVA + 512

`src/unisacc_back.c` is a hand port, and it carries the ANSWERS as bare
literals: `bk_hstd = base + 168`, `bk_zeros(776)`.  They are right today.
The hazard is the cascade: changing PRINTMAX in Python silently invalidates
six numbers in C, and the only thing that would notice is closure.sh
reporting that two images differ at some byte offset -- true, but it does
not say why, and the suite that says why is this one.

Everything here is read out of the two files as text.  Nothing is
generated: a generator would put the C file's layout under Python's
control, and the point of the C back end is that it stands alone.
"""
import os
import re
import sys

R = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, R)

BACK = os.path.join(R, "src", "unisacc_back.c")


def c_literal(pattern, text, what):
    """The integer a C line assigns, by a pattern with one group."""
    m = re.search(pattern, text)
    if not m:
        return None, "no line matching %s (%s)" % (pattern, what)
    return int(m.group(1)), None


def main():
    from unisa import lower
    from unisa.tape import DATA_BASE

    c = open(BACK, encoding="latin-1").read()
    L = lower

    # name -> (what Python says, how to find the C number)
    checks = [
        ("SYSA", L.SYSA, r"bk_sysa = base \+ (\d+)"),
        ("SYSFP", L.SYSFP, r"bk_sysfp = base \+ (\d+)"),
        ("SYSSP", L.SYSSP, r"bk_syssp = base \+ (\d+)"),
        ("WIN_HSTD", L.WIN_HSTD, r"bk_hstd = base \+ (\d+)"),
        ("WIN_WRITTEN", L.WIN_WRITTEN, r"bk_written = base \+ (\d+)"),
        ("WIN_SAVE", L.WIN_SAVE, r"bk_save = base \+ (\d+)"),
        ("WIN_ARGVA", L.WIN_ARGVA, r"bk_argva = base \+ (\d+)"),
        ("DATA_BASE", DATA_BASE, r"#define BK_DATA_BASE (\d+)"),
        # the two zero-fills: Windows reserves the whole extra area, every
        # other OS only the scratch cells and the print buffer
        ("WIN_EXTRA", L.WIN_EXTRA, r"if \(bkos == 2\) bk_zeros\((\d+)\);"),
        ("SCRATCH+PRINTMAX", L.SCRATCH + L.PRINTMAX,
         r"if \(bkos == 2\) bk_zeros\(\d+\); else bk_zeros\((\d+)\);"),
        ("WIN_STACK", L.WIN_STACK, r"bk_stacktop = base \+ \d+ \+ (\d+);"),
        ("WIN_EXTRA (stacktop)", L.WIN_EXTRA,
         r"bk_stacktop = base \+ (\d+) \+ \d+;"),
    ]

    bad = 0
    for name, want, pattern in checks:
        got, why = c_literal(pattern, c, name)
        if why:
            print("  FAIL %-22s %s" % (name, why))
            bad += 1
        elif got != want:
            print("  FAIL %-22s unisacc_back.c says %d, lower.py says %d"
                  % (name, got, want))
            bad += 1

    # The scratch cells themselves, which the C file spells as one line of
    # four: `bk_scr0 = base; bk_scr1 = base + 8; bk_plen = base + 16; ...`
    for name, want, pattern in [
            ("SCR1", 8, r"bk_scr1 = base \+ (\d+)"),
            ("PRINTLEN", 16, r"bk_plen = base \+ (\d+)"),
            ("PRINTBUF", 24, r"bk_pbuf = base \+ (\d+)"),
            ("ARGC", 48, r"bk_argc = base \+ (\d+)"),
            ("ARGV", 56, r"bk_argv = base \+ (\d+)")]:
        got, why = c_literal(pattern, c, name)
        if why or got != want:
            print("  FAIL %-22s %s" % (name, why or
                                       "C says %d, expected %d" % (got, want)))
            bad += 1

    n = len(checks) + 5
    print()
    print("consts  agreed %d   differ %d   (unisa/lower.py vs src/unisacc_back.c)"
          % (n - bad, bad))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
