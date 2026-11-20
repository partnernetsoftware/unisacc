#include <stdio.h>
int main(void){/* result types: clz/ctz/popcount -> int; bswap32 -> uint32 */
printf("%u %u %u\n",(unsigned)sizeof(__builtin_clz(1u)),(unsigned)sizeof(__builtin_bswap64(1ull)),(unsigned)sizeof(__builtin_bswap16(1)));
printf("%d\n",__builtin_popcount(-1));return 0;}
