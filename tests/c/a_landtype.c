/* 0.0.22 csmith seed 1: the result of && and || is a signed int (6.5.13p3, 6.5.14p3).
   The reference kept the right operand's unsignedness, so -12 <= (3 && b) compared
   unsigned and was false. */
#include <stdio.h>
int main(void) {
    int a = -12; unsigned short b = 1; unsigned char c = 1; unsigned int u = 1;
    printf("%d %d %d %d %d\n", a <= (3 && b), a <= (3 && c), a < (0 || u), (1 && u) - 2 < 0, -1 < (b || 0));
    return 0;
}
