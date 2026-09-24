#!/usr/bin/env python3
"""Is the gold C?  The type table, key by key, against the system compiler.
[A-50] [S-15 A1]

Enumeration proves the constructed net equals the gold table -- every key,
exactly -- and it can never prove the table is C.  That limit is stated in
the paper (section 8) and it bit: the signed rows of `_arith` said
`int + int` is a long, the table and the net agreed, every suite was green,
and it took corpus 00200 to show it.

So here the system compiler is the referee.  For every pair of numeric
types and every binary operator the table answers, one line of C asks cc
what the result type is, through C11 _Generic, and whether the expression
is legal at all -- an error on that line means illegal.  One compile
collects every error; a second compile, with those lines commented out,
answers the types.  Then each key is compared with the gold.

Where the table deliberately says something else -- a representation
choice, not a claim about C -- the key is listed in tests/gold.knownfail
with the reason, and a listed key that starts AGREEING fails the audit,
so the list cannot rot.
"""
import os
import re
import subprocess
import sys
import tempfile

R = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, R)

# the numeric types of the table, spelled so that cc means the same thing on
# every platform: plain `char` is unsigned on arm64 Linux, so i8 is `signed
# char`, and `long` is 64 bits on every target this compiler has
C_TYPE = {"i8": "signed char", "i16": "short", "i32": "int", "i64": "long",
          "u8": "unsigned char", "u16": "unsigned short",
          "u32": "unsigned int", "u64": "unsigned long",
          "f32": "float", "f64": "double"}
# _Generic's answer, as the table's vocabulary.  `long long` is the table's
# i64 too: the table has one 64-bit signed type.
GEN = ("signed char: 1, short: 2, int: 3, long: 4, long long: 4, "
       "unsigned char: 5, unsigned short: 6, unsigned int: 7, "
       "unsigned long: 8, unsigned long long: 8, float: 13, double: 14, "
       "char: 1, default: 99")
CODE = {1: "i8", 2: "i16", 3: "i32", 4: "i64", 5: "u8", 6: "u16", 7: "u32",
        8: "u64", 13: "f32", 14: "f64", 99: "other"}
OPS = ("+", "-", "*", "/", "%", "<", "==", "=", "&", ",", "|", "^", "<<", ">>")


def expr(t1, op, t2):
    a, b = "(%s)1" % C_TYPE[t1], "(%s)1" % C_TYPE[t2]
    if op == "=":
        return "v_%s = %s" % (t1, b)          # needs an lvalue of type t1
    return "%s %s %s" % (a, op, b)


def main():
    from unisa import gold
    cases = [(t1, op, t2) for t1 in C_TYPE for op in OPS for t2 in C_TYPE]
    decl = "".join("    %s v_%s = 0;\n" % (C_TYPE[t], t) for t in C_TYPE)
    lines = ["    r[%d] = _Generic((%s), %s);" % (k, expr(*c), GEN)
             for k, c in enumerate(cases)]

    def source(skip):
        body = "\n".join(("    /* illegal */" if k in skip else ln)
                         for k, ln in enumerate(lines))
        return ("#include <stdio.h>\nint main(void) {\n    int r[%d];\n%s"
                "%s\n    for (int k = 0; k < %d; k++) printf(\"%%d\\n\", r[k]);\n"
                "    return 0;\n}\n" % (len(cases), decl, body, len(cases)))

    first_line = 3 + len(C_TYPE) + 1          # where r[0]'s line sits
    tmp = tempfile.mkdtemp()
    src = os.path.join(tmp, "audit.c")
    # pass 1: which lines are errors
    open(src, "w").write(source(set()))
    p = subprocess.run(["cc", "-std=c11", "-w", "-ferror-limit=0",
                        "-fsyntax-only", src], capture_output=True, text=True)
    illegal = set()
    for m in re.finditer(r"audit\.c:(\d+):\d+: error", p.stderr):
        illegal.add(int(m.group(1)) - first_line)
    # pass 2: the types of the rest
    open(src, "w").write(source(illegal))
    exe = os.path.join(tmp, "audit")
    p = subprocess.run(["cc", "-std=c11", "-w", "-o", exe, src],
                       capture_output=True, text=True)
    if p.returncode != 0:
        print("  FAIL the audit program itself did not build:")
        print(p.stderr[:600])
        return 1
    got = [int(x) for x in subprocess.run([exe], capture_output=True,
                                          text=True).stdout.split()]

    known = {}
    kf = os.path.join(R, "tests", "gold.knownfail")
    if os.path.exists(kf):
        for ln in open(kf):
            if ln.strip() and not ln.startswith("#"):
                f = ln.split(None, 3)
                known[(f[0], f[1], f[2])] = f[3].strip() if len(f) > 3 else ""

    agree = differ = knownn = revived = 0
    for k, (t1, op, t2) in enumerate(cases):
        want = "illegal" if k in illegal else CODE.get(got[k], "other")
        have = gold.type_label(t1, op, t2)
        same = (have == want)
        key = (t1, op, t2)
        if key in known:
            if same:
                revived += 1
                print("  REVIVED %s %s %s agrees with cc now -- drop it from "
                      "gold.knownfail" % key)
            else:
                knownn += 1
            continue
        if same:
            agree += 1
        else:
            differ += 1
            if differ <= 40:
                print("  DIFF  (%s) %-2s (%s)   gold says %-7s  cc says %s"
                      % (t1, op, t2, have, want))
    print()
    print("gold_audit  type keys checked %d   agree %d   differ %d   "
          "known deviations %d   (numeric operands, every binary operator)"
          % (len(cases), agree, differ, knownn))
    return 1 if (differ or revived or not agree) else 0


if __name__ == "__main__":
    sys.exit(main())
