/* R13-0b: external trial of 0.0.12, real-world batch.  Source: ~/unisacc-feedback-0.0.12-realworld/15-sizeof-cast-array-bound.c (+ .out.txt for the gcc-vs-unisacc record). */
/* Array bounds that use sizeof or a cast are refused with "not covered:
   constant expression", at file scope, in structs and in blocks.  Plain
   integer/enum arithmetic (E2+1) works.  Lua needs all of these
   (lauxlib.h LUAL_BUFFERSIZE, lopcodes.h luaP_opmodes[NUM_OPCODES],
   ltm.h, lobject.h). */
#include <stdio.h>
enum { E1, E2 };
char a[sizeof(void *)];
char b[(int)(2 * sizeof(double))];
char c[(int)(E2) + 1];
char d[sizeof("abc")];
struct S { char x[sizeof(long)]; };
int main(void) {
    char e[(int)16];
    printf("%d %d %d %d %d %d\n", (int)sizeof a, (int)sizeof b, (int)sizeof c,
           (int)sizeof d, (int)sizeof(struct S), (int)sizeof e);
    return 0;
}
