/* Aggregate assignment uses addresses, including computed members/elements. */
#include <stdio.h>
struct P { int x; int y; };
struct Small { char a; char b; char c; };
struct Wrap { struct P p; };
int main(void) {
    struct P a[2]; struct P b[2]; struct P p;
    a[0].x=11; a[0].y=12; a[1].x=21; a[1].y=22;
    int i=0; b[i++]=a[1]; p=b[0];
    struct Wrap w; w.p=a[0]; b[1]=w.p;
    struct Small s; struct Small t;
    s.a=3; s.b=4; s.c=5; t=s;
    printf("%d %d %d %d %d %d %d %d\n",i,p.x,p.y,b[1].x,b[1].y,t.a,t.b,t.c);
    return 0;
}
