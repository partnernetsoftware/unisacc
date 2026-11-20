#include <stdio.h>
static int f(int x){switch(x){case 0:return 10;case 1:return 20;default:__builtin_unreachable();}}
int main(void){printf("%d\n",f(1));return f(0);}
