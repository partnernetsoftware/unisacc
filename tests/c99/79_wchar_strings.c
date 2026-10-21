/* C99 7.24.4 wide string functions with bodies in <wchar.h>, on ASCII inputs:
 * wcscpy wcsncpy wmemcpy wmemmove (7.24.4.2), wcscat wcsncat (7.24.4.3),
 * wcscmp wcsncmp wmemcmp (7.24.4.4), wcschr wcscspn wcspbrk wcsrchr wcsspn
 * wcsstr wmemchr (7.24.4.5), wmemset (7.24.4.6), and wcstol wcstoul
 * (7.24.4.1.2; wcstoll/wcstoull are not provided).  The exit code is the
 * number of failed checks, 0 when every check passes.  Nothing is printed. */
#include <wchar.h>
#include <stddef.h>
int main(void)
{
    wchar_t a[32], b[32], *e;
    const wchar_t *s = L"hello world";
    int bad = 0;
    wmemset(a, L'x', 32);
    bad += !(wcscpy(a, s) == a && wcscmp(a, s) == 0 && a[11] == L'\0' && a[12] == L'x');
    wmemset(b, L'x', 32);
    bad += !(wcsncpy(b, L"ab", 5) == b && b[0] == L'a' && b[1] == L'b' && b[2] == L'\0' && b[4] == L'\0' && b[5] == L'x');
    bad += !(wcsncpy(b, s, 3) == b && b[2] == L'l' && b[3] == L'\0');
    bad += !(wmemcpy(b, s, 12) == b && wcscmp(b, s) == 0);
    bad += !(wmemmove(b + 2, b, 5) == b + 2 && wmemcmp(b + 2, L"hello", 5) == 0 && b[0] == L'h' && b[1] == L'e');
    bad += !(wmemmove(b, b + 2, 5) == b && wmemcmp(b, L"hello", 5) == 0);
    wcscpy(a, L"foo");
    bad += !(wcscat(a, L"bar") == a && wcscmp(a, L"foobar") == 0);
    bad += !(wcsncat(a, L"bazqux", 3) == a && wcscmp(a, L"foobarbaz") == 0 && wcslen(a) == 9);
    bad += !(wcscmp(L"abc", L"abc") == 0 && wcscmp(L"abc", L"abd") < 0 && wcscmp(L"abd", L"abc") > 0
             && wcscmp(L"ab", L"abc") < 0 && wcscmp(L"", L"") == 0);
    bad += !(wcsncmp(L"abcX", L"abcY", 3) == 0 && wcsncmp(L"abcX", L"abcY", 4) < 0 && wcsncmp(L"a", L"b", 0) == 0);
    bad += !(wmemcmp(L"ab\0c", L"ab\0d", 3) == 0 && wmemcmp(L"ab\0c", L"ab\0d", 4) < 0 && wmemcmp(L"b", L"a", 1) > 0);
    bad += !(wcschr(s, L'o') == s + 4 && wcschr(s, L'\0') == s + 11 && wcschr(s, L'z') == NULL);
    bad += !(wcsrchr(s, L'o') == s + 7 && wcsrchr(s, L'h') == s && wcsrchr(s, L'z') == NULL);
    bad += !(wcsstr(s, L"o w") == s + 4 && wcsstr(s, L"") == s && wcsstr(s, L"world!") == NULL && wcsstr(s, L"hello world") == s);
    bad += !(wcsspn(s, L"leh") == 4 && wcsspn(s, L"z") == 0 && wcscspn(s, L"w") == 6 && wcscspn(s, L"z") == 11);
    bad += !(wcspbrk(s, L"wo") == s + 4 && wcspbrk(s, L"xyz") == NULL);
    bad += !(wmemchr(s, L'w', 11) == s + 6 && wmemchr(s, L'w', 6) == NULL && wmemchr(s, L'\0', 12) == s + 11);
    bad += !(wmemset(a, L'q', 4) == a && a[3] == L'q' && a[4] == L'a');   /* a[4] still from "foobarbaz" */
    bad += !(wcstol(L"  -123xyz", &e, 10) == -123 && *e == L'x' && wcstol(L"0x1f", NULL, 16) == 31
             && wcstol(L"0x1f", NULL, 0) == 31 && wcstol(L"101", NULL, 2) == 5);
    bad += !(wcstoul(L"4294967295", &e, 10) == 4294967295UL && *e == L'\0' && wcstoul(L"zz", &e, 10) == 0 && e != NULL
             && wcstoul(L"077", NULL, 0) == 63);
    return bad;
}
