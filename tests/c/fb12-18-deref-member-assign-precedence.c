/* R13-0b: external trial of 0.0.12, real-world batch.  Source: ~/unisacc-feedback-0.0.12-realworld/18-deref-member-assign-precedence.c (+ .out.txt for the gcc-vs-unisacc record). */
/* `*x.p OP= v` (unary * on a member access, as the left side of an
   assignment) is parsed as `*(x.p OP= v)`:
     *x.ip += 2;   silently advances the POINTER x.ip by 2 ints and leaves
                   the pointee unchanged  (wrong code, no diagnostic)
     *x.ip -= 1;   same
     *x.ip *= 4;   "not covered: pointer or non-int in op= ++ --"
     *x.p = 65;    "not covered: dereference of a non-pointer"
     *px->ip += 2; same as *x.ip += 2
   *(x.ip) += 2, *p += 2 and rvalue *x.p are correct.
   Found in rxi/fe (fe_tostring: *x.p = '\0';). */
#include <stdio.h>
struct B { int *ip; };
int main(void) {
    int iv[4] = { 3, 0, 0, 0 };
    struct B x, *px = &x;
    x.ip = iv;
    *x.ip += 2;
    *px->ip -= 1;
    printf("iv[0]=%d offset=%d\n", iv[0], (int)(x.ip - iv));
    return 0;
}
