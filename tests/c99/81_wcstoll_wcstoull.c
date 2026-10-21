/* C99 7.24.4.1.2 wcstoll and wcstoull, declared in <wchar.h> as prototypes and
 * forwarded to the host's C library: sign and leading space, base 10 with an
 * end pointer, base 16, base 0 with a 0x prefix, and the overflow clamp to
 * LLONG_MAX (the product's <limits.h> defines LLONG_MAX as LONG_MAX, which is
 * the host's value on 64-bit Linux).  The exit code is the number of failed
 * checks, 0 when every check passes.  Nothing is printed. */
#include <wchar.h>
#include <limits.h>
#include <stddef.h>
int main(void)
{
    wchar_t *e;
    const wchar_t *s = L"  -123xyz";
    int bad = 0;
    bad += !(wcstoll(s, &e, 10) == -123 && *e == L'x' && e == s + 6);
    bad += !(wcstoull(L"ff", NULL, 16) == 255);
    bad += !(wcstoll(L"0x1f", NULL, 0) == 31);
    bad += !(wcstoll(L"99999999999999999999", NULL, 10) == LLONG_MAX);
    bad += !(wcstoull(L"18446744073709551615", NULL, 10) == ULLONG_MAX);
    bad += !(sizeof(long long) == 8);
    return bad;
}
