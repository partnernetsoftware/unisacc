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
#include <time.h>     /* struct tm and strftime for wcsftime */

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

/* Wide memory (C99 7.24.4.6) and wide string (7.24.4.1-7.24.4.5) functions:
 * <string.h>'s loops over wchar_t, no locale, no allocation.  The three
 * comparisons compare elements as wchar_t (int, as the host's glibc does)
 * and return -1, 0 or 1, the sign the host returns. */
#if !__UNISA_FTRIM_LIBC || __UN_wmemcpy
static wchar_t *wmemcpy(wchar_t *__u_d, const wchar_t *__u_s, long __u_n) {
    long __u_i;
    __u_i = 0;
    while (__u_i < __u_n) { __u_d[__u_i] = __u_s[__u_i]; __u_i = __u_i + 1; }
    return __u_d;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_wmemmove
static wchar_t *wmemmove(wchar_t *__u_d, const wchar_t *__u_s, long __u_n) {
    long __u_i;
    if (__u_d < __u_s) {
        __u_i = 0;
        while (__u_i < __u_n) { __u_d[__u_i] = __u_s[__u_i]; __u_i = __u_i + 1; }
        return __u_d;
    }
    __u_i = __u_n;
    while (__u_i > 0) { __u_i = __u_i - 1; __u_d[__u_i] = __u_s[__u_i]; }
    return __u_d;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_wmemcmp
static int wmemcmp(const wchar_t *__u_a, const wchar_t *__u_b, long __u_n) {
    long __u_i;
    __u_i = 0;
    while (__u_i < __u_n) {
        if (__u_a[__u_i] < __u_b[__u_i]) return -1;
        if (__u_a[__u_i] > __u_b[__u_i]) return 1;
        __u_i = __u_i + 1;
    }
    return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_wmemchr
static wchar_t *wmemchr(const wchar_t *__u_s, wchar_t __u_c, long __u_n) {
    long __u_i;
    __u_i = 0;
    while (__u_i < __u_n) {
        if (__u_s[__u_i] == __u_c) return (wchar_t *)(__u_s + __u_i);
        __u_i = __u_i + 1;
    }
    return NULL;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_wmemset
static wchar_t *wmemset(wchar_t *__u_s, wchar_t __u_c, long __u_n) {
    long __u_i;
    __u_i = 0;
    while (__u_i < __u_n) { __u_s[__u_i] = __u_c; __u_i = __u_i + 1; }
    return __u_s;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_wcscpy
static wchar_t *wcscpy(wchar_t *__u_d, const wchar_t *__u_s) {
    long __u_i;
    __u_i = 0;
    while (__u_s[__u_i]) { __u_d[__u_i] = __u_s[__u_i]; __u_i = __u_i + 1; }
    __u_d[__u_i] = 0;
    return __u_d;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_wcsncpy
static wchar_t *wcsncpy(wchar_t *__u_d, const wchar_t *__u_s, long __u_n) {
    long __u_i;
    __u_i = 0;
    while (__u_i < __u_n && __u_s[__u_i]) { __u_d[__u_i] = __u_s[__u_i]; __u_i = __u_i + 1; }
    while (__u_i < __u_n) { __u_d[__u_i] = 0; __u_i = __u_i + 1; }
    return __u_d;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_wcscat
static wchar_t *wcscat(wchar_t *__u_d, const wchar_t *__u_s) {
    long __u_i; long __u_j;
    __u_i = 0; while (__u_d[__u_i]) __u_i = __u_i + 1;
    __u_j = 0;
    while (__u_s[__u_j]) { __u_d[__u_i + __u_j] = __u_s[__u_j]; __u_j = __u_j + 1; }
    __u_d[__u_i + __u_j] = 0;
    return __u_d;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_wcsncat
static wchar_t *wcsncat(wchar_t *__u_d, const wchar_t *__u_s, long __u_n) {
    long __u_i; long __u_j;
    __u_i = 0; while (__u_d[__u_i]) __u_i = __u_i + 1;
    __u_j = 0;
    while (__u_j < __u_n) { if (__u_s[__u_j] == 0) break; __u_d[__u_i + __u_j] = __u_s[__u_j]; __u_j = __u_j + 1; }
    __u_d[__u_i + __u_j] = 0;
    return __u_d;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_wcscmp
static int wcscmp(const wchar_t *__u_a, const wchar_t *__u_b) {
    long __u_i;
    __u_i = 0;
    while (__u_a[__u_i] == __u_b[__u_i]) {
        if (__u_a[__u_i] == 0) return 0;
        __u_i = __u_i + 1;
    }
    if (__u_a[__u_i] < __u_b[__u_i]) return -1;
    return 1;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_wcsncmp
static int wcsncmp(const wchar_t *__u_a, const wchar_t *__u_b, long __u_n) {
    long __u_i;
    __u_i = 0;
    while (__u_i < __u_n) {
        if (__u_a[__u_i] != __u_b[__u_i]) {
            if (__u_a[__u_i] < __u_b[__u_i]) return -1;
            return 1;
        }
        if (__u_a[__u_i] == 0) return 0;
        __u_i = __u_i + 1;
    }
    return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_wcschr
static wchar_t *wcschr(const wchar_t *__u_s, wchar_t __u_c) {
    long __u_i;
    __u_i = 0;
    while (__u_s[__u_i] != __u_c) {
        if (__u_s[__u_i] == 0) return NULL;
        __u_i = __u_i + 1;
    }
    return (wchar_t *)(__u_s + __u_i);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_wcsrchr
static wchar_t *wcsrchr(const wchar_t *__u_s, wchar_t __u_c) {
    long __u_i; long __u_last;
    __u_i = 0; __u_last = -1;
    while (1) {
        if (__u_s[__u_i] == __u_c) __u_last = __u_i;
        if (__u_s[__u_i] == 0) break;
        __u_i = __u_i + 1;
    }
    if (__u_last < 0) return NULL;
    return (wchar_t *)(__u_s + __u_last);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_wcsstr
static wchar_t *wcsstr(const wchar_t *__u_h, const wchar_t *__u_n) {
    long __u_i; long __u_j;
    __u_i = 0;
    while (1) {
        __u_j = 0;
        while (__u_n[__u_j]) { if (__u_h[__u_i + __u_j] != __u_n[__u_j]) break; __u_j = __u_j + 1; }
        if (__u_n[__u_j] == 0) return (wchar_t *)(__u_h + __u_i);
        if (__u_h[__u_i] == 0) return NULL;
        __u_i = __u_i + 1;
    }
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_wcsspn
static long wcsspn(const wchar_t *__u_s, const wchar_t *__u_accept) {
    long __u_i; long __u_j;
    __u_i = 0;
    while (__u_s[__u_i]) {
        __u_j = 0;
        while (__u_accept[__u_j] && __u_accept[__u_j] != __u_s[__u_i]) __u_j = __u_j + 1;
        if (__u_accept[__u_j] == 0) return __u_i;
        __u_i = __u_i + 1;
    }
    return __u_i;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_wcscspn
static long wcscspn(const wchar_t *__u_s, const wchar_t *__u_reject) {
    long __u_i; long __u_j;
    __u_i = 0;
    while (__u_s[__u_i]) {
        __u_j = 0;
        while (__u_reject[__u_j]) {
            if (__u_reject[__u_j] == __u_s[__u_i]) return __u_i;
            __u_j = __u_j + 1;
        }
        __u_i = __u_i + 1;
    }
    return __u_i;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_wcspbrk
static wchar_t *wcspbrk(const wchar_t *__u_s, const wchar_t *__u_accept) {
    long __u_i; long __u_j;
    __u_i = 0;
    while (__u_s[__u_i]) {
        __u_j = 0;
        while (__u_accept[__u_j]) {
            if (__u_s[__u_i] == __u_accept[__u_j]) return (wchar_t *)(__u_s + __u_i);
            __u_j = __u_j + 1;
        }
        __u_i = __u_i + 1;
    }
    return NULL;
}
#endif

/* wcstok (C99 7.24.4.5.7): the wide form of strtok, except that the standard
 * gives it a third argument: the caller's own save pointer, so the place is
 * kept in *ptr between calls, not in a static.  A null s continues from *ptr.
 * Leading delimiters are skipped; the token is terminated with a null wide
 * character; after the last token *ptr is set to NULL (as strtok's static
 * and glibc's wcstok do) so the next call returns NULL. */
#if !__UNISA_FTRIM_LIBC || __UN_wcstok
static wchar_t *wcstok(wchar_t *__u_s, const wchar_t *__u_delim, wchar_t **__u_ptr) {
    wchar_t *__u_start; long __u_k; int __u_isd;
    if (__u_s == 0) __u_s = *__u_ptr;
    if (__u_s == 0) return NULL;
    /* skip leading delimiters */
    while (*__u_s) {
        __u_isd = 0; __u_k = 0;
        while (__u_delim[__u_k]) { if (__u_delim[__u_k] == *__u_s) __u_isd = 1; __u_k = __u_k + 1; }
        if (__u_isd == 0) break;
        __u_s = __u_s + 1;
    }
    if (*__u_s == 0) { *__u_ptr = 0; return NULL; }
    __u_start = __u_s;
    while (*__u_s) {
        __u_isd = 0; __u_k = 0;
        while (__u_delim[__u_k]) { if (__u_delim[__u_k] == *__u_s) __u_isd = 1; __u_k = __u_k + 1; }
        if (__u_isd) { *__u_s = 0; *__u_ptr = __u_s + 1; return __u_start; }
        __u_s = __u_s + 1;
    }
    *__u_ptr = 0;
    return __u_start;
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

/* wcsftime (C99 7.24.5.1): strftime over wide characters, in the C locale.
 * The wide format is narrowed into a malloc'd string; a wide character
 * outside 0..127 is not a C-locale format, so 0 is returned and s is left
 * alone (locale-specific formats are out of scope).  strftime then formats
 * into the first maxsize bytes of s itself, and that result -- every byte
 * ASCII in the C locale -- is widened in place from its end (wide element k
 * occupies bytes 4k..4k+3, past every narrow byte still to be read), so no
 * second buffer is needed.  Did not fit (narrow length >= maxsize): 0, and
 * the array's contents are indeterminate, as the standard allows. */
#if !__UNISA_FTRIM_LIBC || __UN_wcsftime
static size_t wcsftime(wchar_t *__u_s, size_t __u_max, const wchar_t *__u_f, const struct tm *__u_tm) {
    char *__u_nf; char *__u_b; size_t __u_n; size_t __u_k;
    __u_n = 0;
    while (__u_f[__u_n]) { if (__u_f[__u_n] < 0 || __u_f[__u_n] > 127) return 0; __u_n = __u_n + 1; }
    __u_nf = (char *)malloc((long)__u_n + 1);
    if (__u_nf == 0) return 0;
    __u_k = 0;
    while (__u_k <= __u_n) { __u_nf[__u_k] = (char)__u_f[__u_k]; __u_k = __u_k + 1; }
    __u_b = (char *)__u_s;
    __u_n = strftime(__u_b, __u_max, __u_nf, __u_tm);
    free(__u_nf);
    if (__u_n == 0) { if (__u_max) __u_s[0] = 0; return 0; }
    __u_s[__u_n] = 0;
    __u_k = __u_n;
    while (__u_k > 0) { __u_k = __u_k - 1; __u_s[__u_k] = (wchar_t)__u_b[__u_k]; }
    return __u_n;
}
#endif

/* Restartable multibyte/wide conversion (C99 7.24.6) in the C locale: one
 * byte per character, a byte below 128 is the character with the same value,
 * a byte of 128 or more is an invalid multibyte character (EILSEQ), a wide
 * character outside 0..127 has no multibyte form.  There is no shift state,
 * so every call is stateless; the mbstate_t is never written, and mbsinit
 * reports the host's rule (a null pointer or a zero count is initial).
 * Return values and errno follow the host's C-locale behaviour exactly;
 * errno is not touched on success.  wcrtomb with a null s stores into no
 * buffer and reports 1 for every wc, as the host does. */
#if !__UNISA_FTRIM_LIBC || __UN_mbsinit
static int mbsinit(const mbstate_t *__u_ps) {
    return __u_ps == 0 || __u_ps->__u_count == 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_btowc
static wint_t btowc(int __u_c) {
    if (__u_c < 0 || __u_c > 127) return WEOF;
    return (wint_t)__u_c;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_wctob
static int wctob(wint_t __u_c) {
    if (__u_c > 127) return -1;
    return (int)__u_c;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_mbrtowc
static size_t mbrtowc(wchar_t *__u_pwc, const char *__u_s, size_t __u_n, mbstate_t *__u_ps) {
    unsigned char __u_c;
    if (__u_s == 0) return 0;
    if (__u_n == 0) return (size_t)-2;
    __u_c = (unsigned char)__u_s[0];
    if (__u_c >= 128) { errno = EILSEQ; return (size_t)-1; }
    if (__u_pwc) *__u_pwc = (wchar_t)__u_c;
    return __u_c == 0 ? 0 : 1;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_mbrlen
static size_t mbrlen(const char *__u_s, size_t __u_n, mbstate_t *__u_ps) {
    unsigned char __u_c;
    if (__u_s == 0) return 0;
    if (__u_n == 0) return (size_t)-2;
    __u_c = (unsigned char)__u_s[0];
    if (__u_c >= 128) { errno = EILSEQ; return (size_t)-1; }
    return __u_c == 0 ? 0 : 1;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_wcrtomb
static size_t wcrtomb(char *__u_s, wchar_t __u_wc, mbstate_t *__u_ps) {
    if (__u_s == 0) return 1;
    if (__u_wc < 0 || __u_wc > 127) { errno = EILSEQ; return (size_t)-1; }
    __u_s[0] = (char)__u_wc;
    return 1;
}
#endif
/* mbsrtowcs/wcsrtombs: with a null dst the whole source is counted (len is
 * ignored) and *src is left alone, even on EILSEQ.  With a dst, up to len
 * elements are stored; *src becomes null once the terminating null has been
 * stored, otherwise the address just past the last element converted, or on
 * EILSEQ the address of the offending character (the elements before it are
 * already stored). */
#if !__UNISA_FTRIM_LIBC || __UN_mbsrtowcs
static size_t mbsrtowcs(wchar_t *__u_dst, const char **__u_src, size_t __u_len, mbstate_t *__u_ps) {
    const char *__u_s; size_t __u_i; unsigned char __u_c;
    __u_s = *__u_src; __u_i = 0;
    for (;;) {
        if (__u_dst && __u_i >= __u_len) { *__u_src = __u_s + __u_i; return __u_i; }
        __u_c = (unsigned char)__u_s[__u_i];
        if (__u_c >= 128) { if (__u_dst) *__u_src = __u_s + __u_i; errno = EILSEQ; return (size_t)-1; }
        if (__u_dst) __u_dst[__u_i] = (wchar_t)__u_c;
        if (__u_c == 0) { if (__u_dst) *__u_src = 0; return __u_i; }
        __u_i = __u_i + 1;
    }
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_wcsrtombs
static size_t wcsrtombs(char *__u_dst, const wchar_t **__u_src, size_t __u_len, mbstate_t *__u_ps) {
    const wchar_t *__u_s; size_t __u_i; wchar_t __u_wc;
    __u_s = *__u_src; __u_i = 0;
    for (;;) {
        if (__u_dst && __u_i >= __u_len) { *__u_src = __u_s + __u_i; return __u_i; }
        __u_wc = __u_s[__u_i];
        if (__u_wc < 0 || __u_wc > 127) { if (__u_dst) *__u_src = __u_s + __u_i; errno = EILSEQ; return (size_t)-1; }
        if (__u_dst) __u_dst[__u_i] = (char)__u_wc;
        if (__u_wc == 0) { if (__u_dst) *__u_src = 0; return __u_i; }
        __u_i = __u_i + 1;
    }
}
#endif

#endif
