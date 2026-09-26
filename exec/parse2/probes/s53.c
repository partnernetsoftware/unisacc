/* Computed lvalues: postfix result, final store and one evaluation of the index. */
#include <stdio.h>
struct S { int n; long *p; };
int main(void) {
    long a[3]; a[0]=10; a[1]=20; a[2]=30;
    struct S s; struct S *q=&s; s.n=4; s.p=a;
    int x=s.n++; int y=q->n--; long *p=s.p++;
    int i=0; long u=a[i++]++; long v=a[1]--;
    printf("%d %d %d %d %d %ld %ld %ld %ld\n", x,y,s.n,i,(int)(s.p-p),u,v,a[0],a[1]);
    return 0;
}
