#include <stdio.h>
static int up(int n){return n+1;}
static int down(int n){return n-1;}
static int (*pick(int which, int (*fallback)(int)))(int);
static int (*pick(int which, int (*fallback)(int)))(int) {
    if(which) return up;
    return fallback;
}
static int seen;
static void note(int n){seen=n;}
static void (*observer(void))(int){return note;}
static int ordinary(int n){return n*2;}
int main(void) {
    int (*p)(int)=pick(0,down);
    int a=p(9); int b=pick(1,down)(3);
    void (*v)(int)=observer();
    v(7);
    void (*nil)(int)=(void (*)(int))0;
    printf("%d %d %d %d %d\n",a,b,seen,nil==0,ordinary(6));
    return 0;
}
