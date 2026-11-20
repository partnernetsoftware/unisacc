#include <stdio.h>
typedef int myint;
int main(void){printf("%d %d %d %d %d\n",__builtin_types_compatible_p(int,myint),__builtin_types_compatible_p(int,long),
__builtin_types_compatible_p(const int,int),__builtin_types_compatible_p(int[],int[5]),__builtin_types_compatible_p(char*,signed char*));return 0;}
