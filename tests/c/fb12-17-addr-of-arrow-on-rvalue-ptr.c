/* R13-0b: external trial of 0.0.12, real-world batch.  Source: ~/unisacc-feedback-0.0.12-realworld/17-addr-of-arrow-on-rvalue-ptr.c (+ .out.txt for the gcc-vs-unisacc record). */
/* &(p + 2)->val and &(p++)->val are refused: "not covered: address of
   non-lvalue".  E->m is an lvalue whenever E is a pointer, even if E
   itself is not an lvalue; (p + 1)->val = 3 and &p[2].val both work.
   Lua's s2v()/setobj macros use &(L->top.p + idx)->val everywhere. */
#include <stdio.h>
struct V { int val; int tt; };
struct V arr[4];
int main(void) {
    struct V *p = arr;
    int *q = &(p + 2)->val;
    int *r = &(p++)->tt;
    *q = 7; *r = 9;
    printf("%d %d %d\n", arr[2].val, arr[0].tt, (int)(p - arr));
    return 0;
}
