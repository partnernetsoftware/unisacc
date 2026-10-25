#include <stdio.h>
struct S { int x; };
struct S g = {7};
struct S bump(struct S s) { s.x++; return s; }
struct S getg(void) { return g; }
int main(void) {
    struct S l = {9};
    struct S a = bump(g);
    struct S b = bump(l);
    struct S c = bump(getg());
    printf("%d %d %d %d %d\n", a.x, b.x, c.x, g.x, l.x);
    return 0;
}
