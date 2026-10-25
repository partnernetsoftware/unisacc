/* compound literals, abstract type names, &(redundant parens), wide chars */
#include <stdint.h>
#include <stddef.h>
struct S { int a; int b; };
struct T { int v; struct S *q; };
struct S *gs = &(struct S){1, 2};        /* static storage duration */
struct T gt = {9, &(struct S){3, 4}};
int sum(struct S *p) { return p->a + p->b; }
int main() {
    struct S *q;
    int *arr;
    long off;
    int32_t w;
    size_t n;
    q = &(struct S){3, 4};
    arr = (int[]){10, 20, 30};
    off = (long) & (((struct S *)0)->b);
    w = 7;
    n = sizeof(struct S);
    printf("%d %d %d %d\n", q->a, sum(q), arr[2], (struct S){7, 8}.b);
    printf("%d %d %d\n", (int)off, (int)w, (int)n);
    /* L'x' is a wide character constant; its value is the character's.
       L"..." is refused: its elements are wider than a byte. */
    printf("%d %d %d %d %d\n", gs->a, gs->b, gt.v, gt.q->b, L'x');
    return 0;
}
