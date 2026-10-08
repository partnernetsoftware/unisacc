#include <stdio.h>
int main(void){
    int x = 4;
    register int a = 3;
    register int *p = &x;
    register const char *q = "r";
    printf("%d %d %c\n", a, *p, *q);
    return 0;
}
