#include <stdio.h>
typedef _Bool B;
struct Bits { B a; B b[2]; B *p; };
static B globals[3] = {0, 256, 0.5};
static B address=&globals, literal="x";
static B result(double x) { return x; }
static int param(B x) { return x; }
int main(void) {
    B a=0.5, b=0, c=(B)0.5, d=2;
    B v[3]={0,256,0.5};
    static B sv[3]={0,256,0.5};
    struct Bits s={2,{0,2},&a};
    B *p=&a;
    int old;
    b=0.5;
    printf("%d %d %d %d %d %d %d %d\n", a,b,c,d,result(0.5),result(256),param(256),param(0.5));
    printf("%d%d%d %d%d%d %d%d%d %d%d%d %d\n",v[0],v[1],v[2],sv[0],sv[1],sv[2],globals[0],globals[1],globals[2],s.a,s.b[0],s.b[1],*s.p);
    a=1; old=a++; printf("%d %d ",old,a);
    a=0; old=a--; printf("%d %d ",old,a);
    a=1; ++a; printf("%d ",a); --a; printf("%d\n",a);
    a=0; a+=0.5; printf("%d ",a);
    a=1; a*=256; printf("%d ",a);
    a=1; a-=1; printf("%d ",a);
    *p=0.5; printf("%d ",*p);
    printf("%d %d %d %d\n",(B)-0.0,(B)256,(B)p,(B)0);
    printf("%d %d\n",address,literal);
    return 0;
}
