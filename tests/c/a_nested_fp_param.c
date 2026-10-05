#include <stdio.h>

typedef struct C { int x; } C;
typedef struct V { int y; } V;

static void callback(C *c, int n, V **v) { c->x = n + (*v)->y; }
static void pick(void (**out)(C *, int, V **)) { *out = callback; }

int main(void) {
    C c = {0};
    V v = {5};
    V *p = &v;
    void (*fn)(C *, int, V **) = 0;
    pick(&fn);
    fn(&c, 7, &p);
    printf("%d\n", c.x);
    return c.x != 12;
}
