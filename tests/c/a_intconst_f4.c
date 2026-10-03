#include <stdio.h>
int main(void) {
    unsigned long long all = 18446744073709551615ULL;
    unsigned long long hex_ll = 0xA51754D42E128A9ALL;
    unsigned long hex_l = 0xffffffffffffffffL;
    unsigned int ui = 4294967295U;
    long dec_l = 2147483648L;
    printf("%llu %llu %lu %u %ld\n", all >> 60, hex_ll >> 60, hex_l >> 60, ui >> 28, dec_l >> 28);
    return 0;
}
