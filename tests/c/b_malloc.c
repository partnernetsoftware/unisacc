/* The allocator reuses what is freed: small enough for the reference VM
   (whose munmap is a no-op), but well past the old 64 KB static pool. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(void) {
    char *p; char *q; long *z; int i; int j; long sum; int ok;
    char *v[300];
    ok = 1;
    for (i = 0; i < 200; i++) {            /* 200 x 4 KB = 800 KB churn */
        p = (char *)malloc(4000);
        if (p == NULL) { ok = 0; break; }
        if (((long)p & 15) != 0) ok = 0;
        memset(p, i & 127, 4000);
        if (p[3999] != (i & 127)) ok = 0;
        free(p);
    }
    printf("churn %d\n", ok);
    q = (char *)malloc(10);
    strcpy(q, "abcdefghi");
    for (i = 1; i <= 8; i++) {             /* grow 40 .. 5120 */
        q = (char *)realloc(q, 10 << (i + 1));
        if (strcmp(q, "abcdefghi") != 0) ok = 0;
    }
    q = (char *)realloc(q, 100000);         /* a mapped block */
    if (strcmp(q, "abcdefghi") != 0) ok = 0;
    q[99999] = 7;
    q = (char *)realloc(q, 5);
    q[4] = 0;
    printf("realloc %d %s\n", ok, q);
    free(q);
    z = (long *)calloc(3000, 8);
    sum = 0;
    for (i = 0; i < 3000; i++) sum = sum + z[i];
    printf("calloc %ld\n", sum);
    free(z);
    sum = 0;
    for (j = 0; j < 3; j++) {
        for (i = 0; i < 300; i++) { v[i] = (char *)malloc(i + 1); v[i][i] = (char)(i & 127); }
        for (i = 0; i < 300; i++) { sum = sum + v[i][i]; free(v[i]); }
    }
    printf("small %ld\n", sum);
    return 0;
}
