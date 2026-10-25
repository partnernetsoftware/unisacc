#include <stdio.h>
struct S { int x, y; };
static struct S g;
static int a[3] = {1, 2, 3};
static int f(void) { return 7; }
static int dead_first = 0 ? f() : 3;
static int dead_last = 1 ? 4 : f();
static int sized = sizeof(f());
static int *address = &g.y;
static int *array = a;
static int (*function)(void) = f;
static int offset = __builtin_offsetof(struct S, y);
enum E { V = 5 };
static int enumerator = V;
static int scalar_then_call(void) { static int x = 3; int y = f(); return y - x; }
static int aggregate_then_call(void) { static struct S s = {3, 4}; int y = f(); return y - s.x; }
int main(void) {
    printf("%d %d %d %d %d %d %d %d %d\n", dead_first, dead_last, sized,
           address == &g.y, array[2], function(), offset, enumerator,
           scalar_then_call() + aggregate_then_call());
    return 0;
}
