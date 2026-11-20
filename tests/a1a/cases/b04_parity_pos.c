#include <stdio.h>
int main(void){printf("%d %d %d\n",__builtin_parity(7u),__builtin_parityl(3ul),__builtin_parityll(1ull<<50));return __builtin_parity(1u);}
