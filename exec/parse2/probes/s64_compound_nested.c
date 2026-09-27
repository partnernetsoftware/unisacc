/* Nested anonymous objects and arrays of structures share INITVALUE. */
#include <stdio.h>
struct P { int x,y; };
int main(void) {
 struct P p=((struct P){(int){3},4});
 int k=0;
 int *a=(int[]){++k,++k};
 int *b=(int[]){7,8};
 printf("%d %d %d %d %d %d\n",p.x,((struct P[]){{1,2},{3,4}})[1].y,a[0],a[1],b[0],k);
 return 0;
}
