/* 0.0.22 csmith seed 44: a local array of struct POINTERS holds pointers; the reference
   laid out struct slots for its initialiser and crashed (2-D case). */
#include <stdio.h>
struct S { long f0; short f1; };
static struct S g = {-6, 1}, h = {7, 2};
static struct S *gp[2] = {&h, &g};          /* seed 143: the global case crashed */
static int f(void) {
    struct S *l[2][3] = {{&g, &h, &g}, {&h, &g, &h}};
    struct S *const m[2] = {&h, &g};
    struct S arr[2] = {{1, 2}, {3, 4}};        /* plain struct arrays stay structs */
    return (int)(l[1][1]->f0 + l[0][1]->f1 * 10 + m[0]->f0 * 100 + gp[1]->f1 * 1000 + arr[0].f1 * 10000);
}
int main(void) { printf("%d\n", f()); return 0; }
