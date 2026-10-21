/* C99 7.12.5.1 acosh, 7.12.5.2 asinh, 7.12.5.3 atanh, 7.12.8.1 erf, 7.12.8.2 erfc,
 * 7.12.8.3 lgamma, 7.12.8.4 tgamma, on arguments whose results are exact.
 * The exit code is 0 when all checks pass; nothing is printed. */
#include <math.h>
int main(void)
{
    volatile double zero = 0.0, one = 1.0, two = 2.0, five = 5.0;
    double inf = one / zero;
    int sum = 0;
    sum += (acosh(one) == 0.0 && acosh(inf) == inf) * 1;
    sum += (asinh(zero) == 0.0 && asinh(inf) == inf && asinh(-inf) == -inf
            && asinh(-zero) == 0.0 && signbit(asinh(-zero))) * 2;
    sum += (atanh(zero) == 0.0 && atanh(one) == inf && atanh(-one) == -inf
            && signbit(atanh(-zero))) * 4;
    sum += (erf(zero) == 0.0 && erf(inf) == 1.0 && erf(-inf) == -1.0 && signbit(erf(-zero))) * 8;
    sum += (erfc(zero) == 1.0 && erfc(inf) == 0.0 && erfc(-inf) == 2.0) * 16;
    sum += (lgamma(one) == 0.0 && lgamma(two) == 0.0 && lgamma(inf) == inf) * 32;
    sum += (tgamma(one) == 1.0 && tgamma(two) == 1.0 && tgamma(five) == 24.0
            && tgamma(5.0 + one) == 120.0 && tgamma(inf) == inf) * 64;
    return 127 - sum;   /* 0 when every check passes; each set bit names a failed check */
}
