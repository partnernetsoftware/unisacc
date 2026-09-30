#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
int main(void) {
    char *e; const char *s = "  +";
    const char *over = "9223372036854775808x";
    const char *hex = "0x";
    long long a, b, c;
    errno = 0; a = strtoll(over, &e, 10);
    printf("%d %d %ld ", a == LLONG_MAX, errno == ERANGE,
           (long)(e - over));
    errno = 0; b = strtoll("-9223372036854775809", &e, 10);
    printf("%d %d ", b == LLONG_MIN, errno == ERANGE);
    c = strtoll(hex, &e, 0);
    printf("%lld %ld ", c, (long)(e - hex));
    c = strtoll(s, &e, 10);
    printf("%lld %d\n", c, e == s);
    return 0;
}
