#include <stdio.h>
int main(void){double d=__builtin_choose_expr(1,2.5,"str");int i=__builtin_choose_expr(0,"x",42);
printf("%g %d %u\n",d,i,(unsigned)sizeof(__builtin_choose_expr(sizeof(int)==4,(char)0,0L)));return 0;}
