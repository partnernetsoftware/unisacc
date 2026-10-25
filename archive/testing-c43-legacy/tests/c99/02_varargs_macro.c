#include <stdio.h>
#define P(fmt, ...) printf(fmt, __VA_ARGS__)
int main(void){ P("%d %d\n", 1, 2); return 0; }
