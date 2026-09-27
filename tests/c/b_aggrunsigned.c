/* Aggregate slots preserve u64 conversion kind above INT64_MAX. */
#include <stdio.h>
struct P { unsigned long u, a[2]; double d; unsigned int j; _Bool b; };
struct N { struct P p; };
unsigned long ga[1]={9223372036854779904.0};
struct P gp={9223372036854779904.0,{9223372036854779904.0,3.5},4,5.75,0.5};
int main(void) {
 unsigned long want=9223372036854779904UL;
 unsigned long a[1]={9223372036854779904.0};
 struct N n={{9223372036854779904.0,{9223372036854779904.0,3.5},4,5.75,0.5}};
 static unsigned long sa[1]={9223372036854779904.0};
 static struct P sp={9223372036854779904.0,{9223372036854779904.0,3.5},4,5.75,0.5};
 unsigned long v=(unsigned long){9223372036854779904.0};
 printf("%d %d %d %d %d %d %d %d %d\n",a[0]==want,n.p.u==want,n.p.a[0]==want,ga[0]==want,gp.u==want,gp.a[0]==want,sa[0]==want,sp.u==want,v==want);
 printf("%d %d %d %d\n",(int)n.p.a[1],(int)n.p.d,(int)n.p.j,n.p.b);
 return 0;
}
