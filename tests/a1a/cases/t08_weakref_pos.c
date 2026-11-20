#include <stdio.h>
static int wr(void) __attribute__((weakref("absent_sym")));
int main(void){printf("%s\n",wr?"present":"absent");return 0;}
