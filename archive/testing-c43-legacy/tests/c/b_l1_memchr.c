#include <stdio.h>
#include <string.h>
int main(void) {
    unsigned char a[4] = {0, 128, 'x', 0};
    unsigned char *p = memchr(a, 128, 3);
    printf("%ld %d %d %ld\n", p ? (long)(p-a) : -1L,
           memchr(a, 'z', 3) == 0, memchr(a, 128, 0) == 0,
           (long)((unsigned char *)memchr(a, 0, 3)-a));
    return 0;
}
