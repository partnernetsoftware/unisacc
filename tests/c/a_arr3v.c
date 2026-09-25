/* three-dimensional arrays with VARIABLE subscripts: the index expression
   reset the inner dimension, so a[i][j][k] strode by the element and read
   a[i][j] as a scalar (segfault; found by the J10 constructor port) */
#include <stdio.h>
long a[4][5][3]; int b[2][3][4]; char c[3][2][5];
int main(void) {
    int i; int j; int k; long s; s = 0;
    i = 2; j = 3; a[i][j][1] = 7; printf("%ld\n", a[2][3][1]);
    for (i = 0; i < 2; i++) for (j = 0; j < 3; j++) for (k = 0; k < 4; k++) b[i][j][k] = i * 100 + j * 10 + k;
    for (i = 0; i < 2; i++) for (j = 0; j < 3; j++) for (k = 0; k < 4; k++) s = s + b[i][j][k] * (k + 1);
    printf("%ld %d %d %d\n", s, b[1][2][3], b[i - 1][2][k - 1], b[1][j - 1][3]);
    j = 1; c[2][j][4] = 'x'; c[2][1][0] = 'y'; printf("%c%c %d\n", c[2][1][4], c[2][j][0], (int)sizeof(a[1]) + (int)sizeof(a[1][2]));
    return 0;
}
