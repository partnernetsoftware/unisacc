#include <stdio.h>
typedef int Count;
struct Pair { int x, y; };
static int add(register Count a, int register b) { return a + b; }
int main(void) {
    register Count a = 3;
    int register b = 4;
    register struct Pair p = {5, 6};
    int c = 4;
    register int *q = &c;
    int sum = 0;
    for (register int i = 0; i < 3; ++i) sum += i;
    printf("%d %d %d %d %d\n", add(a, b), p.x, p.y, *q, sum);
    return 0;
}
