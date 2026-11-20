/* gcc warns (priorities 0..100 reserved) but accepts */
#include <stdio.h>
__attribute__((constructor(50))) static void c(void){printf("c50\n");}
int main(void){return 0;}
