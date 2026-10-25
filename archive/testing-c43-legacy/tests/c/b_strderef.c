/* *"literal" is its first char: the literal sets its whole kind, so a
   struct or pointer-to-pointer expression just before cannot leave it
   an address (it once returned the literal's address, truncated) */
#include <stdio.h>
struct S { int a; int b; };
int f(void) { return *"z"; }
int main(void) {
    struct S s; struct S *sp = &s; int x = 5; int *px = &x; int **ppx = &px;
    char *p = "pq";
    int r;
    s.a = 1; s.b = 2;
    r = sp->b + *"A";
    printf("%d %d\n", f(), r);
    r = **ppx; r = r + *"B" + "cd"[1] + *(p + 1);
    printf("%d %c %s\n", r, *"Q", p);
    return 0;
}
