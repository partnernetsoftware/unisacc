/* Aggregate varargs use one internal address slot and independent value copies. */
#include <stdio.h>
#include <stdarg.h>
struct D { int x; };
struct F { float x; };
struct A { char s[7]; };
struct B { int x; double y; };
struct C { long x,y,z; };
void readall(int fixed, ...) {
 va_list ap,copy; va_start(ap,fixed); va_copy(copy,ap);
 struct A a=va_arg(ap,struct A);
 struct B b=va_arg(ap,struct B);
 struct C c=va_arg(ap,struct C);
 struct D d=va_arg(ap,struct D);
 struct F f=va_arg(ap,struct F);
 int tail=va_arg(ap,int);
 struct A again=va_arg(copy,struct A);
 a.s[0]='Z';
 printf("%c %c %d %.1f %ld %ld %ld %d %.1f %d\n",a.s[0],again.s[0],b.x,b.y,c.x,c.y,c.z,d.x,(double)f.x,tail);
 va_end(copy); va_end(ap);
}
int main(void) {
 struct A a={{'A','B','C','D','E','F',0}};
 struct B b={13,2.5}; struct C c={17,19,23};
 struct D d={31}; struct F f={3.5f};
 readall(0,a,b,c,d,f,29); printf("%c %d\n",a.s[0],b.x);
 return 0;
}
