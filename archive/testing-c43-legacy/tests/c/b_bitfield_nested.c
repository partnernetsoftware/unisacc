#include <stdio.h>
struct A { unsigned a:3; signed b:5; };
struct B { struct A a[2]; unsigned c:5; };
struct B g={{{3,-4},{6,7}},20};
int count; struct B *get(void) { count++; return &g; }
int main(void) { struct B l={{{1,-2},{5,-8}},30}; int old=get()->a[1].b++; int now=(get()->c+=15); printf("%u %d %u %d %u %d %d %d %d\n",l.a[0].a,l.a[0].b,l.a[1].a,l.a[1].b,l.c,old,g.a[1].b,now,count); return 0; }
