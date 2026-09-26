/* Pointer members and arrays use their declared store width in brace init. */
#include <stdio.h>
struct Buf { char *p; long *q; int n, cap; };
int main(void) {
    long n=9;
    struct Buf empty={0};
    struct Buf b={"abc", &n, 3, 8};
    long *a[2]={&n, 0};
    printf("%d %d %d %d %d %ld %d %d %ld %d\n", empty.p==0, empty.q==0,
           empty.n,empty.cap,b.p[1],*b.q,b.n,b.cap,*a[0],a[1]==0);
    return 0;
}
