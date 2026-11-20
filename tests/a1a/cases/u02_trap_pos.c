#include <stdio.h>
int main(int argc,char**argv){(void)argv;printf("before\n");fflush(stdout);if(argc>0)__builtin_trap();return 0;}
