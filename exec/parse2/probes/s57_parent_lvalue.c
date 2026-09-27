#include <stdio.h>
#include <stdarg.h>
typedef long I;
static int copied(int n, ...) {
    va_list a,b; va_start(a,n);
    int first=va_arg(a,int); va_copy(b,a);
    int x=va_arg(a,int); int y=va_arg(b,int);
    int z=va_arg(b,int); int w=va_arg(a,int);
    va_end(a); va_end(b);
    return first+x+y+z+w;
}
int main(void) {
    int a[2]={1,2}; int i=0; int *p=&a[1];
    ((a[i++])) += 4;
    (((*p))) = 9;
    int old=((a[0]))++;
    ((*p)) -= 2;
    int n=0; long cast=(I)++n;
    if(cast!=1 || n!=1) return 1;
    printf("%d %d %d %d %d\n",i,a[0],a[1],old,copied(3,2,3,5));
    return 0;
}
