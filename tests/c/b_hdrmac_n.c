/* A user macro named like a header-internal identifier must not reach the
 * bundled header bodies: #define n 3 before <string.h> once broke memcpy. */
#define n 3
#include <string.h>
#include <stdio.h>
int main(void) {
    char a[8];
    memcpy(a, "abcdefg", 8);
    printf("%s %d\n", a, n);
    return 0;
}
