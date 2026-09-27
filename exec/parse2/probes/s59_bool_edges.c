#include <stdio.h>
typedef _Bool B;
static B sf=0.5f;
static B fromfloat(float x) { B b=x; return b; }
int main(void) {
    B a[3]={0,2,0.5f}; B *p=a; B b=0;
    p++; *p=-0.0f;
    b+=0.5f;
    printf("%d %d %d %d %d %d\n",a[0],a[1],a[2],b,sf,fromfloat(0.5f));
    printf("%ld %ld %ld %ld\n",(long)sizeof(B),(long)sizeof(a),(long)(p-a),(long)sizeof(*p));
    printf("%d %d %d %d\n",(B)-0.0f,(B)-0.5f,(B)0x1p-149f,(B)(void *)p);
    return 0;
}
