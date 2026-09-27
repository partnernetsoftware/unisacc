#include <stdio.h>
long int f(long long int x, unsigned short int y) {return x+y;}
int main(void) {
 short int s=-3; unsigned short int u=65535;
 long int a=7; unsigned long int b=9;
 long long int c=11; unsigned long long int d=13;
 printf("%ld %lu %lu %lu %lu %lu %lu\n",f(s,u)+a+b+c+d,
 (unsigned long)sizeof(short int),(unsigned long)sizeof(unsigned short int),
 (unsigned long)sizeof(long int),(unsigned long)sizeof(unsigned long int),
 (unsigned long)sizeof(long long int),(unsigned long)sizeof(unsigned long long int));
 return (long int)(unsigned short int)-1 != 65535;
}
