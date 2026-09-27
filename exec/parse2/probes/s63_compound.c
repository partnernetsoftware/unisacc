#include <stdio.h>
struct P { int x,y; };
int main(void) {
 struct P p = ((struct P){3,4});
 int *a=(int[5]){1,2,3};
 int *b=(int[]){[2]=9,10};
 int k=0;
 int v=(int){++k};
 printf("%d %d %d %d %d %d\n",p.y,a[4],b[2],b[3],v,k);
 return 0;
}
