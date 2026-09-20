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
int main() {
    printf("%d %d %d %ld\n", addup(3, 1, 2, 3), addup(1, 42),
           addup(5, 1, 1, 1, 1, 1), addl(2, 100000000000L, 1L));
    return 0;
}
