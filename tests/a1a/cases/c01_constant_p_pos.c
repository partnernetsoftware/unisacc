#include <stdio.h>
int main(int argc,char**argv){(void)argv;printf("%d %d %d\n",__builtin_constant_p(3),__builtin_constant_p(1+2*4),__builtin_constant_p("s"[0]));
printf("%d\n",__builtin_constant_p(argc));return 0;}
