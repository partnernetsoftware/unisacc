/* A file-scope `static` object whose type is an ANONYMOUS struct,
     static struct { const char *name; int v; } tbl[] = {...};
   compiles alone, but as soon as the program has a second unit (here
   28-anon-struct-static-multiunit-other.c) the build is rejected with
   "reject: not covered: multi-unit static declarator" (no file:line).
   Named struct types and plain static ints are fine.  Hit by sbase tr.c
   (static struct { char *name; int (*check)(Rune); } classes[]). */
#include <stdio.h>
static struct { const char *name; int v; } tbl[] = { { "a", 1 }, { "b", 2 } };
int other(void);
int main(void) { printf("%s %d\n", tbl[1].name, tbl[1].v + other()); return 0; }
