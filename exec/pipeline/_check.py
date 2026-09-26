"""Shared driver for check_<fmt>.py: main(line_ok, whole_ok) -> exit status."""
import sys


def main(line_ok=None, whole_ok=None):
    b = open(sys.argv[1], "rb").read()
    if b"\0" in b:
        sys.stderr.write("NUL byte\n")
        return 1
    if whole_ok:
        m = whole_ok(b)
        if m:
            sys.stderr.write(m + "\n")
            return 1
    if line_ok:
        for n, l in enumerate(b.decode("latin-1").split("\n"), 1):
            if l and not line_ok(l):
                sys.stderr.write("line %d: %r\n" % (n, l[:80]))
                return 1
    return 0
