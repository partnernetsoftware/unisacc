/* Macros named x, y, z, k, t, r, w before <math.h>. */
#include <stdio.h>
#define x 31
#define y 32
#define z 33
#define k 34
#define t 35
#define r 36
#define w 37
#include <math.h>
int main(void) {
    int e;
    double fr = frexp(48.0, &e);
    printf("%.6f %.6f %.6f\n", sqrt(2.0), pow(2.0, 10.0), fabs(-1.5));
    printf("%.6f %.6f %.6f\n", sin(1.0), cos(1.0), atan2(1.0, 2.0));
    printf("%.6f %.6f %.6f\n", exp(1.0), log(10.0), floor(-2.5));
    printf("%.4f %d %.1f\n", fr, e, ldexp(0.75, 4));
    printf("%d %d %d %d %d %d %d\n", x, y, z, k, t, r, w);
    return 0;
}
