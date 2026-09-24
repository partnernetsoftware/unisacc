#include <stdio.h>
#include <stdlib.h>
static void bye1(void) { printf("bye1\n"); }
static void bye2(void) { printf("bye2\n"); }
int main(void)
{
    div_t q = div(-17, 5); ldiv_t lq = ldiv(1000000007L, 13L);
    printf("%d %d %ld %ld %ld %ld\n", q.quot, q.rem, lq.quot, lq.rem, labs(-1234567890123L), labs(5L));
    atexit(bye1);
    atexit(bye2);
    printf("main done\n");
    return 0;
}
