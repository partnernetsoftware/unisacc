long f(void) { return 0x7fffffffffffffff; }
long h(void) { return 0xffffffffffffffff; }
int g(void) { return 0x1F + 017 + 10u + 5L + 7ul + 3ll + 1234567890123 + 0xABcd + 0 + 0u; }
int main(void) { return g() - 0x80000000 + f() + h(); }
