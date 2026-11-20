#include <stdio.h>
extern int maybe(void) __attribute__((weak));
int main(void){printf("%s\n",maybe?"present":"absent");return maybe?1:0;}
