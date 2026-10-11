/* seed/unisa_fp.c -- C99 reference for unisa.fp (K6-9), point-sample functions.
 *
 *   unisa-fp op3 OP X Y     OP in fadd64 fsub64 fmul64 fdiv64 flt64 fle64 feq64
 *                           fadd32 fsub32 fmul32 fdiv32 flt32 fle32 feq32
 *   unisa-fp op2 OP X       OP in cvtid cvtud cvtis cvtus cvtdi cvtdu cvtsd cvtds fsqrt64 fsqrt32
 *       X, Y are uint64 decimal bit patterns; the result is printed as a uint64 decimal line.
 *
 * Mirrors unisa/fp.py: d/bd/s/bs, _div and _sqrt (canonical NaN 0x7ff8... / 0x7fc00000 as
 * math.nan packs), _i2f32 (int -> float rounded once to 24 bits), _f2int (truncate, saturate,
 * NaN -> 0).  Arithmetic follows the hardware exactly as Python does (double ops, one rounding to
 * float).  Builds with -ffp-contract=off -lm.  Point-sample equivalence over the listed points only;
 * dec_to_f32 (Fraction-exact decimal rounding) is NOT in this reference (out of the C domain).
 */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define M64 0xFFFFFFFFFFFFFFFFull
#define M32 0xFFFFFFFFull

static void die(const char *why, const char *what) {
    fprintf(stderr, "unisa-fp: %s %s\n", why, what ? what : "");
    exit(1);
}

static double d_of(uint64_t x) { double f; uint64_t v = x & M64; memcpy(&f, &v, 8); return f; }
static uint64_t bd_of(double f) { uint64_t u; memcpy(&u, &f, 8); return u; }
static float s_of_bits(uint64_t x) { float f; uint32_t v = (uint32_t)(x & M32); memcpy(&f, &v, 4); return f; }
static double s_of(uint64_t x) { return (double)s_of_bits(x); }

/* bs(f): double -> float bits, nearest-even; NaN -> 0x7FC00000 | sign; overflow -> +-inf */
static uint64_t bs_of(double f) {
    if (isnan(f)) return 0x7FC00000ull | (signbit(f) ? 0x80000000ull : 0);
    {
        float r = (float)f; uint32_t u; memcpy(&u, &r, 4); return u;
    }
}

static double div_py(double a, double b) {
    if (b != 0.0) return a / b;
    if (a != a || a == 0.0) return NAN;          /* python math.nan: positive quiet NaN */
    {
        int neg = (signbit(a) != 0) != (signbit(b) != 0);
        return neg ? -INFINITY : INFINITY;
    }
}
static double sqrt_py(double a) { if (a != a || a < 0) return NAN; return sqrt(a); }


/* _i2f32 for a signed or unsigned 64-bit magnitude (sign handled by caller) */
static uint64_t i2f32_mag(uint64_t v, int neg) {
    if (v < (1ull << 53)) {
        double f = (double)v; return bs_of(neg ? -f : f);
    }
    {
        int bl = 0; uint64_t t = v; while (t) { bl++; t >>= 1; }
        int sh = bl - 24; uint64_t q = v >> sh, rem = v & ((1ull << sh) - 1), half = 1ull << (sh - 1);
        double f;
        if (rem > half || (rem == half && (q & 1))) q++;
        f = ldexp((double)q, sh);      /* q has at most 25 bits: exact; q << sh may exceed uint64 */
        return bs_of(neg ? -f : f);
    }
}

/* _f2int: truncation toward zero, saturating, NaN -> 0 */
static uint64_t f2int(double f, int unsig) {
    double lo, hi;
    if (f != f) return 0;
    if (unsig) { lo = 0.0; hi = 18446744073709551615.0; }
    else { lo = -9223372036854775808.0; hi = 9223372036854775807.0; }
    if (f == INFINITY || f >= hi + 1.0) return unsig ? M64 : (uint64_t)INT64_MAX;
    if (f == -INFINITY || f <= lo - 1.0) return unsig ? 0 : (uint64_t)INT64_MIN;
    if (unsig) return (uint64_t)f;
    return (uint64_t)(int64_t)f;
}

