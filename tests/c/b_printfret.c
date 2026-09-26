/* printf is an ordinary call (991d337): its return value is the count of
   bytes written, and each argument is evaluated exactly once.  No argument
   of one call depends on another (C99 6.5.2.2p10 leaves their order unspecified). */
#include <stdio.h>
int k;
int bump(void) { k = k + 1; return k * 10; }
int main(void) {
    int n; int m;
    n = printf("a %d\n", bump());
    m = printf("%s|%5d|%-3d|%x\n", "zz", bump(), 2, 255);
    printf("%d %d %d\n", n, m, k);
    if (k != 2) return 1;
    return n + m + printf("");
}
