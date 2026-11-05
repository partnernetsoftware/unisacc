/* <string.h> for the unisa C subset: real implementations, compiled from
 * source, because there is no linker and nothing to link against.  A program
 * that includes this header carries the functions it uses -- and, today, the
 * ones it does not. */
#ifndef _UNISA_STRING_H
#define _UNISA_STRING_H
#include <stddef.h>
#include <sys/_word.h>
#define NULL 0

#if !__UNISA_FTRIM_LIBC || __UN_strlen
#if defined(RT_EXTERN)
extern __u_word strlen(const char *__u_s);
#else
__UNISA_LIBC_LINKAGE __u_word strlen(const char *__u_s) {
    __u_word __u_n;
    __u_n = 0;
    while (__u_s[__u_n]) __u_n = __u_n + 1;
    return __u_n;
}
#endif
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strcpy
#if defined(RT_EXTERN)
extern char *strcpy(char *__u_d, const char *__u_s);
#else
__UNISA_LIBC_LINKAGE char *strcpy(char *__u_d, const char *__u_s) {
    __u_word __u_i;
    __u_i = 0;
    while (__u_s[__u_i]) { __u_d[__u_i] = __u_s[__u_i]; __u_i = __u_i + 1; }
    __u_d[__u_i] = 0;
    return __u_d;
}
#endif
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strncpy
#if defined(RT_EXTERN)
extern char *strncpy(char *__u_d, const char *__u_s, __u_word __u_n);
#else
__UNISA_LIBC_LINKAGE char *strncpy(char *__u_d, const char *__u_s, __u_word __u_n) {
    __u_word __u_i;
    __u_i = 0;
    while (__u_i < __u_n) { __u_d[__u_i] = __u_s[__u_i]; if (__u_s[__u_i] == 0) break; __u_i = __u_i + 1; }
    while (__u_i < __u_n) { __u_d[__u_i] = 0; __u_i = __u_i + 1; }
    return __u_d;
}
#endif
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strcat
#if defined(RT_EXTERN)
extern char *strcat(char *__u_d, const char *__u_s);
#else
__UNISA_LIBC_LINKAGE char *strcat(char *__u_d, const char *__u_s) {
    strcpy(__u_d + strlen(__u_d), __u_s);
    return __u_d;
}
#endif
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strcmp
#if defined(RT_EXTERN)
extern int strcmp(const char *__u_a, const char *__u_b);
#else
__UNISA_LIBC_LINKAGE int strcmp(const char *__u_a, const char *__u_b) {
    __u_word __u_i;
    __u_i = 0;
    while (__u_a[__u_i]) { if (__u_a[__u_i] != __u_b[__u_i]) break; __u_i = __u_i + 1; }
    return (__u_a[__u_i] & 255) - (__u_b[__u_i] & 255);
}
#endif
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strncmp
#if defined(RT_EXTERN)
extern int strncmp(const char *__u_a, const char *__u_b, __u_word __u_n);
#else
__UNISA_LIBC_LINKAGE int strncmp(const char *__u_a, const char *__u_b, __u_word __u_n) {
    __u_word __u_i;
    __u_i = 0;
    while (__u_i < __u_n) {
        if (__u_a[__u_i] != __u_b[__u_i]) return (__u_a[__u_i] & 255) - (__u_b[__u_i] & 255);
        if (__u_a[__u_i] == 0) break;
        __u_i = __u_i + 1;
    }
    return 0;
}
#endif
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strchr
#if defined(RT_EXTERN)
extern char *strchr(const char *__u_s, int __u_c);
#else
__UNISA_LIBC_LINKAGE char *strchr(const char *__u_s, int __u_c) {
    __u_word __u_i;
    __u_i = 0;
    while (1) {
        if ((__u_s[__u_i] & 255) == (__u_c & 255)) return (char *)(__u_s + __u_i);
        if (__u_s[__u_i] == 0) return NULL;
        __u_i = __u_i + 1;
    }
}
#endif
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strrchr
#if defined(RT_EXTERN)
extern char *strrchr(const char *__u_s, int __u_c);
#else
__UNISA_LIBC_LINKAGE char *strrchr(const char *__u_s, int __u_c) {
    __u_word __u_i;
    char *__u_r;
    __u_i = 0; __u_r = NULL;
    while (1) {
        if ((__u_s[__u_i] & 255) == (__u_c & 255)) __u_r = (char *)(__u_s + __u_i);
        if (__u_s[__u_i] == 0) return __u_r;
        __u_i = __u_i + 1;
    }
}
#endif
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strstr
#if defined(RT_EXTERN)
extern char *strstr(const char *__u_h, const char *__u_n);
#else
__UNISA_LIBC_LINKAGE char *strstr(const char *__u_h, const char *__u_n) {
    __u_word __u_i; __u_word __u_j;
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
#endif

#if !__UNISA_FTRIM_LIBC || __UN_memset
#if defined(RT_EXTERN)
extern void *memset(void *__u_p, int __u_c, __u_word __u_n);
#else
__UNISA_LIBC_LINKAGE void *memset(void *__u_p, int __u_c, __u_word __u_n) {
    char *__u_d; __u_word __u_i;
    __u_d = (char *)__u_p; __u_i = 0;
    if (__u_n >= 16) {   /* D2: eight bytes a step */
        __u_word __u_w; __u_w = __u_c & 255; __u_w = __u_w * 72340172838076673L;
        while ((((__u_word)(__u_d + __u_i)) & 7) != 0) { __u_d[__u_i] = __u_c; __u_i = __u_i + 1; }
        while (__u_i + 8 <= __u_n) { *(__u_word *)(__u_d + __u_i) = __u_w; __u_i = __u_i + 8; }
    }
    while (__u_i < __u_n) { __u_d[__u_i] = __u_c; __u_i = __u_i + 1; }
    return __u_p;
}
#endif
#endif

#if !__UNISA_FTRIM_LIBC || __UN_memcpy
#if defined(RT_EXTERN)
extern void *memcpy(void *__u_dst, const void *__u_src, __u_word __u_n);
#else
__UNISA_LIBC_LINKAGE void *memcpy(void *__u_dst, const void *__u_src, __u_word __u_n) {
    char *__u_d; char *__u_s; __u_word __u_i;
    __u_d = (char *)__u_dst; __u_s = (char *)__u_src; __u_i = 0;
    /* D2: eight bytes a step when both pointers share an alignment: bytes up to an 8-byte
       boundary, aligned long words, then the tail; mismatched alignments stay bytewise */
    if (__u_n >= 16 && ((((__u_word)__u_d) ^ ((__u_word)__u_s)) & 7) == 0) {
        while ((((__u_word)(__u_d + __u_i)) & 7) != 0) { __u_d[__u_i] = __u_s[__u_i]; __u_i = __u_i + 1; }
        while (__u_i + 8 <= __u_n) { *(__u_word *)(__u_d + __u_i) = *(__u_word *)(__u_s + __u_i); __u_i = __u_i + 8; }
    }
    while (__u_i < __u_n) { __u_d[__u_i] = __u_s[__u_i]; __u_i = __u_i + 1; }
    return __u_dst;
}
#endif
#endif

#if !__UNISA_FTRIM_LIBC || __UN_memmove
#if defined(RT_EXTERN)
extern void *memmove(void *__u_dst, const void *__u_src, __u_word __u_n);
#else
__UNISA_LIBC_LINKAGE void *memmove(void *__u_dst, const void *__u_src, __u_word __u_n) {
    char *__u_d; char *__u_s; __u_word __u_i;
    __u_d = (char *)__u_dst; __u_s = (char *)__u_src;
    if (__u_d < __u_s) return memcpy(__u_dst, __u_src, __u_n);
    __u_i = __u_n;
    while (__u_i > 0) { __u_i = __u_i - 1; __u_d[__u_i] = __u_s[__u_i]; }
    return __u_dst;
}
#endif
#endif

#if !__UNISA_FTRIM_LIBC || __UN_memcmp
#if defined(RT_EXTERN)
extern int memcmp(const void *__u_a, const void *__u_b, __u_word __u_n);
#else
__UNISA_LIBC_LINKAGE int memcmp(const void *__u_a, const void *__u_b, __u_word __u_n) {
    char *__u_x; char *__u_y; __u_word __u_i;
    __u_x = (char *)__u_a; __u_y = (char *)__u_b; __u_i = 0;
    while (__u_i < __u_n) {
        if (__u_x[__u_i] != __u_y[__u_i]) return (__u_x[__u_i] & 255) - (__u_y[__u_i] & 255);
        __u_i = __u_i + 1;
    }
    return 0;
}
#endif
#endif
#if !__UNISA_FTRIM_LIBC || __UN_strncat
#if defined(RT_EXTERN)
extern char *strncat(char *__u_d, const char *__u_s, __u_word __u_n);
#else
__UNISA_LIBC_LINKAGE char *strncat(char *__u_d, const char *__u_s, __u_word __u_n) {
    __u_word __u_i; __u_word __u_j;
    __u_i = 0; while (__u_d[__u_i]) __u_i = __u_i + 1;
    __u_j = 0;
    while (__u_j < __u_n) { if (__u_s[__u_j] == 0) break; __u_d[__u_i + __u_j] = __u_s[__u_j]; __u_j = __u_j + 1; }
    __u_d[__u_i + __u_j] = 0;
    return __u_d;
}
#endif
#endif

/* strtok keeps its place between calls, as the standard says it must; the
   place is one static pointer, which is why strtok is not reentrant. */
static char *_unisa_tok = 0;
#if !__UNISA_FTRIM_LIBC || __UN_strtok
#if defined(RT_EXTERN)
extern char *strtok(char *__u_s, const char *__u_delim);
#else
__UNISA_LIBC_LINKAGE char *strtok(char *__u_s, const char *__u_delim) {
    char *__u_start; __u_word __u_k; int __u_isd;
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
#endif

#if !__UNISA_FTRIM_LIBC || __UN_memchr
#if defined(RT_EXTERN)
extern void *memchr(const void *__u_p, int __u_c, __u_word __u_n);
#else
__UNISA_LIBC_LINKAGE void *memchr(const void *__u_p, int __u_c, __u_word __u_n) {
    const unsigned char *__u_s; __u_word __u_i;
    __u_s = (const unsigned char *)__u_p;
    __u_i = 0;
    while (__u_i < __u_n) {
        if (__u_s[__u_i] == (unsigned char)__u_c) return (void *)(__u_s + __u_i);
        __u_i = __u_i + 1;
    }
    return NULL;
}
#endif
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strspn
#if defined(RT_EXTERN)
extern __u_word strspn(const char *__u_s, const char *__u_accept);
#else
__UNISA_LIBC_LINKAGE __u_word strspn(const char *__u_s, const char *__u_accept) {
    __u_word __u_i; __u_word __u_j; int __u_found;
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
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strpbrk
#if defined(RT_EXTERN)
extern char *strpbrk(const char *__u_s, const char *__u_accept);
#else
__UNISA_LIBC_LINKAGE char *strpbrk(const char *__u_s, const char *__u_accept) {
    __u_word __u_i; __u_word __u_j;
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
#endif

#if !__UNISA_FTRIM_LIBC || __UN_strcspn
#if defined(RT_EXTERN)
extern __u_word strcspn(const char *__u_s, const char *__u_reject);
#else
__UNISA_LIBC_LINKAGE __u_word strcspn(const char *__u_s, const char *__u_reject) {
    __u_word __u_i; __u_word __u_j;
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
#endif

/* The bundled locale has only the C collation order. */
#if !__UNISA_FTRIM_LIBC || __UN_strcoll
#if defined(RT_EXTERN)
extern int strcoll(const char *__u_a, const char *__u_b);
#else
__UNISA_LIBC_LINKAGE int strcoll(const char *__u_a, const char *__u_b) {
    return strcmp(__u_a, __u_b);
}
#endif
#endif

/* 0.0.28 H4 (SQLite's shell): POSIX strdup, forwarded to the system C library on Linux and macOS */
#ifndef _WIN32
char *strdup(const char *__u_s);
#endif
/* C99 7.21.4.5 strxfrm: prototype-only, forwarded to the host's C library on Linux and macOS
 * (the bundled locale is C, whose transformation is the identity). */
#ifndef _WIN32
size_t strxfrm(char *__u_d, const char *__u_s, size_t __u_n);
#endif
/* 0.0.34 L2 (SQLite's Apple path): forwarded to the system C library */
#ifdef __APPLE__
size_t strlcpy(char *__u_d, const char *__u_s, size_t __u_n);
size_t strlcat(char *__u_d, const char *__u_s, size_t __u_n);
#endif
#endif
