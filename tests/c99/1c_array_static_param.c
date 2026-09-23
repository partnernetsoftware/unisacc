#include <stdio.h>
static int first(int a[static 1]){ return a[0]; }
int main(void){ int a[2]={9,8}; printf("%d\n", first(a)); return 0; }
