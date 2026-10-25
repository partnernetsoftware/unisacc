/* Macros named key, base, n, sz, end, lo, hi, mid, cmp before <stdlib.h>. */
#define key 21
#define base 22
#define n 23
#define sz 24
#define end 25
#define lo 26
#define hi 27
#define mid 28
#define cmp 29
#include <stdlib.h>
#include <stdio.h>
static int icmp(const void *a, const void *b) {
    return *(const int *)a - *(const int *)b;
}
int main(void) {
    int arr[6] = {5, 3, 9, 1, 7, 2};
    int k = 7, j;
    int *hit;
    char *e;
    int *m;
    qsort(arr, 6, sizeof arr[0], icmp);
    for (j = 0; j < 6; j++) printf("%d ", arr[j]);
    hit = (int *)bsearch(&k, arr, 6, sizeof arr[0], icmp);
    printf("| %ld\n", hit ? (long)(hit - arr) : -1L);
    printf("%ld %d %ld\n", strtol("  -0x1f rest", &e, 16), atoi("321"), labs(-5L));
    printf("[%s]\n", e);
    m = (int *)malloc(4 * sizeof(int));
    m[3] = 99;
    printf("%d %d %d %d %d %d %d %d %d %d\n", m[3], key, base, n, sz, end, lo, hi, mid, cmp);
    free(m);
    return 0;
}
