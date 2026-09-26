short h(short a, char b){ short s; s = a + b; return s; }
long k(long x){ return x; }
char c(char *p){ return *p; }
int main(){ short t; t = h(1, 2); return k(t) + c(0); }
