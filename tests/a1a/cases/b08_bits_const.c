#include <stdio.h>
/* constant folding: usable in static initializer and array size */
static int t[__builtin_popcount(0xF0u)];
static const int k=__builtin_ctz(16u);
int main(void){printf("%u %d\n",(unsigned)(sizeof t/sizeof t[0]),k);return 0;}
