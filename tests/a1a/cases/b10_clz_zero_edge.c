#include <stdio.h>
/* clz(0)/ctz(0) undefined in gcc: only check the value is not used; sample tests that a guarded use compiles */
static int lz(unsigned v){return v?__builtin_clz(v):32;}
int main(void){printf("%d %d\n",lz(0),lz(1));return 0;}
