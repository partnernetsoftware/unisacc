#include <stdio.h>
static int add(int * restrict a, int * restrict b){ return *a + *b; }
int main(void){ int x=2,y=3; printf("%d\n", add(&x,&y)); return 0; }
