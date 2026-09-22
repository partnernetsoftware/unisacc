/* Floating point [TP]: arithmetic, comparison, conversion at assignment,
   argument, return and initialisation (C99 6.3.1.4-8), in float and double,
   with statics folded at compile time exactly as the program computes. */
int run1(void) {
    double a = 1.5, b = 2.25, c;
    float f = 3.0f, g;
    int i = 7, k;
    c = a * b + i;              /* 10.375 */
    k = (int)(c * 4);           /* 41 */
    g = f / 2 + a;              /* 3.0 */
    printf("%d %d %d\n", k, (int)g, (int)(a < b) + (int)(b <= a) * 10 + (a != b) * 100);
    if (c > 10.0) printf("big ");
    if (!(0.0)) printf("zero-is-false ");
    k = -a < 0 ? 1 : 2;
    printf("%d %d\n", k, (int)(i ? 2.5 * 2 : 0));
    c = 0; c += 1.25; c *= 4; c -= 0.5; c /= 3;
    printf("%d %d\n", (int)(c * 1000), (int)(-c * 1000));
    f = 1; f++; ++f; f--;
    printf("%d\n", (int)(f * 100));
    return 0;
}
double gd = 100.0;
double gsum = 12.34 + 56.78;
float gf = 2.5f;
int gi = 3.9;
double garr[3] = {1.5, 2, -0.25};
struct P { double x; float y; int n; };
struct P gp = {1.25, 0.5f, 3};
double half(double v) { return v / 2; }
float twice(float v) { return v * 2; }
int trunc_it(double v) { return v; }
double sum3(double a, int b, float c) { return a + b + c; }
int run2(void) {
    double d = 7;
    float f = d / 2;
    struct P lp = {2.5, 1.5f, 4};
    double arr[2] = {0.1, 0.2};
    int i = 0;
    printf("%d %d %d %d\n", (int)gd, (int)(gsum * 100), (int)(gf * 10), gi);
    printf("%d %d %d\n", (int)(garr[0] * 4), (int)garr[1], (int)(garr[2] * 100));
    printf("%d %d %d\n", (int)(gp.x * 100), (int)(gp.y * 10), gp.n);
    printf("%d %d %d\n", (int)(lp.x * 10), (int)(lp.y * 10), (int)(f * 10));
    printf("%d %d %d %d\n", (int)(half(9) * 10), (int)(twice(1.25f) * 100), trunc_it(-3.7), (int)(sum3(1.5, 2, 0.25f) * 100));
    arr[1] += arr[0];
    printf("%d %d\n", (int)(arr[1] * 1000000), (int)(sizeof(float) * 10 + sizeof(double)));
    for (d = 0; d < 1; d += 0.25) i++;
    printf("%d\n", i);
    return 0;
}
int main(void) { run1(); run2(); return 0; }
