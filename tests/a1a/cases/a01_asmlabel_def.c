#include <stdio.h>
int f(void) __asm__("renamed_sym");
int f(void){return 7;}
int main(void){printf("%d\n",f());return f();}
