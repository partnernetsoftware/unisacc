/* R13-0b: external trial of 0.0.12, real-world batch.  Source: ~/unisacc-feedback-0.0.12-realworld/08-ulong-max-constant.c (+ .out.txt for the gcc-vs-unisacc record). */
/* <limits.h> defines ULONG_MAX as 18446744073709551615 with no U/UL suffix;
   using it in any arithmetic expression is refused.  With an explicit UL
   literal it works, so the header is the culprit.  Hit by tinyexpr (fac/ncr). */
#include <stdio.h>
#include <limits.h>
int main(void) {
    unsigned long r = 7;
    unsigned long q = ULONG_MAX / r;
    printf("%lu\n", q);
    return 0;
}
/* Also in #if (lua lmathlib.c line 279):
     #if ((ULONG_MAX >> 31) >> 31) >= 3
   -> "reject: not covered: #if integer literal" (no file:line).
   With 18446744073709551615UL or 0xFFFFFFFFFFFFFFFF the #if works.
   Related: UINT_MAX is 4294967295 (type long, not unsigned int), so
   UINT_MAX + 1 is 4294967296 here but 0 with gcc/glibc. */
