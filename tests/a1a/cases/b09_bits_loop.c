#include <stdio.h>
int main(void){unsigned long long s=0;unsigned i;for(i=1;i<2000;i++){s+=__builtin_clz(i)*3+__builtin_ctz(i)+__builtin_popcount(i)+(__builtin_bswap32(i)>>24);}
printf("%llu\n",s);return (int)(s&0x7f);}
