/* Shared scalar/aggregate/assignment conversion; no trial expression parse. */
#include <stdio.h>
struct P {double d; float f; int i; unsigned long u; _Bool b;};
double gd=3; float gf=2.5; int gi=3.75;
int main(void) {
 unsigned long big=9223372036854775808UL;
 double d=3, fromu=big;
 float f=d;
 int i=2.75;
 unsigned int j=3.75;
 unsigned long u=fromu;
 _Bool b=0.5;
 double da[2]={2,3.25f}; float fa[2]={4,5.25}; int ia[2]={6.75,7.75f};
 struct P p={8,9.5,10.75,11.5,0.5};
 printf("%d %d %d %d %d %d\n",(int)d,(int)f,i,(int)j,u==big,b);
 d=f; f=2; i=d; j=f; u=fromu; b=f;
 printf("%d %d %d %d %d %d\n",(int)d,(int)f,i,(int)j,u==big,b);
 printf("%d %d %d %d %d %d\n",(int)da[0],(int)(da[1]*4),(int)fa[0],(int)(fa[1]*4),ia[0],ia[1]);
 printf("%d %d %d %d %d / %d %d %d\n",(int)p.d,(int)p.f,p.i,(int)p.u,p.b,(int)gd,(int)gf,gi);
 static double sd=3; static float sf=4.5; static int si=5.75; static unsigned long su=9223372036854779904.0; static _Bool sb=0.5;
 printf("%d %d %d %d %d\n",(int)sd,(int)sf,si,su==9223372036854779904UL,sb);
 return 0;
}
