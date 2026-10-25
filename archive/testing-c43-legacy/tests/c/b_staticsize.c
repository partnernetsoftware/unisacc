#include <stdio.h>
int main(void) {
    int n = 7;
    int a[3];
    static int m = sizeof n;
    static int k = sizeof a;
    printf("%d %d %d\n", m, k, n);
    return m != 4 || k != 12 || n != 7;
}
