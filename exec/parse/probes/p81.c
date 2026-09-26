struct P { int a; long b; };
int main(void) { struct P x; struct P y; x.a = 1; y = x; return y.a; }
