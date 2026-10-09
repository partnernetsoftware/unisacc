typedef struct { int a; long b; } P;
struct Q { char c; int d; };
long pf(P *p, struct Q q);
typedef unsigned P2;
int main(void){ P p = {1, 40}; struct Q q = {0, 2}; P2 k = 0; return pf(&p, q) + k != 43; }
