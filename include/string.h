/* <string.h> for the unisa C subset: real implementations, compiled from
 * source, because there is no linker and nothing to link against.  A program
 * that includes this header carries the functions it uses -- and, today, the
 * ones it does not. */
#ifndef _UNISA_STRING_H
#define _UNISA_STRING_H
#include <stddef.h>
#define NULL 0

static long strlen(const char *__u_s) {
    long __u_n;
    __u_n = 0;
    while (__u_s[__u_n]) __u_n = __u_n + 1;
    return __u_n;
}

static char *strcpy(char *__u_d, const char *__u_s) {
    long __u_i;
    __u_i = 0;
    while (__u_s[__u_i]) { __u_d[__u_i] = __u_s[__u_i]; __u_i = __u_i + 1; }
    __u_d[__u_i] = 0;
    return __u_d;
}

static char *strncpy(char *__u_d, const char *__u_s, long __u_n) {
    long __u_i;
    __u_i = 0;
    while (__u_i < __u_n) { __u_d[__u_i] = __u_s[__u_i]; if (__u_s[__u_i] == 0) break; __u_i = __u_i + 1; }
    while (__u_i < __u_n) { __u_d[__u_i] = 0; __u_i = __u_i + 1; }
    return __u_d;
}

static char *strcat(char *__u_d, const char *__u_s) {
    strcpy(__u_d + strlen(__u_d), __u_s);
    return __u_d;
}

static int strcmp(const char *__u_a, const char *__u_b) {
    long __u_i;
    __u_i = 0;
    while (__u_a[__u_i]) { if (__u_a[__u_i] != __u_b[__u_i]) break; __u_i = __u_i + 1; }
    return (__u_a[__u_i] & 255) - (__u_b[__u_i] & 255);
}

static int strncmp(const char *__u_a, const char *__u_b, long __u_n) {
    long __u_i;
    __u_i = 0;
    while (__u_i < __u_n) {
        if (__u_a[__u_i] != __u_b[__u_i]) return (__u_a[__u_i] & 255) - (__u_b[__u_i] & 255);
        if (__u_a[__u_i] == 0) break;
        __u_i = __u_i + 1;
    }
    return 0;
}

static char *strchr(const char *__u_s, int __u_c) {
    long __u_i;
    __u_i = 0;
    while (1) {
        if ((__u_s[__u_i] & 255) == (__u_c & 255)) return (char *)(__u_s + __u_i);
        if (__u_s[__u_i] == 0) return NULL;
        __u_i = __u_i + 1;
    }
}

static char *strrchr(const char *__u_s, int __u_c) {
    long __u_i;
    char *__u_r;
    __u_i = 0; __u_r = NULL;
    while (1) {
        if ((__u_s[__u_i] & 255) == (__u_c & 255)) __u_r = (char *)(__u_s + __u_i);
        if (__u_s[__u_i] == 0) return __u_r;
        __u_i = __u_i + 1;
    }
}

static char *strstr(const char *__u_h, const char *__u_n) {
    long __u_i; long __u_j;
    __u_i = 0;
    while (__u_h[__u_i]) {
        __u_j = 0;
        while (__u_n[__u_j]) { if (__u_h[__u_i + __u_j] != __u_n[__u_j]) break; __u_j = __u_j + 1; }
        if (__u_n[__u_j] == 0) return (char *)(__u_h + __u_i);
        __u_i = __u_i + 1;
    }
    return NULL;
}

static void *memset(void *__u_p, int __u_c, long __u_n) {
    char *__u_d; long __u_i;
    __u_d = (char *)__u_p; __u_i = 0;
    while (__u_i < __u_n) { __u_d[__u_i] = __u_c; __u_i = __u_i + 1; }
    return __u_p;
}

static void *memcpy(void *__u_dst, const void *__u_src, long __u_n) {
    char *__u_d; char *__u_s; long __u_i;
    __u_d = (char *)__u_dst; __u_s = (char *)__u_src; __u_i = 0;
    while (__u_i < __u_n) { __u_d[__u_i] = __u_s[__u_i]; __u_i = __u_i + 1; }
    return __u_dst;
}

static void *memmove(void *__u_dst, const void *__u_src, long __u_n) {
    char *__u_d; char *__u_s; long __u_i;
    __u_d = (char *)__u_dst; __u_s = (char *)__u_src;
    if (__u_d < __u_s) return memcpy(__u_dst, __u_src, __u_n);
    __u_i = __u_n;
    while (__u_i > 0) { __u_i = __u_i - 1; __u_d[__u_i] = __u_s[__u_i]; }
    return __u_dst;
}

static int memcmp(const void *__u_a, const void *__u_b, long __u_n) {
    char *__u_x; char *__u_y; long __u_i;
    __u_x = (char *)__u_a; __u_y = (char *)__u_b; __u_i = 0;
    while (__u_i < __u_n) {
        if (__u_x[__u_i] != __u_y[__u_i]) return (__u_x[__u_i] & 255) - (__u_y[__u_i] & 255);
        __u_i = __u_i + 1;
    }
    return 0;
}
static char *strncat(char *__u_d, const char *__u_s, long __u_n) {
    long __u_i; long __u_j;
    __u_i = 0; while (__u_d[__u_i]) __u_i = __u_i + 1;
    __u_j = 0;
    while (__u_j < __u_n) { if (__u_s[__u_j] == 0) break; __u_d[__u_i + __u_j] = __u_s[__u_j]; __u_j = __u_j + 1; }
    __u_d[__u_i + __u_j] = 0;
    return __u_d;
}

/* strtok keeps its place between calls, as the standard says it must; the
   place is one static pointer, which is why strtok is not reentrant. */
static char *_unisa_tok = 0;
static char *strtok(char *__u_s, const char *__u_delim) {
    char *__u_start; long __u_k; int __u_isd;
    if (__u_s == 0) __u_s = _unisa_tok;
    if (__u_s == 0) return 0;
    /* skip leading delimiters */
    while (*__u_s) {
        __u_isd = 0; __u_k = 0;
        while (__u_delim[__u_k]) { if (__u_delim[__u_k] == *__u_s) __u_isd = 1; __u_k = __u_k + 1; }
        if (__u_isd == 0) break;
        __u_s = __u_s + 1;
    }
    if (*__u_s == 0) { _unisa_tok = 0; return 0; }
    __u_start = __u_s;
    while (*__u_s) {
        __u_isd = 0; __u_k = 0;
        while (__u_delim[__u_k]) { if (__u_delim[__u_k] == *__u_s) __u_isd = 1; __u_k = __u_k + 1; }
        if (__u_isd) { *__u_s = 0; _unisa_tok = __u_s + 1; return __u_start; }
        __u_s = __u_s + 1;
    }
    _unisa_tok = 0;
    return __u_start;
}

#endif
