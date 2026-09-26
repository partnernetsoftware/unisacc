double g(long v, char c, short s, unsigned char uc) { double d; double sc; sc = 0.1; d = 2.0; d = d * 10.0 + v; d = d + v * sc; d = c - d; d = d * s; d = uc / d; return d; }
int main() { g(3, 4, 5, 6); return 0; }
