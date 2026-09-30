/* Six tape arguments leave no register for the callee address.  The callm
   tape operation reads it from the next stack slot before entering kern. */
#include <stdio.h>

typedef void (*sum6)(int *, int, int, int, int, int);
struct Dispatch { sum6 run; };

static void kern(int *out, int a, int b, int c, int d, int e) {
    *out = a + b + c + d + e;
}

int main(void) {
    struct Dispatch slot;
    sum6 direct = kern;
    int result = 0;
    slot.run = kern;
    slot.run(&result, 1, 2, 3, 4, 5);
    printf("%d\n", result);
    direct(&result, 2, 2, 3, 4, 5);
    printf("%d\n", result);
    return 0;
}
