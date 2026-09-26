int f(short *p, int i, char *q, long **r){ int k; p[i] = 9; q[i + 1] = p[i] + 2; r[1][0] = q[1]; k = p[i] + q[2] + r[1][0] + *(p + i); q[0]; return k * 2; }
int main(){ short s; char c; long l; long *pl; pl = &l; s = 1; return f(&s, 0, &c, &pl) != 0; }
