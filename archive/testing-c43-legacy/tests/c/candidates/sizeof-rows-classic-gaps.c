/* 0.0.43 candidate, not run by any suite: rows of multi-dimensional pointers to arrays and &*row.
   cc, the Python reference and the product agree; the classic reference says 8/4 for these
   (research/c43-array-pointer-member-miscompile.md). Move to tests/c/ when classic is fixed. */
#include <stdio.h>
struct S { int x; char c[3]; };
int a[4][3];
int b3[2][3][4];
struct S sa[5][2];
int *pa[2][3];
int g(void) { return 1; }
int main(void) {
    int (*ip)[3] = a; char (*cp)[5] = 0; int (*mp)[3][2] = 0; struct S (*sp)[2] = sa;
    int *(*pp)[3] = pa; double (*dp)[7] = 0; int n = 1;
    int (*bp)[3][4] = b3;
    printf("B %zu %zu %zu\n", sizeof *(cp+1), sizeof *(mp+1), sizeof **(mp+1));
    printf("G %zu %zu %zu\n", sizeof (*(ip+1)+1), sizeof (*(ip+1)+0), sizeof &*(ip+1));
    printf("L %zu %zu %zu\n", sizeof *(bp+1), sizeof **(bp+1), sizeof *(*(bp+1)+1));
    printf("O %zu %zu %zu\n", sizeof (*(ip+1))[0], sizeof (*(mp+1))[1], sizeof *(*(mp+1)+1));
    return 0;
}
