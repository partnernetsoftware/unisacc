#include <stdio.h>
int main(void){printf("%d %d %d %d\n",__builtin_ffs(0),__builtin_ffs(8),__builtin_ffsl(1l<<40),__builtin_ffsll(-1ll));return __builtin_ffs(0x10);}
