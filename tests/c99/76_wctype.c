/* C99 7.25 <wctype.h>: the twelve iswXXX classifiers, towlower/towupper,
 * wctype/iswctype and wctrans/towctrans, forwarded to the host C library and
 * measured on ASCII code points in the "C" locale (the only locale a program
 * starts in).  The exit code is 0 when every check passes; each set bit names
 * a failed check (the exit code is the sum of the failed checks' weights, kept
 * below 256).  Nothing is printed. */
#include <wctype.h>
int main(void)
{
    int sum = 0;
    sum += (iswalpha(L'a') && iswalpha(L'Z') && !iswalpha(L'1') && !iswalpha(L' ')) * 1;
    sum += (iswdigit(L'7') && !iswdigit(L'a') && iswxdigit(L'f') && iswxdigit(L'A') && !iswxdigit(L'g')) * 2;
    sum += (iswalnum(L'q') && iswalnum(L'9') && !iswalnum(L'_') && !iswalnum(L'.')) * 4;
    sum += (iswupper(L'Q') && !iswupper(L'q') && iswlower(L'q') && !iswlower(L'Q') && !iswlower(L'5')) * 8;
    sum += (iswspace(L' ') && iswspace(L'\n') && iswspace(L'\t') && !iswspace(L'x')
            && iswblank(L' ') && iswblank(L'\t') && !iswblank(L'\n')) * 16;
    sum += (iswcntrl(L'\n') && iswcntrl(L'\x7f') && !iswcntrl(L'a')
            && iswprint(L'a') && iswprint(L' ') && !iswprint(L'\n')
            && iswgraph(L'a') && iswgraph(L'!') && !iswgraph(L' ')
            && iswpunct(L'!') && iswpunct(L'_') && !iswpunct(L'a') && !iswpunct(L' ')) * 32;
    sum += (towupper(L'a') == L'A' && towupper(L'A') == L'A' && towupper(L'1') == L'1'
            && towlower(L'A') == L'a' && towlower(L'a') == L'a' && towlower(L'.') == L'.') * 64;
    sum += (wctype("alpha") != 0 && iswctype(L'a', wctype("alpha")) && !iswctype(L'1', wctype("alpha"))
            && iswctype(L'1', wctype("digit")) && !iswctype(L'a', wctype("digit"))
            && iswctype(L' ', wctype("space")) && wctype("no such class") == 0
            && wctrans("toupper") != 0 && towctrans(L'a', wctrans("toupper")) == L'A'
            && towctrans(L'A', wctrans("tolower")) == L'a' && towctrans(L'1', wctrans("tolower")) == L'1'
            && wctrans("no such mapping") == 0) * 128;
    return 255 - sum;
}
