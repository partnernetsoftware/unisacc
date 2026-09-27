#include <stdio.h>
int add(int x) { return x + 2; }
int main(void) {
    int f(int), n = 3, g(int (*)(int), int);
    int add(int);
    { int f(int x); n = f(n); }
    printf("%d %d %d\n", n, f(4), g(add, 5));
    return 0;
}
int f(int x) { return x + 10; }
int g(int (*fn)(int), int x) { return fn(x); }
