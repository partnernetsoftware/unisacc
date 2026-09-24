#!/usr/bin/env python3
"""Random C programs, for the cases nobody thought to write. [A-46]

Every other suite tests something a person imagined: a probe, a corpus
entry, a C99 feature, a data shape.  The bugs that survive are the ones in
the COMBINATIONS nobody combined -- a cast inside a comparison inside a
loop whose induction variable is a `char`.

So: generate programs, run them under the system compiler and under
unisacc, and compare the output.  The system compiler is the oracle, which
is why every generated program must have DEFINED behaviour -- if it does
not, the two are allowed to differ and the suite becomes noise.  The rules
that keep it defined are, in order of how easily they are got wrong:

  * signed overflow is undefined, so signed arithmetic is done on values
    masked into a range where it cannot overflow, and anything that could
    grow without bound is unsigned (where wrapping is defined)
  * division and remainder guard the divisor, and never divide INT_MIN
    by -1
  * shift counts are 0..31 and the shifted value is unsigned
  * no variable is read before it is written
  * no pointer arithmetic leaves its array
  * the order of evaluation of arguments is unspecified, so no expression
    has two side effects in it

A failing seed names a program that regenerates byte for byte, which is
the whole reason the generator is deterministic and seeded.
"""
import re
import sys

MASK = 0xFFFFFFFF
# The eight shapes a program can take [S-15 A3].  `int` is the original:
# signed arithmetic through every control structure.  The rest each aim at
# a place a wrong type-table key could hide: unsigned wrap, char/short
# narrowing and promotion, structs by value, pointer arithmetic, floats,
# recursion, and a mix of widths in one function.  The first run of the
# seven new classes found three C-front-end bugs the ninety hand-written
# probes had never touched.
CLASSES = ("int", "unsigned", "narrow", "struct", "pointer", "float",
           "recursion", "mixed")


class Rnd:
    """mulberry32 -- the generator this project uses everywhere, so a seed
    means the same sequence here as it does in the training code."""

    def __init__(self, seed):
        self.s = seed & MASK

    def next(self):
        self.s = (self.s + 0x6D2B79F5) & MASK
        t = self.s
        t = ((t ^ (t >> 15)) * (t | 1)) & MASK
        t ^= (t + ((t ^ (t >> 7)) * (t | 61) & MASK)) & MASK
        t &= MASK
        return (t ^ (t >> 14)) & MASK

    def n(self, k):
        return self.next() % k

    def pick(self, seq):
        return seq[self.n(len(seq))]


