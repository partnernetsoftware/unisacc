#include <stdio.h>
int main(void) {
    unsigned long a = 18446744073709551615UL;
    unsigned long b = 18446744073709551614ULL;
    unsigned long c = 10000000000000000000UL;
    unsigned long d = 9223372036854775808UL;
    printf("%d %d %d %d\n", a == ~0UL, a-b == 1, c > d, d >> 63 == 1);
    return 0;
}
