#include <stdio.h>
/* C99 6.10.3.5: a macro is defined from its #define to its #undef, and a
   redefinition takes effect from where it is written.  Both front ends used
   to expand the whole file with the FINAL definitions, so this printed
   "2 2 3" -- and #undef did nothing at all in the C one. */
#define V 1
int a = V;
#undef V
#define V 2
int b = V;
#define W V
#undef V
#define V 3
int c = W;
int main(void) { printf("%d %d %d\n", a, b, c); return 0; }
