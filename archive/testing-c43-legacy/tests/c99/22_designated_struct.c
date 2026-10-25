#include <stdio.h>
struct P { int x, y, z; };
int main(void){ struct P p = { .y = 7, .x = 2 }; printf("%d %d %d\n", p.x, p.y, p.z); return 0; }
