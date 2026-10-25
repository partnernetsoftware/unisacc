#include <stdio.h>
int main(void){ int a=1,b=2,c; c = (a++, b++, a>b ? a : b); printf("%d %d %d\n", a,b,c); return 0; }
