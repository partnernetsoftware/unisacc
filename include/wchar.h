/* wchar.h -- enough of it for wide string literals.
 *
 * `wchar_t` is four bytes on every target we emit.  Windows' own is two, but
 * nothing here calls a -W entry point, and one width is what keeps a wide
 * literal target-independent: `--fold` compares six lowerings of ONE tape,
 * so a literal whose element size depended on the target would break that.
 */
#ifndef _UNISA_WCHAR_H
#define _UNISA_WCHAR_H

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

#endif
