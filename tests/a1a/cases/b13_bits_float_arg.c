#include <stdio.h>
/* gcc converts double to unsigned silently: 3.9 -> 3 */
int main(void){printf("%d\n",__builtin_popcount(3.9));return 0;}
