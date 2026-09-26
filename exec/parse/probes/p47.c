/* unsigned char/short parameters and pointees: stored 64, loaded at size and masked; scaled by size */
int f(unsigned char c, unsigned short s, char d) { return c + s + d; }
int g(unsigned char *p, unsigned short *q, char unsigned k) { *p = 300; *q = 70000; return *p + p[0] + *q + q[0] + k; }
int h(unsigned char *p, unsigned short *q) { return p[1] + q[1]; }
int main() { unsigned char b = 1; unsigned short w = 2; return f(300, 70000, 5) + g(&b, &w, 2); }
