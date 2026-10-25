/* 0.0.22 declmatrix: functions named like the tape's registers (r0..r7) hung or crashed
   programs built by the product (0.0.20 and 0.0.21; the reference was right).  Legal C
   identifiers must never collide with an internal spelling. */
#include <stdio.h>
static int r0(void) { return 1; }
static int r1(int x) { return x + 1; }
int r7(int a, int b) { return a * b; }
static int g = 9;
static int *r3(void) { return &g; }
int main(void) {
    printf("%d %d %d %d\n", r0(), r1(4), r7(6, 7), *r3());
    return 0;
}
