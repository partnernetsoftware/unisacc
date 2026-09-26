typedef struct S T;
int f(long l, char *p){ int x; char *q; T *t; x = 300; q = (char *)l; t = (T *)p; q = (void *)t; x = (char)x + (short)l + (int)(long)p + (x); return *(int *)q + (long)x + *((char *)q + 1); }
int main(){ int v; v = 7; return f(5, (char *)&v, 1) != 0; }
