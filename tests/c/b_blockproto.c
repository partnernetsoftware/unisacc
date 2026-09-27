/* Block prototypes must keep parameter conversions without allocating locals. */
#include <stdio.h>
int main(void) {
    double f(double), g(float);
    int h(_Bool);
    double a = f(3);
    { double f(double x); a = a + f(4); }
    printf("%.1f %.1f %d\n", a, g(2), h(7));
    return 0;
}
double f(double x) { return x + 0.5; }
double g(float x) { return x + 0.25; }
int h(_Bool x) { return x; }
