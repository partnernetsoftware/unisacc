#include <stdio.h>
struct Small { char x; };
struct Small *small[4];
long guard = 77;
struct Small obj;
struct Large {long a,b,c;};
struct Large *large[3];
int main(void){
 small[1]=&obj;
 printf("%ld %lu %lu %d\n",guard,(unsigned long)sizeof(small),(unsigned long)sizeof(large),small[1]==&obj);
 return guard!=77;
}
