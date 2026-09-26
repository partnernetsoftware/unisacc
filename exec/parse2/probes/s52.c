/* A complex sizeof operand falls back to expression typing, without calling it. */
#include <stdio.h>
int calls, item;
int *f(void) { calls++; return &item; }
int main(void) {
    int a = sizeof *f();
    int b = sizeof(*f());
    printf("%d %d %d\n", a, b, calls);
    return a != sizeof(int) || b != sizeof(int) || calls != 0;
}
