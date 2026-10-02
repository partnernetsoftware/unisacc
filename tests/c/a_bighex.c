/* 0.0.22 csmith seeds 108/128/168: a hex or octal constant past LLONG_MAX is unsigned long
   long (C99 6.4.4.1), with or without an LL suffix.  The reference compared it signed. */
#include <stdio.h>
int main(void) {
    long long p = 5;
    printf("%d %d %d %d\n", p <= 0xAB4624F099C83E0CLL, p <= 0xAB4624F099C83E0C, 0xFFFFFFFFFFFFFFFF > 0, p < 01777777777777777777777);
    return 0;
}
