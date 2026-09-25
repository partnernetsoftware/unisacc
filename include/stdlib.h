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

static void *malloc(long __u_n) {
    char *__u_p;
    __u_n = (__u_n + 15) & ~15;
    if (_unisa_brk + __u_n > _UNISA_ARENA) return NULL;
    __u_p = _unisa_heap + _unisa_brk;
    _unisa_brk = _unisa_brk + __u_n;
    return __u_p;
}

static void free(void *__u_p) { }

/* `exit` is the one libc function that cannot be written in C: it must not
 * return.  `__exit` is the tape's gate to the OS, so this is a one-line
 * wrapper around it -- and `abort` is the same call with the status a shell
 * reports for SIGABRT. */
static void (*_unisa_atexit[32])(void);
static int _unisa_natexit = 0;
static void exit(int __u_code) {
    /* the atexit handlers, last registered first (C99 7.20.4.3p3) */
    while (_unisa_natexit > 0) {
        _unisa_natexit = _unisa_natexit - 1;
        _unisa_atexit[_unisa_natexit]();
    }
    __exit(__u_code);
}
static void abort(void) { __exit(134); }

static void *calloc(long __u_n, long __u_sz) {
    char *__u_p; long __u_total; long __u_i;
    __u_total = __u_n * __u_sz;
    __u_p = (char *)malloc(__u_total);
    if (__u_p == NULL) return NULL;
    __u_i = 0;
    while (__u_i < __u_total) { __u_p[__u_i] = 0; __u_i = __u_i + 1; }
    return __u_p;
}

static void *realloc(void *__u_old, long __u_n) {
    char *__u_p; char *__u_o; long __u_i;
    __u_p = (char *)malloc(__u_n);
    if (__u_p == NULL) return NULL;
    if (__u_old != NULL) {
        __u_o = (char *)__u_old;
        __u_i = 0;
        while (__u_i < __u_n) { __u_p[__u_i] = __u_o[__u_i]; __u_i = __u_i + 1; }
    }
    return __u_p;
}

static int abs(int __u_v) { if (__u_v < 0) return 0 - __u_v; return __u_v; }

static int atoi(const char *__u_s) {
    int __u_v; int __u_sign; long __u_i;
    __u_v = 0; __u_sign = 1; __u_i = 0;
    while (__u_s[__u_i] == 32) __u_i = __u_i + 1;
    if (__u_s[__u_i] == 45) { __u_sign = 0 - 1; __u_i = __u_i + 1; }
    else { if (__u_s[__u_i] == 43) __u_i = __u_i + 1; }
    while (__u_s[__u_i] >= 48) { if (__u_s[__u_i] > 57) break; __u_v = __u_v * 10 + (__u_s[__u_i] - 48); __u_i = __u_i + 1; }
    return __u_v * __u_sign;
}

/* strtol/strtoul/strtod: the conversions C89 and C99 put here.  `end` is
 * written when it is not null, which is how callers tell "no digits" from
 * "the value happened to be zero". */
static long strtol(const char *__u_s, char **__u_end, int __u_base) {
    long __u_v; int __u_sign; long __u_i; int __u_d;
    __u_v = 0; __u_sign = 1; __u_i = 0;
    while (__u_s[__u_i] == 32 || (__u_s[__u_i] >= 9 && __u_s[__u_i] <= 13)) __u_i = __u_i + 1;
    if (__u_s[__u_i] == 45) { __u_sign = 0 - 1; __u_i = __u_i + 1; }
    else { if (__u_s[__u_i] == 43) __u_i = __u_i + 1; }
    if (__u_base == 0) {
        __u_base = 10;
        if (__u_s[__u_i] == 48) {
            if (__u_s[__u_i+1] == 120 || __u_s[__u_i+1] == 88) { __u_base = 16; __u_i = __u_i + 2; }
            else __u_base = 8;
        }
    } else { if (__u_base == 16) { if (__u_s[__u_i] == 48) {
        if (__u_s[__u_i+1] == 120 || __u_s[__u_i+1] == 88) __u_i = __u_i + 2; } } }
    while (__u_s[__u_i]) {
        __u_d = 0 - 1;
        if (__u_s[__u_i] >= 48 && __u_s[__u_i] <= 57) __u_d = __u_s[__u_i] - 48;
        else { if (__u_s[__u_i] >= 97 && __u_s[__u_i] <= 122) __u_d = __u_s[__u_i] - 97 + 10;
        else { if (__u_s[__u_i] >= 65 && __u_s[__u_i] <= 90) __u_d = __u_s[__u_i] - 65 + 10; } }
        if (__u_d < 0 || __u_d >= __u_base) break;
        __u_v = __u_v * __u_base + __u_d; __u_i = __u_i + 1;
    }
    if (__u_end) *__u_end = (char *)(__u_s + __u_i);
    return __u_v * __u_sign;
}
static unsigned long strtoul(const char *__u_s, char **__u_end, int __u_base) {
    return (unsigned long)strtol(__u_s, __u_end, __u_base);
}
static long atol(const char *__u_s) { return strtol(__u_s, 0, 10); }

