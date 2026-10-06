/* 0.0.30 H3: wctype.h and fenv.h forward to the system C library */
#include <stdio.h>
#include <wctype.h>
#include <fenv.h>
int main(void) {
    volatile double one = 1.0, three = 3.0, zero = 0.0, q;
    int r0, r1, up, dn;
    printf("alpha %d %d digit %d space %d upper %d lower %c%c\n", iswalpha('a') != 0, iswalpha('1') != 0,
           iswdigit('7') != 0, iswspace(' ') != 0, (int)towupper('q'), (int)towlower('Q'), (int)towlower('z'));
    printf("wctype %d %d\n", iswctype('x', wctype("alpha")) != 0, iswctype('x', wctype("digit")) != 0);
    r0 = fegetround() == FE_TONEAREST;
    fesetround(FE_UPWARD); up = (one / three) > 0.3333333333333333;
    fesetround(FE_DOWNWARD); dn = (one / three) < 0.33333333333333337;
    r1 = fegetround() == FE_DOWNWARD;
    fesetround(FE_TONEAREST);
    feclearexcept(FE_ALL_EXCEPT);
    q = one / zero;
    printf("round %d %d %d %d divbyzero %d inexact %d q>0 %d\n", r0, up, dn, r1,
           fetestexcept(FE_DIVBYZERO) != 0, fetestexcept(FE_INEXACT) != 0, q > 0);
    return 0;
}
