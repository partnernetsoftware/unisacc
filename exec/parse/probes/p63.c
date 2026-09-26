typedef char *va_list;
long f(double x, int n, ...) {
  double d; double e; unsigned long b; va_list ap;
  d = x; e = d;
  va_start(ap, n);
  d = va_arg(ap, double);
  b = *(unsigned long *)&d;
  return b;
}
double g(double y) { double z; z = y; return z; }
int main() { double q; double *pq; pq = &q; q = *pq; *pq = q; return 0; }
