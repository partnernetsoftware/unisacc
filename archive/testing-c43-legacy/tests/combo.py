#!/usr/bin/env python3
"""R19-7: construct-combination differential.

Exhaustive verification proves each constructed network equals its decision
table; it says nothing about which COMBINATIONS of constructs the tables and
the structural code cover.  dsh hit one twice (a function returning a struct,
called in `return f()`): the reference compiled it, the product said
"unknown identifier".  This enumerates value-producing constructs over types
and positions, and for every program checks:
  * the reference compiles it and the program prints what cc's prints;
  * the product (PRODUCT_COM) writes the same tape as the reference, or
    refuses it by name ("not covered: ...") -- never with a misleading
    diagnostic such as "unknown identifier" on a name that is declared.
Usage: tests/combo.py UA [PRODUCT_COM]   (cc must be on PATH)
"""
import os, subprocess, sys, tempfile, itertools

UA = sys.argv[1]
COM = sys.argv[2] if len(sys.argv) > 2 else os.environ.get("PRODUCT_COM", "")
BOUND = os.path.join(os.path.dirname(os.path.abspath(__file__)), "bound.py")

TYPES = {   # name: (declaration prefix, a value expression of type T built from n, scalar to print)
    "int":    ("typedef int T;", "(T)(n * 3)", "%d", "v"),
    "long":   ("typedef long T;", "(T)n * 100000L", "%ld", "v"),
    "char":   ("typedef char T;", "(T)(65 + n)", "%d", "v"),
    "double": ("typedef double T;", "(T)n / 4.0", "%.3f", "v"),
    "ptr":    ("typedef int *T; static int cells[8];", "(T)&cells[n % 8]", "%d", "(int)(v - cells)"),
    "small":  ("typedef struct { int a; int b; } T;", "mk(n)", "%d", "v.a + v.b"),
    "big":    ("typedef struct { int k; char raw[16]; int n; } T;", "mk(n)", "%d", "v.k + v.n + v.raw[3]"),
}
MK = {
    "small": "static T mk(int n) { T t; t.a = n; t.b = 2 * n; return t; }",
    "big":   "static T mk(int n) { T t; int i; t.k = n; t.n = 7; for (i = 0; i < 16; i++) t.raw[i] = (char)i; return t; }",
}
POSITIONS = {   # how the value of f(n) is used
    "return":  "T g(int n) { return f(n); }",
    "init":    "T g(int n) { T v = f(n); return v; }",
    "assign":  "T g(int n) { T v; v = f(n); return v; }",
    "arg":     "T id(T x) { return x; }\nT g(int n) { return id(f(n)); }",
    "ternary": "T g(int n) { return n > 0 ? f(n) : f(1); }",
    "nested":  "T h(int n) { return f(n); }\nT g(int n) { return h(n + 0); }",
}

def program(ty, pos):
    decl, expr, fmt, scalar = TYPES[ty]
    return "\n".join([
        "#include <stdio.h>", decl, MK.get(ty, ""),
        "T f(int n) { return %s; }" % expr,
        POSITIONS[pos],
        "int main(void) { T v = g(5); printf(\"%s\\n\", %s); return 0; }" % (fmt, scalar),
    ]) + "\n"

def run(cmd, timeout=20):
    return subprocess.run(["python3", BOUND, str(timeout)] + cmd, capture_output=True)

def main():
    tmp = tempfile.mkdtemp()
    ok = bad = refused = 0
    for ty, pos in itertools.product(TYPES, POSITIONS):
        name = "%s-%s" % (ty, pos)
        src = os.path.join(tmp, name + ".c")
        open(src, "w").write(program(ty, pos))
        exe = os.path.join(tmp, name + ".cc")
        c = run(["cc", "-std=c99", "-w", "-o", exe, src])
        if c.returncode:
            print("  skip %-16s cc refuses it" % name); continue
        want = run([exe]).stdout
        r = run([UA, src])                                   # compile and run in memory
        if r.returncode or r.stdout != want:
            bad += 1
            print("  FAIL %-16s reference: rc %d got %r want %r %s" % (name, r.returncode, r.stdout[:40], want[:40], r.stderr.decode(errors="replace")[:120]))
            continue
        if COM:
            rt = run([UA, src, "-S", "-o", "-"])
            pt = run(["sh", COM, src, "-S", "-o", "-"], 40)
            err = pt.stderr.decode(errors="replace")
            if pt.returncode == 0 and pt.stdout == rt.stdout:
                pass
            elif pt.returncode and "not covered" in err:
                refused += 1
                print("  UNS  %-16s product refuses by name: %s" % (name, err.strip().splitlines()[-1][:100]))
            else:
                bad += 1
                why = "tape differs" if pt.returncode == 0 else "misleading refusal: " + err.strip().splitlines()[0][:100] if err.strip() else "rc %d" % pt.returncode
                print("  FAIL %-16s product: %s" % (name, why))
                continue
        ok += 1
    print()
    print("combo  ok %d   wrong %d   refused-by-name %d%s" % (ok, bad, refused, "" if COM else "   (no PRODUCT_COM: reference only)"))
    return 0 if bad == 0 and ok > 0 else 1

if __name__ == "__main__":
    sys.exit(main())