class Gen:
    def __init__(self, seed, cls="int"):
        self.r = Rnd(seed)
        self.seed = seed
        self.cls = cls               # one of CLASSES: which program() shape
        self.depth = 0
        # A monotonic counter, not the nesting depth: two blocks at the
        # same depth would otherwise declare the same name, and a name
        # like `p0` also collides with the parameter `p0`.
        self.uid = 0

    def uniq(self, stem):
        self.uid += 1
        return "%s_%d" % (stem, self.uid)

    # -- expressions ------------------------------------------------------
    def ivar(self, vars):
        return self.r.pick(vars)

    def small(self):
        """A literal small enough that sums of a few cannot overflow."""
        return str(self.r.n(100) - 50)

    def expr(self, vars, d=0):
        """A signed int expression whose value stays in [-2^20, 2^20], so
        that adding or multiplying two of them cannot overflow."""
        if d >= 3 or self.r.n(100) < 25:
            if self.r.n(2):
                return self.small()
            return "(%s & 1023)" % self.ivar(vars)
        k = self.r.n(10)
        a = self.expr(vars, d + 1)
        b = self.expr(vars, d + 1)
        if k == 0:
            return "(%s + %s)" % (a, b)
        if k == 1:
            return "(%s - %s)" % (a, b)
        if k == 2:
            return "((%s) * ((%s) & 15))" % (a, b)
        if k == 3:
            return "((%s) / (((%s) & 7) + 1))" % (a, b)
        if k == 4:
            return "((%s) %% (((%s) & 7) + 1))" % (a, b)
        if k == 5:
            return "((%s) < (%s) ? (%s) : (%s))" % (a, b, a, b)
        if k == 6:
            return "((%s) ^ (%s))" % (a, b)
        if k == 7:
            return "(int)((unsigned)(%s) >> (%d))" % (a, self.r.n(8))
        if k == 8:
            return "((%s) == (%s))" % (a, b)
        return "(-(%s))" % a

    def cond(self, vars, d=0):
        a = self.expr(vars, d)
        b = self.expr(vars, d)
        return "(%s) %s (%s)" % (a, self.r.pick(["<", ">", "<=", ">=", "==", "!="]), b)

    # -- statements -------------------------------------------------------
    def stmt(self, vars, d, out, readonly=()):
        # `vars` is what may be ASSIGNED; `rd` is what may be READ.  The
        # two differ exactly at loop counters.
        rd = list(vars) + list(readonly)
        k = self.r.n(10)
        pad = "    " * (d + 1)
        if d >= 3:
            k = self.r.n(3)
        if k == 0:
            out.append("%s%s = %s;" % (pad, self.ivar(vars), self.expr(rd)))
        elif k == 1:
            out.append("%s%s += %s;" % (pad, self.ivar(vars), self.expr(rd)))
        elif k == 2:
            out.append("%s%s = (%s) ? %s : %s;" %
                       (pad, self.ivar(vars), self.cond(rd),
                        self.expr(rd), self.expr(rd)))
        elif k == 3:
            out.append("%sif (%s) {" % (pad, self.cond(rd)))
            self.block(vars, d + 1, out, readonly)
            out.append("%s} else {" % pad)
            self.block(vars, d + 1, out, readonly)
            out.append("%s}" % pad)
        elif k == 4:
            # A bounded loop.  The induction variable is NOT added to the
            # assignable set: the first version of this passed `vars + [i]`
            # down, the body reassigned the counter, and a generated
            # program ran for more than twenty seconds under cc as well as
            # under unisacc -- a generator that writes non-terminating
            # programs measures the timeout, not the compiler.
            i = self.uniq("i")
            out.append("%sfor (int %s = 0; %s < %d; %s++) {"
                       % (pad, i, i, 1 + self.r.n(6), i))
            self.block(vars, d + 1, out, list(readonly) + [i])
            out.append("%s}" % pad)
        elif k == 5:
            i = self.uniq("w")
            out.append("%s{ int %s = %d;" % (pad, i, 1 + self.r.n(5)))
            out.append("%s  while (%s > 0) {" % (pad, i))
            self.block(vars, d + 1, out, list(readonly) + [i])
            out.append("%s    %s--;" % (pad, i))
            out.append("%s  } }" % pad)
        elif k == 6:
            out.append("%sswitch ((%s) & 3) {" % (pad, self.expr(rd)))
            for c in range(self.r.n(3) + 1):
                # braces: a label labels a STATEMENT (C99 6.8.1), and a
                # declaration is not one.  New clang takes `case 0: int x;`
                # as a C23 extension; the macOS-14 runner's clang refused
                # eight seeds of the first version for exactly this.
                out.append("%scase %d: {" % (pad, c))
                self.block(vars, d + 1, out, readonly)
                if self.r.n(2):
                    out.append("%s    break;" % pad)
                out.append("%s}" % pad)
            out.append("%sdefault: %s ^= 1; }" % (pad, self.ivar(vars)))
        elif k == 7:
            # an array, indexed only inside its bounds
            a = self.uniq("arr")
            j = self.uniq("j")
            n = 2 + self.r.n(6)
            out.append("%sint %s[%d];" % (pad, a, n))
            out.append("%sfor (int %s = 0; %s < %d; %s++) %s[%s] = %s;"
                       % (pad, j, j, n, j, a, j, self.expr(rd)))
            out.append("%s%s += %s[(%s) & %d];"
                       % (pad, self.ivar(vars), a, self.expr(rd), n - 1))
        elif k == 8:
            # a pointer that never leaves its object
            p = self.uniq("ptr")
            v = self.ivar(vars)
            out.append("%sint *%s = &%s; *%s = (*%s) + %s;"
                       % (pad, p, v, p, p, self.small()))
        else:
            out.append("%s%s = %s;" % (pad, self.ivar(vars), self.expr(rd)))

    def block(self, vars, d, out, readonly=()):
        for _ in range(1 + self.r.n(3)):
            self.stmt(vars, d, out, readonly)

    # -- the programs: one shape per class --------------------------------
    def prog_int(self):
        L = ["/* generated by tests/gen_prog.py, seed %d -- do not edit */" % self.seed,
             "#include <stdio.h>", ""]
        fns = []                       # (name, arity), so the calls match
        for f in range(1 + self.r.n(3)):
            nm = "f%d" % f
            nargs = 1 + self.r.n(3)
            fns.append((nm, nargs))
            params = ", ".join("int p%d" % i for i in range(nargs))
            vars = ["p%d" % i for i in range(nargs)]
            L.append("static int %s(%s)" % (nm, params))
            L.append("{")
            L.append("    int a = %s, b = %s, c = %s;"
                     % (self.small(), self.small(), self.small()))
            self.block(vars + ["a", "b", "c"], 0, L)
            L.append("    return a + b + c + %s;" % vars[0])
            L.append("}")
            L.append("")
        L.append("int main(void)")
        L.append("{")
        L.append("    long total = 0;")
        for nm, nargs in fns:
            args = ", ".join(self.small() for _ in range(nargs))
            L.append("    total += %s(%s);" % (nm, args))
        L.append('    printf("%ld\\n", total);')
        L.append("    return 0;")
        L.append("}")
        return "\n".join(L) + "\n"

    # -- unsigned: wrapping is defined, so anything goes ------------------
    def uexpr(self, vars, d=0):
        if d >= 3 or self.r.n(100) < 25:
            if self.r.n(2):
                return "%uu" % self.r.n(1 << 16)
            return self.ivar(vars)
        k = self.r.n(8)
        a, b = self.uexpr(vars, d + 1), self.uexpr(vars, d + 1)
        if k == 0: return "(%s + %s)" % (a, b)
        if k == 1: return "(%s - %s)" % (a, b)
        if k == 2: return "(%s * %s)" % (a, b)
        if k == 3: return "(%s / ((%s & 15u) + 1u))" % (a, b)
        if k == 4: return "(%s %% ((%s & 15u) + 1u))" % (a, b)
        if k == 5: return "(%s >> (%s & 31u))" % (a, b)
        if k == 6: return "(%s << (%s & 31u))" % (a, b)
        return "(%s < %s)" % (a, b)

    def prog_unsigned(self, L):
        L.append("static unsigned f0(unsigned p0, unsigned p1)")
        L.append("{")
        L.append("    unsigned a = %uu, b = %uu; unsigned long w = 0;"
                 % (self.r.next(), self.r.next()))
        for _ in range(4 + self.r.n(6)):
            v = self.r.pick(["a", "b", "p0", "p1"])
            L.append("    %s = %s;" % (v, self.uexpr(["a", "b", "p0", "p1"])))
            L.append("    w = w * 31u + %s;" % v)
        L.append("    return a ^ b ^ p0 ^ p1 ^ (unsigned)(w >> 7);")
        L.append("}")
        L.append("int main(void)")
        L.append("{")
        L.append('    printf("%%u %%lu\\n", f0(%uu, %uu), '
                 '(unsigned long)f0(%uu, 3u) * 0x9E3779B97F4A7C15ul);'
                 % (self.r.next(), self.r.next(), self.r.next()))
        L.append("    return 0;")
        L.append("}")

    # -- narrow: char/short store and read back, promotion in arithmetic -
    def prog_narrow(self, L):
        tys = ["signed char", "unsigned char", "short", "unsigned short"]
        names = ["c%d" % i for i in range(4)]
        L.append("int main(void)")
        L.append("{")
        for t, n in zip(tys, names):
            L.append("    %s %s = %d;" % (t, n, self.r.n(200) - 100))
        L.append("    int acc = 0;")
        for _ in range(6 + self.r.n(8)):
            n = self.r.pick(names)
            k = self.r.n(5)
            e = self.expr(names)              # int-typed, in range
            if k == 0:
                L.append("    %s = %s;" % (n, e))          # narrowing store
            elif k == 1:
                L.append("    %s += %s;" % (n, self.small()))
            elif k == 2:
                L.append("    %s = (%s) * 3;" % (n, self.r.pick(names)))
            elif k == 3:
                L.append("    acc += (%s < %s) + (%s == %s);"
                         % (n, self.r.pick(names), n, self.r.pick(names)))
            else:
                L.append("    %s++;" % n)
            L.append("    acc = acc * 7 + %s;" % n)
        L.append('    printf("%d %d %d %d %d\\n", c0, c1, c2, c3, acc);')
        L.append("    return 0;")
        L.append("}")

    # -- struct: pass and return by value, nested, arrays of them ---------
    def prog_struct(self, L):
        n = 2 + self.r.n(3)
        fields = ["f%d" % i for i in range(n)]
        L.append("struct S { %s; };" % "; ".join(
            "%s %s" % (self.r.pick(["int", "long", "char", "short"]), f)
            for f in fields))
        L.append("struct T { struct S s; int tag; };")
        L.append("static struct S mk(int k)")
        L.append("{ struct S s; %s return s; }" % " ".join(
            "s.%s = k * %d + %d;" % (f, self.r.n(9) - 4, self.r.n(50)) for f in fields))
        L.append("static long sum(struct S s)")
        L.append("{ return %s; }" % " + ".join("(long)s." + f for f in fields))
        L.append("static struct T wrap(struct S s, int t)")
        L.append("{ struct T r; r.s = s; r.tag = t; r.s.%s += t; return r; }"
                 % self.r.pick(fields))
        L.append("int main(void)")
        L.append("{")
        L.append("    struct S arr[4]; long total = 0; int i;")
        L.append("    for (i = 0; i < 4; i++) arr[i] = mk(i - %d);" % self.r.n(4))
        for _ in range(3 + self.r.n(5)):
            k = self.r.n(4)
            i = self.r.n(4)
            if k == 0:
                L.append("    total += sum(arr[%d]);" % i)
            elif k == 1:
                L.append("    { struct T t = wrap(arr[%d], %d); total += t.tag * 3 + sum(t.s); }"
                         % (i, int(self.small())))
            elif k == 2:
                L.append("    arr[%d] = arr[%d]; arr[%d].%s -= %d;"
                         % (i, self.r.n(4), i, self.r.pick(fields), self.r.n(20)))
            else:
                L.append("    { struct S *p = &arr[%d]; p->%s = p->%s + %d; total += sum(*p); }"
                         % (i, self.r.pick(fields), self.r.pick(fields), self.r.n(9)))
        L.append('    printf("%ld\\n", total);')
        L.append("    return 0;")
        L.append("}")

    # -- pointer: arithmetic that stays inside its array ------------------
    def prog_pointer(self, L):
        n = 4 + self.r.n(8)
        L.append("static int walk(int *p, int n)")
        L.append("{ int *e = p + n; long s = 0; while (p < e) { s = s * 3 + *p; p++; } return (int)(s & 0xFFFF); }")
        L.append("int main(void)")
        L.append("{")
        L.append("    int a[%d]; int *p = a; int *q; long t = 0; int i;" % n)
        L.append("    for (i = 0; i < %d; i++) a[i] = i * %d - %d;" % (n, self.r.n(7), self.r.n(9)))
        for _ in range(4 + self.r.n(6)):
            k = self.r.n(6)
            if k == 0:
                L.append("    p = a + %d; *p += %s;" % (self.r.n(n), self.small()))
            elif k == 1:
                L.append("    q = &a[%d]; t += q - a; t += *(q - %d) ;" % (n - 1, self.r.n(n - 1)))
            elif k == 2:
                L.append("    q = a + %d; p = a + %d; t += (p < q) + (p == q) + (q - p);"
                         % (self.r.n(n), self.r.n(n)))
            elif k == 3:
                L.append("    t += walk(a + %d, %d);" % (self.r.n(n // 2), n // 2))
            elif k == 4:
                L.append("    { int **pp = &p; *pp = a + %d; **pp = %s; t += a[%d]; }"
                         % (self.r.n(n), self.small(), self.r.n(n)))
            else:
                L.append("    p = a; for (i = 0; i < %d; i++) t += p[i] * (i + 1);" % n)
        L.append('    printf("%ld\\n", t);')
        L.append("    return 0;")
        L.append("}")

    # -- float: double arithmetic, printed as a scaled integer so the two
    # printf implementations' rounding of %f is not what is compared ------
    def fexpr(self, vars, d=0):
        if d >= 3 or self.r.n(100) < 30:
            if self.r.n(2):
                return "%d.%d" % (self.r.n(50) - 25, self.r.n(100))
            return self.ivar(vars)
        k = self.r.n(6)
        a, b = self.fexpr(vars, d + 1), self.fexpr(vars, d + 1)
        if k == 0: return "(%s + %s)" % (a, b)
        if k == 1: return "(%s - %s)" % (a, b)
        if k == 2: return "(%s * %s)" % (a, b)
        if k == 3: return "(%s / (%s + 100.5))" % (a, "(%s < 0 ? -(%s) : (%s))" % (b, b, b))
        if k == 4: return "((%s) < (%s) ? (%s) : (%s))" % (a, b, a, b)
        return "(double)(int)(%s)" % a

    def prog_float(self, L):
        vs = ["x", "y", "z"]
        L.append("static double g(double x, int n) { double r = x; while (n-- > 0) r = r * 0.5 + 1.25; return r; }")
        L.append("int main(void)")
        L.append("{")
        L.append("    double x = %d.5, y = %d.25, z = 0.0; float f = 1.5f; int k = 3;"
                 % (self.r.n(20), self.r.n(20)))
        for _ in range(5 + self.r.n(6)):
            k = self.r.n(5)
            v = self.r.pick(vs)
            if k == 0:
                L.append("    %s = %s;" % (v, self.fexpr(vs)))
            elif k == 1:
                L.append("    %s = g(%s, %d);" % (v, self.r.pick(vs), self.r.n(4)))
            elif k == 2:
                L.append("    f = (float)(%s) * 0.75f; %s = f + k;" % (self.r.pick(vs), v))
            elif k == 3:
                L.append("    k = (int)(%s) & 255; %s = %s + k;" % (self.r.pick(vs), v, v))
            else:
                L.append("    if (%s < %s) %s = -%s;" % (self.r.pick(vs), self.r.pick(vs), v, v))
            # clamp so the magnitude stays where every double is exact enough
            L.append("    if (%s > 1e6 || %s < -1e6) %s = %s / 1024.0;" % (v, v, v, v))
        L.append('    printf("%ld %ld %ld %d\\n", (long)(x * 1000), (long)(y * 1000), (long)(z * 1000), k);')
        L.append("    return 0;")
        L.append("}")

    # -- recursion: depth bounded, mutual, with an accumulator ------------
    def prog_recursion(self, L):
        m = 2 + self.r.n(5)
        L.append("static int odd(int n);")
        L.append("static int even(int n) { if (n <= 0) return 1; return odd(n - 1) + %d; }" % self.r.n(3))
        L.append("static int odd(int n) { if (n <= 0) return 0; return even(n - 1) * %d; }" % (1 + self.r.n(2)))
        L.append("static long rec(int n, long acc)")
        L.append("{")
        L.append("    if (n <= 0) return acc;")
        L.append("    acc = acc * %d + n;" % (1 + self.r.n(3)))
        L.append("    if ((n & %d) == 0) return rec(n - 1, acc) + rec(n - 2, acc & 0xFFF);" % (1 + self.r.n(3)))
        L.append("    return rec(n - 1, acc + %d);" % self.r.n(9))
        L.append("}")
        L.append("static int ack(int m, int n) { if (m == 0) return n + 1; if (n == 0) return ack(m - 1, 1); return ack(m - 1, ack(m, n - 1)); }")
        L.append("int main(void)")
        L.append("{")
        L.append('    printf("%%ld %%d %%d\\n", rec(%d, %d), even(%d) + odd(%d), ack(2, %d));'
                 % (8 + self.r.n(10), self.r.n(5), m, m + 1, self.r.n(5)))
        L.append("    return 0;")
        L.append("}")

    # -- mixed: the int machinery of the base generator, but the locals
    # are a mix of widths and signedness, and the printf shows them all --
    def prog_mixed(self, L):
        tys = ["int", "long", "unsigned", "short", "unsigned char", "long"]
        vars = ["v%d" % i for i in range(4)]
        picked = [self.r.pick(tys) for _ in vars]
        L.append("int main(void)")
        L.append("{")
        for t, v in zip(picked, vars):
            L.append("    %s %s = %s;" % (t, v, self.small()))
        n0 = len(L)
        self.block(vars, 0, L)
        # the base generator's pointer statement is `int *ptr = &v`; here a
        # v may be a short or an unsigned char, and writing an int through
        # it is undefined, so the pointer takes the variable's own type
        ty = dict(zip(vars, picked))
        for j in range(n0, len(L)):
            m = re.match(r"(\s*)int \*(ptr_\d+) = &(v\d);", L[j])
            if m:
                L[j] = L[j].replace("int *", ty[m.group(3)] + " *", 1)
        L.append('    printf("%%ld %%ld %%ld %%ld\\n", %s);'
                 % ", ".join("(long)" + v for v in vars))
        L.append("    return 0;")
        L.append("}")

    def program(self):
        if self.cls == "int":
            return self.prog_int()
        L = ["/* generated by tests/gen_prog.py, class %s, seed %d -- do not edit */"
             % (self.cls, self.seed), "#include <stdio.h>", ""]
        getattr(self, "prog_" + self.cls)(L)
        return "\n".join(L) + "\n"


def main(argv):
    out = argv[1]
    n = int(argv[2]) if len(argv) > 2 else 40
    first = int(argv[3]) if len(argv) > 3 else 1
    cls = argv[4] if len(argv) > 4 else "int"
    if cls not in CLASSES:
        sys.exit("no such class %r; one of %s" % (cls, ", ".join(CLASSES)))
    import os
    os.makedirs(out, exist_ok=True)
    for seed in range(first, first + n):
        open("%s/%s_%05d.c" % (out, cls[0], seed), "w").write(Gen(seed, cls).program())
    print(n)


if __name__ == "__main__":
    main(sys.argv)
