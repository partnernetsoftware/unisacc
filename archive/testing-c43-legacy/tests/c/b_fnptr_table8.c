/* An inferred file-scope extent must cover all eight pointer stores. */
#include <stdio.h>
static int f0(void) { return 0; }
static int f1(void) { return 1; }
static int f2(void) { return 2; }
static int f3(void) { return 3; }
static int f4(void) { return 4; }
static int f5(void) { return 5; }
static int f6(void) { return 6; }
static int f7(void) { return 7; }
static int (*a[])(void) = {f0, f1, f2, f3, f4, f5, f6, f7};
static int (*b[9])(void) = {f7, f6, f5, f4, f3, f2, f1, f0};
int main(void) {
    int i; int sum = 0;
    for (i = 0; i < 8; i++) sum += a[i]() + b[i]();
    printf("%d %d %d %d\n", sum, b[8] == 0, (int)sizeof a, (int)sizeof b);
    return 0;
}
