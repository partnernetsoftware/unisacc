/* a call's width comes from the callee's return type (C99 6.5.2.2p5) */
#include <stdio.h>
unsigned u(void) { return 0; }
short s(void) { return -1; }
unsigned char c(void) { return 200; }
unsigned short us(void) { return 65535; }
unsigned u2(void) { return -1; }
unsigned char c2(void) { return 300; }
short s2(void) { return 40000; }
int main(void) {
    unsigned x;
    printf("%u %d\n", u() - 1, u() - 1 > 5);
    printf("%d %d\n", (u() - 1) >> 1 == 2147483647, u() + 0 == 0);
    printf("%d %d\n", s() < 0, s() == -1);
    printf("%d %d %d\n", c() + 100, c() > 100, us() + 1);
    x = u() - 1;
    printf("%u %d\n", x, (long)(u() - 1) > 0);
    printf("%d %d %d\n", (long)u2() > 0, c2() + 0, s2() < 0);
    return 0;
}
