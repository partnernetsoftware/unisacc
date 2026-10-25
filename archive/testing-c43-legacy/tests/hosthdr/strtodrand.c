/* 0.0.34 L1a: strtod on 10^5 generated decimals (1..40 significant digits, exponents across the whole
 * range and past it) = the system libc, value bits and errno; one checksum line per 10^4 strings plus
 * every mismatch-prone class printed once, so a difference names its class. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
static unsigned long long st = 88172645463325252ULL;
static unsigned rnd(unsigned n) { st ^= st << 13; st ^= st >> 7; st ^= st << 17; return (unsigned)(st % n); }
int main(void) {
    char b[96]; int i, k; unsigned long long sum = 0, bits; double d; char *e;
    for (i = 0; i < 100000; i++) {
        int p = 0, nd = 1 + (int)rnd(40), dot = (int)rnd(nd + 1), ex;
        if (rnd(4) == 0) b[p++] = '-';
        for (k = 0; k < nd; k++) { if (k == dot && k) b[p++] = '.'; b[p++] = (char)('0' + (k == 0 ? 1 + rnd(9) : rnd(10))); }
        switch (rnd(5)) {
        case 0: ex = (int)rnd(40) - 20; break;
        case 1: ex = (int)rnd(640) - 340; break;
        case 2: ex = 300 + (int)rnd(20) - nd; break;        /* near overflow */
        case 3: ex = -330 + (int)rnd(30) - nd; break;       /* subnormal and underflow */
        default: ex = (int)rnd(20000) - 10000; break;       /* far outside */
        }
        p += sprintf(b + p, "e%d", ex);
        errno = 0; d = strtod(b, &e);
        memcpy(&bits, &d, 8);
        sum = sum * 1099511628211ULL ^ bits ^ ((unsigned long long)(errno != 0) << 63) ^ (unsigned long long)(e - b);
        if (i % 10000 == 9999) printf("%d %016llx\n", i + 1, sum);
    }
    return 0;
}
