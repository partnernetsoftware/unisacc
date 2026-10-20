/* wchar.h -- enough of it for wide string literals.
 *
 * `wchar_t` is four bytes on every target we emit.  Windows' own is two, but
 * nothing here calls a -W entry point, and one width is what keeps a wide
 * literal target-independent: `--fold` compares six lowerings of ONE tape,
 * so a literal whose element size depended on the target would break that.
 */
#ifndef _UNISA_WCHAR_H
#define _UNISA_WCHAR_H
#include <stdlib.h>   /* errno, ERANGE, LONG_MIN/LONG_MAX and _unisa_strdigit for wcstol/wcstoul */

#ifndef _UNISA_WCHAR_T
#define _UNISA_WCHAR_T
typedef int wchar_t;
#endif

#ifndef _UNISA_WINT_T
#define _UNISA_WINT_T
typedef unsigned int wint_t;
#endif

/* C99 7.24.1p2: an object type (other than array) holding a conversion state.
 * The host's (glibc __mbstate_t) is a count and a four-byte union: eight bytes. */
typedef struct { int __u_count; unsigned int __u_wch; } mbstate_t;

#define WEOF (0-1)

#if !__UNISA_FTRIM_LIBC || __UN_wcslen
static long wcslen(const wchar_t *__u_s) {
    long __u_n;
    __u_n = 0;
    while (__u_s[__u_n]) __u_n = __u_n + 1;
    return __u_n;
}
#endif

/* wcstol/wcstoul (C99 7.24.4.1.5, 7.24.4.1.6): <stdlib.h>'s strtol/strtoul
 * over wide characters, in the C locale.  White space is the six ASCII
 * characters of isspace(); the sign, the 0x prefix and the digits are the
 * ASCII ones; any other wide character ends the subject sequence.  The
 * magnitude is accumulated in two 32-bit limbs exactly as _unisa_strmag
 * does, so overflow clamps to LONG_MAX/LONG_MIN/ULONG_MAX with ERANGE and
 * still consumes every valid digit.  No digits: *end = nptr and 0. */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_wcsmag
static int _unisa_wcsmag(const wchar_t *__u_s, wchar_t **__u_end, int __u_base,
                         long *__u_hi, long *__u_lo, int *__u_neg) {
    long __u_i; long __u_start; long __u_l; long __u_h; int __u_d; int __u_over; int __u_auto;
    __u_auto = __u_base == 0; __u_i = 0; *__u_hi = 0; *__u_lo = 0; *__u_neg = 0; __u_over = 0;
    if (__u_end) *__u_end = (wchar_t *)__u_s;
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
            __u_d = _unisa_strdigit((int)__u_s[__u_i+2]);
            if (__u_d >= 0 && __u_d < 16) {
                /* Only base 0 or 16 may accept a hex prefix. */
                if (__u_base == 16 || __u_auto) { __u_base = 16; __u_i = __u_i + 2; }
            }
        }
    }
    __u_start = __u_i;
    while (__u_s[__u_i]) {
        __u_d = _unisa_strdigit((int)__u_s[__u_i]);
        if (__u_d < 0 || __u_d >= __u_base) break;
        if (!__u_over) {
            __u_l = *__u_lo * __u_base + __u_d;
            __u_h = *__u_hi * __u_base + __u_l / 4294967296;
            if (__u_h > 4294967295) __u_over = 1;
            else { *__u_hi = __u_h; *__u_lo = __u_l % 4294967296; }
        }
        __u_i = __u_i + 1;
    }
    if (__u_end && __u_i != __u_start) *__u_end = (wchar_t *)(__u_s + __u_i);
    return __u_over;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_wcstol
static long wcstol(const wchar_t *__u_s, wchar_t **__u_end, int __u_base) {
    long __u_hi; long __u_lo; long __u_v; int __u_neg; int __u_over;
    __u_over = _unisa_wcsmag(__u_s, __u_end, __u_base, &__u_hi, &__u_lo, &__u_neg);
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
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_wcstoul
static unsigned long wcstoul(const wchar_t *__u_s, wchar_t **__u_end, int __u_base) {
    long __u_hi; long __u_lo; unsigned long __u_v; int __u_neg; int __u_over;
    __u_over = _unisa_wcsmag(__u_s, __u_end, __u_base, &__u_hi, &__u_lo, &__u_neg);
    if (__u_over) { errno = ERANGE; return (unsigned long)0 - 1; }
    __u_v = ((unsigned long)__u_hi << 32) | (unsigned long)__u_lo;
    if (__u_neg) return (unsigned long)0 - __u_v;
    return __u_v;
}
#endif

#endif
