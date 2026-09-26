static void (*tab[4])(void);
static int (*gf)(int);
static int twice(int x) { return x + x; }
static void nop(void) { }
static int app(int (*f)(int), int v) { int (*g)(int); g = f; return g(v) + f(v) + (*f)(v); }
int main(void) { int k; k = 1; gf = twice; tab[k] = nop; tab[k](); return app(twice, 3) + gf(2); }
