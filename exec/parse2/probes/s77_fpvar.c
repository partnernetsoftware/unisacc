#include <stdio.h>
#include <stdarg.h>
int sum(int n, ...) {
    va_list ap; int i; int s = 0;
    va_start(ap, n);
    for (i = 0; i < n; i++) s += va_arg(ap, int);
    va_end(ap); return s;
}
typedef int (*VF)(int, ...);
int one(int x) { return x + 1; }
int eight(int a,int b,int c,int d,int e,int f,int g,int h) {
    return a+b+c+d+e+f+g+h;
}
int apply(int (*f)(int, ...)) { return f(3, 4, 5, 6); }
int main(void) {
    VF f = sum;
    int (*many)(int,int,int,int,int,int,int,int) = eight;
    int a = f(3, 1, 2, 3);
    int b = (*f)(2, 10, f(1, 5));
    int c;
    { int (*f)(int) = one; c = f(8); }
    printf("%d %d %d %d %d %d\n", a, b, c, f(0), apply(f), many(1,2,3,4,5,6,7,8));
    return 0;
}
