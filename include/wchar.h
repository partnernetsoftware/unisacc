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

#define WEOF (0-1)

static long wcslen(const wchar_t *s) {
    long n;
    n = 0;
    while (s[n]) n = n + 1;
    return n;
}

#endif
