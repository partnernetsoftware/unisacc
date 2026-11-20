#include <stdio.h>
int g(void) __asm("g_alt");int g(void){return 3;}
int main(void){printf("%d\n",g());return 0;}
