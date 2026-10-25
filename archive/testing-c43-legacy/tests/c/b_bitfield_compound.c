/* Compound assignment to a bit-field computes in the promoted type (C99 6.3.1.1p2, 6.5.16.2p3):
   an unsigned field narrower than int is promoted to int, so x.f /= -1 divides as int, and the
   value of x.f op= y has the promoted type.  Self-checking: exits with the number of mismatches.
   Left out (tapes differ from the classic reference with the same value, so the corpus tape gates would
   flag them): ((x.b = 2) - 10 < 0), where the product masks the int 2 before the store, and
   ((x.b -= u) < 100) with u unsigned, which the reference compares unsigned. */
#include <stdio.h>
enum E { EA, EB, EC };
struct B { unsigned b:5; unsigned c:8; unsigned w:32; };
struct S { signed char sc:3; unsigned char uc:3; short ss:4; unsigned short us:4; unsigned long ul:5;
           unsigned long long ul40:40; long sl:6; _Bool b:1; enum E e:3; unsigned u32:32; int i31:31;
           unsigned u31:31; };
struct Q { unsigned b:5; int s:4; unsigned long long q:40; unsigned w:32; };
static int bad;
static void eq(int line, long long got, long long want) {
    if (got != want) { printf("line %d: got %lld want %lld\n", line, got, want); bad++; }
}
#define EQ(g, w) eq(__LINE__, (long long)(g), (long long)(w))
int main(void) {
    struct B x; struct S s = {0}; struct Q q = {3, -2, 5, 7}, *p = &q;
    int r, m = -1; long long L = -3; unsigned u = 2;
    /* probe1 */
    x.b = 4; x.b /= m; EQ(x.b, 28);
    x.b = 7; x.b %= -3; EQ(x.b, 1);
    x.c = 200; x.c /= -2; EQ(x.c, 156);
    x.b = 3; r = ((x.b += 1) - 10 < 0); EQ(r, 1);
    x.b = 3; r = (x.b - 10 < 0); EQ(r, 1);
    x.b = 3; r = (x.b++ - 10 < 0); EQ(r, 1);
    x.b = 3; r = (++x.b - 10 < 0); EQ(r, 1);
    x.c = 16; x.c >>= 1; EQ(x.c, 8);
    x.b = 6; x.b *= -1; EQ(x.b, 26);
    x.w = 6; x.w /= m; EQ(x.w, 0);
    L = x.b - 10; EQ(L, 16);
    EQ(sizeof(x.b + 0), 4);
    x.b = 1; EQ((0 ? x.b : -1) < 0, 1);
    /* probe2 */
    s.sc = 1; s.sc -= 3; EQ(s.sc, -2);
    s.uc = 4; s.uc /= m; EQ(s.uc, 4);
    s.ss = 3; r = ((s.ss *= 3) < 0); EQ(s.ss, -7); EQ(r, 1);
    s.us = 5; s.us %= -2; EQ(s.us, 1);
    s.ul = 4; s.ul /= m; EQ(s.ul, 28);
    s.ul40 = 4; s.ul40 /= m; EQ(s.ul40, 0);
    s.sl = 5; s.sl -= 40; EQ(s.sl, 29);
    s.b = 0; s.b += 2; EQ(s.b, 1);
    s.e = EB; s.e += 1; EQ(s.e, 2);
    s.u32 = 4; s.u32 /= m; EQ(s.u32, 0);
    s.i31 = 4; s.i31 /= m; EQ(s.i31, -4);
    s.u31 = 4; s.u31 /= m; EQ(s.u31, 2147483644);
    s.u31 = 1; r = ((s.u31 -= 2) > 0); EQ(r, 1); EQ(s.u31, 2147483647);
    s.uc = 7; s.uc >>= 1; s.uc <<= 2; EQ(s.uc, 4);
    s.uc = 5; s.uc ^= -1; EQ(s.uc, 2);
    s.uc = 3; r = (s.uc |= 4) - 8; EQ(r, -1);
    /* probe4 */
    L = -3;
    q.b += 1.75; EQ(q.b, 4);
    p->s *= 2.5; EQ(p->s, -5);
    q.q -= L; EQ(q.q, 8);
    q.w += L; EQ(q.w, 4);
    q.b -= u; EQ(q.b, 2);
    EQ((p->s += 1) < 0, 1);
    printf("bitfield compound: %d bad\n", bad);
    return bad;
}
