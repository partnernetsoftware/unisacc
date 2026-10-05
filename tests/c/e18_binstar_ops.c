/* 0.0.28 E18/E19 (postfix ++/--, an index): `(T)(x)` after a binary `*` whose left operand follows any operator stays a cast */
#include <stdio.h>
typedef long long i64;
struct G { int a; int b; };
static struct G g = {3, 4};
static int pick(int k) {
    int x = 2, y = 5;
    switch (k) { case 1: return x * (i64)(g).b; default: break; }
    return y * (i64)(g).a;
}
int main(void) {
    i64 v = 1, w = 16, x = 2;
    static const char *(names[]) = { "x", "y" };
    v <<= x * (i64)(g).a; w >>= x * (i64)(g).a - 4;
    i64 r1 = v << x * (i64)(g).a, r2 = w / x * (i64)(g).b;
    i64 r3 = (v ? x * (i64)(g).b : 0), r4 = (v, x * (i64)(g).a);
    i64 r5 = x++ * (i64)(g).a, r6 = x-- * (i64)(g).b, r7 = "ab"[1] * (i64)(g).a;
    v /= x * (i64)(g).a; v %= x * (i64)(g).b; v &= x * (i64)(g).b; v |= x * (i64)(g).a; v ^= x * (i64)(g).a;
    printf("%lld %lld %lld %lld %lld %lld %lld %lld %lld %d %d %s%s\n", v, w, r1, r2, r3, r4, r5, r6, r7, pick(1), pick(0), names[0], names[1]);
    return 0;
}
