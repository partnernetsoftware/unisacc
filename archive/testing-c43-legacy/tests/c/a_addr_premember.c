#include <stdio.h>

struct N { int v; };
struct S { struct N *p; };

static int *take(struct S *s) { return &(--s->p)->v; }

int main(void)
{
    struct N a[2];
    struct S s;
    a[0].v = 9;
    a[1].v = 17;
    s.p = a + 1;
    int v = *take(&s);          /* sequenced: argument order is unspecified, and gcc evaluates right to left */
    printf("%d %d\n", v, s.p == a);
    return 0;
}
