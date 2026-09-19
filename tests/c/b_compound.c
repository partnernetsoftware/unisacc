/* compound literals, abstract type names, &(redundant parens) */
#include <stdint.h>
#include <stddef.h>
struct S { int a; int b; };
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
    return 0;
}
