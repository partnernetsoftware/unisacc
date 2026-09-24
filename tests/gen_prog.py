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
import sys

MASK = 0xFFFFFFFF


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
    def __init__(self, seed):
        self.r = Rnd(seed)
        self.seed = seed
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
    def stmt(self, vars, d, out):
        k = self.r.n(10)
        pad = "    " * (d + 1)
        if d >= 3:
            k = self.r.n(3)
        if k == 0:
            out.append("%s%s = %s;" % (pad, self.ivar(vars), self.expr(vars)))
        elif k == 1:
            out.append("%s%s += %s;" % (pad, self.ivar(vars), self.expr(vars)))
        elif k == 2:
            out.append("%s%s = (%s) ? %s : %s;" %
                       (pad, self.ivar(vars), self.cond(vars),
                        self.expr(vars), self.expr(vars)))
        elif k == 3:
            out.append("%sif (%s) {" % (pad, self.cond(vars)))
            self.block(vars, d + 1, out)
            out.append("%s} else {" % pad)
            self.block(vars, d + 1, out)
            out.append("%s}" % pad)
        elif k == 4:
            # a bounded loop: the count is a literal, so it always ends
            i = self.uniq("i")
            out.append("%sfor (int %s = 0; %s < %d; %s++) {"
                       % (pad, i, i, 1 + self.r.n(6), i))
            self.block(vars + [i], d + 1, out)
            out.append("%s}" % pad)
        elif k == 5:
            i = self.uniq("w")
            out.append("%s{ int %s = %d;" % (pad, i, 1 + self.r.n(5)))
            out.append("%s  while (%s > 0) {" % (pad, i))
            self.block(vars + [i], d + 1, out)
            out.append("%s    %s--;" % (pad, i))
            out.append("%s  } }" % pad)
        elif k == 6:
            out.append("%sswitch ((%s) & 3) {" % (pad, self.expr(vars)))
            for c in range(self.r.n(3) + 1):
                out.append("%scase %d:" % (pad, c))
                self.block(vars, d + 1, out)
                if self.r.n(2):
                    out.append("%s    break;" % pad)
            out.append("%sdefault: %s ^= 1; }" % (pad, self.ivar(vars)))
        elif k == 7:
            # an array, indexed only inside its bounds
            a = self.uniq("arr")
            j = self.uniq("j")
            n = 2 + self.r.n(6)
            out.append("%sint %s[%d];" % (pad, a, n))
            out.append("%sfor (int %s = 0; %s < %d; %s++) %s[%s] = %s;"
                       % (pad, j, j, n, j, a, j, self.expr(vars)))
            out.append("%s%s += %s[(%s) & %d];"
                       % (pad, self.ivar(vars), a, self.expr(vars), n - 1))
        elif k == 8:
            # a pointer that never leaves its object
            p = self.uniq("ptr")
            v = self.ivar(vars)
            out.append("%sint *%s = &%s; *%s = (*%s) + %s;"
                       % (pad, p, v, p, p, self.small()))
        else:
            out.append("%s%s = %s;" % (pad, self.ivar(vars), self.expr(vars)))

    def block(self, vars, d, out):
        for _ in range(1 + self.r.n(3)):
            self.stmt(vars, d, out)

    # -- the program ------------------------------------------------------
    def program(self):
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


def main(argv):
    out = argv[1]
    n = int(argv[2]) if len(argv) > 2 else 40
    first = int(argv[3]) if len(argv) > 3 else 1
    import os
    os.makedirs(out, exist_ok=True)
    for seed in range(first, first + n):
        open("%s/s%05d.c" % (out, seed), "w").write(Gen(seed).program())
    print(n)


if __name__ == "__main__":
    main(sys.argv)
