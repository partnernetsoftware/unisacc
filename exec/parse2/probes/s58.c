/* Aggregate arguments are private copies, including short tails and stacked args. */
#include <stdio.h>
struct Tiny { unsigned char a,b,c; };
struct Tiny make(int n) { struct Tiny x; x.a=n; x.b=n+1; x.c=n+2; return x; }
int pair(struct Tiny a, struct Tiny b) { a.a=9; return a.a*100+b.a*10+b.c; }
int many(int a,int b,int c,int d,int e,int f,struct Tiny x,int g) {
    x.b=8;
    return a+b+c+d+e+f+x.a+x.b+x.c+g;
}
int main(void) {
    struct Tiny t; int p,m;
    t=make(3);
    p=pair(make(1),make(2));
    m=many(1,2,3,4,5,6,t,7);
    printf("%d %d %d %d %d\n",p,m,t.a,t.b,t.c);
    return 0;
}
