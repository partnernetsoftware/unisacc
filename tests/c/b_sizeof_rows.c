/* 0.0.43: sizeof of a row reached through a pointer to an array -- *(P+n), (P+n)[i], *(A+n) --
   is the row's size, not the pointer's; a cast or a comma operand is the decayed pointer.
   The product said 8 for every row (research/c43-array-pointer-member-miscompile.md). */
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
    printf("A %zu %zu %zu %zu\n", sizeof *(ip+1), sizeof(*(ip+2)), sizeof(*(2+ip)), sizeof *(ip-0));
    printf("C %zu %zu %zu\n", sizeof *(sp+1), sizeof *(pp+1), sizeof *(dp+n));
    printf("D %zu %zu %zu\n", sizeof *(a+1), sizeof *(b3+1), sizeof **(b3+1));
    printf("E %zu %zu %zu\n", sizeof (ip+1)[0], sizeof((ip+1)[0]), sizeof (a+1)[0]);
    printf("F %zu %zu %zu\n", sizeof **(ip+1), sizeof ***(mp+1), sizeof **(pp+1));
    printf("H %zu %zu %zu\n", sizeof !*(ip+1), sizeof ((int*)*(ip+1)), sizeof (0, *(ip+1)));
    printf("I %zu %zu %zu\n", sizeof (n ? *(ip+1) : *(ip+2)), sizeof (*(ip+1) == 0), sizeof (*(sp+1)));
    printf("K %zu %zu %zu %zu\n", sizeof ip, sizeof (ip+1), sizeof *ip, sizeof ip[1]);
    printf("M %zu %zu %zu\n", sizeof (*(ip+1)), sizeof ((*(ip+1))), sizeof (*(ip+g())));
    printf("N %zu %zu\n", sizeof (sizeof *(ip+1)), sizeof *(ip+1) * 2);
    printf("P %zu %zu\n", sizeof (n++, *(ip+1)), sizeof (*(cp+1) + 1));
    return 0;
}
