#include <stdio.h>
int main(void){unsigned char x=200;unsigned short s=65000;signed char c=100;int a,b,d;x++;x++;a=x++;s++;b=s--;c++;d=c--;printf("%d %d %d %d %d %d\n",x,a,s,b,c,d);return 0;}
