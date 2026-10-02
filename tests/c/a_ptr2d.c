/* T *m[n][k] as a struct member and a local: m[i][j] is a pointer, not a T (lua lstring.c strcache) */
#include <stdio.h>
typedef struct T { long a, b, c, d; } T;
struct G { int x; T *cache[3][2]; T *one[3]; };
int main(void) { static struct G gg; struct G *g = &gg; T t1 = {1,2,3,4}; T *p = &t1; int i, j;
  for (i = 0; i < 3; i++) { g->one[i] = p; for (j = 0; j < 2; j++) g->cache[i][j] = p; }
  printf("%ld %ld %ld\n", g->cache[2][1]->c, g->one[1]->d, (long)(g->cache[1][0] == p)); return 0; }
