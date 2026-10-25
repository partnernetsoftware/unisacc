/* C99 7.12.9.3 nearbyint, 7.12.9.5 llrint, 7.12.9.7 lround and llround, on values whose
 * results are exact (halfway cases, large magnitudes, zero).  The exit code is 0
 * when all checks pass; nothing is printed. */
#include <math.h>
int main(void)
{
    volatile double h = 2.5, n = -2.5, t = 3.5, q = 0.5, big = 1e17, z = 0.0, a = 1234567.4;
    int sum = 0;
    /* default rounding is to nearest, ties to even */
    sum += (llrint(h) == 2 && llrint(t) == 4 && llrint(n) == -2 && llrint(-q) == 0
            && llrint(a) == 1234567 && llrint(-a - 0.2) == -1234568 && llrint(z) == 0) * 1;
    sum += (llrint(big) == 100000000000000000LL && llrint(-big) == -100000000000000000LL) * 2;
    /* llround rounds halfway cases away from zero */
    sum += (llround(h) == 3 && llround(t) == 4 && llround(n) == -3 && llround(-q) == -1
            && llround(q) == 1 && llround(a) == 1234567 && llround(z) == 0) * 4;
    sum += (llround(big) == 100000000000000000LL && llround(-big) == -100000000000000000LL) * 8;
    /* nearbyint: like rint, in the current (default) direction */
    sum += (nearbyint(h) == 2.0 && nearbyint(t) == 4.0 && nearbyint(n) == -2.0
            && nearbyint(-q) == 0.0 && nearbyint(2.75) == 3.0 && nearbyint(-2.25) == -2.0
            && nearbyint(big) == big && nearbyint(z) == 0.0) * 16;
    sum += (lround(h) == 3 && lround(t) == 4 && lround(n) == -3 && lround(-q) == -1
            && lround(q) == 1 && lround(a) == 1234567 && lround(z) == 0) * 32;
    return 63 - sum;   /* 0 when every check passes; each set bit names a failed check */
}
