/* R13-0b: external trial of 0.0.12, real-world batch.  Source: ~/unisacc-feedback-0.0.12-realworld/08b-ulong-max-in-if.c (+ .out.txt for the gcc-vs-unisacc record). */
#include <limits.h>
#if ((ULONG_MAX >> 31) >> 31) >= 3
int x = 1;
#else
int x = 2;
#endif
#include <stdio.h>
int main(void){ printf("%d %lu\n", x, (unsigned long)(UINT_MAX + 1)); return 0; }
