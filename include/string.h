/* <string.h> for the unisa C subset: real implementations, compiled from
 * source, because there is no linker and nothing to link against.  A program
 * that includes this header carries the functions it uses -- and, today, the
 * ones it does not. */
#ifndef _UNISA_STRING_H
#define _UNISA_STRING_H
#include <stddef.h>
#define NULL 0

#if !__UNISA_FTRIM_LIBC || __UN_strlen
static long strlen(const char *__u_s) {
    long __u_n;
    __u_n = 0;
    while (__u_s[__u_n]) __u_n = __u_n + 1;
    return __u_n;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strcpy
static char *strcpy(char *__u_d, const char *__u_s) {
    long __u_i;
    __u_i = 0;
    while (__u_s[__u_i]) { __u_d[__u_i] = __u_s[__u_i]; __u_i = __u_i + 1; }
    __u_d[__u_i] = 0;
    return __u_d;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strncpy
static char *strncpy(char *__u_d, const char *__u_s, long __u_n) {
    long __u_i;
    __u_i = 0;
    while (__u_i < __u_n) { __u_d[__u_i] = __u_s[__u_i]; if (__u_s[__u_i] == 0) break; __u_i = __u_i + 1; }
    while (__u_i < __u_n) { __u_d[__u_i] = 0; __u_i = __u_i + 1; }
    return __u_d;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strcat
static char *strcat(char *__u_d, const char *__u_s) {
    strcpy(__u_d + strlen(__u_d), __u_s);
    return __u_d;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strcmp
static int strcmp(const char *__u_a, const char *__u_b) {
    long __u_i;
    __u_i = 0;
    while (__u_a[__u_i]) { if (__u_a[__u_i] != __u_b[__u_i]) break; __u_i = __u_i + 1; }
    return (__u_a[__u_i] & 255) - (__u_b[__u_i] & 255);
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strncmp
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
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strchr
static char *strchr(const char *__u_s, int __u_c) {
    long __u_i;
    __u_i = 0;
    while (1) {
        if ((__u_s[__u_i] & 255) == (__u_c & 255)) return (char *)(__u_s + __u_i);
        if (__u_s[__u_i] == 0) return NULL;
        __u_i = __u_i + 1;
    }
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strrchr
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
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strstr
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
#endif

#if !__UNISA_FTRIM_LIBC || __UN_memset
static void *memset(void *__u_p, int __u_c, long __u_n) {
    char *__u_d; long __u_i;
    __u_d = (char *)__u_p; __u_i = 0;
    if (__u_n >= 16) {   /* D2: eight bytes a step */
        long __u_w; __u_w = __u_c & 255; __u_w = __u_w * 72340172838076673L;
        while ((((long)(__u_d + __u_i)) & 7) != 0) { __u_d[__u_i] = __u_c; __u_i = __u_i + 1; }
        while (__u_i + 8 <= __u_n) { *(long *)(__u_d + __u_i) = __u_w; __u_i = __u_i + 8; }
    }
    while (__u_i < __u_n) { __u_d[__u_i] = __u_c; __u_i = __u_i + 1; }
    return __u_p;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_memcpy
static void *memcpy(void *__u_dst, const void *__u_src, long __u_n) {
    char *__u_d; char *__u_s; long __u_i;
    __u_d = (char *)__u_dst; __u_s = (char *)__u_src; __u_i = 0;
    /* D2: eight bytes a step when both pointers share an alignment: bytes up to an 8-byte
       boundary, aligned long words, then the tail; mismatched alignments stay bytewise */
    if (__u_n >= 16 && ((((long)__u_d) ^ ((long)__u_s)) & 7) == 0) {
        while ((((long)(__u_d + __u_i)) & 7) != 0) { __u_d[__u_i] = __u_s[__u_i]; __u_i = __u_i + 1; }
        while (__u_i + 8 <= __u_n) { *(long *)(__u_d + __u_i) = *(long *)(__u_s + __u_i); __u_i = __u_i + 8; }
    }
    while (__u_i < __u_n) { __u_d[__u_i] = __u_s[__u_i]; __u_i = __u_i + 1; }
    return __u_dst;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_memmove
static void *memmove(void *__u_dst, const void *__u_src, long __u_n) {
    char *__u_d; char *__u_s; long __u_i;
    __u_d = (char *)__u_dst; __u_s = (char *)__u_src;
    if (__u_d < __u_s) return memcpy(__u_dst, __u_src, __u_n);
    __u_i = __u_n;
    while (__u_i > 0) { __u_i = __u_i - 1; __u_d[__u_i] = __u_s[__u_i]; }
    return __u_dst;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_memcmp
static int memcmp(const void *__u_a, const void *__u_b, long __u_n) {
    char *__u_x; char *__u_y; long __u_i;
    __u_x = (char *)__u_a; __u_y = (char *)__u_b; __u_i = 0;
    while (__u_i < __u_n) {
        if (__u_x[__u_i] != __u_y[__u_i]) return (__u_x[__u_i] & 255) - (__u_y[__u_i] & 255);
        __u_i = __u_i + 1;
    }
    return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_strncat
static char *strncat(char *__u_d, const char *__u_s, long __u_n) {
    long __u_i; long __u_j;
    __u_i = 0; while (__u_d[__u_i]) __u_i = __u_i + 1;
    __u_j = 0;
    while (__u_j < __u_n) { if (__u_s[__u_j] == 0) break; __u_d[__u_i + __u_j] = __u_s[__u_j]; __u_j = __u_j + 1; }
    __u_d[__u_i + __u_j] = 0;
    return __u_d;
}
#endif

/* strtok keeps its place between calls, as the standard says it must; the
   place is one static pointer, which is why strtok is not reentrant. */
static char *_unisa_tok = 0;
#if !__UNISA_FTRIM_LIBC || __UN_strtok
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

#if !__UNISA_FTRIM_LIBC || __UN_memchr
static void *memchr(const void *__u_p, int __u_c, long __u_n) {
    const unsigned char *__u_s; long __u_i;
    __u_s = (const unsigned char *)__u_p;
    __u_i = 0;
    while (__u_i < __u_n) {
        if (__u_s[__u_i] == (unsigned char)__u_c) return (void *)(__u_s + __u_i);
        __u_i = __u_i + 1;
    }
    return NULL;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strspn
static long strspn(const char *__u_s, const char *__u_accept) {
    long __u_i; long __u_j; int __u_found;
    __u_i = 0;
    while (__u_s[__u_i]) {
        __u_found = 0; __u_j = 0;
        while (__u_accept[__u_j]) {
            if (__u_s[__u_i] == __u_accept[__u_j]) __u_found = 1;
            __u_j = __u_j + 1;
        }
        if (!__u_found) break;
        __u_i = __u_i + 1;
    }
    return __u_i;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strpbrk
static char *strpbrk(const char *__u_s, const char *__u_accept) {
    long __u_i; long __u_j;
    __u_i = 0;
    while (__u_s[__u_i]) {
        __u_j = 0;
        while (__u_accept[__u_j]) {
            if (__u_s[__u_i] == __u_accept[__u_j]) return (char *)(__u_s + __u_i);
            __u_j = __u_j + 1;
        }
        __u_i = __u_i + 1;
    }
    return NULL;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strcspn
static long strcspn(const char *__u_s, const char *__u_reject) {
    long __u_i; long __u_j;
    __u_i = 0;
    while (__u_s[__u_i]) {
        __u_j = 0;
        while (__u_reject[__u_j]) {
            if (__u_s[__u_i] == __u_reject[__u_j]) return __u_i;
            __u_j = __u_j + 1;
        }
        __u_i = __u_i + 1;
    }
    return __u_i;
}
#endif

/* The bundled locale has only the C collation order. */
#if !__UNISA_FTRIM_LIBC || __UN_strcoll
static int strcoll(const char *__u_a, const char *__u_b) {
    return strcmp(__u_a, __u_b);
}
#endif

/* 0.0.28 H4 (SQLite's shell): POSIX strdup, forwarded to the system C library on Linux and macOS */
#ifndef _WIN32
char *strdup(const char *__u_s);
#endif
/* 0.0.34 L2 (SQLite's Apple path): forwarded to the system C library */
#ifdef __APPLE__
size_t strlcpy(char *__u_d, const char *__u_s, size_t __u_n);
size_t strlcat(char *__u_d, const char *__u_s, size_t __u_n);
#endif
#endif
