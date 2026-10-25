#include <stdio.h>
#include <errno.h>
#include <float.h>
#include <iso646.h>
int main(void)
{
    int x = 6, y = 3;
    errno = 0;
    printf("%d %d %d\n", errno, EDOM, ERANGE);
    printf("%d %d %d %d\n", FLT_DIG, DBL_DIG, FLT_MANT_DIG, DBL_MANT_DIG);
    printf("%d %d %d\n", DBL_MAX_EXP, DBL_MIN_EXP, FLT_RADIX);
    printf("%.6e %.6e %.10e\n", FLT_EPSILON, (double)FLT_MAX, DBL_EPSILON);
    printf("%d %d %d %d\n", x > 1 and y > 1, x < 1 or y > 1, not (x == y), x not_eq y);
    x and_eq 4; y or_eq 8;
    printf("%d %d %d %d\n", x, y, x bitor y, compl 0);
    return 0;
}
