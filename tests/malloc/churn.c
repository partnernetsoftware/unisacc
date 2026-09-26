/* More than 100 MB of churn with a peak of a few MB, realloc growth and
   shrink with content checks, calloc zeroing of reused memory and overflow,
   and many small allocations live at once.  Run by tests/malloc.sh: too
   much work for the reference VM, so it is not a tests/c probe. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SLOTS 64
static long seed = 12345;
static long rnd(void) {
    seed = (seed * 1103515245 + 12345) & 0x7fffffff;
    return seed;
}
int main(void) {
    char *slot[SLOTS]; long len[SLOTS]; int i; long it; long total; int bad; long k;
    char **many; long *z; char *d;
    unsigned char *b; long n; long m;
    bad = 0; total = 0;
    for (i = 0; i < SLOTS; i++) { slot[i] = NULL; len[i] = 0; }
    for (it = 0; it < 4000; it++) {
        i = (int)(rnd() % SLOTS);
        if (slot[i] != NULL) {
            if (slot[i][0] != (char)(i + 1) || slot[i][len[i] - 1] != (char)(i + 2)) bad++;
            if (((long)slot[i] & 15) != 0) bad++;
            free(slot[i]);
        }
        if (rnd() % 4 == 0) len[i] = 1 + rnd() % 300000;
        else len[i] = 1 + rnd() % 20000;
        slot[i] = (char *)malloc(len[i]);
        if (slot[i] == NULL) { printf("NULL at %ld\n", it); return 1; }
        memset(slot[i], 0x5a, len[i]);
        slot[i][0] = (char)(i + 1); slot[i][len[i] - 1] = (char)(i + 2);
        total = total + len[i];
    }
    for (i = 0; i < SLOTS; i++) free(slot[i]);
    printf("churn %d total>100MB %d\n", bad, total > 100000000);
    n = 1; b = (unsigned char *)malloc(1); b[0] = 0;
    while (n < 4000000) {
        m = n * 3 / 2 + 1;
        b = (unsigned char *)realloc(b, m);
        for (k = n; k < m; k++) b[k] = (unsigned char)(k * 7);
        n = m;
    }
    for (k = 0; k < n; k++) if (b[k] != (unsigned char)(k * 7)) { bad++; break; }
    while (n > 10) {
        n = n / 3;
        b = (unsigned char *)realloc(b, n);
        for (k = 0; k < n; k++) if (b[k] != (unsigned char)(k * 7)) { bad++; break; }
    }
    free(b);
    printf("realloc %d\n", bad);
    for (it = 0; it < 50; it++) {
        d = (char *)malloc(50000); memset(d, 0xff, 50000); free(d);
        z = (long *)calloc(6250, 8);
        for (k = 0; k < 6250; k++) if (z[k] != 0) { bad++; break; }
        free(z);
        d = (char *)malloc(400000); memset(d, 0xff, 400000); free(d);
        z = (long *)calloc(50000, 8);
        for (k = 0; k < 50000; k++) if (z[k] != 0) { bad++; break; }
        free(z);
    }
    printf("calloc %d\n", bad);
    printf("overflow %d %d\n", calloc(0x4000000000000000, 4) == NULL,
           calloc(4, 0x4000000000000000) == NULL);
    many = (char **)malloc(200000 * sizeof(char *));
    for (k = 0; k < 200000; k++) { many[k] = (char *)malloc(1 + k % 40); many[k][0] = (char)k; }
    for (k = 0; k < 200000; k++) { if (many[k][0] != (char)k) bad++; free(many[k]); }
    free(many);
    printf("many %d\n", bad);
    return bad != 0;
}
