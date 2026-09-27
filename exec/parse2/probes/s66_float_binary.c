/* Binary common types: f32/f64, signed integers and u64 above INT64_MAX. */
#include <stdio.h>
int main(void) {
 float a=1.5f,b=2.0f; double d=2.25; int n=3; unsigned long u=9223372036854775808UL;
 printf("%d %d %d %d\n",(int)(a+b),(int)(b-a),(int)(a*b),(int)(b/a));
 printf("%d %d %d %d %d %d\n",a<b,a>b,a<=b,a>=b,a==b,a!=b);
 printf("%d %d %d %d\n",(int)(a+d),(int)(d+a),(int)(a+n),(int)(n+a));
 printf("%d %d %d %d\n",u>a,a<u,u>d,d<u);
 printf("%d %d %d %d\n", (a+0.0f)==a, (d+0.0)==d, (u+a)>0.0f, (a+u)>0.0f);
 return 0;
}
