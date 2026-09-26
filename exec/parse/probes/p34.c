long f(long a, short b, char c){ long x; x = a * b - c; return x; }
char g(char c){ return c + 1; }
short h(short *p){ return *p; }
int main(){ short s; char d; long *q; s = 300; d = g(66); s += d; return f(3, h(&s), d) != 0; }
