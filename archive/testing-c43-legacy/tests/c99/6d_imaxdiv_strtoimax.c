/* C99 7.8.2.2 imaxdiv, 7.8.2.3 strtoimax/strtoumax, 7.8.2.4 wcstoimax/wcstoumax.
 * The exit code counts the failed checks (0 when all pass); nothing is printed. */
#include <inttypes.h>
#include <stddef.h>
#include <wchar.h>
int main(void)
{
    unsigned long sum = 0;
    imaxdiv_t d = imaxdiv((intmax_t)-1000000007, (intmax_t)13);
    char *end; wchar_t *wend;
    intmax_t a, b; uintmax_t u, wu;
    sum += (unsigned long)(d.quot == -76923077 && d.rem == -6) * 1;
    a = strtoimax("  -123456789012345", &end, 10);
    sum += (unsigned long)(a == -123456789012345 && *end == 0) * 2;
    b = strtoimax("0x7fffZ", &end, 16);
    sum += (unsigned long)(b == 32767 && *end == 'Z') * 4;
    u = strtoumax("18446744073709551615", &end, 10);
    sum += (unsigned long)(u == 18446744073709551615UL && *end == 0) * 8;
    u = strtoumax("0777q", &end, 0);
    sum += (unsigned long)(u == 511 && *end == 'q') * 16;
    a = wcstoimax(L"-987654321", &wend, 10);
    sum += (unsigned long)(a == -987654321 && *wend == 0) * 32;
    wu = wcstoumax(L"ff/", &wend, 16);
    sum += (unsigned long)(wu == 255 && *wend == L'/') * 64;
    a = wcstoimax(L"  +42abc", &wend, 36);
    sum += (unsigned long)(a == 4L * 36 * 36 * 36 * 36 + 2L * 36 * 36 * 36 + 10 * 36 * 36 + 11 * 36 + 12 && *wend == 0) * 128;
    return (int)(255 - (sum & 255));   /* 0 when every check passes; each set bit names a failed check */
}
