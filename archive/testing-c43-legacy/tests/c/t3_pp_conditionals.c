/* 0.0.29 T3: #if expression operators, nested conditional skipping (#elif/#else/#undef/#error in dead
   branches), and directives with odd spacing -- preprocessor rows no probe reached */
#include <stdio.h>
#define A 3
#define B (A << 2)
#if (A ? B : 0) == 12 && -A < 0 && !0 && ~0 == -1 && (A | 4) == 7 && (A ^ 1) == 2 && (B >> 1) == 6 && A % 2 && A / 3 == 1
#  define EXPR 1
#elif defined(B) || defined C
#  define EXPR 2
#else
#  define EXPR 3
#endif
#ifdef NOT_DEFINED
#  error this branch is skipped
#  if 1/0
#  endif
#elif !defined(A)
#  error also skipped
#else
#  undef A
#  define A 5
#endif
#ifndef A
#  define DEAD 1
#endif
  #   if   A   >=   5   &&   A   <=   5
#define SPACED 1
  #   endif
#if 0
#pragma something_unknown
#include "does-not-exist.h"
#endif
int main(void) {
    printf("%d %d %d %d\n", EXPR, A, SPACED, (int)sizeof("x" "yz"));
    return 0;
}
