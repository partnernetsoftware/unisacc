/* a typedef name that carries a pointer: typedef char *vl; parameters and locals of it */
typedef char *vl;
typedef long *lp;
int f(vl p, lp q) { vl r; r = p + 1; return r[0] + p[2] + q[1]; }
int main() { char c[4]; long n[2]; c[1] = 2; c[2] = 3; n[1] = 4; return f(c, n); }
