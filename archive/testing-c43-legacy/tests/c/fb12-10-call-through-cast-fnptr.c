/* R13-0b: external trial of 0.0.12, real-world batch.  Source: ~/unisacc-feedback-0.0.12-realworld/10-call-through-cast-fnptr.c (+ .out.txt for the gcc-vs-unisacc record). */
/* Calling through a cast to a function-pointer type,
   ((double (*)(void)) vp)(), is refused: "not covered: function pointer
   cast result type".  tinyexpr stores functions as const void * and calls
   them this way (te_eval).  Scope: only an INLINE cast whose return type is
   not int fails (double/(double) and double/(void) fail; int (*)(int) and
   int (*)(void) work; a typedef'd  ((dfn)h)(5.0)  works).  The generic
   function pointer case (gfn -> double(*)(double)) is strictly conforming. */
#include <stdio.h>
static double seven(void) { return 7.0; }
static double half(double x) { return x / 2; }
typedef void (*gfn)(void);
int main(void) {
    const void *vp = (const void *)seven;
    gfn g = (gfn)half;
    double d = ((double (*)(void))vp)();
    double i = ((double (*)(double))g)(21.0);
    printf("%g %g\n", d, i);
    return 0;
}
