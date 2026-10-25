#include <stdio.h>
static int f(int n){ return n<2 ? n : f(n-1)+f(n-2); }
int main(void){ printf("%d\n", f(12)); return 0; }
