typedef struct { int a; long b; } P;
struct Q { char c; int d; };
long pf(P *p, struct Q q){ return p->b + p->a + q.d; }
