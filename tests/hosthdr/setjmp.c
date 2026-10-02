/* setjmp/longjmp (C99 7.13): intrinsics, compared with the system compiler.
   The shape is Lua's luaD_rawrunprotected: a jmp_buf member of a struct on
   the stack, longjmp from a deeper recursion, nested handlers, value 0 -> 1. */
#include <setjmp.h>
#include <stdio.h>
struct lj { struct lj *prev; jmp_buf b; volatile int status; };
static struct lj *cur;
static void thrower(int n, int v) { char pad[32]; pad[0] = (char)n;
    if (n == 0) { cur->status = 9 + pad[0]; longjmp(cur->b, v); } thrower(n - 1, v); }
static int protect(int n, int v) { struct lj l; int r; l.status = 0; l.prev = cur; cur = &l;
    if ((r = setjmp(l.b)) == 0) thrower(n, v); cur = l.prev; return l.status * 10 + r; }
static int nested(void) { struct lj o; o.prev = cur; cur = &o;
    if (setjmp(o.b) == 0) { int x = protect(3, 2); printf("inner %d\n", x); longjmp(o.b, 5); }
    cur = o.prev; return 7; }
int main(void) {
    printf("%d %d %d\n", protect(5, 1), protect(0, 0), protect(40, -3));
    printf("nested %d\n", nested());
    return 0;
}
