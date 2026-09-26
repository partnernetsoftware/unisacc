int h() { return 7; }
int g(int a, int b, int c, int d, int e, int f, int g2, int h2) { int x = 1, y; { int z = 2; y = z; } { int w; w = 3; } x = a && b || c; x = ~a + +b; if (x) return 1; ; return (a >> 2) % b; }
int main() { int k; k = h(); if (k) { k = 0; } return g(1,2,3,4,5,6,7,8) != 0 >= 1; }
