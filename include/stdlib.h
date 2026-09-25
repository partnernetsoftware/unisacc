/* <stdlib.h> for the unisa C subset.  The allocator is a bump allocator over
 * a static arena: `free` is a no-op and the arena is part of the image, which
 * is honest about what this compiler is for.  Nothing here needs a linker. */
#ifndef _UNISA_STDLIB_H
#define _UNISA_STDLIB_H
#include <stddef.h>
#define NULL 0
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define RAND_MAX 32767

int exit();

#define _UNISA_ARENA 65536
static char _unisa_heap[_UNISA_ARENA];
static long _unisa_brk = 0;

static void *malloc(long n) {
    char *p;
    n = (n + 15) & ~15;
    if (_unisa_brk + n > _UNISA_ARENA) return NULL;
    p = _unisa_heap + _unisa_brk;
    _unisa_brk = _unisa_brk + n;
    return p;
}

static void free(void *p) { }

/* `exit` is the one libc function that cannot be written in C: it must not
 * return.  `__exit` is the tape's gate to the OS, so this is a one-line
 * wrapper around it -- and `abort` is the same call with the status a shell
 * reports for SIGABRT. */
static void (*_unisa_atexit[32])(void);
static int _unisa_natexit = 0;
static void exit(int code) {
    /* the atexit handlers, last registered first (C99 7.20.4.3p3) */
    while (_unisa_natexit > 0) {
        _unisa_natexit = _unisa_natexit - 1;
        _unisa_atexit[_unisa_natexit]();
    }
    __exit(code);
}
static void abort(void) { __exit(134); }

static void *calloc(long n, long sz) {
    char *p; long total; long i;
    total = n * sz;
    p = (char *)malloc(total);
    if (p == NULL) return NULL;
    i = 0;
    while (i < total) { p[i] = 0; i = i + 1; }
    return p;
}

static void *realloc(void *old, long n) {
    char *p; char *o; long i;
    p = (char *)malloc(n);
    if (p == NULL) return NULL;
    if (old != NULL) {
        o = (char *)old;
        i = 0;
        while (i < n) { p[i] = o[i]; i = i + 1; }
    }
    return p;
}

static int abs(int v) { if (v < 0) return 0 - v; return v; }

static int atoi(const char *s) {
    int v; int sign; long i;
    v = 0; sign = 1; i = 0;
    while (s[i] == 32) i = i + 1;
    if (s[i] == 45) { sign = 0 - 1; i = i + 1; }
    else { if (s[i] == 43) i = i + 1; }
    while (s[i] >= 48) { if (s[i] > 57) break; v = v * 10 + (s[i] - 48); i = i + 1; }
    return v * sign;
}

/* strtol/strtoul/strtod: the conversions C89 and C99 put here.  `end` is
 * written when it is not null, which is how callers tell "no digits" from
 * "the value happened to be zero". */
static long strtol(const char *s, char **end, int base) {
    long v; int sign; long i; int d;
    v = 0; sign = 1; i = 0;
    while (s[i] == 32 || (s[i] >= 9 && s[i] <= 13)) i = i + 1;
    if (s[i] == 45) { sign = 0 - 1; i = i + 1; }
    else { if (s[i] == 43) i = i + 1; }
    if (base == 0) {
        base = 10;
        if (s[i] == 48) {
            if (s[i+1] == 120 || s[i+1] == 88) { base = 16; i = i + 2; }
            else base = 8;
        }
    } else { if (base == 16) { if (s[i] == 48) {
        if (s[i+1] == 120 || s[i+1] == 88) i = i + 2; } } }
    while (s[i]) {
        d = 0 - 1;
        if (s[i] >= 48 && s[i] <= 57) d = s[i] - 48;
        else { if (s[i] >= 97 && s[i] <= 122) d = s[i] - 97 + 10;
        else { if (s[i] >= 65 && s[i] <= 90) d = s[i] - 65 + 10; } }
        if (d < 0 || d >= base) break;
        v = v * base + d; i = i + 1;
    }
    if (end) *end = (char *)(s + i);
    return v * sign;
}
static unsigned long strtoul(const char *s, char **end, int base) {
    return (unsigned long)strtol(s, end, base);
}
static long atol(const char *s) { return strtol(s, 0, 10); }

/* The fractional part is accumulated as an integer and scaled once, so a
 * long run of digits does not lose the low ones to repeated division. */
