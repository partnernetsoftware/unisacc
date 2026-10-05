#include <stdio.h>
#include <stdlib.h>
static int cmp(const void*a,const void*b){int x=*(const int*)a,y=*(const int*)b;return (x>y)-(x<y);}
int main(void){int n=5000000;int*v=malloc(n*sizeof*v);unsigned s=1;for(int i=0;i<n;i++){s=s*1103515245u+12345u;v[i]=(int)(s>>1);}qsort(v,n,sizeof*v,cmp);printf("%d\n",v[n/2]);return 0;}
