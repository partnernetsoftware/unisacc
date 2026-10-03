#include <stdio.h>

struct S { int x; };
typedef void (*Fn)(struct S *, void *);

static void touch(struct S *s, void *unused)
{
    (void)unused;
    s->x = 3;
}

static void run(struct S *s, Fn f)
{
    (*f)(s, 0);
    s->x = s->x + 4;
}

int main(void)
{
    struct S s;
    int *p = &s.x;
    s.x = 0;
    run(&s, touch);
    *p = *p + 2;
    printf("%d\n", s.x);
    return 0;
}
