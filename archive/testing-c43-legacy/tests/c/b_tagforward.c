#include <stdio.h>
struct S {int x;};
int main(void) {
 struct S o; o.x=7;
 {struct S; struct S *p; struct S {long y;}; struct S i; p=&i;p->y=11;
  printf("%lu %ld\n",(unsigned long)sizeof(*p),p->y);}
 {struct S q;q.x=13;printf("%lu %d %d\n",(unsigned long)sizeof q,q.x,o.x);}
 return 0;
}
