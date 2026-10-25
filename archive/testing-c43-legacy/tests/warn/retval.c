#include <stdio.h>
int f(int a) {
    if (a > 0) return 1;
}
int g(int a) {
    if (a) { return 1; } else { return 2; }
}
int main(void) {
    printf("%d\n", f(1) + g(0));
    return 0;
}
