/* <inttypes.h> for the unisa C subset: the printf/scanf length prefixes
 * C99 added, on top of <stdint.h>'s types.
 *
 * On every target this compiler has, `long` is 64 bits and `int` is 32, so
 * the 64-bit prefix is "l" and the 32-bit one is empty -- the same choice
 * an LP64 platform's own header makes.
 * LLP64 Windows (_WIN64) is the exception: long is 32 bits there, the 64-bit
 * types are long long, and their prefix is "ll".
 */
#ifndef _UNISA_INTTYPES_H
#define _UNISA_INTTYPES_H
#include <stdint.h>
#include <stdlib.h>   /* strtol and strtoul, which strtoimax and strtoumax wrap */
#include <wchar.h>    /* wcstol and wcstoul, which wcstoimax and wcstoumax wrap */
#include <sys/_word.h>

#define PRId8   "d"
#define PRId16  "d"
#define PRId32  "d"
#if defined(_WIN64)
#define PRId64  "lld"
#else
#define PRId64  "ld"
#endif
#define PRIi8   "i"
#define PRIi16  "i"
#define PRIi32  "i"
#if defined(_WIN64)
#define PRIi64  "lli"
#else
#define PRIi64  "li"
#endif
#define PRIu8   "u"
#define PRIu16  "u"
#define PRIu32  "u"
#if defined(_WIN64)
#define PRIu64  "llu"
#else
#define PRIu64  "lu"
#endif
#define PRIx8   "x"
#define PRIx16  "x"
#define PRIx32  "x"
#if defined(_WIN64)
#define PRIx64  "llx"
#define PRIX64  "llX"
#define PRIo64  "llo"
#define PRIdPTR "lld"
#define PRIuPTR "llu"
#define PRIxPTR "llx"
#define PRIdMAX "lld"
#define PRIuMAX "llu"
#else
#define PRIx64  "lx"
#define PRIX64  "lX"
#define PRIo64  "lo"
#define PRIdPTR "ld"
#define PRIuPTR "lu"
#define PRIxPTR "lx"
#define PRIdMAX "ld"
#define PRIuMAX "lu"
#endif

#define SCNd32  "d"
#if defined(_WIN64)
#define SCNd64  "lld"
#else
#define SCNd64  "ld"
#endif
#define SCNu32  "u"
#if defined(_WIN64)
#define SCNu64  "llu"
#define SCNx64  "llx"
#else
#define SCNu64  "lu"
#define SCNx64  "lx"
#endif

typedef __u_word intmax_t;
typedef __u_uword uintmax_t;
typedef struct { __u_word quot; __u_word rem; } imaxdiv_t;
#if !__UNISA_FTRIM_LIBC || __UN_imaxabs
static __u_word imaxabs(__u_word __u_v) { if (__u_v < 0) return 0 - __u_v; return __u_v; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_imaxdiv
static imaxdiv_t imaxdiv(__u_word __u_a, __u_word __u_b) { imaxdiv_t __u_r; __u_r.quot = __u_a / __u_b; __u_r.rem = __u_a % __u_b; return __u_r; }
#endif
/* intmax_t is long here, so the C99 7.8.2 conversions are strtol/strtoul exactly.
   On LLP64 Windows long is 32 bits, so the 64-bit value is converted from the shared
   magnitude routine directly (the same one strtol/wcstol already use). */
#if !__UNISA_FTRIM_LIBC || __UN_strtoimax
static __u_word strtoimax(const char *__u_s, char **__u_end, int __u_base) {
#if defined(_WIN64)
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
#if !__UNISA_FTRIM_LIBC || __UN_strtoumax
static __u_uword strtoumax(const char *__u_s, char **__u_end, int __u_base) {
#if defined(_WIN64)
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
#if !__UNISA_FTRIM_LIBC || __UN_wcstoimax
static __u_word wcstoimax(const wchar_t *__u_s, wchar_t **__u_end, int __u_base) {
#if defined(_WIN64)
    __u_word __u_hi; __u_word __u_lo; __u_word __u_v; int __u_neg; int __u_over;
    __u_over = _unisa_wcsmag(__u_s, __u_end, __u_base, &__u_hi, &__u_lo, &__u_neg);
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
    return wcstol(__u_s, __u_end, __u_base);
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_wcstoumax
static __u_uword wcstoumax(const wchar_t *__u_s, wchar_t **__u_end, int __u_base) {
#if defined(_WIN64)
    __u_word __u_hi; __u_word __u_lo; __u_uword __u_v; int __u_neg; int __u_over;
    __u_over = _unisa_wcsmag(__u_s, __u_end, __u_base, &__u_hi, &__u_lo, &__u_neg);
    if (__u_over) { errno = ERANGE; return (__u_uword)0 - 1; }
    __u_v = ((__u_uword)__u_hi << 32) | (__u_uword)__u_lo;
    if (__u_neg) return (__u_uword)0 - __u_v;
    return __u_v;
#else
    return wcstoul(__u_s, __u_end, __u_base);
#endif
}
#endif
#endif
