#include <stdio.h>
int main(void) {
    /* Test signed narrowing, independent of implementation-defined plain char. */
    signed char c = 127; short s = 32767; int i = 2147483647;
    long one = 1; int ci, si, ii;
    ci = (c += 1); si = (s += 1); ii = (i += one);
    printf("%d %d %d %d %d %d\n", c, ci, s, si, i, ii);
    return 0;
}
