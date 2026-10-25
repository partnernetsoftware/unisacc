#include <stdio.h>
struct S { int x; };
static struct S g;
static int add(int x, int y) { return x + y; }
int main(void) {
    g.x = 9;
    printf("%d %d\n", (g.x, 246), add(g.x, 246));
    return 0;
}
