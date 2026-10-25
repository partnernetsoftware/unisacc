/* 0.0.29 T3: rescanning of nested and self-referential macros, token pasting, and an empty macro --
   preprocessor rescan rows no probe reached */
#include <stdio.h>
#define CAT(a, b) a ## b
#define XCAT(a, b) CAT(a, b)
#define TWICE(x) ((x) * 2)
#define APPLY(f, x) f(x)
#define EMPTY
int val = 1;
#define val (val + 1)
int main(void) {
    int CAT(va, lue) = XCAT(1, 2);
    int n = APPLY(TWICE, CAT(3, 4)) EMPTY;
    printf("%d %d %d\n", val, value, n);
    return 0;
}
