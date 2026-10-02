/* 0.0.22 csmith seeds 10 and 17: the value of an assignment is the left operand's value after the
   store (6.5.16p3), converted to its type.  The reference passed the right operand on. */
#include <stdio.h>
int main(void) {
    unsigned int a; unsigned char c; signed char s; short h; long long g;
    g = (a = -2LL); printf("%lld\n", g);
    g = (c = 300); printf("%lld\n", g);
    g = (s = 200); printf("%lld\n", g);
    g = (h = 70000); printf("%lld\n", g);
    printf("%d\n", (c = 255) + 1 > 255);
    { long long w; unsigned u = 191; printf("%d %d\n", (w = -1) < u, ((w = -1) | 1) < u); }   /* seed 17: typed as the left operand */
    return 0;
}
