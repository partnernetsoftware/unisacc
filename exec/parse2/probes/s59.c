/* The same list initialization for global, automatic and static storage. */
#include <stdio.h>
struct P { char *p; int n; };
char *names[3]={"ab","cd","ef"};
int g[2][3]={{1},{2,3}};
struct P global={"gh",7};
int f(int bump) {
    static char *lines[]={"xy","zt","uv"};
    static int a[2][3]={{4},{5,6}};
    static struct P p={"jk",8};
    int b[2][3]={{4},{5,6}};
    p.n=p.n+bump;
    a[0][0]=a[0][0]+bump;
    printf("%c %s %d %d %d %d %c %d\n",names[1][1],lines[2],g[1][2],
           a[0][0],a[1][1],b[1][2],p.p[1],p.n);
    return 0;
}
int main(void) { f(1); f(2); printf("%s %d\n",global.p,global.n); return 0; }
