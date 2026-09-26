/* printf is an ordinary call (991d337): its return value is the count of
   bytes written, and its arguments are evaluated once, left to right */
#include <stdio.h>
int k;
int bump(void) { k = k + 1; return k * 10; }
int main(void) {
    int n; int m;
    n = printf("a %d\n", bump());
    m = printf("%s|%5d|%-3d|%x\n", "zz", bump(), k, 255);
    printf("%d %d %d\n", n, m, k);
    return n + m + printf("");
}
