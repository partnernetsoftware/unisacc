/* C99 7.24.4.5.7 wcstok, 7.24.5.1 wcsftime, 7.24.6.1.1 btowc, 7.24.6.1.2 wctob,
 * 7.24.6.2.1 mbsinit, 7.24.6.3.1 mbrlen, 7.24.6.3.2 mbrtowc, 7.24.6.3.3 wcrtomb,
 * 7.24.6.4.1 mbsrtowcs, 7.24.6.4.2 wcsrtombs, 7.20.7.2 mbtowc, 7.20.7.3 wctomb,
 * 7.20.8.1 mbstowcs, 7.20.8.2 wcstombs, in the C locale (one byte per character).
 * The exit code counts the failed checks (0 when all pass); nothing is printed. */
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
int main(void)
{
    unsigned long sum = 0;
    wchar_t buf[64], *tok, *save, wc;
    char cbuf[64];
    const char *src;
    mbstate_t st;
    struct tm t;
    size_t r;
    /* wcstok */
    wcscpy(buf, L"alpha,beta;;gamma");
    tok = wcstok(buf, L",;", &save);
    sum += (tok && wcscmp(tok, L"alpha") == 0) * 1;
    tok = wcstok(NULL, L",;", &save);
    sum += (tok && wcscmp(tok, L"beta") == 0) * 2;
    tok = wcstok(NULL, L",;", &save);
    sum += (tok && wcscmp(tok, L"gamma") == 0 && wcstok(NULL, L",;", &save) == NULL) * 4;
    /* wcsftime */
    memset(&t, 0, sizeof t);
    t.tm_year = 124; t.tm_mon = 2; t.tm_mday = 5; t.tm_hour = 13; t.tm_min = 7; t.tm_sec = 9; t.tm_wday = 2; t.tm_yday = 64;
    r = wcsftime(buf, 64, L"%Y-%m-%d %H:%M:%S %a %b %j%%", &t);
    sum += (r == 32 && wcscmp(buf, L"2024-03-05 13:07:09 Tue Mar 065%") == 0) * 8;
    sum += (wcsftime(buf, 4, L"%Y-%m-%d", &t) == 0) * 16;
    /* btowc, wctob */
    sum += (btowc('A') == L'A' && btowc(0) == 0 && btowc(EOF) == WEOF && wctob(L'z') == 'z' && wctob(WEOF) == EOF) * 32;
    /* mbsinit, mbrlen, mbrtowc, wcrtomb */
    memset(&st, 0, sizeof st);
    sum += (mbsinit(&st) != 0 && mbsinit(NULL) != 0) * 64;
    sum += (mbrlen("xy", 2, &st) == 1 && mbrlen("", 1, &st) == 0 && mbrlen("q", 0, &st) == (size_t)-2) * 128;
    sum += (mbrtowc(&wc, "Q!", 2, &st) == 1 && wc == L'Q' && mbrtowc(&wc, "", 1, &st) == 0 && wc == 0
            && mbrtowc(NULL, NULL, 0, &st) == 0) * 256;
    errno = 0;
    sum += (mbrtowc(&wc, "\xc3\xa9", 2, &st) == (size_t)-1 && errno == EILSEQ) * 512;
    sum += (wcrtomb(cbuf, L'k', &st) == 1 && cbuf[0] == 'k' && wcrtomb(NULL, L'k', &st) == 1) * 1024;
    /* mbsrtowcs, wcsrtombs */
    src = "hello";
    r = mbsrtowcs(buf, &src, 64, &st);
    sum += (r == 5 && src == NULL && wcscmp(buf, L"hello") == 0) * 2048;
    src = "hello";
    r = mbsrtowcs(buf, &src, 2, &st);
    sum += (r == 2 && src != NULL && strcmp(src, "llo") == 0 && buf[0] == L'h' && buf[1] == L'e') * 4096;
    src = "count me";
    sum += (mbsrtowcs(NULL, &src, 0, &st) == 8 && strcmp(src, "count me") == 0) * 8192;
    {
        const wchar_t *wsrc = L"wide";
        r = wcsrtombs(cbuf, &wsrc, 64, &st);
        sum += (r == 4 && wsrc == NULL && strcmp(cbuf, "wide") == 0) * 16384;
        wsrc = L"wide";
        r = wcsrtombs(cbuf, &wsrc, 3, &st);
        sum += (r == 3 && wsrc != NULL && wcscmp(wsrc, L"e") == 0 && memcmp(cbuf, "wid", 3) == 0) * 32768;
        wsrc = L"abc";
        sum += (wcsrtombs(NULL, &wsrc, 0, &st) == 3 && wcscmp(wsrc, L"abc") == 0) * 65536;
    }
    /* mbtowc, wctomb, mbstowcs, wcstombs (7.20.7, 7.20.8) */
    sum += (mbtowc(&wc, "Zq", 2) == 1 && wc == L'Z' && mbtowc(&wc, "", 1) == 0 && mbtowc(NULL, NULL, 0) == 0) * 131072;
    sum += (wctomb(cbuf, L'm') == 1 && cbuf[0] == 'm') * 262144;
    sum += (mbstowcs(buf, "abcdef", 64) == 6 && wcscmp(buf, L"abcdef") == 0 && mbstowcs(buf, "abcdef", 3) == 3
            && mbstowcs(NULL, "abcdef", 0) == 6) * 524288;
    sum += (wcstombs(cbuf, L"xyz", 64) == 3 && strcmp(cbuf, "xyz") == 0 && wcstombs(NULL, L"xyz", 0) == 3
            && wcstombs(cbuf, L"xyz", 2) == 2) * 1048576;
    /* 21 checks: count the failed ones (0 when every check passes; stays below 128) */
    { int failed = 0; unsigned long m; for (m = 1; m < (1UL << 21); m <<= 1) if (!(sum & m)) failed++; return failed; }
}
