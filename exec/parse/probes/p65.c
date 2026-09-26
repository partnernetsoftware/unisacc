double g(double *p, float *q, int v, char c) { double d; double sc; d = 0.0; d = d * 10.0 + v; sc = 0.1; d = d + v * sc; sc = sc * 0.1; d = d / 10.0; d = 0.0 - d; d = d - c + 1.5; *p = d; *q = (float) d; sc = 123.456 * 1.; sc = 0.3 + 2.5 + 0.7 + 100000000.0 + 0.000001 + 3.14159265358979 + 9007199254740993.0; return d; }
int main() { double d; float *f; f = 0; g(&d, f, 3, 7); return 0; }
