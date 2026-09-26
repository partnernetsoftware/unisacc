double g(double x) { return x; }
int main(void) { double d; int i; i = 3; d = 0.5; d = d * 10.0 + i; d = i - d; if (d < 2.0) i = 1; if (d > i) i = 2; i = (int)d; d = (double)i; d = g(i); return i; }
