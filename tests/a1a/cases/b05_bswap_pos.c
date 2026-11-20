#include <stdio.h>
int main(void){unsigned short a=__builtin_bswap16((unsigned short)0x1234);unsigned b=__builtin_bswap32(0x11223344u);unsigned long long c=__builtin_bswap64(0x0102030405060708ull);
printf("%04x %08x %016llx\n",a,b,c);return (int)(__builtin_bswap32(0x01000000u));}
