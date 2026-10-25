/* <math.h>: fdlibm's algorithms, within one ulp -- printed at ten digits,
   where a correct result and the platform libm's agree. */
#include <math.h>
int main(void) {
    double v[8] = {0.5, 1.0, 2.0, 3.0, 10.0, 0.1, 12.34, 0.7853981633974483};
    int i;
    for (i = 0; i < 8; i++) {
        double x = v[i];
        printf("%.10g %.10g %.10g %.10g %.10g %.10g\n", sin(x), cos(x), tan(x), exp(x), log(x), log10(x));
        printf("%.10g %.10g %.10g %.10g %.10g %.10g\n", atan(x), atan2(x, -1.5), sqrt(x), pow(x, 2.5), pow(-x, 3), sinh(x));
        printf("%.10g %.10g %.10g %.10g %.10g %.10g\n", cosh(x), tanh(x), floor(-x), ceil(-x), round(x * 1.5), fmod(x * 7.3, 1.1));
    }
    printf("%f %f %f %f\n", sin(2), exp(1), pow(2, 10), sqrt(2));
    printf("%.10g %.10g %.10g\n", asin(0.5), acos(0.5), hypot(3, 4));
    return 0;
}
