/* Floating conversions at their edges (C99 6.3.1.4-5, 6.3.1.8): unsigned
   64-bit values past 2^63 both ways, integers past 2^53 rounded once to a
   float, float <-> double, truncation toward zero, and the comparisons a NaN
   must fail.  Each is one or two instructions on one ISA and a sequence on
   the other, so each is checked on all of them. */
int main(void) {
    unsigned long big = 18446744073709551615UL;      /* 2^64 - 1 */
    unsigned long mid = 9223372036854775808UL;       /* 2^63 */
    long l53 = 9007199254740993L;                    /* 2^53 + 1 */
    double d, nan, zero = 0.0, nzero = -0.0;
    float f;
    unsigned long u;
    d = big;  printf("%.1f\n", d);
    d = mid;  printf("%.1f\n", d);
    f = big;  printf("%.1f\n", (double)f);
    f = l53;  printf("%.1f\n", (double)f);
    d = l53;  printf("%.1f\n", d);
    u = (unsigned long)1e19;  printf("%lu\n", u);
    u = (unsigned long)9.3e18; printf("%lu\n", u);
    printf("%d %d %d %d\n", (int)2.9, (int)-2.9, (int)(float)-0.5, (int)1e9);
    f = 0.1;  d = f;  printf("%.17g %.9g\n", d, (double)f);
    d = 1.0 / 3; f = d; printf("%.9g\n", (double)f);
    nan = zero / zero;
    printf("%d %d %d %d %d\n", nan < 1, nan > 1, nan == nan, nan != nan, !nan);
    printf("%d %d %d\n", !nzero, nzero == zero, (int)(1.0 / nzero < 0));
    printf("%.3f %.3f\n", 1e300 * 1e10 > 1e308 ? 1.0 : 0.0, -(1e300 * 1e10) < 0 ? 2.0 : 0.0);
    return 0;
}
