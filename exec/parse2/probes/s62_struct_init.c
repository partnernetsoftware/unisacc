#include <stdio.h>
struct S { char c; int n; long v; };
struct B { char x[3]; };
static int calls;
static struct S make(int n) { struct S s={65,0,17}; s.n=n; calls++; return s; }
int main(void) {
 struct S a=make(9), b=a; struct S c=make(12); struct B x={{1,2,3}}, y=x;
 b.n=4;
 printf("%d %d %d %d %ld %d %d %d\n",calls,a.n,b.n,c.n,c.v,y.x[0],y.x[1],y.x[2]);
 return 0;
}
