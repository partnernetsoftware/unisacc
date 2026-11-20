#include <stdio.h>
int main(void){unsigned x=1u;unsigned long y=1ul;unsigned long long z=1ull;
printf("%d %d %d\n",__builtin_clz(x),__builtin_clzl(y),__builtin_clzll(z));
printf("%d %d\n",__builtin_clz(0x80000000u),__builtin_clzll(0x00F0000000000000ull));return __builtin_clz(255u);}
