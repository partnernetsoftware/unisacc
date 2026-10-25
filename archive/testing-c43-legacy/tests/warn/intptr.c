#include <stdio.h>
int main(void) {
    long l = 5; int *p; int *q = 0; char *r = (char *)0;
    p = l;
    q = 0;
    printf("%d %d %d\n", p == q, q == 0, r == 0);
    return 0;
}
