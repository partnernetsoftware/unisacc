/* Parentheses do not destroy an lvalue.  `(*p)++` is how half of a regex
   engine is written, and we used to refuse it: the binary ladder loads at
   its innermost level, so a parenthesised expression came back as a value. */
#include <stdio.h>

struct P { int x; int y; };

int main(void)
{
    int a = 1, b[4], i = 2;
    int *p = &a;
    struct P s;
    struct P *sp = &s;

    (a) = 5;
    (*p)++;
    (*p) += 3;
    b[0] = 0; b[1] = 0; b[2] = 0; b[3] = 0;
    (b[i])++;
    (b[i]) = (b[i]) * 7 + 1;
    s.x = 10; s.y = 20;
    (s.x)--;
    (sp->y) += 2;
    printf("%d %d %d %d\n", a, b[2], s.x, s.y);
    printf("%d %d\n", (a), ((a)));
    return 0;
}
