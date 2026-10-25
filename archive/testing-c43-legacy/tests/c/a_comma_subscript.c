#include <stdio.h>
static int a[5];
static int add(int x, int y) { return x + y; }
int main(void) {
    a[4] = 9;
    printf("%d %d\n", (a[4], 246), add(a[4], 246));
    return 0;
}
