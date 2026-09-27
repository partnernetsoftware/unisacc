#include <stdio.h>
static int sum(int n, int a[n]) { int r=0; for(int i=0;i<n;i++)r+=a[i]; return r; }
static int first(const int a[static 1]) { return a[0]; }
static int byte(char a[]) { a[1]=67; return a[1]; }
static int flag(_Bool a[2]) { a[1]=9; return a[0]+a[1]; }
static double real(double a[const 2]) { return a[0]+a[1]; }
int main(void) {
 int a[3]={2,3,4}; char b[2]={65,66}; _Bool c[2]={1,0}; double d[2]={1.5,2.5};
 printf("%d %d %d %d %d\n",sum(3,a),first(a),byte(b),flag(c),(int)real(d)); return 0;
}
