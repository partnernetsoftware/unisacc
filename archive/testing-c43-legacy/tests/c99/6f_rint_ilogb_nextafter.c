/* C99 7.12.6.5 ilogb, 7.12.6.11 logb, 7.12.9.4 rint, 7.12.9.5 lrint,
 * 7.12.11.2 nan, 7.12.11.3 nextafter, 7.12.11.4 nexttoward, on values whose
 * results are exact.  The exit code is 0 when all checks pass; nothing is printed. */
#include <math.h>
#include <float.h>
int main(void)
{
    volatile double zero = 0.0;
    double inf = 1.0 / zero, n;
    int sum = 0;
    sum += (ilogb(1.0) == 0 && ilogb(1024.0) == 10 && ilogb(0.75) == -1 && ilogb(-3.0) == 1) * 1;
    sum += (ilogb(DBL_MIN) == DBL_MIN_EXP - 1 && ilogb(DBL_MIN / 4.0) == DBL_MIN_EXP - 3) * 2;
    sum += (logb(1.0) == 0.0 && logb(1024.0) == 10.0 && logb(0.75) == -1.0 && logb(-3.0) == 1.0
            && logb(inf) == inf && logb(-inf) == inf) * 4;
    sum += (rint(2.5) == 2.0 && rint(3.5) == 4.0 && rint(-2.5) == -2.0 && rint(-0.5) == 0.0
            && rint(2.75) == 3.0 && rint(-2.25) == -2.0 && rint(1e17) == 1e17) * 8;
    sum += (lrint(2.5) == 2 && lrint(3.5) == 4 && lrint(-2.5) == -2 && lrint(1234567.4) == 1234567
            && lrint(-1234567.6) == -1234568 && lrint(0.0) == 0) * 16;
    n = nan("");
    sum += (n != n && nan("123") != nan("123")) * 32;
    sum += (nextafter(1.0, 2.0) == 1.0 + DBL_EPSILON && nextafter(1.0, 0.0) == 1.0 - DBL_EPSILON / 2
            && nextafter(0.0, 1.0) > 0.0 && nextafter(0.0, 1.0) / 2.0 == 0.0
            && nextafter(DBL_MAX, inf) == inf && nextafter(3.0, 3.0) == 3.0
            && nextafter(-1.0, 0.0) == -1.0 + DBL_EPSILON / 2) * 64;
    sum += (nexttoward(1.0, 2.0) == 1.0 + DBL_EPSILON && nexttoward(1.0, 0.0) == 1.0 - DBL_EPSILON / 2
            && nexttoward(-DBL_MAX, -inf) == -inf && nexttoward(5.0, 5.0) == 5.0) * 128;
    return 255 - sum;   /* 0 when every check passes; each set bit names a failed check */
}
