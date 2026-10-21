/* C99 7.21.4.5 strxfrm, 7.24.4.4.2 wcscoll, 7.24.4.4.4 wcsxfrm, in the "C" locale
 * (the only locale a program starts in): the transformation is the identity and
 * collation orders like strcmp/wcscmp.  The exit code is 0 when all checks pass;
 * nothing is printed. */
#include <string.h>
#include <wchar.h>
#include <stddef.h>
int main(void)
{
    char buf[16];
    wchar_t wbuf[16];
    const char *s = "apple";
    const wchar_t *ws = L"apple", *wt = L"apricot";
    volatile size_t cap = sizeof buf;
    int sum = 0;
    memset(buf, 'x', sizeof buf);
    sum += (strxfrm(buf, s, cap) == 5 && strcmp(buf, "apple") == 0 && buf[5] == '\0') * 1;
    sum += (strxfrm(buf, "", cap) == 0 && buf[0] == '\0') * 2;
    sum += (strxfrm(NULL, s, 0) == 5 && strxfrm(buf, "banana", 3) == 6) * 4;   /* n too small: only the length */
    sum += (wcscoll(ws, ws) == 0 && wcscoll(ws, wt) < 0 && wcscoll(wt, ws) > 0
            && wcscoll(L"", ws) < 0 && wcscoll(L"b", L"abc") > 0) * 8;
    memset(wbuf, 0x7f, sizeof wbuf);
    sum += (wcsxfrm(wbuf, ws, 16) == 5 && wcscmp(wbuf, L"apple") == 0 && wbuf[5] == L'\0') * 16;
    sum += (wcsxfrm(wbuf, L"", 16) == 0 && wbuf[0] == L'\0' && wcsxfrm(NULL, wt, 0) == 7) * 32;
    return 63 - sum;   /* 0 when every check passes; each set bit names a failed check */
}
