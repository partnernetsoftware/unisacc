#include <stdio.h>
int hits;
double rhs(void){hits++;return 0.5;}
int main(void){
 double d=1; float f=2; int i=7; unsigned long u=9223372036854775808UL; _Bool b=0;
 d+=1.5;d*=2;d-=1;d/=2;
 f+=1.5;f*=2;f-=1;f/=2;
 i+=2.75;i*=0.5;i-=0.5;i/=2.0;
 u+=4096.0;
 b+=0.5;b*=0.0;b-=0.5;
 struct P {float x;} p={1}; p.x+=0.5;
 double a[2]={1,2};int n=0;
 a[n++]+=rhs();
 printf("%d %d %d %d %d %d %d %d\n",(int)(d*4),(int)(f*4),i,u==9223372036854779904UL,b,(int)(a[0]*4),n,hits);
 printf("%d\n",(int)(p.x*4));
 return 0;
}
