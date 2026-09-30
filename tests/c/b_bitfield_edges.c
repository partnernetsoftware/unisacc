#include <stdio.h>
struct B { _Bool b:1; signed a:5; unsigned c:5; }; union U { unsigned a:3; signed b:5; }; struct Z { unsigned a:3; unsigned :0; unsigned b:3; };
int main(void){struct B x={1,0,0}; union U u={5}; struct Z z={3,4}; int a=x.a--; int c=x.c--; printf("%d %d %d %u %d %d %d %d %d %d\n",a,x.a,c,x.c,x.b,u.a,z.a,z.b,(int)sizeof(x.a+1),(int)sizeof(+(x.a))); x.b++; printf("%d ",x.b); x.b+=1; printf("%d\n",x.b); return 0;}
