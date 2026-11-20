#include <stdio.h>
int impl(int x){return x+1;}
int pub(int) __attribute__((alias("impl")));
int gv=9; extern int gv2 __attribute__((alias("gv")));
int main(void){gv2=11;printf("%d %d\n",pub(4),gv);return pub(1);}
