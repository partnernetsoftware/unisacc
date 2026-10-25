#include <stdio.h>
/* C99 6.5.3.3p5: the result of ! is int, even on a pointer operand */
int main(void) {
    int a = 1; int *q = 0; int *r = &a;
    printf("%d\n", a + !q);
    printf("%d\n", a + !r);
    printf("%d\n", (int)sizeof(!q));
    return 0;
}
