#include <stdio.h>
#include <stdlib.h>
static int cmp(const void *a, const void *b) { return *(const int *)a - *(const int *)b; }
struct P { int k; char n[8]; };
static int pcmp(const void *a, const void *b) { return ((const struct P *)a)->k - ((const struct P *)b)->k; }
int main(void)
{
    int v[9] = {5, 3, 9, 1, 7, 3, 8, 2, 6}; int key; int *f; int i;
    struct P ps[3]; struct P *pf; struct P pk;
    qsort(v, 9, sizeof v[0], cmp);
    for (i = 0; i < 9; i++) printf("%d ", v[i]);
    key = 7; f = bsearch(&key, v, 9, sizeof v[0], cmp);
    printf("| %d at %ld", *f, (long)(f - v));
    key = 4; f = bsearch(&key, v, 9, sizeof v[0], cmp);
    printf(" | %s\n", f ? "found" : "absent");
    ps[0].k = 30; ps[0].n[0] = 'c'; ps[0].n[1] = 0;
    ps[1].k = 10; ps[1].n[0] = 'a'; ps[1].n[1] = 0;
    ps[2].k = 20; ps[2].n[0] = 'b'; ps[2].n[1] = 0;
    qsort(ps, 3, sizeof ps[0], pcmp);
    pk.k = 20; pf = bsearch(&pk, ps, 3, sizeof ps[0], pcmp);
    printf("%s%s%s %s\n", ps[0].n, ps[1].n, ps[2].n, pf->n);
    return 0;
}
