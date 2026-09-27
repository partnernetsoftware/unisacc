/* Exact decimal conversion, token classification, f32 and unordered !=. */
int main(void) {
    union { double d; unsigned long u; } x;
    union { float f; unsigned int u; } y;
    double a;
    x.d = 1.00000000000000011102230246251565404236316680908203126;
    if (x.u != 0x3ff0000000000001UL) return 1;
    x.d = 4.9406564584124654e-324;
    if (x.u != 1) return 2;
    x.d = 1e309;
    if (x.u != 0x7ff0000000000000UL) return 3;
    y.f = 1.000000059604644775390625f;
    if (y.u != 0x3f800000U) return 4;
    y.f = 1.000000059604644775390626f;
    if (y.u != 0x3f800001U) return 5;
    x.d = 009.5;
    if (x.d != 9.5) return 6;
    a = .5;
    if (a != 0.5E+0) return 7;
    x.u = 0x7ff8000000000001UL;
    if (!(x.d != x.d)) return 8;
    if (0x1e != 30 || 'e' != 101) return 9;
    return 0;
}
