#!/usr/bin/env python3
"""Is the prec table C?  Every ordered pair of binary operators, against the
system compiler.  [referee ledger: prec]

`prec` gives each binary operator a binding strength (1 `||` .. 10 `* / %`).
The enumeration proves the net equals that table; it cannot prove the table
is C.  Here cc is the referee: for each pair (o1, o2) the line
    a o1 b o2 c
is compiled by cc and its value compared with the value the TABLE implies --
o2 binds tighter when prec(o2) > prec(o1), else (a o1 b) o2 c (all these
operators are left-associative).  A pair counts only where the two groupings
give different values for some constants tried; a pair no constants tell
apart is reported as undetermined, not as passed.
"""
import itertools
import os
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PREC = {}
for ln in open(os.path.join(ROOT, "weights", "gold", "prec.tsv")):
    f = ln.rstrip("\n").split("\t")
    if ln.startswith("#") or len(f) != 2 or not f[1].isdigit():
        continue
    PREC[f[0]] = int(f[1])
CONSTS = [(7, 3, 2), (5, 0, 1), (1, 2, 3), (0, 1, 1), (6, 6, 1), (12, 5, 3)]


def c_op(o, x, y):
    """C's value of x o y for small non-negative ints (None: undefined here)"""
    if o in ("/", "%") and y == 0:
        return None
    if o in ("<<", ">>") and (not 0 <= y < 31 or x < 0 or x >= 1 << 20):
        return None
    if o == "/":
        q = abs(x) // abs(y)
        return q if (x < 0) == (y < 0) else -q
    if o == "%":
        return x - y * c_op("/", x, y)
    f = {"+": lambda: x + y, "-": lambda: x - y, "*": lambda: x * y, "<<": lambda: x << y, ">>": lambda: x >> y,
         "<": lambda: int(x < y), ">": lambda: int(x > y), "<=": lambda: int(x <= y), ">=": lambda: int(x >= y),
         "==": lambda: int(x == y), "!=": lambda: int(x != y), "&": lambda: x & y, "^": lambda: x ^ y, "|": lambda: x | y,
         "&&": lambda: int(bool(x) and bool(y)), "||": lambda: int(bool(x) or bool(y))}[o]
    return f()


def grouped(o1, o2, a, b, c, right):
    if right:
        r = c_op(o2, b, c)
        return None if r is None else c_op(o1, a, r)
    l = c_op(o1, a, b)
    return None if l is None else c_op(o2, l, c)


def main():
    lines, keys = [], []
    for o1, o2 in itertools.product(sorted(PREC), repeat=2):
        for a, b, c in CONSTS:
            L, R = grouped(o1, o2, a, b, c, False), grouped(o1, o2, a, b, c, True)
            if L is None or R is None or L == R:
                continue
            keys.append((o1, o2, (a, b, c), L, R))
            lines.append('    printf("%%d\\n", %d %s %d %s %d);' % (a, o1, b, o2, c))
            break
        else:
            keys.append((o1, o2, None, None, None))
    src = "#include <stdio.h>\nint main(void) {\n" + "\n".join(lines) + "\n    return 0;\n}\n"
    with tempfile.TemporaryDirectory() as d:
        open(os.path.join(d, "p.c"), "w").write(src)
        subprocess.run(["cc", "-w", "-Wno-error", "-Wno-parentheses", "-o", os.path.join(d, "p"), os.path.join(d, "p.c")], check=True, timeout=60)
        out = subprocess.run([os.path.join(d, "p")], capture_output=True, timeout=10).stdout.split()
    got = iter(int(x) for x in out)
    agree = differ = undet = 0
    for o1, o2, abc, L, R in keys:
        if abc is None:
            undet += 1
            continue
        v = next(got)
        table = R if PREC[o2] > PREC[o1] else L
        if v == table:
            agree += 1
        else:
            differ += 1
            print("  DIFFER  %d %s %d %s %d: cc %d, the table implies %d" % (abc[0], o1, abc[1], o2, abc[2], v, table))
    print("prec_audit  pairs %d   agree %d   differ %d   undetermined %d" % (len(keys), agree, differ, undet))
    return 1 if differ or not agree else 0


if __name__ == "__main__":
    sys.exit(main())
