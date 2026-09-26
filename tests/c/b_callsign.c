#include <stdio.h>
/* a call's signedness comes from the callee's return type, not its last argument */
long g(unsigned long x) { return (long)x; }
unsigned long h(long x) { return (unsigned long)x; }
int main(void) {
    long n = -9; unsigned long v = (unsigned long)n;
    printf("%ld\n", g(v) / 2);
    printf("%d\n", g(v) < 0);
    printf("%lu\n", h(n) / 2);
    printf("%ld\n", g(v) >> 1);
    return 0;
}
