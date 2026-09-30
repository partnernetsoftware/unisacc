/* #line with a macro operand (C99 6.10.4p5; c-testsuite 00152): the covered
   form is one object-like macro whose body is a digit sequence. */
#include <stdio.h>
#define LN 1000
#line LN
int a = __LINE__;
int main(void) { printf("%d %d\n", a, __LINE__); return 0; }