static double strtod(const char *s, char **end) {
    double v; double frac; double scale; int sign; long i; int any;
    long e; int esign;
    v = 0.0; sign = 1; i = 0; any = 0;
    while (s[i] == 32 || (s[i] >= 9 && s[i] <= 13)) i = i + 1;
    if (s[i] == 45) { sign = 0 - 1; i = i + 1; }
    else { if (s[i] == 43) i = i + 1; }
    while (s[i] >= 48 && s[i] <= 57) { v = v * 10.0 + (double)(s[i] - 48); i = i + 1; any = 1; }
    if (s[i] == 46) {
        i = i + 1; frac = 0.0; scale = 1.0;
        while (s[i] >= 48 && s[i] <= 57) {
            frac = frac * 10.0 + (double)(s[i] - 48); scale = scale * 10.0;
            i = i + 1; any = 1;
        }
        if (scale > 1.0) v = v + frac / scale;
    }
    if (any) { if (s[i] == 101 || s[i] == 69) {
        long j; j = i + 1; esign = 1;
        if (s[j] == 45) { esign = 0 - 1; j = j + 1; }
        else { if (s[j] == 43) j = j + 1; }
        if (s[j] >= 48 && s[j] <= 57) {
            e = 0;
            while (s[j] >= 48 && s[j] <= 57) { e = e * 10 + (s[j] - 48); j = j + 1; }
            i = j;
            while (e > 0) { if (esign > 0) v = v * 10.0; else v = v / 10.0; e = e - 1; }
        }
    } }
    if (end) *end = (char *)(s + (any ? i : 0));
    return v * (double)sign;
}
static float strtof(const char *s, char **end) { return (float)strtod(s, end); }
static double strtold(const char *s, char **end) { return strtod(s, end); }
static double atof(const char *s) { return strtod(s, 0); }

static long _unisa_seed = 1;
static int rand(void) {
    _unisa_seed = _unisa_seed * 1103515245 + 12345;
    return (int)((_unisa_seed >> 16) & 32767);
}
static void srand(int s) { _unisa_seed = s; }
/* ---- C99 7.20.6-7.20.7 and the pieces of 7.20.4 a program can rely on
   here: labs, div, qsort, bsearch, atexit.  Real code, like the rest of
   this header: the sort is a heap sort, so its worst case is bounded and
   it needs no stack beyond one element's bytes [S-15 D2]. */
static long labs(long v) { if (v < 0) return 0 - v; return v; }
static long long llabs(long long v) { if (v < 0) return 0 - v; return v; }
typedef struct { int quot; int rem; } div_t;
typedef struct { long quot; long rem; } ldiv_t;
static div_t div(int a, int b) { div_t r; r.quot = a / b; r.rem = a % b; return r; }
static ldiv_t ldiv(long a, long b) { ldiv_t r; r.quot = a / b; r.rem = a % b; return r; }

static void _unisa_swap(char *a, char *b, long n) {
    long i; char t;
    i = 0;
    while (i < n) { t = a[i]; a[i] = b[i]; b[i] = t; i = i + 1; }
}

/* Sift element `i` of a heap of `n` elements down to its place. */
static void _unisa_sift(char *base, long n, long sz, long i, int (*cmp)(const void *, const void *)) {
    long c;
    while (1) {
        c = 2 * i + 1;
        if (c >= n) return;
        if (c + 1 < n) { if (cmp(base + c * sz, base + (c + 1) * sz) < 0) c = c + 1; }
        if (cmp(base + i * sz, base + c * sz) >= 0) return;
        _unisa_swap(base + i * sz, base + c * sz, sz);
        i = c;
    }
}

static void qsort(void *v, long n, long sz, int (*cmp)(const void *, const void *)) {
    char *base; long i;
    base = (char *)v;
    if (n < 2) return;
    i = n / 2;
    while (i > 0) { i = i - 1; _unisa_sift(base, n, sz, i, cmp); }
    i = n;
    while (i > 1) {
        i = i - 1;
        _unisa_swap(base, base + i * sz, sz);
        _unisa_sift(base, i, sz, 0, cmp);
    }
}

static void *bsearch(const void *key, const void *v, long n, long sz, int (*cmp)(const void *, const void *)) {
    char *base; long lo; long hi; long mid; int c;
    base = (char *)v; lo = 0; hi = n;
    while (lo < hi) {
        mid = lo + (hi - lo) / 2;
        c = cmp(key, base + mid * sz);
        if (c == 0) return base + mid * sz;
        if (c < 0) hi = mid; else lo = mid + 1;
    }
    return 0;
}

/* atexit: the handlers run, last registered first, from `exit` -- which is
   also where a return from main ends up, since the entry stub calls `exit`
   whenever this header defined one.  Thirty-two is C's minimum (7.20.4.2). */
static int atexit(void (*fn)(void)) {
    if (_unisa_natexit >= 32) return 0 - 1;
    _unisa_atexit[_unisa_natexit] = fn;
    _unisa_natexit = _unisa_natexit + 1;
    return 0;
}

/* The environment is where a Unix kernel leaves it: after argv's NULL.
   `__argv(k)` reads that array itself, so the walk starts past argc.  On
   Windows nothing is there and getenv answers NULL. [S-15 D2] */
static char *getenv(const char *name) {
    int k; int i; char *e;
    k = __argc() + 1;
    while ((e = __argv(k)) != 0) {
        i = 0;
        while (name[i] && e[i] == name[i]) i = i + 1;
        if (name[i] == 0 && e[i] == 61) return e + i + 1;
        k = k + 1;
    }
    return 0;
}

#endif
