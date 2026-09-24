/* A u8/u16/u32 value in a register is always zero-extended -- the
   invariant both front ends now keep, after the eight-class fuzz [S-15 A3]
   found `21 - (v & 1023)` (int minus u32, a u32) widened to long as a
   negative number, in both front ends at once.  Every line here is a
   narrow unsigned RESULT that is then widened, returned, passed or
   compared: binary ops, compound assignment (in the common type: `h >>= 1`
   on a u64 used to shift the sign in), ++x, -x, ~x. */
#include <stdio.h>
static long f(unsigned a, unsigned b) { return a - b; }
static long g(unsigned a, unsigned b) { long r = a - b; return r; }
static int part1(void)
{
    unsigned u = 5; long l = 100; int i = 21; unsigned char c = 250; unsigned short s = 65530;
    printf("%ld %ld %ld\n", (long)((unsigned)(-5) >> 4), (long)(c + 10), (long)(unsigned char)(c + 10));
    printf("%ld %ld %ld %ld\n", (long)(i - u * 10), l + (u - 6), f(3, 4), g(3, 4));
    printf("%ld %ld %d\n", (long)(s + 10), (long)(unsigned short)(s + 10), (u - 6) > 0);
    printf("%ld %lu\n", (long)(u * 1000000000u), (unsigned long)(u - 6));
    return 0;
}
static int part2(void)
{
    unsigned long h = 0x8000000000000001ul; unsigned u = 0x80000001u; unsigned char c = 250; unsigned v = 7;
    h >>= 1; printf("%lu\n", h);
    h = 0x8000000000000001ul; h /= 3; printf("%lu\n", h);
    u >>= 1; printf("%u %ld\n", u, (long)u);
    u = 0x80000001u; u /= 3; printf("%u\n", u);
    c += 10; printf("%d %ld %ld\n", c, (long)(c += 250), (long)++c);
    v -= 9; printf("%u %ld %ld %ld\n", v, (long)v, (long)(v -= 1), (long)~v);
    printf("%ld %ld %ld\n", (long)-v, (long)-c, (long)(~c));
    return 0;
}
int main(void) { part1(); part2(); return 0; }
