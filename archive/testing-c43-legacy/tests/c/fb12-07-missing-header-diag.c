/* R13-0b: external trial of 0.0.12, real-world batch.  Source: ~/unisacc-feedback-0.0.12-realworld/07-missing-header-diag.c (+ .out.txt for the gcc-vs-unisacc record). */
/* A missing header (<locale.h> is not bundled; same for any unknown name)
   gives a diagnostic that names neither the header nor the file:line,
   and without -I claims "no include directory was given". */
#include <locale.h>
#include <stdio.h>
int main(void) { printf("%d\n", LC_ALL >= 0); return 0; }
