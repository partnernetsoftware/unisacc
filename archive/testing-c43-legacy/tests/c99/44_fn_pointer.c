#include <stdio.h>
static int add(int a,int b){return a+b;}
static int mul(int a,int b){return a*b;}
int main(void){ int (*f[2])(int,int) = {add,mul}; printf("%d %d\n", f[0](3,4), f[1](3,4)); return 0; }
