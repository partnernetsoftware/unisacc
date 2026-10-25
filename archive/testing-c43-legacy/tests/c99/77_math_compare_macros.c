/* C99 7.12.14 comparison macros isgreater, isgreaterequal, isless, islessequal,
 * islessgreater, isunordered on exact values: ordered pairs, equal values, and a
 * quiet NaN (every relational macro is 0 on an unordered pair; isunordered is 1).
 * The exit code is 0 when every check passes; each set bit names a failed check.
 * Nothing is printed. */
#include <math.h>
int main(void)
{
    volatile double a = 1.5, b = 2.5, n = NAN;
    int sum = 0;
    sum += (isgreater(b, a) && !isgreater(a, b) && !isgreater(a, a) && !isgreater(n, a) && !isgreater(a, n)) * 1;
    sum += (isgreaterequal(b, a) && isgreaterequal(a, a) && !isgreaterequal(a, b) && !isgreaterequal(n, n)) * 2;
    sum += (isless(a, b) && !isless(b, a) && !isless(a, a) && !isless(a, n) && !isless(n, a)) * 4;
    sum += (islessequal(a, b) && islessequal(a, a) && !islessequal(b, a) && !islessequal(n, a)) * 8;
    sum += (islessgreater(a, b) && islessgreater(b, a) && !islessgreater(a, a) && !islessgreater(a, n)) * 16;
    sum += (isunordered(n, a) && isunordered(a, n) && isunordered(n, n) && !isunordered(a, b) && !isunordered(a, a)) * 32;
    sum += (isgreater(0.0, -0.0) == 0 && isgreaterequal(0.0, -0.0) == 1 && islessgreater(0.0, -0.0) == 0
            && isless(-1e300, 1e300) == 1 && isunordered(0.0, -0.0) == 0) * 64;
    return 127 - sum;
}
