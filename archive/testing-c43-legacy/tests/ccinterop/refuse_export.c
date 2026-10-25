/* an `extern`-declared definition with a double argument cannot be exported yet */
extern double half(double x);
double half(double x) { return x / 2; }
int main(void) { return (int)half(4.0); }
