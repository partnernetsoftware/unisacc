struct P { char c; long x; short s; int *p; int i; };
int f(void) { struct P a; struct P *q; a.i = 5; a.x = 7; q = &a; q->s = 3; return a.i + q->s; }
int main(void) { return f(); }
