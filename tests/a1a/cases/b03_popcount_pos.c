#include <stdio.h>
int main(void){printf("%d %d %d %d\n",__builtin_popcount(0xFFu),__builtin_popcountl(~0ul),__builtin_popcountll(0x5555555555555555ull),__builtin_popcount(0u));return __builtin_popcount(7u);}
