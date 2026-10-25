#include <stdio.h>
long f0();
long f6();
long f7();
long f8();
long typed8(long,long,long,long,long,long,long,long);
int main(void) {
    long a=f0();
    long b=f6(1L,2L,3L,4L,5L,6L);
    long c=f7(1L,2L,3L,4L,5L,6L,7L);
    long d=f8(1L,2L,3L,4L,5L,6L,7L,8L);
    long e=f8(1L,2L,3L,4L,5L,6L,7L,f0());
    long f=typed8(1L,2L,3L,4L,5L,6L,7L,8L);
    printf("%ld %ld %ld %ld %ld %ld\n",a,b,c,d,e,f);
    return a!=8 || b!=91 || c!=140 || d!=204 || e!=204 || f!=204;
}
long f0(void){return 8;}
long f6(long a,long b,long c,long d,long e,long f){return a+2*b+3*c+4*d+5*e+6*f;}
long f7(long a,long b,long c,long d,long e,long f,long g){return a+2*b+3*c+4*d+5*e+6*f+7*g;}
long f8(long a,long b,long c,long d,long e,long f,long g,long h){return a+2*b+3*c+4*d+5*e+6*f+7*g+8*h;}
long typed8(long a,long b,long c,long d,long e,long f,long g,long h){return f8(a,b,c,d,e,f,g,h);}