/* The fractional part is accumulated as an integer and scaled once, so a
 * long run of digits does not lose the low ones to repeated division. */
static double strtod(const char *__u_s, char **__u_end) {
    double __u_v; double __u_frac; double __u_scale; int __u_sign; long __u_i; int __u_any;
    long __u_e; int __u_esign;
    __u_v = 0.0; __u_sign = 1; __u_i = 0; __u_any = 0;
    while (__u_s[__u_i] == 32 || (__u_s[__u_i] >= 9 && __u_s[__u_i] <= 13)) __u_i = __u_i + 1;
    if (__u_s[__u_i] == 45) { __u_sign = 0 - 1; __u_i = __u_i + 1; }
    else { if (__u_s[__u_i] == 43) __u_i = __u_i + 1; }
    while (__u_s[__u_i] >= 48 && __u_s[__u_i] <= 57) { __u_v = __u_v * 10.0 + (double)(__u_s[__u_i] - 48); __u_i = __u_i + 1; __u_any = 1; }
    if (__u_s[__u_i] == 46) {
        __u_i = __u_i + 1; __u_frac = 0.0; __u_scale = 1.0;
        while (__u_s[__u_i] >= 48 && __u_s[__u_i] <= 57) {
            __u_frac = __u_frac * 10.0 + (double)(__u_s[__u_i] - 48); __u_scale = __u_scale * 10.0;
            __u_i = __u_i + 1; __u_any = 1;
        }
        if (__u_scale > 1.0) __u_v = __u_v + __u_frac / __u_scale;
    }
    if (__u_any) { if (__u_s[__u_i] == 101 || __u_s[__u_i] == 69) {
        long __u_j; __u_j = __u_i + 1; __u_esign = 1;
        if (__u_s[__u_j] == 45) { __u_esign = 0 - 1; __u_j = __u_j + 1; }
        else { if (__u_s[__u_j] == 43) __u_j = __u_j + 1; }
        if (__u_s[__u_j] >= 48 && __u_s[__u_j] <= 57) {
            __u_e = 0;
            while (__u_s[__u_j] >= 48 && __u_s[__u_j] <= 57) { __u_e = __u_e * 10 + (__u_s[__u_j] - 48); __u_j = __u_j + 1; }
            __u_i = __u_j;
            while (__u_e > 0) { if (__u_esign > 0) __u_v = __u_v * 10.0; else __u_v = __u_v / 10.0; __u_e = __u_e - 1; }
        }
    } }
    if (__u_end) *__u_end = (char *)(__u_s + (__u_any ? __u_i : 0));
    return __u_v * (double)__u_sign;
}
static float strtof(const char *__u_s, char **__u_end) { return (float)strtod(__u_s, __u_end); }
static double strtold(const char *__u_s, char **__u_end) { return strtod(__u_s, __u_end); }
static double atof(const char *__u_s) { return strtod(__u_s, 0); }

static long _unisa_seed = 1;
static int rand(void) {
    _unisa_seed = _unisa_seed * 1103515245 + 12345;
    return (int)((_unisa_seed >> 16) & 32767);
}
static void srand(int __u_s) { _unisa_seed = __u_s; }
/* ---- C99 7.20.6-7.20.7 and the pieces of 7.20.4 a program can rely on
   here: labs, div, qsort, bsearch, atexit.  Real code, like the rest of
   this header: the sort is a heap sort, so its worst case is bounded and
   it needs no stack beyond one element's bytes [S-15 D2]. */
