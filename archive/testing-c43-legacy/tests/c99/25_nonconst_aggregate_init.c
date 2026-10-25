#include <stdio.h>
int main(void){ int n = 3; struct { int a, b; } s = { n, n*2 }; printf("%d %d\n", s.a, s.b); return 0; }
