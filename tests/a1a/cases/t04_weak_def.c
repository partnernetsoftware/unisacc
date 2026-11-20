#include <stdio.h>
__attribute__((weak)) int hook(void){return 1;}
int main(void){printf("%d\n",hook());return hook();}
