/* Fixed arguments beyond slot eight keep their declared conversion kind. */
#include <stdio.h>
#include <stdarg.h>

float weighted(float a, float b, float c, float d, float e,
               float f, float g, float h, float i) {
    return a + 2*b + 3*c + 4*d + 5*e + 6*f + 7*g + 8*h + 9*i;
}
float later(float, float, float, float, float, float, float, float, float);
float later(float, float, float, float, float, float, float, float, float);
double mixed(int a, float b, double c, int *p, float e, int f,
             double g, float h, float i, double j, float k) {
    return a + 2*b + 3*c + *p + 5*e + 6*f + 7*g + 8*h + 9*i + 10*j + 11*k;
}
float other(float a, float b, float c, float d, float e,
            float f, float g, float h, double i) {
    return a + b + c + d + e + f + g + h + i;
}
double tails(float a, float b, float c, float d, float e,
             float f, float g, float h, float i, int tag, ...) {
    va_list ap;
    double x, y;
    va_start(ap, tag);
    x = va_arg(ap, double);
    y = va_arg(ap, double);
    va_end(ap);
    return a + b + c + d + e + f + g + h + i + tag + x + y;
}
int main(void) {
    int bad = 0;
    int p = 4;
    if (weighted(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f) != 285.0f) bad = bad + 1;
    /* Integer/double expressions must also convert to fixed float slots. */
    if (later(1, 2.0, 3, 4.0, 5, 6.0, 7, 8.0, 9.0) != 285.0f) bad = bad + 2;
    if (mixed(1, 2.0, 3.0f, &p, 5.0, 6, 7.0f, 8, 9.0, 10.0f, 11) != 494.0) bad = bad + 4;
    {
        float later(float, float, float, float, float, float, float, float, float);
        if (later(1, 2, 3, 4, 5, 6, 7, 8, 9.0) != 285.0f) bad = bad + 8;
    }
    /* A new block reuses symbol slots with a different ninth kind. */
    {
        float other(float, float, float, float, float, float, float, float, double);
        if (other(1, 2, 3, 4, 5, 6, 7, 8, 9.0f) != 45.0f) bad = bad + 16;
    }
    if (later(1, 2, 3, 4, 5, 6, 7, 8, 9.0) != 285.0f) bad = bad + 32;
    if (tails(1, 2, 3, 4, 5, 6, 7, 8, 9.0, 10, 11.25f, 12.5f) != 78.75) bad = bad + 64;
    printf("fixedfp many %d\n", bad);
    return bad != 0;
}
float later(float a, float b, float c, float d, float e,
            float f, float g, float h, float i) {
    return weighted(a, b, c, d, e, f, g, h, i);
}
