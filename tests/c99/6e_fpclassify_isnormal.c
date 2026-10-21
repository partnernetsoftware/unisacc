/* C99 7.12.3.1 fpclassify and 7.12.3.5 isnormal over every class: nan, inf,
 * zero (both signs), subnormal, normal.  The exit code is 0 when all checks pass. */
#include <math.h>
#include <float.h>
int main(void)
{
    volatile double zero = 0.0;
    double nanv = zero / zero, inf = 1.0 / zero, sub = DBL_MIN / 4.0;
    int sum = 0;
    sum += (fpclassify(nanv) == FP_NAN) * 1;
    sum += (fpclassify(inf) == FP_INFINITE) * 2;
    sum += (fpclassify(-inf) == FP_INFINITE) * 4;
    sum += (fpclassify(zero) == FP_ZERO && fpclassify(-zero) == FP_ZERO) * 8;
    sum += (fpclassify(sub) == FP_SUBNORMAL && fpclassify(-sub) == FP_SUBNORMAL) * 16;
    sum += (fpclassify(1.5) == FP_NORMAL && fpclassify(DBL_MIN) == FP_NORMAL && fpclassify(DBL_MAX) == FP_NORMAL) * 32;
    sum += (isnormal(1.5) && isnormal(-DBL_MIN) && isnormal(DBL_MAX)) * 64;
    sum += (!isnormal(zero) && !isnormal(sub) && !isnormal(inf) && !isnormal(nanv)) * 128;
    return 255 - sum;   /* 0 when every check passes; each set bit names a failed check */
}
