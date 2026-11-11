#include <stdio.h>
static const unsigned u[][4] = {{1,2,3}};
int main(void){ static const int v[][2] = {{1},{2}}; const int x[][3] = {{1,2},{3}}; printf("%d %d %d\n", (int)sizeof u, (int)sizeof v, (int)sizeof x); return 0; }
