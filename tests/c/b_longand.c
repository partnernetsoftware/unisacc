/* Binary long & long must not become the table's address-of sentinel. */
#include <stdio.h>
int main(void) {
    long a = 5, b = 3;
    long *p = &a;
    printf("%ld %ld %ld %lu %d\n", (a & b) + 1, (a & b) - 1,
           (a & b) << 1, sizeof(a & b), p == &a);
    return 0;
}
