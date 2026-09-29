/* R13-0b: external trial of 0.0.12, real-world batch.  Source: ~/unisacc-feedback-0.0.12-realworld/26-stdint-missing-limits.c (+ .out.txt for the gcc-vs-unisacc record). */
/* <stdint.h> defines only INTn_MIN/MAX and UINTn_MAX.  C99 7.18 also
   requires SIZE_MAX, PTRDIFF_MIN/MAX, INTPTR_MIN/MAX, UINTPTR_MAX,
   INTMAX_MIN/MAX, UINTMAX_MAX and the INTn_C()/UINTn_C() macros.
   SIZE_MAX is used by sbase (head, fold, nl, expand, unexpand via
   MIN(LLONG_MAX, SIZE_MAX)) -> "unknown identifier".  (UINT64_MAX also has
   no UL suffix, same problem as bug 08.) */
#include <stdio.h>
#include <stdint.h>
int main(void) {
    printf("%d\n", SIZE_MAX > 0);
    printf("%d %d\n", UINTPTR_MAX > 0, PTRDIFF_MAX > 0);
    printf("%d %lld\n", INTMAX_MAX > 0, (long long)INT64_C(5));
    return 0;
}
