#include <stdio.h>
typedef struct { int a, b; } B;
static B mk(int a){ B r; r.a = a; r.b = a + 1; return r; }
static B wrap(int a){ return mk(a); }
int main(void){ B v = wrap(2); printf("%d %d\n", v.a, v.b); return 0; }
