/* sizeof has size_t type, independent of the last operand's kind. */
#include <stdio.h>
struct P { int x; int y; };
int main(void) {
    int x = 1; int *p = &x; struct P s;
    printf("%d %d %d %d\n", (int)sizeof(sizeof x),
           (int)sizeof(sizeof(int)), (int)sizeof(sizeof p),
           (int)sizeof(sizeof s));
    printf("%d %d %d\n", sizeof x < -1, sizeof(int) < -1,
           (sizeof x + -8) > 0);
    return 0;
}
