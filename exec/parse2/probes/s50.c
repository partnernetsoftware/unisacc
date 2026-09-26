/* sizeof walks scalar operands for type, never evaluates their side effects. */
#include <stdio.h>
int calls;
int touch(void) { calls++; return 9; }
int main(void) {
    int x = 3;
    short s = 2;
    static int saved = sizeof(x + 1);
    printf("%d %d %d %d %d %d\n", (int)sizeof(++x), (int)sizeof(touch()),
           (int)sizeof((s + 1) << 2), (int)sizeof(!&x),
           (int)sizeof(sizeof(x + 1)), saved);
    printf("%d %d\n", x, calls);
    return x != 3 || calls != 0 || saved != sizeof(int);
}
