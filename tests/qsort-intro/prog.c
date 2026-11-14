/* qsort introsort test program; built by tools/check-qsort-intro.cx with -I <root>/include so that
 * <stdlib.h> is the workspace header. Prints "PASS<TAB>name" / "FAIL<TAB>name" lines. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* compile-time probe: only the introsort header has this symbol */
static void *g_probe = (void *)_unisa_qintro;

static int g_bad, g_total;
static void ok(int c, const char *name) { g_total++; printf("%s\t%s\n", c ? "PASS" : "FAIL", name); if (!c) g_bad = 1; }

static unsigned long long g_rng = 88172645463325252ULL;
static unsigned long long rnd(void) { g_rng ^= g_rng << 13; g_rng ^= g_rng >> 7; g_rng ^= g_rng << 17; return g_rng; }

static long g_sz, g_k, g_ncmp, g_oob;
static char *g_lo, *g_hi;
static unsigned long keyof(const char *p) {
    unsigned long k = 0; long i;
    for (i = g_k - 1; i >= 0; i--) k = (k << 8) | (unsigned char)p[i];
    return k;
}
static int cmpk(const void *a, const void *b) {
    unsigned long x, y;
    g_ncmp++;
    if ((const char *)a < g_lo || (const char *)a + g_sz > g_hi || (const char *)b < g_lo || (const char *)b + g_sz > g_hi) g_oob++;
    x = keyof((const char *)a); y = keyof((const char *)b);
    return x < y ? -1 : (x > y ? 1 : 0);
}

#define NDIST 9
static unsigned long genkey(int d, long i, long n, unsigned long mask) {
    unsigned long v;
    long h = n / 2;
    switch (d) {
    case 0: v = (unsigned long)rnd(); break;
    case 1: v = (unsigned long)i; break;
    case 2: v = (unsigned long)(n - i); break;
    case 3: v = 7; break;
    case 4: v = (unsigned long)(i < h ? i : n - i); break;
    case 5: v = (unsigned long)(rnd() % 4); break;
    case 6: v = (unsigned long)(i % 17); break;
    case 7: v = (unsigned long)((i & 1) ? i : n - i); break;
    default: v = (unsigned long)(rnd() % 3) * 1000UL + (unsigned long)(i / 7); break;
    }
    return v & mask;
}

static void putkey(char *p, unsigned long k) { long i; for (i = 0; i < g_k; i++) { p[i] = (char)(k & 255); k >>= 8; } }

/* hash of a whole element: multiset check independent of order */
static void eh(const char *p, unsigned long long *s, unsigned long long *x) {
    unsigned long long h = 1469598103934665603ULL; long i;
    for (i = 0; i < g_sz; i++) { h ^= (unsigned char)p[i]; h *= 1099511628211ULL; }
    h ^= h >> 29; h *= 0xbf58476d1ce4e5b9ULL; h ^= h >> 32;
    *s += h; *x ^= (h << 7) | (h >> 57);
}

static unsigned long *g_ka, *g_kb;
static void msort(unsigned long *a, unsigned long *t, long n) {
    long m, i, j, k;
    if (n < 2) return;
    m = n / 2; msort(a, t, m); msort(a + m, t, n - m);
    i = 0; j = m; k = 0;
    while (i < m && j < n) t[k++] = a[i] <= a[j] ? a[i++] : a[j++];
    while (i < m) t[k++] = a[i++];
    while (j < n) t[k++] = a[j++];
    for (i = 0; i < n; i++) a[i] = t[i];
}

static long g_cases, g_fail;
/* one case: size sz, n elements, distribution d, base offset off */
static void one(long sz, long n, int d, long off) {
    char *raw = (char *)malloc((size_t)(n * sz + 16));
    char *b = raw + off;
    unsigned long mask; long i, j; int good = 1;
    unsigned long long s0 = 0, x0 = 0, s1 = 0, x1 = 0;
    g_sz = sz; g_k = sz < 4 ? sz : 4;
    mask = g_k >= 4 ? 0xffffffffUL : ((1UL << (8 * g_k)) - 1);
    g_lo = b; g_hi = b + n * sz; g_oob = 0;
    for (i = 0; i < n; i++) {
        char *p = b + i * sz;
        putkey(p, genkey(d, i, n, mask));
        for (j = g_k; j < sz; j++) p[j] = (char)(rnd() & 255);
        g_ka[i] = keyof(p);
        eh(p, &s0, &x0);
    }
    msort(g_ka, g_kb, n);
    qsort(b, (size_t)n, (size_t)sz, cmpk);
    for (i = 0; i < n; i++) {
        char *p = b + i * sz;
        if (keyof(p) != g_ka[i]) good = 0;
        eh(p, &s1, &x1);
    }
    if (s0 != s1 || x0 != x1) good = 0;
    if (g_oob) good = 0;
    g_cases++;
    if (!good) { g_fail++; if (g_fail < 6) printf("# bad sz=%ld n=%ld d=%d off=%ld\n", sz, n, d, off); }
    free(raw);
}

