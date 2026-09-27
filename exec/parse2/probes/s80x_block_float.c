/* Regression: block prototypes must preserve parameter conversions.
   Host cc prints 3.5; the pre-fix product at 4a84274 printed 0.5. */
#include <stdio.h>
int main(void) { double f(double); printf("%.1f\n", f(3)); return 0; }
double f(double x) { return x + 0.5; }
