#include <stdio.h>
int counter __asm__("ctr_sym") = 5;
int main(void){counter++;printf("%d\n",counter);return counter;}
