/* a call's value is unsigned iff its last argument was; no arguments: signed */
long g(long x) { return x; }
long h(long x, long y) { return x; }
long g0() { return 1; }
long f(unsigned long v, long s) {
  long w;
  w = g(v) / 2; w = g(s) / 3; w = h(v, s) / 4; w = h(s, v) / 5; w = g(v + 1) / 7; w = g(s + v) / 8;
  w = v; w = g0() / 2;
  v / 3; w = g0() / 4;
  w = (v, g0()) / 5;
  w = v + g0() / 6;
  w = s + g0() / 7;
  w = h(v, g(s)) / 9; w = h(s, g(v)) % 9;
  return w;
}
int main() { return f(100, 7); }
