/* A one-element array member is still an array: `char x[1]` in a struct
   was width-tested as `n > 1` and stayed a scalar, so `a.x` of a by-value
   struct parameter loaded the byte and passed it as a pointer.  The
   -Wformat calibration over the corpus found it [S-15 C4]; c-testsuite
   00204 crashed. */
#include <stdio.h>
struct s1 { char x[1]; };
struct s2 { int v[1]; long tail; };
static void fa(struct s1 a) { printf("%.1s\n", a.x); }
static long fb(struct s2 b) { return b.v[0] * 10 + b.tail; }
int main(void)
{
    struct s1 v; struct s2 w; char y[1];
    v.x[0] = 'k'; y[0] = 'z'; w.v[0] = 4; w.tail = 2;
    printf("%.1s %ld %d\n", y, fb(w), (int)sizeof v.x + (int)sizeof w.v);
    fa(v);
    return 0;
}
