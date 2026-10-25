#include <stdio.h>
struct B { int a,b,c,d; };
static struct B bump(struct B x){ x.a++; x.d++; return x; }
int main(void){ struct B v={1,2,3,4}; v = bump(v); printf("%d %d\n", v.a, v.d); return 0; }
