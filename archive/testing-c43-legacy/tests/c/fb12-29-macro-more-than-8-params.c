/* R13-0b: external trial of 0.0.12, real-world batch.  Source: ~/unisacc-feedback-0.0.12-realworld/29-macro-more-than-8-params.c (+ .out.txt for the gcc-vs-unisacc record). */
/* Invoking a function-like macro with 9 or more parameters fails with
   "reject: not covered: function-like macro invocation" (no file:line);
   8 parameters work.  C99 5.2.4.1 requires at least 127.  sqlite3.c's
   WAGGREGATE(zName,nArg,arg,nc,xStep,xFinal,xValue,xInverse,f) has 9, so
   the sqlite amalgamation cannot be preprocessed. */
#include <stdio.h>
#define SUM8(a,b,c,d,e,f,g,h)   (a+b+c+d+e+f+g+h)
#define SUM9(a,b,c,d,e,f,g,h,i) (a+b+c+d+e+f+g+h+i)
int main(void) {
    printf("%d\n", SUM8(1,2,3,4,5,6,7,8));
    printf("%d\n", SUM9(1,2,3,4,5,6,7,8,9));
    return 0;
}
