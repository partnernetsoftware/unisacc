#include <stdio.h>
static int sum(int n, int a[n]){ int s=0; for(int i=0;i<n;i++) s+=a[i]; return s; }
int main(void){ int a[3]={1,2,3}; printf("%d\n", sum(3,a)); return 0; }
