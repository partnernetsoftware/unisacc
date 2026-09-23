#include <stdio.h>
struct P { int x, y; };
static int sx(struct P p){ return p.x + p.y; }
int main(void){ printf("%d\n", sx((struct P){3,4})); return 0; }
