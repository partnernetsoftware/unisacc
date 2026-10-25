/* a constant's own kind (u suffix, hex beyond INT_MAX): it used to keep the
   unsigned/struct state of the operand before it -- the unit's first
   expression, and one right after a struct operand, compared signed */
#include <stdio.h>
struct S { int a; long b; };
int first(void) { return -1 < 1u; }
int after_struct(void) {
    struct S s; struct S t;
    s.a = 1; s.b = 2; t = s;
    return -1 < 1u;
}
long mix(void) { return 0x80000000 + -1 < 0; }
int main(void) {
    unsigned int u = 1u - 2u;
    printf("%d %d %ld %d\n", first(), after_struct(), mix(), u > 5u);
    return first() + after_struct();
}