/* McIlroy adversary against quicksort: must stay O(n log n) */
static int *g_val, g_nsolid, g_cand, g_gas;
static int advcmp(const void *x, const void *y) {
    int a = *(const int *)x, b = *(const int *)y;
    g_ncmp++;
    if (g_val[a] == g_gas && g_val[b] == g_gas) { if (a == g_cand) g_val[a] = g_nsolid++; else g_val[b] = g_nsolid++; }
    if (g_val[a] == g_gas) g_cand = a; else if (g_val[b] == g_gas) g_cand = b;
    return g_val[a] - g_val[b];
}
static int adversary(long n) {
    int *ptr = (int *)malloc(sizeof(int) * n); long i, lg = 0, t = n; int good = 1;
    g_val = (int *)malloc(sizeof(int) * n);
    g_gas = (int)n; g_nsolid = 0; g_cand = 0; g_ncmp = 0;
    for (i = 0; i < n; i++) { g_val[i] = g_gas; ptr[i] = (int)i; }
    qsort(ptr, (size_t)n, sizeof(int), advcmp);
    for (i = 1; i < n; i++) if (g_val[ptr[i - 1]] > g_val[ptr[i]]) good = 0;
    { char *seen = (char *)calloc((size_t)n, 1); for (i = 0; i < n; i++) { if (ptr[i] < 0 || ptr[i] >= n || seen[ptr[i]]) good = 0; else seen[ptr[i]] = 1; } free(seen); }
    while (t > 1) { t /= 2; lg++; }
    if (g_ncmp > 6 * n * (lg + 1)) good = 0;
    free(ptr); free(g_val);
    return good;
}

static int gi_cmp(const void *a, const void *b) { int x = *(const int *)a, y = *(const int *)b; return x < y ? -1 : (x > y); }

int main(void) {
    static const long sizes[] = { 1, 2, 3, 4, 5, 7, 8, 12, 16, 24, 33, 100 };
    static const long offs[] = { 0, 1, 4 };
    long si, oi, n, bigs[3] = { 300, 1000, 5000 };
    int d, bi, allok;
    long i;
    g_ka = (unsigned long *)malloc(sizeof(unsigned long) * 20000);
    g_kb = (unsigned long *)malloc(sizeof(unsigned long) * 20000);
    ok(g_probe != 0, "workspace-header-has-introsort");
    /* nmemb 0..64, all sizes, all distributions, three base alignments */
    for (si = 0; si < 12; si++) {
        g_cases = 0; g_fail = 0;
        for (oi = 0; oi < 3; oi++)
            for (d = 0; d < NDIST; d++)
                for (n = 0; n <= 64; n++) one(sizes[si], n, d, offs[oi]);
        { char nm[64]; sprintf(nm, "small-n0..64-sz%ld-cases%ld", sizes[si], g_cases); ok(g_fail == 0, nm); }
    }
    for (si = 0; si < 12; si++) {
        g_cases = 0; g_fail = 0;
        for (bi = 0; bi < 3; bi++)
            for (d = 0; d < NDIST; d++) one(sizes[si], bigs[bi], d, (si & 1) ? 1 : 0);
        { char nm[64]; sprintf(nm, "large-sz%ld-cases%ld", sizes[si], g_cases); ok(g_fail == 0, nm); }
    }
    g_cases = 0; g_fail = 0;
    for (d = 0; d < NDIST; d++) one(4, 20000, d, 0);
    for (d = 0; d < NDIST; d++) one(8, 20000, d, 0);
    ok(g_fail == 0, "large-n20000-sz4-sz8");
    /* nmemb 0/1 and size 0 are safe no-ops */
    { char c[4] = { 3, 2, 1, 0 }; qsort(c, 0, 1, cmpk); qsort(c, 1, 1, cmpk); qsort(c, 4, 0, cmpk); ok(c[0] == 3 && c[3] == 0, "n0-n1-sz0-noop"); }
    { int e = 1; qsort(&e, 1, 4, gi_cmp); qsort(0, 0, 4, gi_cmp); ok(e == 1, "n1-null0"); }
    /* worst case: McIlroy adversary stays O(n log n) and sorted */
    allok = 1;
    for (i = 100; i <= 20000; i *= 10) if (!adversary(i)) allok = 0;
    ok(allok, "mcilroy-adversary-nlogn");
    ok(adversary(5000), "mcilroy-adversary-5000");
    printf("TOTAL\t%d\n", g_total);
    return g_bad;
}
