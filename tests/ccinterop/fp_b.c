/* cc interop slice 2B/2C, half B (compiled by cc, no libc) */
double dadd(double a, double b) { return a + b; }
float fmulf(float a, float b) { return a * b; }
double mixd(int a, double b, long c, float d, char *e, double f) { return a * b + c + d + e[1] + f; }
float tofl(double x) { return (float)x; }
double fromfl(float x) { return x; }
long dtol(double x, int k) { return (long)x * k; }
int fcmp(float a, double b) { return a == b; }
double nine(double a, double b, double c, double d, double e, double f, double g, double h, double i, double j) { return a + 2 * b + 3 * c + 4 * d + 5 * e + 6 * f + 7 * g + 8 * h + 9 * i + 10 * j; }
long sum8(long a, long b, long c, long d, long e, long f, long g, long h) { return a + b + c + d + e + f + g * 10 + h * 100; }
long sum10(long a, int b, long c, int d, long e, long f, long g, long h, int i, long j) { return a + b + c + d + e + f + g + h * 3 + i * 7 + j; }
double mix10(int a, double b, long c, float d, int e, double f, long g, long h, long i, long j) { return a + b * 2 + c * 3 + d * 4 + e * 5 + f * 6 + g * 7 + h * 8 + i * 9 + j * 10; }
long ptr8(char *a, long b, long c, long d, long e, long f, long g, char *h) { return (h - a) * 1000 + b + c + d + e + f + g + a[0]; }
