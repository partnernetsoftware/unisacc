/* unsigned long: every binary operator's unsigned spelling (binsel u) when either operand is unsigned long; compound assignment likewise */
long f(unsigned long v, int b, long s) {
  int d; unsigned long w;
  d = v % b; v = v / b; w = v;
  d = v < w; d = v > 3; d = v <= s; d = v >= w; d = v == 0; d = v != s;
  w = v >> 3; w = v << 2; w = v + b; w = v - s; w = v * b; w = v & 7; w = v | 7; w = v ^ 7;
  w = s % v; w = s / v; w = -v; w = ~v; w = !v;
  w += 3; w /= 3; w %= b; w >>= 1; w++; ++w;
  if (v) d = 1;
  return v;
}
int main() { return 0; }
