/* VLA elements retain the declared type, independent of the bound's cast. */
#include <stdio.h>
struct P{int x,y;};
int main(void){
 int n=2;int x=-1;int *px=&x;
 int *a[n];int **b[n];struct P *p[n];struct P obj={3,5};
 int c[(long)n];double d[(int)n];unsigned char e[(int)n];
 a[0]=&x;b[0]=&px;p[0]=&obj;c[0]=7;d[0]=2.5;e[0]=255;
 printf("%d %d %d %d %d %d %d\n",*a[0]<0,**b[0]<0,p[0]->y,c[0],d[0]==2.5,e[0],(int)sizeof a);
 return 0;
}
