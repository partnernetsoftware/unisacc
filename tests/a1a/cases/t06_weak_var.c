#include <stdio.h>
int __attribute__((weak)) wv=4;
int main(void){printf("%d\n",wv);return wv;}
