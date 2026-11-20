#include <stdio.h>
static int g=__builtin_constant_p(7)?1:2;
int main(void){printf("%d\n",g);return 0;}
