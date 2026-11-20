#include <stdio.h>
int main(void){printf("%d %d %d\n",__builtin_ctz(8u),__builtin_ctzl(1ul<<40),__builtin_ctzll(1ull<<63));return __builtin_ctz(0x100u);}
