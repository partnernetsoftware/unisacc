/* C99 7.6 <fenv.h>: the eleven functions, forwarded to the host C library.
 * Rounding: fesetround/fegetround round trip through every mode and back to
 * FE_TONEAREST.  Exceptions: feclearexcept/fetestexcept on FE_ALL_EXCEPT,
 * feraiseexcept of one flag, fegetexceptflag/fesetexceptflag round trip.
 * Environment: fegetenv/fesetenv round trip, feholdexcept clears the flags,
 * feupdateenv restores the saved flags.  The exit code is 0 when every check
 * passes; each set bit names a failed check.  Nothing is printed. */
#include <fenv.h>
int main(void)
{
    fenv_t env, held;
    fexcept_t flag;
    int sum = 0;
    sum += (fesetround(FE_TONEAREST) == 0 && fegetround() == FE_TONEAREST) * 1;
    sum += (fesetround(FE_UPWARD) == 0 && fegetround() == FE_UPWARD
            && fesetround(FE_DOWNWARD) == 0 && fegetround() == FE_DOWNWARD
            && fesetround(FE_TOWARDZERO) == 0 && fegetround() == FE_TOWARDZERO
            && fesetround(FE_TONEAREST) == 0 && fegetround() == FE_TONEAREST) * 2;
    sum += (feclearexcept(FE_ALL_EXCEPT) == 0 && fetestexcept(FE_ALL_EXCEPT) == 0) * 4;
    sum += (feraiseexcept(FE_INEXACT) == 0 && fetestexcept(FE_ALL_EXCEPT) == FE_INEXACT
            && fetestexcept(FE_DIVBYZERO) == 0) * 8;
    sum += (fegetexceptflag(&flag, FE_INEXACT) == 0 && feclearexcept(FE_ALL_EXCEPT) == 0
            && fetestexcept(FE_INEXACT) == 0 && fesetexceptflag(&flag, FE_INEXACT) == 0
            && fetestexcept(FE_INEXACT) == FE_INEXACT) * 16;
    sum += (fegetenv(&env) == 0 && feclearexcept(FE_ALL_EXCEPT) == 0 && fetestexcept(FE_INEXACT) == 0
            && fesetenv(&env) == 0 && fetestexcept(FE_INEXACT) == FE_INEXACT) * 32;
    sum += (feholdexcept(&held) == 0 && fetestexcept(FE_ALL_EXCEPT) == 0
            && feraiseexcept(FE_DIVBYZERO) == 0 && feupdateenv(&held) == 0
            && fetestexcept(FE_ALL_EXCEPT) == (FE_INEXACT | FE_DIVBYZERO)) * 64;
    sum += (feclearexcept(FE_ALL_EXCEPT) == 0 && fesetenv(FE_DFL_ENV) == 0
            && fetestexcept(FE_ALL_EXCEPT) == 0 && fegetround() == FE_TONEAREST) * 128;
    return 255 - sum;
}
