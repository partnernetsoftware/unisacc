/* 0.0.29 H4': va_arg(ap, T *) carries T's struct type, as a cast does: sqlite3_config copies
   a struct of function pointers through *va_arg(ap, sqlite3_mem_methods *) (`va_arg(...)->m`:
   tests/forward/h4_sqlite.c, which the Python control route does not read) */
#include <stdio.h>
#include <stdarg.h>
typedef struct { int (*f)(int); long a, b; } Methods;
static int twice(int x) { return 2 * x; }
static Methods cfg;
static int config(int op, ...) {
    va_list ap;
    va_start(ap, op);
    if (op == 1) cfg = *va_arg(ap, Methods *);
    va_end(ap);
    return 0;
}
int main(void) {
    static const Methods m = { twice, 3, 4 };
    config(1, &m);
    printf("%d %ld %ld\n", cfg.f(21), cfg.a, cfg.b);
    return 0;
}
