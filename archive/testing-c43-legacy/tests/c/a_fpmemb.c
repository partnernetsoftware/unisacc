/* (*s->f)(...) through a function-pointer member (lua lmem.c firsttry), and a 2-D array-of-pointers member indexed twice (lua lstring.c strcache) -- compared with cc */
#include <stdio.h>
typedef void *(*Alloc)(void *ud, void *p, long o, long n);
typedef struct G { Alloc frealloc; void *ud; } G;
static void *al(void *ud, void *p, long o, long n) { return (char *)ud + n; }
#define try(g,b,o,n) ((*g->frealloc)(g->ud, b, o, n))
int main(void) { G gg; G *g = &gg; char buf[4]; gg.frealloc = al; gg.ud = buf;
  printf("%d\n", (int)((char *)try(g, NULL, 0, 3) - buf)); printf("%d\n", (int)((char*)(*g->frealloc)(g->ud,0,0,2) - buf)); printf("%d\n", (int)((char*)g->frealloc(g->ud,0,0,1) - buf)); return 0; }
