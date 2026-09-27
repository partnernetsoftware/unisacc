/* Known product defect: block prototype drops parameter conversions.
   Host cc prints 3.5; private product reference at 4a84274 prints 0.5.
   Kept outside the equal list until signature handling is repaired. */
#include <stdio.h>
int main(void) { double f(double); printf("%.1f\n", f(3)); return 0; }
double f(double x) { return x + 0.5; }