static int64_t s64(uint64_t x) { return (int64_t)x; }

static int parse_u64(const char *s, uint64_t *out) {
    char *end = NULL; unsigned long long v;
    if (!*s) return 0;
    v = strtoull(s, &end, 10);
    if (*end) return 0;
    *out = (uint64_t)v; return 1;
}

int main(int argc, char **argv) {
    uint64_t x, y, r;
    if (argc == 5 && !strcmp(argv[1], "op3")) {
        const char *op = argv[2];
        if (!parse_u64(argv[3], &x) || !parse_u64(argv[4], &y)) die("bad uint64 argument", NULL);
        if (!strcmp(op, "fadd64") || !strcmp(op, "fsub64") || !strcmp(op, "fmul64") || !strcmp(op, "fdiv64") ||
            !strcmp(op, "flt64") || !strcmp(op, "fle64") || !strcmp(op, "feq64")) {
            double a = d_of(x), b = d_of(y);
            if (!strcmp(op, "fadd64")) r = bd_of(a + b);
            else if (!strcmp(op, "fsub64")) r = bd_of(a - b);
            else if (!strcmp(op, "fmul64")) r = bd_of(a * b);
            else if (!strcmp(op, "fdiv64")) r = bd_of(div_py(a, b));
            else if (!strcmp(op, "flt64")) r = a < b ? 1 : 0;
            else if (!strcmp(op, "fle64")) r = a <= b ? 1 : 0;
            else r = a == b ? 1 : 0;
        } else if (!strcmp(op, "fadd32") || !strcmp(op, "fsub32") || !strcmp(op, "fmul32") || !strcmp(op, "fdiv32") ||
                   !strcmp(op, "flt32") || !strcmp(op, "fle32") || !strcmp(op, "feq32")) {
            double a = s_of(x), b = s_of(y);
            if (!strcmp(op, "fadd32")) r = bs_of(a + b);
            else if (!strcmp(op, "fsub32")) r = bs_of(a - b);
            else if (!strcmp(op, "fmul32")) r = bs_of(a * b);
            else if (!strcmp(op, "fdiv32")) r = bs_of(div_py(a, b));
            else if (!strcmp(op, "flt32")) r = a < b ? 1 : 0;
            else if (!strcmp(op, "fle32")) r = a <= b ? 1 : 0;
            else r = a == b ? 1 : 0;
        } else die("op3 op outside the sampled set", op);
        printf("%llu\n", (unsigned long long)r);
        return 0;
    }
    if (argc == 4 && !strcmp(argv[1], "op2")) {
        const char *op = argv[2];
        if (!parse_u64(argv[3], &x)) die("bad uint64 argument", NULL);
        if (!strcmp(op, "cvtid")) r = bd_of((double)s64(x));
        else if (!strcmp(op, "cvtud")) r = bd_of((double)(x & M64));
        else if (!strcmp(op, "cvtis")) { int64_t v = s64(x); r = i2f32_mag(v < 0 ? (uint64_t)(-(v + 1)) + 1 : (uint64_t)v, v < 0); }
        else if (!strcmp(op, "cvtus")) r = i2f32_mag(x & M64, 0);
        else if (!strcmp(op, "cvtdi")) r = f2int(d_of(x), 0);
        else if (!strcmp(op, "cvtdu")) r = f2int(d_of(x), 1);
        else if (!strcmp(op, "cvtsd")) r = bd_of(s_of(x));
        else if (!strcmp(op, "cvtds")) r = bs_of(d_of(x));
        else if (!strcmp(op, "fsqrt64")) r = bd_of(sqrt_py(d_of(x)));
        else if (!strcmp(op, "fsqrt32")) r = bs_of(sqrt_py(s_of(x)));
        else die("op2 op outside the sampled set", op);
        printf("%llu\n", (unsigned long long)r);
        return 0;
    }
    fprintf(stderr, "usage: unisa-fp op3 OP X Y | op2 OP X\n");
    return 2;
}
