/* a call's result is typed by the FUNCTION: an int result after a row-pointer
   argument was scaled like that pointer, and pointer results stepped by bytes */
#include <stdio.h>
long rows[4][3]; int iv[8]; long lv[8]; char cv[8]; long *pp[4];
int nl(long *c) { return (int)c[0]; }
int *ip(int k) { return iv + k; }
long *lp(void) { return lv + 1; }
char *cp(void) { return cv; }
long **ppp(void) { return pp; }
int main(void) {
    int H; int k; H = 2; rows[H][0] = 5;
    for (k = 0; k < 8; k++) { iv[k] = k * 10; lv[k] = k * 100; cv[k] = 'a' + k; }
    pp[1] = lv + 3;
    printf("%d %d %d\n", -(nl(rows[H]) - 1), nl(rows[H]) + 1, 2 * nl(rows[H]) - 3);
    printf("%d %d %ld %c %c\n", *(ip(1) + 2), ip(2)[1], *(lp() + 2), *(cp() + 3), cp()[4]);
    printf("%ld %ld\n", *(*(ppp() + 1)), ppp()[1][1]);
    printf("%d\n", (int)(ip(5) - ip(1)));
    return 0;
}
