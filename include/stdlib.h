/* <stdlib.h> for the unisa C subset.  The allocator takes its memory from the
 * OS through the __mmap/__munmap gates and reuses what is freed; nothing here
 * needs a linker. */
#ifndef _UNISA_STDLIB_H
#define _UNISA_STDLIB_H
#include <stddef.h>
#include <errno.h>
#include <limits.h>
#include <sys/_word.h>
#define NULL 0
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define RAND_MAX 32767

/* The allocator.  Memory comes from the OS in 1 MB chunks (mmap, or
 * VirtualAlloc on Windows); a block is a 16-byte header -- its size, then its
 * class -- and the payload, so every pointer handed out is 16-aligned.
 *   small: header + request <= 64 KB is rounded up to a power of two, 32 ..
 *          65536 (classes 0 .. 11).  A freed block goes on its class's list
 *          (the link lives in the payload) and the next request of that class
 *          takes it back.  Nothing is split or coalesced, so nothing can be
 *          corrupted by it: a block is always exactly its class's size.
 *   large: anything bigger is mapped on its own, rounded to 64 KB, class -1,
 *          and `free` returns it to the OS. */
#ifdef _WIN32
#define _UNISA_MAP(__u_n) __mmap(0, (__u_n), 0x3000, 4, 0, 0)
#else
#ifdef __linux__
#define _UNISA_MAP(__u_n) __mmap(0, (__u_n), 3, 0x22, 0 - 1, 0)
#else
#define _UNISA_MAP(__u_n) __mmap(0, (__u_n), 3, 0x1002, 0 - 1, 0)
#endif
#endif
#define _UNISA_CHUNK 1048576
static char *_unisa_freel[12];
static char *_unisa_cur = 0;
static __u_word _unisa_left = 0;

