#!/usr/bin/env python3
"""The numbers the two back ends must agree on, checked. [A-43]

`unisa/lower.py` derives the scratch-area layout from named constants that
build on each other:

    SCRATCH = 144 ... PRINTMAX = 24
    WIN_HSTD = SCRATCH + PRINTMAX ... WIN_EXTRA = WIN_ARGVA + 512

`src/back_*.c` is a hand port, and it carries the ANSWERS as bare
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

BACK = [os.path.join(R, "src", n) for n in ("back_lower.c", "back_encode.c", "back_image.c")]


def c_literal(pattern, text, what):
    """The integer a C line assigns, by a pattern with one group."""
    m = re.search(pattern, text)
    if not m:
        return None, "no line matching %s (%s)" % (pattern, what)
    return int(m.group(1)), None


def main():
    from unisa import lower
    from unisa.tape import DATA_BASE

    c = "".join(open(f, encoding="latin-1").read() for f in BACK)
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

    # The three image writers.  C spells these in decimal (4294967296 is
    # macho.VMADDR = 0x100000000), which is exactly why a drift would be
    # invisible to a reader: nobody recognises 5368709120 as 0x140000000.
    from unisa.image import elf, macho, pe
    ehdr_and_phdrs = 64 + 56 * elf.NPH          # where ELF text starts
    checks += [
        ("elf.VADDR (text)", elf.VADDR, r"bk_textva = (\d+) \+ \d+;"),
        ("elf.VADDR (data)", elf.VADDR, r"bk_datava = (\d+) \+ bk_round\(\d+ \+ bktlen, 4096\)"),
        ("elf headers", ehdr_and_phdrs, r"bk_textva = \d+ \+ (\d+);"),
        ("elf.PAGE", elf.PAGE, r"bk_datava = \d+ \+ bk_round\(\d+ \+ bktlen, (\d+)\)"),
        ("elf entry", elf.VADDR, r"w64\((\d+) \+ \d+ \+ bk_entry\)"),
        ("macho.VMADDR (text)", macho.VMADDR, r"bk_textva = (\d+) \+ h;"),
        ("macho.VMADDR (data)", macho.VMADDR, r"bk_datava = (\d+) \+ bk_round\(h \+ bktlen"),
        ("macho.PAGE", macho.PAGE, r"bk_datava = \d+ \+ bk_round\(h \+ bktlen, (\d+)\)"),
        ("pe.IMAGEBASE (text)", pe.IMAGEBASE, r"bk_textva = (\d+) \+ 4096;"),
        ("pe.TEXT_RVA", pe.TEXT_RVA, r"bk_textva = \d+ \+ (\d+);\s*\n[^\n]*\n[^\n]*bk_datava = 5368709120"),
        ("pe.IMAGEBASE (data)", pe.IMAGEBASE, r"bk_datava = (\d+) \+ rd"),
        ("pe.IMAGEBASE (IAT)", pe.IMAGEBASE, r"bk_imp\[i\] = (\d+) \+ rd"),
        ("len(pe.IMPORTS)", len(pe.IMPORTS), r"#define BK_NIMP (\d+)"),
    ]

    bad = 0
    for name, want, pattern in checks:
        got, why = c_literal(pattern, c, name)
        if why:
            print("  FAIL %-22s %s" % (name, why))
            bad += 1
        elif got != want:
            print("  FAIL %-22s back end says %d, lower.py says %d"
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
    print("consts  agreed %d   differ %d   (unisa/lower.py vs src/back_*.c)"
          % (n - bad, bad))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
