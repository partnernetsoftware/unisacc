/* Comma declarators share a base type, not the previous pointer depth. */
#include <stdio.h>
struct Mixed { char *p, c, *q; int n, cap; long a[2], b[3]; };
int main(void) {
    struct Mixed m;
    m.p = "x"; m.c = 3; m.q = "y"; m.n = 4; m.cap = 5;
    m.a[1] = 6; m.b[2] = 7;
    printf("%d %d %d %d %d %d %d %d\n", (int)sizeof m,
           m.p[0], m.c, m.q[0], m.n, m.cap, (int)m.a[1], (int)m.b[2]);
    return 0;
}
