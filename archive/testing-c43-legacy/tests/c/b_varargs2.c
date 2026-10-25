/* variadic definitions: all arguments on the stack, va_list is a pointer */
#include <stdarg.h>
int addup(int n, ...) {
    va_list ap;
    int i;
    int s;
    va_start(ap, n);
    s = 0;
    for (i = 0; i < n; i = i + 1) s = s + va_arg(ap, int);
    va_end(ap);
    return s;
}
long addl(int n, ...) {
    va_list ap;
    int i;
    long s;
    va_start(ap, n);
    s = 0;
    for (i = 0; i < n; i = i + 1) s = s + va_arg(ap, long);
    va_end(ap);
    return s;
}
int (*vp)(int, ...) = &addup;      /* called through a pointer, still stacked */
int fred(int p) { return p + 1; }
int (*fp)(int) = &fred;
int main() {
    printf("%d %d %d %ld\n", addup(3, 1, 2, 3), addup(1, 42),
           addup(5, 1, 1, 1, 1, 1), addl(2, 100000000000L, 1L));
    printf("%d %d %d\n", vp(3, 1, 2, 3), fp(1), (*fp)(2));
    return 0;
}