static long labs(long __u_v) { if (__u_v < 0) return 0 - __u_v; return __u_v; }
static long long llabs(long long __u_v) { if (__u_v < 0) return 0 - __u_v; return __u_v; }
typedef struct { int quot; int rem; } div_t;
typedef struct { long quot; long rem; } ldiv_t;
static div_t div(int __u_a, int __u_b) { div_t __u_r; __u_r.quot = __u_a / __u_b; __u_r.rem = __u_a % __u_b; return __u_r; }
static ldiv_t ldiv(long __u_a, long __u_b) { ldiv_t __u_r; __u_r.quot = __u_a / __u_b; __u_r.rem = __u_a % __u_b; return __u_r; }

static void _unisa_swap(char *__u_a, char *__u_b, long __u_n) {
    long __u_i; char __u_t;
    __u_i = 0;
    while (__u_i < __u_n) { __u_t = __u_a[__u_i]; __u_a[__u_i] = __u_b[__u_i]; __u_b[__u_i] = __u_t; __u_i = __u_i + 1; }
}

/* Sift element `i` of a heap of `n` elements down to its place. */
static void _unisa_sift(char *__u_base, long __u_n, long __u_sz, long __u_i, int (*__u_cmp)(const void *, const void *)) {
    long __u_c;
    while (1) {
        __u_c = 2 * __u_i + 1;
        if (__u_c >= __u_n) return;
        if (__u_c + 1 < __u_n) { if (__u_cmp(__u_base + __u_c * __u_sz, __u_base + (__u_c + 1) * __u_sz) < 0) __u_c = __u_c + 1; }
        if (__u_cmp(__u_base + __u_i * __u_sz, __u_base + __u_c * __u_sz) >= 0) return;
        _unisa_swap(__u_base + __u_i * __u_sz, __u_base + __u_c * __u_sz, __u_sz);
        __u_i = __u_c;
    }
}

static void qsort(void *__u_v, long __u_n, long __u_sz, int (*__u_cmp)(const void *, const void *)) {
    char *__u_base; long __u_i;
    __u_base = (char *)__u_v;
    if (__u_n < 2) return;
    __u_i = __u_n / 2;
    while (__u_i > 0) { __u_i = __u_i - 1; _unisa_sift(__u_base, __u_n, __u_sz, __u_i, __u_cmp); }
    __u_i = __u_n;
    while (__u_i > 1) {
        __u_i = __u_i - 1;
        _unisa_swap(__u_base, __u_base + __u_i * __u_sz, __u_sz);
        _unisa_sift(__u_base, __u_i, __u_sz, 0, __u_cmp);
    }
}

static void *bsearch(const void *__u_key, const void *__u_v, long __u_n, long __u_sz, int (*__u_cmp)(const void *, const void *)) {
    char *__u_base; long __u_lo; long __u_hi; long __u_mid; int __u_c;
    __u_base = (char *)__u_v; __u_lo = 0; __u_hi = __u_n;
    while (__u_lo < __u_hi) {
        __u_mid = __u_lo + (__u_hi - __u_lo) / 2;
        __u_c = __u_cmp(__u_key, __u_base + __u_mid * __u_sz);
        if (__u_c == 0) return __u_base + __u_mid * __u_sz;
        if (__u_c < 0) __u_hi = __u_mid; else __u_lo = __u_mid + 1;
    }
    return 0;
}

/* atexit: the handlers run, last registered first, from `exit` -- which is
   also where a return from main ends up, since the entry stub calls `exit`
   whenever this header defined one.  Thirty-two is C's minimum (7.20.4.2). */
static int atexit(void (*__u_fn)(void)) {
    if (_unisa_natexit >= 32) return 0 - 1;
    _unisa_atexit[_unisa_natexit] = __u_fn;
    _unisa_natexit = _unisa_natexit + 1;
    return 0;
}

/* The environment is where a Unix kernel leaves it: after argv's NULL.
   `__argv(k)` reads that array itself, so the walk starts past argc.  On
   Windows nothing is there and getenv answers NULL. [S-15 D2] */
static char *getenv(const char *__u_name) {
    int __u_k; int __u_i; char *__u_e;
    __u_k = __argc() + 1;
    while ((__u_e = __argv(__u_k)) != 0) {
        __u_i = 0;
        while (__u_name[__u_i] && __u_e[__u_i] == __u_name[__u_i]) __u_i = __u_i + 1;
        if (__u_name[__u_i] == 0 && __u_e[__u_i] == 61) return __u_e + __u_i + 1;
        __u_k = __u_k + 1;
    }
    return 0;
}

#endif
