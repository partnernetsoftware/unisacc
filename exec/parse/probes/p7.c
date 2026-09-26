int g(int a, int b, int c, int d, int e, int f) { int x = 1, y; { int z = 2; y = z; { int a = 5; x = a; } } { int w; w = 3; } x = a && b || c && !d; x = ~a + +b - -c; if (x) return 1; ; return (a >> 2) % b / c * d << e >> f < a > b <= c >= d == e != f & a ^ b | c; }
int main() { int k; k = g(1, 2, 3, 4, 5, 6); if (k) { k = 0; } return g(k, k = 2, 3, 4, 5, 6) != 0 >= 1; }
