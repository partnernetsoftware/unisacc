/* Structs passed by value.  The callee copies the caller's object into its
   own slot, and that copy runs through r0-r2 -- which is where the arguments
   it has not spilled yet are still sitting.  `f(R a, R b)` copied `a` over
   the register holding `b`, so `b` was a second copy of `a`. */
#include <stdio.h>

typedef struct {
    unsigned char type;
    union { unsigned char ch; unsigned char *ccl; } u;
} R;

struct Big { int a, b, c, d, e; };

static int one(R p, int k) { return p.type * 100 + p.u.ch + k; }
static int two(R a, R b)   { return a.type * 10 + b.type; }
static int three(R a, int i, R b, int j) { return a.type + i + b.type * 10 + j; }
static int big(struct Big g, int k) { return g.a + g.b + g.c + g.d + g.e + k; }
static R bump(R p) { p.type = p.type + 1; return p; }

int main(void)
{
    R r, s, t;
    struct Big g;
    r.type = 7; r.u.ch = 'A';
    s.type = 3; s.u.ch = 'B';
    g.a = 1; g.b = 2; g.c = 3; g.d = 4; g.e = 5;

    printf("%d %d %d\n", (int)sizeof(R), one(r, 1), two(r, s));
    printf("%d %d\n", three(r, 100, s, 1000), big(g, 10));
    t = bump(r);
    printf("%d %d %d\n", (int)t.type, (int)r.type, (int)t.u.ch);
    return 0;
}