#if !__UNISA_FTRIM_LIBC || __UN__unisa_map
static char *_unisa_map(__u_word __u_n) {
    __u_word __u_r;
    __u_r = (__u_word)_UNISA_MAP(__u_n);
    if (__u_r == 0 - 1 || __u_r == 0) return NULL;
    return (char *)__u_r;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_malloc
static void *malloc(__u_word __u_n) {
    __u_word __u_sz; int __u_k; char *__u_p;
    if (__u_n < 0 || __u_n > 0x100000000000) return NULL;
    __u_sz = __u_n + 16;
    if (__u_sz > 65536) {
        __u_sz = (__u_sz + 65535) & ~65535;
        __u_p = _unisa_map(__u_sz);
        if (__u_p == NULL) return NULL;
        *(__u_word *)__u_p = __u_sz;
        *(__u_word *)(__u_p + 8) = 0 - 1;
        return __u_p + 16;
    }
    __u_k = 0;
    while ((32 << __u_k) < __u_sz) __u_k = __u_k + 1;
    __u_sz = 32 << __u_k;
    __u_p = _unisa_freel[__u_k];
    if (__u_p != NULL) {
        _unisa_freel[__u_k] = *(char **)(__u_p + 16);
        return __u_p + 16;
    }
    if (_unisa_left < __u_sz) {
        __u_p = _unisa_map(_UNISA_CHUNK);
        if (__u_p == NULL) return NULL;
        _unisa_cur = __u_p;
        _unisa_left = _UNISA_CHUNK;
    }
    __u_p = _unisa_cur;
    _unisa_cur = _unisa_cur + __u_sz;
    _unisa_left = _unisa_left - __u_sz;
    *(__u_word *)__u_p = __u_sz;
    *(__u_word *)(__u_p + 8) = __u_k;
    return __u_p + 16;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_free
static void free(void *__u_q) {
    char *__u_p; __u_word __u_k;
    if (__u_q == NULL) return;
    __u_p = (char *)__u_q - 16;
    __u_k = *(__u_word *)(__u_p + 8);
    if (__u_k < 0) { __munmap(__u_p, *(__u_word *)__u_p); return; }
    *(char **)(__u_p + 16) = _unisa_freel[__u_k];
    _unisa_freel[__u_k] = __u_p;
}
#endif

/* `exit` is the one libc function that cannot be written in C: it must not
 * return.  `__exit` is the tape's gate to the OS, so this is a one-line
 * wrapper around it -- and `abort` is the same call with the status a shell
 * reports for SIGABRT. */
#include <sys/_exit.h>   /* exit, the atexit table and the stdio flush hook (shared with <stdio.h>) */
#if !__UNISA_FTRIM_LIBC || __UN_abort
static void abort(void) { __exit(134); }
#endif

#if !__UNISA_FTRIM_LIBC || __UN_calloc
static void *calloc(__u_word __u_n, __u_word __u_sz) {
    __u_word *__u_p; __u_word __u_total; __u_word __u_i;
    if (__u_n < 0 || __u_sz < 0) return NULL;
    if (__u_sz != 0 && __u_n > 0x100000000000 / __u_sz) return NULL;
    __u_total = __u_n * __u_sz;
    __u_p = (__u_word *)malloc(__u_total);
    if (__u_p == NULL) return NULL;
    /* a payload is a multiple of 16 bytes, so whole longs stay inside it */
    __u_total = (__u_total + 7) / 8;
    __u_i = 0;
    while (__u_i < __u_total) { __u_p[__u_i] = 0; __u_i = __u_i + 1; }
    return __u_p;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_realloc
static void *realloc(void *__u_old, __u_word __u_n) {
    __u_word *__u_p; __u_word *__u_o; __u_word __u_i; __u_word __u_cap;
    if (__u_old == NULL) return malloc(__u_n);
    if (__u_n < 0) return NULL;
    __u_cap = *(__u_word *)((char *)__u_old - 16) - 16;
    if (__u_n <= __u_cap && (__u_cap <= 65536 || __u_n > __u_cap / 2)) return __u_old;
    __u_p = (__u_word *)malloc(__u_n);
    if (__u_p == NULL) return NULL;
    if (__u_n < __u_cap) __u_cap = __u_n;
    __u_cap = (__u_cap + 7) / 8;
    __u_o = (__u_word *)__u_old;
    __u_i = 0;
    while (__u_i < __u_cap) { __u_p[__u_i] = __u_o[__u_i]; __u_i = __u_i + 1; }
    free(__u_old);
    return __u_p;
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_abs
static int abs(int __u_v) { if (__u_v < 0) return 0 - __u_v; return __u_v; }
#endif

#if !__UNISA_FTRIM_LIBC || __UN_atoi
static int atoi(const char *__u_s) {
    int __u_v; int __u_sign; __u_word __u_i;
    __u_v = 0; __u_sign = 1; __u_i = 0;
    while (__u_s[__u_i] == 32) __u_i = __u_i + 1;
    if (__u_s[__u_i] == 45) { __u_sign = 0 - 1; __u_i = __u_i + 1; }
    else { if (__u_s[__u_i] == 43) __u_i = __u_i + 1; }
    while (__u_s[__u_i] >= 48) { if (__u_s[__u_i] > 57) break; __u_v = __u_v * 10 + (__u_s[__u_i] - 48); __u_i = __u_i + 1; }
    return __u_v * __u_sign;
}
#endif

/* strtol/strtoul/strtod: the conversions C89 and C99 put here.  `end` is
 * written when it is not null, which is how callers tell "no digits" from
 * "the value happened to be zero". */
/* Accumulate two 32-bit limbs in nonnegative longs.  Their products by
 * base (<=36) fit signed long, including at ULONG_MAX; no signed wrap or
 * unsigned division is needed.  Overflow still consumes every valid digit. */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_strdigit
static int _unisa_strdigit(int __u_c) {
    if (__u_c >= 48 && __u_c <= 57) return __u_c - 48;
    if (__u_c >= 97 && __u_c <= 122) return __u_c - 97 + 10;
    if (__u_c >= 65 && __u_c <= 90) return __u_c - 65 + 10;
    return 0 - 1;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__unisa_strmag
static int _unisa_strmag(const char *__u_s, char **__u_end, int __u_base,
                       __u_word *__u_hi, __u_word *__u_lo, int *__u_neg) {
    __u_word __u_i; __u_word __u_start; __u_word __u_l; __u_word __u_h; int __u_d; int __u_over; int __u_auto;
    __u_auto = __u_base == 0; __u_i = 0; *__u_hi = 0; *__u_lo = 0; *__u_neg = 0; __u_over = 0;
    if (__u_end) *__u_end = (char *)__u_s;
    if (__u_base != 0 && (__u_base < 2 || __u_base > 36)) return 0;
    while (__u_s[__u_i] == 32 || (__u_s[__u_i] >= 9 && __u_s[__u_i] <= 13)) __u_i = __u_i + 1;
    if (__u_s[__u_i] == 45) { *__u_neg = 1; __u_i = __u_i + 1; }
    else { if (__u_s[__u_i] == 43) __u_i = __u_i + 1; }
    if (__u_base == 0) {
        __u_base = 10;
        if (__u_s[__u_i] == 48) __u_base = 8;
    }
    if (__u_base == 8 || __u_base == 16) {
        if (__u_s[__u_i] == 48 && (__u_s[__u_i+1] == 120 || __u_s[__u_i+1] == 88)) {
            __u_d = _unisa_strdigit(__u_s[__u_i+2]);
            if (__u_d >= 0 && __u_d < 16) {
                /* Only base 0 or 16 may accept a hex prefix. */
                if (__u_base == 16 || __u_auto) { __u_base = 16; __u_i = __u_i + 2; }
            }
        }
    }
    __u_start = __u_i;
    while (__u_s[__u_i]) {
        __u_d = _unisa_strdigit(__u_s[__u_i]);
        if (__u_d < 0 || __u_d >= __u_base) break;
        if (!__u_over) {
            __u_l = *__u_lo * __u_base + __u_d;
            __u_h = *__u_hi * __u_base + __u_l / 4294967296;
            if (__u_h > 4294967295) __u_over = 1;
            else { *__u_hi = __u_h; *__u_lo = __u_l % 4294967296; }
        }
        __u_i = __u_i + 1;
    }
    if (__u_end && __u_i != __u_start) *__u_end = (char *)(__u_s + __u_i);
    return __u_over;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_strtol
static long strtol(const char *__u_s, char **__u_end, int __u_base) {
#if defined(_WIN64)
    /* LLP64: long is 32 bits, so the range is [LONG_MIN, LONG_MAX], not the 64-bit one */
    __u_word __u_hi; __u_word __u_lo; __u_uword __u_m; int __u_neg; int __u_over;
    __u_over = _unisa_strmag(__u_s, __u_end, __u_base, &__u_hi, &__u_lo, &__u_neg);
    __u_m = ((__u_uword)__u_hi << 32) | (__u_uword)__u_lo;
    if (__u_over || (!__u_neg && __u_m > (__u_uword)LONG_MAX) ||
        (__u_neg && __u_m > (__u_uword)LONG_MAX + 1)) {
        errno = ERANGE;
        if (__u_neg) return LONG_MIN;
        return LONG_MAX;
    }
    if (__u_neg) return (long)(0 - (__u_word)__u_m);
    return (long)__u_m;
#else
    long __u_hi; long __u_lo; long __u_v; int __u_neg; int __u_over;
    __u_over = _unisa_strmag(__u_s, __u_end, __u_base, &__u_hi, &__u_lo, &__u_neg);
    if (__u_over || __u_hi > 2147483648 ||
        (__u_hi == 2147483648 && (__u_lo != 0 || !__u_neg))) {
        errno = ERANGE;
        if (__u_neg) return LONG_MIN;
        return LONG_MAX;
    }
    if (__u_hi == 2147483648) return LONG_MIN;
    __u_v = __u_hi * 4294967296 + __u_lo;
    if (__u_neg) return 0 - __u_v;
    return __u_v;
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_strtoul
static unsigned long strtoul(const char *__u_s, char **__u_end, int __u_base) {
#if defined(_WIN64)
    /* LLP64: unsigned long is 32 bits; a magnitude past ULONG_MAX is ERANGE, then '-' negates */
    __u_word __u_hi; __u_word __u_lo; unsigned long __u_v; int __u_neg; int __u_over;
    __u_over = _unisa_strmag(__u_s, __u_end, __u_base, &__u_hi, &__u_lo, &__u_neg);
    if (__u_over || __u_hi != 0) { errno = ERANGE; return (unsigned long)0 - 1; }
    __u_v = (unsigned long)__u_lo;
    if (__u_neg) return (unsigned long)0 - __u_v;
    return __u_v;
#else
    long __u_hi; long __u_lo; unsigned long __u_v; int __u_neg; int __u_over;
    __u_over = _unisa_strmag(__u_s, __u_end, __u_base, &__u_hi, &__u_lo, &__u_neg);
    if (__u_over) { errno = ERANGE; return (unsigned long)0 - 1; }
    __u_v = ((unsigned long)__u_hi << 32) | (unsigned long)__u_lo;
    if (__u_neg) return (unsigned long)0 - __u_v;
    return __u_v;
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_atol
static long atol(const char *__u_s) { return strtol(__u_s, 0, 10); }
#endif

/* This compiler uses 64-bit long and long long on all six targets.  Keep the
 * conversion, overflow clamp, ERANGE and end-pointer rules in one routine. */
#if !__UNISA_FTRIM_LIBC || __UN_strtoll
static long long strtoll(const char *__u_s, char **__u_end, int __u_base) {
#if defined(_WIN64)
    /* LLP64: strtol is 32-bit here, so the 64-bit range is converted and clamped directly */
    __u_word __u_hi; __u_word __u_lo; __u_word __u_v; int __u_neg; int __u_over;
    __u_over = _unisa_strmag(__u_s, __u_end, __u_base, &__u_hi, &__u_lo, &__u_neg);
    if (__u_over || __u_hi > 2147483648 ||
        (__u_hi == 2147483648 && (__u_lo != 0 || !__u_neg))) {
        errno = ERANGE;
        if (__u_neg) return LLONG_MIN;
        return LLONG_MAX;
    }
    if (__u_hi == 2147483648) return LLONG_MIN;
    __u_v = __u_hi * 4294967296 + __u_lo;
    if (__u_neg) return 0 - __u_v;
    return __u_v;
#else
    return strtol(__u_s, __u_end, __u_base);
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_strtoull
static unsigned long long strtoull(const char *__u_s, char **__u_end, int __u_base) {
#if defined(_WIN64)
    /* LLP64: strtoul is 32-bit here, so the 64-bit value is assembled directly */
    __u_word __u_hi; __u_word __u_lo; __u_uword __u_v; int __u_neg; int __u_over;
    __u_over = _unisa_strmag(__u_s, __u_end, __u_base, &__u_hi, &__u_lo, &__u_neg);
    if (__u_over) { errno = ERANGE; return (__u_uword)0 - 1; }
    __u_v = ((__u_uword)__u_hi << 32) | (__u_uword)__u_lo;
    if (__u_neg) return (__u_uword)0 - __u_v;
    return __u_v;
#else
    return strtoul(__u_s, __u_end, __u_base);
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_atoll
static long long atoll(const char *__u_s) { return strtoll(__u_s, 0, 10); }
#endif

/* The fractional part is accumulated as an integer and scaled once, so a
 * long run of digits does not lose the low ones to repeated division. */
/* mkstemp (POSIX, 0.0.34 L1b): replace the trailing XXXXXX and create the file
   exclusively, retrying on EEXIST; the letters come from a counter mixed with the
   address of a local, so two processes running the same binary still differ. */
/* No <fcntl.h> here: an untrimmed tape (the Python front end, --fold) would carry
   every fcntl.h body, and those name host calls Windows lowering refuses.  The
   exclusive-create flags are spelled out instead (O_RDWR|O_CREAT|O_EXCL per OS). */
#ifdef _WIN32
#include <sys/_win.h>
#endif
#if !__UNISA_FTRIM_LIBC || __UN_mkstemp
static int mkstemp(char *__u_t) {
    static __u_uword __u_seq;
    __u_word __u_n; __u_word __u_j; int __u_try; __u_word __u_r; __u_uword __u_v;
    __u_n = 0; while (__u_t[__u_n]) __u_n = __u_n + 1;
    if (__u_n < 6) { errno = EINVAL; return 0 - 1; }
    __u_j = __u_n - 6; while (__u_j < __u_n) { if (__u_t[__u_j] != 88) { errno = EINVAL; return 0 - 1; } __u_j = __u_j + 1; }
    __u_try = 0;
    while (__u_try < 100) {
        __u_seq = __u_seq + 1;
        __u_v = ((__u_uword)&__u_try >> 4) * 2654435761u + __u_seq * 40503u;
        __u_j = __u_n - 6;
        while (__u_j < __u_n) { __u_t[__u_j] = (char)(97 + __u_v % 26); __u_v = __u_v / 26 + __u_seq * 7; __u_j = __u_j + 1; }
#ifdef _WIN32
        /* GENERIC_READ|GENERIC_WRITE, CREATE_NEW. The gate returns -1; errno comes from GetLastError. */
        __u_r = __open(__u_t, 0xC0000000L, 1);
        if (__u_r >= 0) return (int)__u_r;
        { int __u_e; _ux_err(__u_e); errno = __u_e; }
#else
#ifdef __APPLE__
        __u_r = __open(__u_t, 0xA02, 384);
#else
        __u_r = __open(__u_t, 0302, 384);
#endif
        if (__u_r >= 0) return (int)__u_r;
        errno = (int)(0 - __u_r);
#endif
        if (errno != EEXIST) return 0 - 1;
        __u_try = __u_try + 1;
    }
    return 0 - 1;
}
#endif
/* realpath (POSIX, 0.0.37 C3): open the path and ask the kernel for the name it resolved
   (Apple F_GETPATH; Linux readlink of /proc/self/fd/N), so symlinks, "." and ".." are the
   kernel's answer, not a lexical guess.  The path must exist, as POSIX requires; a file
   the caller cannot open fails with the open's errno.  NULL resolved: malloc(PATH_MAX).
   Windows (2026-10-26, after Cosmopolitan): open the path and ask GetFinalPathNameByHandleA
   (VOLUME_NAME_DOS), then drop a leading \\?\ or turn \\?\UNC\ into \\. The __open gate
   cannot pass FILE_FLAG_BACKUP_SEMANTICS, so a directory does not open; that case, and any
   final-name failure, falls back to GetFileAttributesA plus GetFullPathNameA. The fallback
   is absolute and does not resolve a symlink. This slice has not been executed on Windows. */
#ifndef __UNISA_PYFRONT
#if !__UNISA_FTRIM_LIBC || __UN_realpath
static char *realpath(const char *__u_p, char *__u_out) {
    char __u_tmp[PATH_MAX]; __u_word __u_fd; __u_word __u_r; __u_word __u_n; __u_word __u_k; char *__u_d;
#ifdef _WIN32
    static __u_word __u_attr, __u_full, __u_fin; __u_word __u_a; __u_word __u_h; __u_word __u_off; int __u_e;
    if (!__u_attr) {
        _ux_load(__u_attr, "GetFileAttributesA");
        _ux_load(__u_full, "GetFullPathNameA");
        _ux_load(__u_fin, "GetFinalPathNameByHandleA");
    }
    __u_n = 0;
    __u_h = __open((char *)__u_p, 0x80000000L, 3);          /* GENERIC_READ, OPEN_EXISTING */
    if (__u_h >= 0) {
        /* Length excludes the NUL. Zero, or a size that does not fit, is not a path. */
        _ux_go(__u_n, __u_fin, __u_h, (__u_word)__u_tmp, PATH_MAX, 0);
        __u_n = __u_n & 0xFFFFFFFFL;
        __close(__u_h);
        if (__u_n == 0 || __u_n >= PATH_MAX) __u_n = 0;
    }
    if (__u_n > 0 && __u_n >= 4 && __u_tmp[0] == 92 && __u_tmp[1] == 92 && __u_tmp[2] == 63 && __u_tmp[3] == 92) {
        __u_off = 4;
        if (__u_n >= 8 && (__u_tmp[4] == 85 || __u_tmp[4] == 117) && (__u_tmp[5] == 78 || __u_tmp[5] == 110) &&
            (__u_tmp[6] == 67 || __u_tmp[6] == 99) && __u_tmp[7] == 92) {
            __u_tmp[0] = 92; __u_tmp[1] = 92; __u_off = 8; __u_k = 2;
        } else __u_k = 0;
        __u_a = __u_off;
        while (__u_a <= __u_n) { __u_tmp[__u_k] = __u_tmp[__u_a]; __u_k = __u_k + 1; __u_a = __u_a + 1; }
        __u_n = __u_k - 1;
    }
    if (__u_n == 0) {
        _ux_go(__u_a, __u_attr, (__u_word)__u_p, 0, 0, 0);
        __u_a = __u_a & 0xFFFFFFFFL;
        if (__u_a == 0xFFFFFFFFL) { _ux_err(__u_e); errno = __u_e; return 0; }
        _ux_go(__u_n, __u_full, (__u_word)__u_p, PATH_MAX, (__u_word)__u_tmp, 0);
        __u_n = __u_n & 0xFFFFFFFFL;
        if (__u_n == 0) { _ux_err(__u_e); errno = __u_e; return 0; }
        if (__u_n >= PATH_MAX) { errno = ENAMETOOLONG; return 0; }
    }
    __u_d = __u_out;
    if (!__u_d) { __u_d = (char *)malloc(PATH_MAX); if (!__u_d) { errno = ENOMEM; return 0; } }
    __u_k = 0; while (__u_k <= __u_n) { __u_d[__u_k] = __u_tmp[__u_k]; __u_k = __u_k + 1; }
    return __u_d;
#else
#ifdef __APPLE__
    __u_fd = __open((char *)__u_p, 0, 0);
    if (__u_fd < 0) { errno = (int)(0 - __u_fd); return 0; }
    __u_r = __fcntl(__u_fd, 50, (long)__u_tmp);   /* F_GETPATH */
    __close(__u_fd);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return 0; }
    __u_n = 0; while (__u_tmp[__u_n]) __u_n = __u_n + 1;
#else
    char __u_fdp[32]; long __u_v; char __u_dig[24];
    __u_fd = __open((char *)__u_p, 010000000, 0);   /* O_PATH: no read permission needed */
    if (__u_fd < 0) { errno = (int)(0 - __u_fd); return 0; }
    __u_k = 0; while ("/proc/self/fd/"[__u_k]) { __u_fdp[__u_k] = "/proc/self/fd/"[__u_k]; __u_k = __u_k + 1; }
    __u_v = __u_fd; __u_n = 0;
    do { __u_dig[__u_n] = (char)(48 + __u_v % 10); __u_v = __u_v / 10; __u_n = __u_n + 1; } while (__u_v);
    while (__u_n) { __u_n = __u_n - 1; __u_fdp[__u_k] = __u_dig[__u_n]; __u_k = __u_k + 1; }
    __u_fdp[__u_k] = 0;
#ifdef __x86_64__
    __u_r = __syscall6(89, (long)__u_fdp, (long)__u_tmp, PATH_MAX - 1, 0, 0);          /* readlink */
#else
    __u_r = __syscall6(78, -100, (long)__u_fdp, (long)__u_tmp, PATH_MAX - 1, 0);       /* readlinkat */
#endif
    __close(__u_fd);
    if (__u_r < 0) { errno = (int)(0 - __u_r); return 0; }
    if (__u_r >= PATH_MAX - 1) { errno = ENAMETOOLONG; return 0; }
    __u_n = __u_r; __u_tmp[__u_n] = 0;
#endif
    __u_d = __u_out;
    if (!__u_d) { __u_d = (char *)malloc(PATH_MAX); if (!__u_d) { errno = ENOMEM; return 0; } }
    __u_k = 0; while (__u_k <= __u_n) { __u_d[__u_k] = __u_tmp[__u_k]; __u_k = __u_k + 1; }
    return __u_d;
#endif
}
#endif
#endif
/* mkdtemp (POSIX, 0.0.35 M1): mkstemp's names, but a directory, mode 0700.
   Windows (2026-10-26, after Cosmopolitan): CreateDirectoryA, mode ignored.
   The template must end in XXXXXX. EEXIST retries. Any other error, and a
   retry limit, put XXXXXX back and return NULL. __mkdir stays on the Unix
   branch: it has no Windows import, and the Python front end folds one tape. */
#ifndef __UNISA_PYFRONT
#if !__UNISA_FTRIM_LIBC || __UN_mkdtemp
static char *mkdtemp(char *__u_t) {
    static __u_uword __u_seq;
    __u_word __u_n; __u_word __u_j; int __u_try; __u_word __u_r; __u_uword __u_v;
    __u_n = 0; while (__u_t[__u_n]) __u_n = __u_n + 1;
    if (__u_n < 6) { errno = EINVAL; return 0; }
    __u_j = __u_n - 6; while (__u_j < __u_n) { if (__u_t[__u_j] != 88) { errno = EINVAL; return 0; } __u_j = __u_j + 1; }
    __u_try = 0;
    while (__u_try < 100) {
        __u_seq = __u_seq + 1;
        __u_v = ((__u_uword)&__u_try >> 4) * 2654435761u + __u_seq * 40503u + 17;
        __u_j = __u_n - 6;
        while (__u_j < __u_n) { __u_t[__u_j] = (char)(97 + __u_v % 26); __u_v = __u_v / 26 + __u_seq * 7; __u_j = __u_j + 1; }
#ifdef _WIN32
        {   static __u_word __u_cd; __u_word __u_ok; int __u_e;
            if (!__u_cd) _ux_load(__u_cd, "CreateDirectoryA");
            _ux_go(__u_ok, __u_cd, (__u_word)__u_t, 0, 0, 0);
            __u_ok = __u_ok & 0xFFFFFFFFL;
            if (__u_ok) return __u_t;
            _ux_err(__u_e); errno = __u_e;
            if (errno != EEXIST) {
                __u_j = __u_n - 6; while (__u_j < __u_n) { __u_t[__u_j] = 88; __u_j = __u_j + 1; }
                return 0;
            }
        }
#else
        __u_r = __mkdir(__u_t, 448, 0);
        if (__u_r >= 0) return __u_t;
        errno = (int)(0 - __u_r);
        if (errno != EEXIST) return 0;
#endif
        __u_try = __u_try + 1;
    }
#ifdef _WIN32
    __u_j = __u_n - 6; while (__u_j < __u_n) { __u_t[__u_j] = 88; __u_j = __u_j + 1; }
#endif
    return 0;
}
#endif
#endif
/* strtod's slow path (more than 19 digits, or a power of ten past 22) is a
 * guess a few ulp off; these compare the decimal with the halfway points around
 * it in big integers (base 2^32) and step to the correctly rounded double. */
#define _U_BL 200
#if !__UNISA_FTRIM_LIBC || __UN__u_bn_muls
static void _u_bn_muls(unsigned int *__u_a, int *__u_n, unsigned int __u_k) {
    __u_uword __u_c; int __u_i; __u_c = 0;
    for (__u_i = 0; __u_i < *__u_n; __u_i++) { __u_c = __u_c + (__u_uword)__u_a[__u_i] * __u_k; __u_a[__u_i] = (unsigned int)__u_c; __u_c = __u_c >> 32; }
    if (__u_c && *__u_n < _U_BL) { __u_a[*__u_n] = (unsigned int)__u_c; *__u_n = *__u_n + 1; }
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__u_bn_shl
static void _u_bn_shl(unsigned int *__u_a, int *__u_n, __u_word __u_b) {
    int __u_w; int __u_s; int __u_i;
    __u_w = (int)(__u_b >> 5); __u_s = (int)(__u_b & 31);
    if (*__u_n == 0) return;
    if (__u_s) _u_bn_muls(__u_a, __u_n, 1u << __u_s);
    if (__u_w) {
        if (*__u_n + __u_w > _U_BL) __u_w = _U_BL - *__u_n;
        for (__u_i = *__u_n - 1; __u_i >= 0; __u_i--) __u_a[__u_i + __u_w] = __u_a[__u_i];
        for (__u_i = 0; __u_i < __u_w; __u_i++) __u_a[__u_i] = 0;
        *__u_n = *__u_n + __u_w;
    }
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__u_bn_pow10
static void _u_bn_pow10(unsigned int *__u_a, int *__u_n, __u_word __u_x) {
    while (__u_x >= 9) { _u_bn_muls(__u_a, __u_n, 1000000000u); __u_x = __u_x - 9; }
    while (__u_x > 0) { _u_bn_muls(__u_a, __u_n, 10u); __u_x = __u_x - 1; }
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__u_bn_cmp
static int _u_bn_cmp(unsigned int *__u_a, int __u_na, unsigned int *__u_b, int __u_nb) {
    int __u_i;
    while (__u_na > 0 && __u_a[__u_na - 1] == 0) __u_na = __u_na - 1;
    while (__u_nb > 0 && __u_b[__u_nb - 1] == 0) __u_nb = __u_nb - 1;
    if (__u_na != __u_nb) return __u_na > __u_nb ? 1 : 0 - 1;
    for (__u_i = __u_na - 1; __u_i >= 0; __u_i--) if (__u_a[__u_i] != __u_b[__u_i]) return __u_a[__u_i] > __u_b[__u_i] ? 1 : 0 - 1;
    return 0;
}
#endif
/* sign of D*10^x10 - (2m+1)*2^(e-1), the halfway point above v; D = the digits
   s[0..len) skipping '.', at most 780 significant (the rest only make it bigger) */
#if !__UNISA_FTRIM_LIBC || __UN__u_sd_cmp
static int _u_sd_cmp(const char *__u_s, __u_word __u_len, __u_word __u_x10, double __u_v) {
    static unsigned int __u_d[_U_BL]; static unsigned int __u_h[_U_BL];
    int __u_nd; int __u_nh; int __u_sticky; int __u_c; __u_word __u_i; __u_word __u_cnt; __u_word __u_xx;
    __u_uword __u_bits; __u_uword __u_m; __u_word __u_e;
    __u_nd = 0; __u_sticky = 0; __u_cnt = 0; __u_xx = __u_x10;
    for (__u_i = 0; __u_i < __u_len; __u_i++) {
        int __u_ch; __u_uword __u_c2; int __u_j;
        __u_ch = __u_s[__u_i];
        if (__u_ch == 46) continue;
        if (__u_cnt >= 780) { if (__u_ch != 48) __u_sticky = 1; __u_xx = __u_xx + 1; continue; }
        if (__u_nd == 0 && __u_ch == 48) continue;
        if (__u_nd == 0) { __u_d[0] = 0; __u_nd = 1; }
        _u_bn_muls(__u_d, &__u_nd, 10u);
        __u_c2 = (__u_uword)(__u_ch - 48);
        for (__u_j = 0; __u_c2 && __u_j < __u_nd; __u_j++) { __u_c2 = __u_c2 + __u_d[__u_j]; __u_d[__u_j] = (unsigned int)__u_c2; __u_c2 = __u_c2 >> 32; }
        if (__u_c2 && __u_nd < _U_BL) { __u_d[__u_nd] = (unsigned int)__u_c2; __u_nd = __u_nd + 1; }
        __u_cnt = __u_cnt + 1;
    }
    if (__u_nd == 0) return 0 - 1;
    __u_bits = *(__u_uword *)&__u_v;
    __u_e = (__u_word)((__u_bits >> 52) & 2047); __u_m = __u_bits & 4503599627370495;
    if (__u_e == 0) __u_e = 1; else __u_m = __u_m | 4503599627370496;
    __u_e = __u_e - 1075;
    __u_m = 2 * __u_m + 1;
    __u_nh = 2; __u_h[0] = (unsigned int)__u_m; __u_h[1] = (unsigned int)(__u_m >> 32);
    if (__u_xx > 0) _u_bn_pow10(__u_d, &__u_nd, __u_xx); else _u_bn_pow10(__u_h, &__u_nh, 0 - __u_xx);
    if (__u_e - 1 > 0) _u_bn_shl(__u_h, &__u_nh, __u_e - 1); else _u_bn_shl(__u_d, &__u_nd, 1 - __u_e);
    __u_c = _u_bn_cmp(__u_d, __u_nd, __u_h, __u_nh);
    if (__u_c == 0 && __u_sticky) __u_c = 1;
    return __u_c;
}
#endif
/* walk the guess (finite, >= 0, a few ulp off) to the correctly rounded double */
#if !__UNISA_FTRIM_LIBC || __UN__u_sd_fix
static double _u_sd_fix(const char *__u_s, __u_word __u_len, __u_word __u_x10, double __u_v) {
    __u_uword __u_b; __u_uword __u_lb; int __u_c; int __u_k;
    for (__u_k = 0; __u_k < 64; __u_k++) {
        __u_b = *(__u_uword *)&__u_v;
        if (__u_b >= 9218868437227405312) return __u_v;              /* inf or nan */
        __u_c = _u_sd_cmp(__u_s, __u_len, __u_x10, __u_v);
        if (__u_c > 0 || (__u_c == 0 && (__u_b & 1))) { __u_b = __u_b + 1; __u_v = *(double *)&__u_b; continue; }
        if (__u_b == 0) return __u_v;
        __u_lb = __u_b - 1;
        __u_c = _u_sd_cmp(__u_s, __u_len, __u_x10, *(double *)&__u_lb);
        if (__u_c < 0 || (__u_c == 0 && !(__u_lb & 1))) { __u_v = *(double *)&__u_lb; continue; }
        return __u_v;
    }
    return __u_v;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_strtod
static double strtod(const char *__u_s, char **__u_end) {
    double __u_v; int __u_sign; __u_word __u_i; int __u_any;
    __u_word __u_e; int __u_esign;
    __u_v = 0.0; __u_sign = 1; __u_i = 0; __u_any = 0;
    while (__u_s[__u_i] == 32 || (__u_s[__u_i] >= 9 && __u_s[__u_i] <= 13)) __u_i = __u_i + 1;
    if (__u_s[__u_i] == 45) { __u_sign = 0 - 1; __u_i = __u_i + 1; }
    else { if (__u_s[__u_i] == 43) __u_i = __u_i + 1; }
    /* C99 hexadecimal form: 0x hex-digits [. hex-digits] [p [+-] decimal] */
    if (__u_s[__u_i] == 48 && (__u_s[__u_i + 1] | 32) == 120) {
        __u_word __u_k; int __u_d; __u_word __u_be; int __u_hany;
        __u_k = __u_i + 2; __u_hany = 0; __u_be = 0;
        while (1) {
            __u_d = __u_s[__u_k];
            if (__u_d >= 48 && __u_d <= 57) __u_d = __u_d - 48;
            else { if ((__u_d | 32) >= 97 && (__u_d | 32) <= 102) __u_d = (__u_d | 32) - 87; else break; }
            if (__u_v < 72057594037927936.0) __u_v = __u_v * 16.0 + (double)__u_d; else __u_be = __u_be + 4;
            __u_k = __u_k + 1; __u_hany = 1;
        }
        if (__u_s[__u_k] == 46) {
            __u_k = __u_k + 1;
            while (1) {
                __u_d = __u_s[__u_k];
                if (__u_d >= 48 && __u_d <= 57) __u_d = __u_d - 48;
                else { if ((__u_d | 32) >= 97 && (__u_d | 32) <= 102) __u_d = (__u_d | 32) - 87; else break; }
                if (__u_v < 72057594037927936.0) { __u_v = __u_v * 16.0 + (double)__u_d; __u_be = __u_be - 4; }
                __u_k = __u_k + 1; __u_hany = 1;
            }
        }
        if (__u_hany == 0) { if (__u_end) *__u_end = (char *)(__u_s + __u_i + 1); return 0.0 * (double)__u_sign; }
        if ((__u_s[__u_k] | 32) == 112) {
            __u_word __u_j; __u_j = __u_k + 1; __u_esign = 1;
            if (__u_s[__u_j] == 45) { __u_esign = 0 - 1; __u_j = __u_j + 1; }
            else { if (__u_s[__u_j] == 43) __u_j = __u_j + 1; }
            if (__u_s[__u_j] >= 48 && __u_s[__u_j] <= 57) {
                __u_e = 0;
                while (__u_s[__u_j] >= 48 && __u_s[__u_j] <= 57) { if (__u_e < 100000) __u_e = __u_e * 10 + (__u_s[__u_j] - 48); __u_j = __u_j + 1; }
                __u_k = __u_j; __u_be = __u_be + __u_esign * __u_e;
            }
        }
        if (__u_v != 0.0) {
            while (__u_be > 0) { __u_v = __u_v * 2.0; __u_be = __u_be - 1; }
            while (__u_be < 0) { __u_v = __u_v / 2.0; __u_be = __u_be + 1; if (__u_v == 0.0) break; }
        }
        if (__u_end) *__u_end = (char *)(__u_s + __u_k);
        return __u_v * (double)__u_sign;
    }
    /* Up to 19 significant digits are kept exactly in an integer and scaled
     * once by a power of ten; with at most 2^53 and |power| <= 22 that is a
     * single rounding (Clinger's fast path), so 2^63 and 10^i come out exact. */
    {   __u_uword __u_m; __u_word __u_x10; double __u_p; __u_word __u_q;
        __u_word __u_ds; __u_word __u_de; __u_word __u_fd; __u_word __u_ex; int __u_drop;
        __u_m = 0; __u_x10 = 0; __u_ds = __u_i; __u_fd = 0; __u_ex = 0; __u_drop = 0;
        while (__u_s[__u_i] >= 48 && __u_s[__u_i] <= 57) {
            if (__u_m < 1844674407370955161) __u_m = __u_m * 10 + (__u_uword)(__u_s[__u_i] - 48); else { __u_x10 = __u_x10 + 1; __u_drop = 1; }
            __u_i = __u_i + 1; __u_any = 1;
        }
        if (__u_s[__u_i] == 46) {
            __u_i = __u_i + 1;
            while (__u_s[__u_i] >= 48 && __u_s[__u_i] <= 57) {
                if (__u_m < 1844674407370955161) { __u_m = __u_m * 10 + (__u_uword)(__u_s[__u_i] - 48); __u_x10 = __u_x10 - 1; } else __u_drop = 1;
                __u_i = __u_i + 1; __u_any = 1; __u_fd = __u_fd + 1;
            }
        }
        __u_de = __u_i;
        if (__u_any) { if (__u_s[__u_i] == 101 || __u_s[__u_i] == 69) {
            __u_word __u_j; __u_j = __u_i + 1; __u_esign = 1;
            if (__u_s[__u_j] == 45) { __u_esign = 0 - 1; __u_j = __u_j + 1; }
            else { if (__u_s[__u_j] == 43) __u_j = __u_j + 1; }
            if (__u_s[__u_j] >= 48 && __u_s[__u_j] <= 57) {
                __u_e = 0;
                while (__u_s[__u_j] >= 48 && __u_s[__u_j] <= 57) { if (__u_e < 100000) __u_e = __u_e * 10 + (__u_s[__u_j] - 48); __u_j = __u_j + 1; }
                __u_i = __u_j; __u_x10 = __u_x10 + __u_esign * __u_e; __u_ex = __u_esign * __u_e;
            }
        } }
        while (__u_m != 0 && __u_x10 < 0 && __u_m % 10 == 0) { __u_m = __u_m / 10; __u_x10 = __u_x10 + 1; }
        __u_drop = __u_drop || __u_m > 9007199254740992 || __u_x10 > 22 || __u_x10 < 0 - 22;
        if (__u_m >> 63) __u_v = (double)(__u_word)(__u_m >> 1) * 2.0 + (double)(__u_word)(__u_m & 1); else __u_v = (double)(__u_word)__u_m;
        /* decimal magnitude of the leading digit: past 10^309 the value overflows, below 10^-325
           it underflows to zero -- decided here, because the big-integer walk below only moves a
           few ulp and its integers do not reach such exponents */
        if (__u_m != 0) {
            __u_uword __u_t; __u_word __u_mag; __u_t = __u_m; __u_mag = __u_x10 - 1;
            while (__u_t) { __u_t = __u_t / 10; __u_mag = __u_mag + 1; }
            if (__u_mag > 309) { __u_m = 0; __u_v = 1.0e300 * 1.0e300; __u_drop = 0; }
            else { if (__u_mag < 0 - 325) { __u_m = 0; __u_v = 0.0; __u_drop = 0; errno = ERANGE; } }
        }
        if (__u_m != 0) {
            while (__u_x10 > 22) { __u_v = __u_v * 1.0e22; __u_x10 = __u_x10 - 22; }
            while (__u_x10 < 0 - 22) { __u_v = __u_v / 1.0e22; __u_x10 = __u_x10 + 22; }
            __u_p = 1.0; __u_q = __u_x10 < 0 ? 0 - __u_x10 : __u_x10;
            while (__u_q > 0) { __u_p = __u_p * 10.0; __u_q = __u_q - 1; }
            if (__u_x10 > 0) __u_v = __u_v * __u_p; else { if (__u_x10 < 0) __u_v = __u_v / __u_p; }
        }
        if (__u_m != 0 && __u_drop) {
            if (__u_v > 1.7976931348623157e308) __u_v = 1.7976931348623157e308;
            __u_v = _u_sd_fix(__u_s + __u_ds, __u_de - __u_ds, __u_ex - __u_fd, __u_v);
        }
        /* C99 7.20.1.3p10: overflow, and underflow (zero or subnormal from a nonzero decimal), set ERANGE */
        if (__u_v > 1.7976931348623157e308) errno = ERANGE;
        else { if (__u_m != 0 && __u_v < 2.2250738585072014e-308) errno = ERANGE; }
    }
    if (__u_end) *__u_end = (char *)(__u_s + (__u_any ? __u_i : 0));
    return __u_v * (double)__u_sign;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_strtof
static float strtof(const char *__u_s, char **__u_end) { return (float)strtod(__u_s, __u_end); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_strtold
static double strtold(const char *__u_s, char **__u_end) { return strtod(__u_s, __u_end); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_atof
static double atof(const char *__u_s) { return strtod(__u_s, 0); }
#endif

static __u_word _unisa_seed = 1;
#if !__UNISA_FTRIM_LIBC || __UN_rand
static int rand(void) {
    _unisa_seed = _unisa_seed * 1103515245 + 12345;
    return (int)((_unisa_seed >> 16) & 32767);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_srand
static void srand(int __u_s) { _unisa_seed = __u_s; }
#endif
/* ---- C99 7.20.6-7.20.7 and the pieces of 7.20.4 a program can rely on
   here: labs, div, qsort, bsearch, atexit.  Real code, like the rest of
   this header: the sort is a heap sort, so its worst case is bounded and
   it needs no stack beyond one element's bytes [S-15 D2]. */
#if !__UNISA_FTRIM_LIBC || __UN_labs
static long labs(long __u_v) { if (__u_v < 0) return 0 - __u_v; return __u_v; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_llabs
static long long llabs(long long __u_v) { if (__u_v < 0) return 0 - __u_v; return __u_v; }
#endif
typedef struct { int quot; int rem; } div_t;
typedef struct { long quot; long rem; } ldiv_t;
typedef struct { long long quot; long long rem; } lldiv_t;
#if !__UNISA_FTRIM_LIBC || __UN_div
static div_t div(int __u_a, int __u_b) { div_t __u_r; __u_r.quot = __u_a / __u_b; __u_r.rem = __u_a % __u_b; return __u_r; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_ldiv
static ldiv_t ldiv(long __u_a, long __u_b) { ldiv_t __u_r; __u_r.quot = __u_a / __u_b; __u_r.rem = __u_a % __u_b; return __u_r; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_lldiv
static lldiv_t lldiv(long long __u_a, long long __u_b) { lldiv_t __u_r; __u_r.quot = __u_a / __u_b; __u_r.rem = __u_a % __u_b; return __u_r; }
#endif

#if !__UNISA_FTRIM_LIBC || __UN__unisa_swap
static void _unisa_swap(char *__u_a, char *__u_b, __u_word __u_n) {
    __u_word __u_i; char __u_t;
    __u_i = 0;
    while (__u_i < __u_n) { __u_t = __u_a[__u_i]; __u_a[__u_i] = __u_b[__u_i]; __u_b[__u_i] = __u_t; __u_i = __u_i + 1; }
}
#endif

/* Sift element `i` of a heap of `n` elements down to its place. */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sift
static void _unisa_sift(char *__u_base, __u_word __u_n, __u_word __u_sz, __u_word __u_i, int (*__u_cmp)(const void *, const void *)) {
    __u_word __u_c;
    while (1) {
        __u_c = 2 * __u_i + 1;
        if (__u_c >= __u_n) return;
        if (__u_c + 1 < __u_n) { if (__u_cmp(__u_base + __u_c * __u_sz, __u_base + (__u_c + 1) * __u_sz) < 0) __u_c = __u_c + 1; }
        if (__u_cmp(__u_base + __u_i * __u_sz, __u_base + __u_c * __u_sz) >= 0) return;
        _unisa_swap(__u_base + __u_i * __u_sz, __u_base + __u_c * __u_sz, __u_sz);
        __u_i = __u_c;
    }
}
#endif

#if !__UNISA_FTRIM_LIBC || __UN_qsort
static void qsort(void *__u_v, __u_word __u_n, __u_word __u_sz, int (*__u_cmp)(const void *, const void *)) {
    char *__u_base; __u_word __u_i;
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
#endif

#if !__UNISA_FTRIM_LIBC || __UN_bsearch
static void *bsearch(const void *__u_key, const void *__u_v, __u_word __u_n, __u_word __u_sz, int (*__u_cmp)(const void *, const void *)) {
    char *__u_base; __u_word __u_lo; __u_word __u_hi; __u_word __u_mid; int __u_c;
    __u_base = (char *)__u_v; __u_lo = 0; __u_hi = __u_n;
    while (__u_lo < __u_hi) {
        __u_mid = __u_lo + (__u_hi - __u_lo) / 2;
        __u_c = __u_cmp(__u_key, __u_base + __u_mid * __u_sz);
        if (__u_c == 0) return __u_base + __u_mid * __u_sz;
        if (__u_c < 0) __u_hi = __u_mid; else __u_lo = __u_mid + 1;
    }
    return 0;
}
#endif

/* atexit: the handlers run, last registered first, from `exit` -- which is
   also where a return from main ends up, since the entry stub calls `exit`
   whenever this header defined one.  Thirty-two is C's minimum (7.20.4.2). */
#if !__UNISA_FTRIM_LIBC || __UN_atexit
static int atexit(void (*__u_fn)(void)) {
    if (_unisa_natexit >= 32) return 0 - 1;
    _unisa_atexit[_unisa_natexit] = __u_fn;
    _unisa_natexit = _unisa_natexit + 1;
    return 0;
}
#endif

/* The environment is where a Unix kernel leaves it: after argv's NULL.
   `__argv(k)` reads that array itself, so the walk starts past argc.  It is
   copied once into environ (below), and environ is the one store: getenv
   reads it, setenv/unsetenv edit it in place (POSIX), and every child --
   execv/execvp/execl*, system, popen -- is handed it, so a variable set
   here is what a child sees (it was a 16-name override table of 255 bytes
   each that only getenv consulted, until 0.0.43). [S-15 D2] */
#ifdef _WIN32
#endif
/* retired with the override table (0.0.43); kept unreferenced so a program that calls no
   environment function keeps its bytes -- delete together with the next bss change */
static char _unisa_envk[16][256];
static int _unisa_envf[16];
/* 0.0.31 H1'': the process environment, one `char **` for the whole program: external linkage,
   a tentative definition in every unit, merged at link (C99 6.9.2).  <unistd.h> names it `environ`.
   A constructor fills it before main from the block the kernel left after argv (0.0.28 F1);
   -ftrim-libc keeps it as a root (unisa/libneed.py ROOTS). */
char **_unisa_env;
static char *_unisa_envv[4096];
#if !__UNISA_FTRIM_LIBC || __UN__unisa_environ_init
__attribute__((constructor)) static void _unisa_environ_init(void) {
    int __u_k; int __u_n;
    if (_unisa_env) return;                 /* another unit's constructor already filled it */
    __u_k = __argc() + 1; __u_n = 0;
    while (__u_n < 4095 && __argv(__u_k) != 0) { _unisa_envv[__u_n] = __argv(__u_k); __u_n = __u_n + 1; __u_k = __u_k + 1; }
    _unisa_envv[__u_n] = 0;
    _unisa_env = _unisa_envv;
}
#endif
/* Windows reads the real environment through the host channel in 0.0.22,
   when the product lowers it (the compiler itself must not need it yet). */
/* the index in environ of the entry "NAME=...", or -1 (NAME holds no '=') */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_envfind
static __u_word _unisa_envfind(const char *__u_name) {
    __u_word __u_k; int __u_i; char *__u_e;
    if (!_unisa_env) _unisa_environ_init();   /* called from a constructor that ran first */
    for (__u_k = 0; (__u_e = _unisa_env[__u_k]) != 0; __u_k++) {
        __u_i = 0;
        while (__u_name[__u_i] && __u_e[__u_i] == __u_name[__u_i]) __u_i = __u_i + 1;
        if (__u_name[__u_i] == 0 && __u_e[__u_i] == 61) return __u_k;
    }
    return -1;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_getenv
static char *getenv(const char *__u_name) {
    __u_word __u_k; int __u_i;
    __u_k = _unisa_envfind(__u_name);
    if (__u_k < 0) return 0;
    __u_i = 0; while (_unisa_env[__u_k][__u_i] != 61) __u_i = __u_i + 1;
    return _unisa_env[__u_k] + __u_i + 1;
}
#endif
/* setenv: "NAME=VALUE" in one malloc'd string, any length, put in environ in place of
   NAME's entry or appended.  An entry is never freed: a pointer getenv returned stays
   valid, as glibc and the BSDs keep them.  Appending past the kernel's copy (or past an
   array the program assigned to environ) moves environ to a malloc'd array twice as big. */
#if !__UNISA_FTRIM_LIBC || __UN_setenv
static char **_unisa_envheap; static __u_word _unisa_envcap;
#endif
#if !__UNISA_FTRIM_LIBC || __UN_setenv
static int setenv(const char *__u_name, const char *__u_val, int __u_over) {
    __u_word __u_k; __u_word __u_n; __u_word __u_i; __u_word __u_j; char *__u_s; char **__u_a;
    if (!__u_name || !__u_name[0]) { errno = EINVAL; return -1; }
    for (__u_i = 0; __u_name[__u_i]; __u_i++) if (__u_name[__u_i] == 61) { errno = EINVAL; return -1; }
    __u_k = _unisa_envfind(__u_name);
    if (__u_k >= 0 && !__u_over) return 0;
    __u_j = 0; while (__u_val[__u_j]) __u_j = __u_j + 1;
    __u_s = (char *)malloc(__u_i + __u_j + 2);
    if (!__u_s) { errno = ENOMEM; return -1; }
    for (__u_j = 0; __u_name[__u_j]; __u_j++) __u_s[__u_j] = __u_name[__u_j];
    __u_s[__u_i] = 61;
    for (__u_j = 0; __u_val[__u_j]; __u_j++) __u_s[__u_i + 1 + __u_j] = __u_val[__u_j];
    __u_s[__u_i + 1 + __u_j] = 0;
    if (__u_k >= 0) { _unisa_env[__u_k] = __u_s; return 0; }
    __u_n = 0; while (_unisa_env[__u_n]) __u_n = __u_n + 1;
    if (!((_unisa_env == _unisa_envv && __u_n + 1 < 4096) || (_unisa_env == _unisa_envheap && __u_n + 1 < _unisa_envcap))) {
        __u_a = (char **)malloc((2 * __u_n + 16) * sizeof(char *));
        if (!__u_a) { free(__u_s); errno = ENOMEM; return -1; }
        for (__u_k = 0; __u_k < __u_n; __u_k++) __u_a[__u_k] = _unisa_env[__u_k];
        if (_unisa_env == _unisa_envheap) free(_unisa_envheap);
        _unisa_envheap = __u_a; _unisa_envcap = 2 * __u_n + 16; _unisa_env = __u_a;
    }
    _unisa_env[__u_n] = __u_s; _unisa_env[__u_n + 1] = 0;
    return 0;
}
#endif
/* unsetenv: every "NAME=..." entry leaves environ, the rest keep their order */
#if !__UNISA_FTRIM_LIBC || __UN_unsetenv
static int unsetenv(const char *__u_name) {
    __u_word __u_k; __u_word __u_m; int __u_i;
    if (!__u_name || !__u_name[0]) { errno = EINVAL; return -1; }
    for (__u_i = 0; __u_name[__u_i]; __u_i++) if (__u_name[__u_i] == 61) { errno = EINVAL; return -1; }
    while ((__u_k = _unisa_envfind(__u_name)) >= 0) {
        __u_m = __u_k; while (_unisa_env[__u_m]) { _unisa_env[__u_m] = _unisa_env[__u_m + 1]; __u_m = __u_m + 1; }
    }
    return 0;
}
#endif

/* system: /bin/sh -c CMD in a child, waited for; the kernel's calls, no
   host library (0.0.21: the compiler must build without the host channel).
   Windows: not yet (0.0.22, through the host channel). */
#if !defined(_WIN32) && !defined(__UNISA_PYFRONT)   /* the Python front end keeps every body and folds one tape across targets */
#if !__UNISA_FTRIM_LIBC || __UN_system
static int system(const char *__u_cmd) {
    char **__u_env; char *__u_argv[4]; long __u_pid; long __u_me; int __u_st;
    if (__u_cmd == 0) return 1;
    __u_argv[0] = "sh"; __u_argv[1] = "-c"; __u_argv[2] = (char *)__u_cmd; __u_argv[3] = 0;
    if (!_unisa_env) _unisa_environ_init();   /* the child gets environ as it stands now */
    __u_env = _unisa_env;
#ifdef __APPLE__
#ifdef __x86_64__
    __u_me = __syscall6(0x2000000L + 20, 0, 0, 0, 0, 0);
    __u_pid = __syscall6(0x2000000L + 2, 0, 0, 0, 0, 0);
    if (__u_pid >= 0 && __syscall6(0x2000000L + 20, 0, 0, 0, 0, 0) != __u_me) __u_pid = 0;
#else
    __u_me = __syscall6(20, 0, 0, 0, 0, 0);
    __u_pid = __syscall6(2, 0, 0, 0, 0, 0);
    if (__u_pid >= 0 && __syscall6(20, 0, 0, 0, 0, 0) != __u_me) __u_pid = 0;
#endif
#elif defined(__x86_64__)
    __u_me = 0; __u_pid = __syscall6(57, 0, 0, 0, 0, 0);
#else
    __u_me = 0; __u_pid = __syscall6(220, 17, 0, 0, 0, 0);
#endif
    if (__u_pid < 0) return -1;
    if (__u_pid == 0) {
#if defined(__APPLE__) && defined(__x86_64__)
        __syscall6(0x2000000L + 59, (long)"/bin/sh", (long)__u_argv, (long)__u_env, 0, 0);
#elif defined(__APPLE__) || defined(__x86_64__)
        __syscall6(59, (long)"/bin/sh", (long)__u_argv, (long)__u_env, 0, 0);
#else
        __syscall6(221, (long)"/bin/sh", (long)__u_argv, (long)__u_env, 0, 0);
#endif
        __exit(127);
    }
    __u_st = 0;
#if defined(__APPLE__) && defined(__x86_64__)
    if (__syscall6(0x2000000L + 7, __u_pid, (long)&__u_st, 0, 0, 0) < 0) return -1;
#elif defined(__APPLE__)
    if (__syscall6(7, __u_pid, (long)&__u_st, 0, 0, 0) < 0) return -1;
#elif defined(__x86_64__)
    if (__syscall6(61, __u_pid, (long)&__u_st, 0, 0, 0) < 0) return -1;
#else
    if (__syscall6(260, __u_pid, (long)&__u_st, 0, 0, 0) < 0) return -1;
#endif
    return __u_st;
}
#endif
#endif
/* mbtowc/wctomb/mbstowcs/wcstombs (C99 7.20.7, 7.20.8) in the C locale,
 * where the multibyte encoding is one byte per character: a byte below 128
 * is the character with the same value, a byte of 128 or more is an invalid
 * multibyte character (EILSEQ), and no shift state exists.  Return values
 * and errno follow the host's C-locale behaviour exactly; a wide character
 * outside 0..127 has no multibyte form.  No function touches errno on
 * success.  mbtowc with n == 0: the empty string still converts to 0, any
 * other string is "incomplete" and gives -1 without errno. */
#if !__UNISA_FTRIM_LIBC || __UN_mbtowc
static int mbtowc(wchar_t *__u_pwc, const char *__u_s, size_t __u_n) {
    unsigned char __u_c;
    if (__u_s == 0) return 0;
    if (__u_s[0] == 0) { if (__u_pwc) *__u_pwc = 0; return 0; }
    if (__u_n == 0) return -1;
    __u_c = (unsigned char)__u_s[0];
    if (__u_c >= 128) { errno = EILSEQ; return -1; }
    if (__u_pwc) *__u_pwc = (wchar_t)__u_c;
    return 1;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_wctomb
static int wctomb(char *__u_s, wchar_t __u_wc) {
    if (__u_s == 0) return 0;
    if (__u_wc < 0 || __u_wc > 127) { errno = EILSEQ; return -1; }
    __u_s[0] = (char)__u_wc;
    return 1;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_mbstowcs
static size_t mbstowcs(wchar_t *__u_dst, const char *__u_src, size_t __u_n) {
    size_t __u_i; unsigned char __u_c;
    __u_i = 0;
    for (;;) {
        if (__u_dst && __u_i >= __u_n) return __u_i;
        __u_c = (unsigned char)__u_src[__u_i];
        if (__u_c >= 128) { errno = EILSEQ; return (size_t)-1; }
        if (__u_dst) __u_dst[__u_i] = (wchar_t)__u_c;
        if (__u_c == 0) return __u_i;
        __u_i = __u_i + 1;
    }
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_wcstombs
static size_t wcstombs(char *__u_dst, const wchar_t *__u_src, size_t __u_n) {
    size_t __u_i; wchar_t __u_wc;
    __u_i = 0;
    for (;;) {
        if (__u_dst && __u_i >= __u_n) return __u_i;
        __u_wc = __u_src[__u_i];
        if (__u_wc < 0 || __u_wc > 127) { errno = EILSEQ; return (size_t)-1; }
        if (__u_dst) __u_dst[__u_i] = (char)__u_wc;
        if (__u_wc == 0) return __u_i;
        __u_i = __u_i + 1;
    }
}
#endif
/* 0.0.34 L2 (SQLite's Apple path): forwarded to the system C library */
#ifdef __APPLE__
long random(void);
void srandomdev(void);
#endif
#endif
