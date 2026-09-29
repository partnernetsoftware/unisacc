/* R13-0b: external trial of 0.0.12, real-world batch.  Source: ~/unisacc-feedback-0.0.12-realworld/12-long-unsigned-int.c (+ .out.txt for the gcc-vs-unisacc record). */
/* Type specifiers in a non-canonical order are refused:
   "long unsigned int x" -> "not covered: declaration".  C allows the
   specifiers in any order (6.7.2p2); cJSON.c uses `long unsigned int`. */
#include <stdio.h>
int main(void) {
    long unsigned int a = 5;
    int long b = 6;
    unsigned long c = 7;
    printf("%lu %ld %lu\n", a, b, c);
    return 0;
}
