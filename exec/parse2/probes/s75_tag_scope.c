#include <stdio.h>
struct S { int a; };
int first(void) {
 struct S o; o.a=1;
 {struct S {long x;} i; i.x=2;
  printf("%lu %ld\n",(unsigned long)sizeof i,i.x);
  {struct S {char y;} j; j.y=3; printf("%lu %d\n",(unsigned long)sizeof j,j.y);}
  {struct S k; k.x=4; printf("%lu %ld\n",(unsigned long)sizeof k,k.x);}
 }
 {struct S p; p.a=5; printf("%lu %d\n",(unsigned long)sizeof p,p.a);}
 printf("%lu %d\n",(unsigned long)sizeof o,o.a);return 0;
}
int main(void) {struct S q;q.a=6;first();printf("%lu %d\n",(unsigned long)sizeof q,q.a);return 0;}
