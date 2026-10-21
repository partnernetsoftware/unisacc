/* C99 7.12.10.2 remainder, 7.12.10.3 remquo, 7.12.13.1 fma, on values whose
 * results are exact.  The exit code is 0 when all checks pass; nothing is printed. */
#include <math.h>
int main(void)
{
    volatile double seven = 7.0, two = 2.0, ten = 10.0, three = 3.0, one = 1.0, five = 5.0;
    int sum = 0, q = 99;
    /* remainder is x - n*y with n = x/y rounded to nearest, ties to even: 7/2 = 3.5 -> n = 4 */
    sum += (remainder(seven, two) == -1.0 && remainder(-seven, two) == 1.0
            && remainder(ten, three) == 1.0 && remainder(five, two) == 1.0
            && remainder(5.5, 1.0) == -0.5 && remainder(6.0, three) == 0.0) * 1;
    /* remquo: same remainder, plus the sign and at least 3 low bits of n */
    sum += (remquo(seven, two, &q) == -1.0 && (q & 7) == 4 && q > 0) * 2;
    sum += (remquo(ten, three, &q) == 1.0 && (q & 7) == 3 && q > 0) * 4;
    sum += (remquo(-ten, three, &q) == -1.0 && (-q & 7) == 3 && q < 0) * 8;
    /* fma: one rounding on x*y + z; these fit exactly */
    sum += (fma(two, three, one) == 7.0 && fma(-two, three, one) == -5.0
            && fma(2.5, 4.0, -ten) == 0.0 && fma(0.5, 0.5, 0.75) == 1.0
            && fma(1e8, 1e8, 1.0) == 1e16 + 1.0) * 16;
    return 31 - sum;   /* 0 when every check passes; each set bit names a failed check */
}
