/* A macro call inside an argument that is NOT the last one: the inner
   call's arguments must not replace the outer call's (C99 6.10.3.1).
   `F(G(1), 2)` once lost the 2. */
#include <stdio.h>
#define G(x) x
#define TWICE(x) ((x) * 2)
#define PAIR(a, b) ((a) * 100 + (b))
#define TRI(a, b, c) ((a) * 10000 + (b) * 100 + (c))
#define STR(x) #x
#define XSTR(x) STR(x)
#define SEL(a, b) b
int main(void) {
    printf("%d %d %d\n", PAIR(G(1), 2), PAIR(TWICE(3), G(4)), PAIR(3, G(4)));
    printf("%d %d\n", TRI(G(1), TWICE(G(2)), 3), TRI(PAIR(0, 1), G(2), G(G(3))));
    printf("%s %d\n", XSTR(PAIR(G(5), 6)), SEL(G(7), PAIR(G(8), 9)));
    return 0;
}
