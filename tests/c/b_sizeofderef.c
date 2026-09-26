/* Descriptor sizes, including remaining array dimensions; no operand loads. */
#include <stdio.h>
typedef long Wide;
typedef struct { int n; long x; } Pair;
enum Kind { K0, K1 };
int main(void) {
    char **p = 0; long ***q = 0; Pair *r = 0;
    int a[2][3];
    int b[2][3][4];
    printf("%d %d %d %d %d %d %d\n", (int)sizeof *p,
           (int)sizeof **p, (int)sizeof(**q), (int)sizeof *r,
           (int)sizeof(Wide), (int)sizeof(Pair), (int)sizeof(enum Kind));
    printf("%d %d %d\n", (int)sizeof a, (int)sizeof *a, (int)sizeof **a);
    printf("%d %d %d %d\n", (int)sizeof b, (int)sizeof *b, (int)sizeof **b, (int)sizeof ***b);
    return 0;
}
