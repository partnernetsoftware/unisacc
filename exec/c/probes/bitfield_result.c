/* Assignment values include the bit-field width conversion. */
#include <stdio.h>
struct B { signed a:5; unsigned b:5; };
struct W { unsigned long all:64; signed long sign:64; unsigned long tail:63; };
int main(void) {
    struct B x; struct W w; int r, v=34;
    x.a=15; r=(x.a+=1); printf("%d %d ",r,x.a);
    x.b=31; r=(x.b+=1); printf("%d %u ",r,x.b);
    r=(x.a=v); printf("%d %d ",r,x.a);
    r=++x.a; printf("%d %d ",r,x.a);
    r=x.a++; printf("%d %d\n",r,x.a);
    w.all=~0UL; w.sign=-1; w.tail=~0UL;
    printf("%lu %ld %lu\n",w.all,w.sign,w.tail);
    return 0;
}
