#include <stdio.h>
__attribute__((noinline)) static int sq(int x){return x*x;}
int main(void){printf("%d\n",sq(9));return sq(2);}
