#!/usr/bin/env python3
"""Is the tyinfo table C?  Each scalar type's size, signedness and narrowness,
against the system compiler.  [referee ledger: tyinfo]

For each type the table answers, cc prints sizeof(T), whether (T)-1 < 0
(signedness, integer types only), and whether sizeof(T) < sizeof(long)
(narrower than a register, integer types only).  Rows that are a
representation choice, not a C fact -- void's size, the 8 given to arr, fn,
struct and illegal, narrow on floats -- are skipped and listed.
"""
import os
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CT = {"i8": "signed char", "i16": "short", "i32": "int", "i64": "long", "u8": "unsigned char",
      "u16": "unsigned short", "u32": "unsigned int", "u64": "unsigned long", "ptr": "void *",
      "f32": "float", "f64": "double"}
INT = ("i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64")


def main():
    rows = {}
    for ln in open(os.path.join(ROOT, "weights", "gold", "tyinfo.tsv")):
        f = ln.rstrip("\n").split("\t")
        if ln.startswith("#") or len(f) != 4 or not f[1].isdigit():
            continue
        rows[f[0]] = (int(f[1]), int(f[2]), int(f[3]))
    ts = [t for t in rows if t in CT]
    src = ["#include <stdio.h>", "int main(void) {"]
    for t in ts:
        c = CT[t]
        sg = "((%s)-1 < 0)" % c if t in INT else "0"
        src.append('    printf("%%d %%d %%d\\n", (int)sizeof(%s), %s ? 0 : %d, (int)(sizeof(%s) < sizeof(long)));'
                   % (c, sg, 1 if t in INT else 0, c))
    src += ["    return 0;", "}"]
    with tempfile.TemporaryDirectory() as d:
        open(os.path.join(d, "t.c"), "w").write("\n".join(src) + "\n")
        subprocess.run(["cc", "-w", "-o", os.path.join(d, "t"), os.path.join(d, "t.c")], check=True, timeout=60)
        pr = subprocess.run([os.path.join(d, "t")], capture_output=True, timeout=10)
        out = pr.stdout.decode().split("\n")
    if pr.returncode != 0 or len([l for l in out if l]) != len(ts):
        print("tyinfo_audit  the probe exited %d with %d of %d lines" % (pr.returncode, len([l for l in out if l]), len(ts)))
        return 1
    agree = differ = 0
    for t, line in zip(ts, out):
        size, uns, narrow = (int(x) for x in line.split())
        gsize, guns, gnarrow = rows[t]
        checks = [("size", gsize, size)]
        if t in INT:
            checks += [("uns", guns, uns), ("narrow", gnarrow, narrow)]
        for name, g, c in checks:
            if g == c:
                agree += 1
            else:
                differ += 1
                print("  DIFFER  %s %s: table %d, cc %d" % (t, name, g, c))
    skipped = sorted(set(rows) - set(ts))
    print("tyinfo_audit  facts %d   agree %d   differ %d   skipped rows %s" % (agree + differ, agree, differ, " ".join(skipped)))
    return 1 if differ or not agree else 0


if __name__ == "__main__":
    sys.exit(main())
