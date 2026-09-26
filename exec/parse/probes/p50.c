/* unsigned long through pointers, calls, casts, ++/--, !, shifts by an unsigned count (s >> v is lshr64) */
long g(long x) { return x; }
long f(unsigned long v, int b, long s, unsigned long *up) {
  int d; unsigned long w; d = 7;
  w = (v + 1) / 2; w = -v / 2; w = ~v >> 1; w = s >> v; w = b << v;
  d %= v; d <<= v; d /= b; s >>= 2; w -= s;
  w = *up / 3; w = up[1] % 7; w = up[b] >> 1; *up = w; up[2] = v;
  w = g(v) / 2; w = (long)v / 2; w = (v, s) / 2;
  w = v++ / 2; w = --v >> 1; w = (v && s) / 2;
  if (v < 0) d = 1;
  while (v) { v = v / b; }
  return (!v) / 2 + w + d;
}
int main() { return 0; }
