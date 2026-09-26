/* Static arrays use the same dimension and row layout as local/global ones. */
#include <stdio.h>
int f(int bump) {
    static int a[2][3] = {{1}, {2, 3}}, b[2][2][3] = {{{4}, {5, 6}}, {{7}, {8, 9}}};
    static char z[2][3];
    a[0][0] += bump;
    b[1][1][2] += bump;
    printf("%d %d %d %d %d %d %d\n", (int)sizeof a, (int)sizeof b,
           (int)sizeof z, a[0][0], a[1][2], b[1][1][1], b[1][1][2]);
    return z[1][2];
}
int main(void) { f(1); return f(2); }
