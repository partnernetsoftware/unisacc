#include <stdio.h>
int main(void){unsigned h=2166136261u;for(int r=0;r<30000;r++)for(int i=0;i<10000;i++){h^=(unsigned)(i*r);h*=16777619u;}printf("%u\n",h);return 0;}
