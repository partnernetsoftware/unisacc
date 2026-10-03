#include <stdio.h>
int main(void) {
    unsigned long long all = 18446744073709551615ULL;
    unsigned long long hex_ll = 0xA51754D42E128A9ALL;
    unsigned long long hex_plain = 0xAB4624F099C83E0C;
    unsigned long hex_l = 0xffffffffffffffffL;
    unsigned int ui = 4294967295U;
    long dec_l = 2147483648L;
    printf("%llu %llu %llu %lu %u %ld %d %d\n", all >> 60, hex_ll >> 60,
           hex_plain >> 60, hex_l >> 60, ui >> 28, dec_l >> 28,
           (long long)5 <= 0xAB4624F099C83E0C,
           0x8000000000000000 > 0);
    return 0;
}
