#include <stdio.h>
#include <stdlib.h>
int main(void){int n=60000000,c=0;char*s=calloc(n+1,1);for(int i=2;i<=n;i++)if(!s[i]){c++;for(long j=(long)i*i;j<=n;j+=i)s[j]=1;}printf("%d\n",c);return 0;}
