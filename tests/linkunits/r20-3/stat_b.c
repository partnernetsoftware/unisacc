#include <stdio.h>
static int s = 2;
int get(void);
int main(void){printf("%d %d\n",get(),s);return 0;}
