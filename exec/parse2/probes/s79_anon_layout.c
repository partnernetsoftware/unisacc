#include <stdio.h>
struct A { char lead; union { long x; int y; }; struct { short u; int v[2]; }; char end; };
struct A g = {1, 2, 3, {4, 5}, 6};
struct N { struct { union { int a; int b; }; int c; }; int d; };
union U { struct { int p; int q; }; long z; };
int main(void) {
    struct A l = {7, 8, 9, {10, 11}, 12};
    struct N n = {13, 14, 15};
    union U u = {16, 17};
    printf("%ld %d %ld %d %d %d %d\n", (long)sizeof(g), g.lead, g.x, g.u, g.v[0], g.v[1], g.end);
    printf("%d %ld %d %d %d %d\n", l.lead, l.x, l.u, l.v[0], l.v[1], l.end);
    printf("%d %d %d %d %ld %d %d %ld\n", n.a, n.b, n.c, n.d, (long)sizeof(n), u.p, u.q, (long)sizeof(u));
    return 0;
}
