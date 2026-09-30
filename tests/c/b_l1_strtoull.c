#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
int main(void) {
    char *e; unsigned long long a, b;
    const char *over = "18446744073709551616z";
    const char *minus = "-1";
    errno = 0; a = strtoull(over, &e, 10);
    printf("%d %d %ld ", a == ULLONG_MAX, errno == ERANGE,
           (long)(e - over));
    errno = 0; b = strtoull(minus, &e, 10);
    printf("%d %d %ld ", b == ULLONG_MAX, errno, (long)(e - minus));
    printf("%llu\n", strtoull("0xff", &e, 0));
    return 0;
}
