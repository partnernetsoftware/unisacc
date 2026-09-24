/* <string.h> for the unisa C subset: real implementations, compiled from
 * source, because there is no linker and nothing to link against.  A program
 * that includes this header carries the functions it uses -- and, today, the
 * ones it does not. */
#ifndef _UNISA_STRING_H
#define _UNISA_STRING_H
#include <stddef.h>
#define NULL 0

static long strlen(const char *s) {
    long n;
    n = 0;
    while (s[n]) n = n + 1;
    return n;
}

static char *strcpy(char *d, const char *s) {
    long i;
    i = 0;
    while (s[i]) { d[i] = s[i]; i = i + 1; }
    d[i] = 0;
    return d;
}

static char *strncpy(char *d, const char *s, long n) {
    long i;
    i = 0;
    while (i < n) { d[i] = s[i]; if (s[i] == 0) break; i = i + 1; }
    while (i < n) { d[i] = 0; i = i + 1; }
    return d;
}

static char *strcat(char *d, const char *s) {
    strcpy(d + strlen(d), s);
    return d;
}

static int strcmp(const char *a, const char *b) {
    long i;
    i = 0;
    while (a[i]) { if (a[i] != b[i]) break; i = i + 1; }
    return (a[i] & 255) - (b[i] & 255);
}

static int strncmp(const char *a, const char *b, long n) {
    long i;
    i = 0;
    while (i < n) {
        if (a[i] != b[i]) return (a[i] & 255) - (b[i] & 255);
        if (a[i] == 0) break;
        i = i + 1;
    }
    return 0;
}

static char *strchr(const char *s, int c) {
    long i;
    i = 0;
    while (1) {
        if ((s[i] & 255) == (c & 255)) return (char *)(s + i);
        if (s[i] == 0) return NULL;
        i = i + 1;
    }
}

static char *strrchr(const char *s, int c) {
    long i;
    char *r;
    i = 0; r = NULL;
    while (1) {
        if ((s[i] & 255) == (c & 255)) r = (char *)(s + i);
        if (s[i] == 0) return r;
        i = i + 1;
    }
}

static char *strstr(const char *h, const char *n) {
    long i; long j;
    i = 0;
    while (h[i]) {
        j = 0;
        while (n[j]) { if (h[i + j] != n[j]) break; j = j + 1; }
        if (n[j] == 0) return (char *)(h + i);
        i = i + 1;
    }
    return NULL;
}

static void *memset(void *p, int c, long n) {
    char *d; long i;
    d = (char *)p; i = 0;
    while (i < n) { d[i] = c; i = i + 1; }
    return p;
}

static void *memcpy(void *dst, const void *src, long n) {
    char *d; char *s; long i;
    d = (char *)dst; s = (char *)src; i = 0;
    while (i < n) { d[i] = s[i]; i = i + 1; }
    return dst;
}

static void *memmove(void *dst, const void *src, long n) {
    char *d; char *s; long i;
    d = (char *)dst; s = (char *)src;
    if (d < s) return memcpy(dst, src, n);
    i = n;
    while (i > 0) { i = i - 1; d[i] = s[i]; }
    return dst;
}

static int memcmp(const void *a, const void *b, long n) {
    char *x; char *y; long i;
    x = (char *)a; y = (char *)b; i = 0;
    while (i < n) {
        if (x[i] != y[i]) return (x[i] & 255) - (y[i] & 255);
        i = i + 1;
    }
    return 0;
}
static char *strncat(char *d, const char *s, long n) {
    long i; long j;
    i = 0; while (d[i]) i = i + 1;
    j = 0;
    while (j < n) { if (s[j] == 0) break; d[i + j] = s[j]; j = j + 1; }
    d[i + j] = 0;
    return d;
}

/* strtok keeps its place between calls, as the standard says it must; the
   place is one static pointer, which is why strtok is not reentrant. */
static char *_unisa_tok = 0;
static char *strtok(char *s, const char *delim) {
    char *start; long k; int isd;
    if (s == 0) s = _unisa_tok;
    if (s == 0) return 0;
    /* skip leading delimiters */
    while (*s) {
        isd = 0; k = 0;
        while (delim[k]) { if (delim[k] == *s) isd = 1; k = k + 1; }
        if (isd == 0) break;
        s = s + 1;
    }
    if (*s == 0) { _unisa_tok = 0; return 0; }
    start = s;
    while (*s) {
        isd = 0; k = 0;
        while (delim[k]) { if (delim[k] == *s) isd = 1; k = k + 1; }
        if (isd) { *s = 0; _unisa_tok = s + 1; return start; }
        s = s + 1;
    }
    _unisa_tok = 0;
    return start;
}

#endif
