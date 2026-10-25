/* Three things the widened fuzz found in the C front end on its first run
   [S-15 A3], none of which any hand-written probe had touched:
     - a parameter's sizeof was its 8-byte SLOT, so `unsigned p` sat on the
       type axis as u64 and `(b * p) / 13u` was never narrowed to 32 bits
     - a decimal constant above INT_MAX with a u suffix was a long, not the
       unsigned int C99 6.4.4.1 makes it
     - `*p` of a struct pointer loaded the struct's first bytes and passed
       THOSE as its address: `sum(*p)` segfaulted in the callee */
#include <stdio.h>
struct S { short f0; int f1; short f2; };
static long sum(struct S s) { return (long)s.f0 * 100 + (long)s.f1 * 10 + (long)s.f2; }
static unsigned f(unsigned p0, unsigned char c, short h)
{
    unsigned b = 3655922532u;
    p0 = ((b * p0) / 13u) < (23264u * (b << 18));
    return p0 + (unsigned)sizeof p0 * 10 + (unsigned)sizeof c * 100 + (unsigned)sizeof h * 1000
           + ((3655922532u * 3u) / 7u) % 1000u * 10000u;
}
int main(void)
{
    struct S v, w, *p = &v;
    v.f0 = 1; v.f1 = 2; v.f2 = 3;
    w = *p; (*p).f2 = 7;
    printf("%u %ld %ld %d %d\n", f(2974557701u, 1, 2), sum(*p), sum(w), (*p).f1 + p->f2,
           (int)sizeof 3655922532u + (int)sizeof 4294967295 + (int)sizeof 3655922532ul);
    return 0;
}
