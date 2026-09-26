/* calls through function pointers: a parameter T (*f)(...), f(x) and (*f)(x); a function name as a value */
int add(int a, int b) { return a + b; }
int ap(int (*f)(int, int), int x) { return f(x, 2) + (*f)(3, x); }
int main() { return ap(add, 5); }
