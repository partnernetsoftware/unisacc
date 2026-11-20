#include <stdio.h>
__attribute__((visibility("hidden"))) int hid(void){return 3;}
__attribute__((visibility("default"))) int pubf(void){return 4;}
__attribute__((visibility("protected"))) int prot(void){return 5;}
int main(void){printf("%d %d %d\n",hid(),pubf(),prot());return 0;}
