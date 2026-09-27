/* Unary arithmetic types, one evaluation, and the unisacc long-double/F64 alias. */
#include <stdio.h>
struct Holder { long double value; };
long double identity(long double x) { return x; }
int main(void) {
    float f = 1.5f;
    double d = 2.5;
    signed char c = -7;
    unsigned char u = 255;
    short s = -300;
    unsigned short us = 65535;
    _Bool b = 1;
    int n = 0;
    long double ld = 3.25;
    struct Holder h;
    h.value = identity(ld);
    int once = +(n += 1);
    printf("%f %f %d %d %d %d %d %d %d\n", +f, +d, +c, +u, +s, +us, +b, once, n);
    printf("%d %d %d %d %d\n", (int)sizeof(+c), (int)sizeof(+u), (int)sizeof(+s), (int)sizeof(+us), (int)sizeof(+b));
#ifdef __UNISA__
    if (sizeof(long double) != sizeof(double) || sizeof(h) != sizeof(double)) return 1;
#else
    if (sizeof(h) != sizeof(long double)) return 1;
#endif
    printf("%f %f\n", (double)h.value, (double)(long double)4.5);
    return 0;
}
