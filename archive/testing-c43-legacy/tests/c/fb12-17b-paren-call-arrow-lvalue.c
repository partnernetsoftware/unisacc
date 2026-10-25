/* R13-0b: external trial of 0.0.12, real-world batch.  Source: ~/unisacc-feedback-0.0.12-realworld/17b-paren-call-arrow-lvalue.c (+ .out.txt for the gcc-vs-unisacc record). */
/* Same family as 17: a parenthesized member access through a function-call
   result, used as an lvalue, is refused ("address of non-lvalue"):
   (get()->cdr.o) = 5;   without the parentheses it works.  rxi/fe does
   this via its accessor macro: #define cdr(x) ((x)->cdr.o)
   ... cdr(getbound(sym, &nil)) = v; */
#include <stdio.h>
union U { int o; double d; };
struct N { union U car, cdr; };
struct N g;
struct N *get(void) { return &g; }
#define cdr(x) ((x)->cdr.o)
int main(void) {
    cdr(get()) = 5;
    printf("%d\n", g.cdr.o);
    return 0;
}
