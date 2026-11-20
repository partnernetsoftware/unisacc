#include <stdio.h>
__attribute__((section(".text.hotpath"))) int hot(int x){return x*2;}
__attribute__((section("mydata"))) int tbl[3]={1,2,3};
__attribute__((section(".rodata.cfg"))) const char cfg[]="cfg";
int main(void){printf("%d %d %s\n",hot(tbl[2]),tbl[0],cfg);return hot(1);}
