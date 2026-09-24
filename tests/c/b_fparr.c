/* A file-scope array of function pointers, stored into and called through
   -- `static void (*tab[4])(void)`.  The local form had long worked; the
   global one was laid out as ONE 8-byte scalar indexed by byte, and the
   first program to write one was <stdlib.h>'s atexit table [S-15 D2]. */
#include <stdio.h>
static void a(void) { printf("a\n"); }
static void b(void) { printf("b\n"); }
static void (*tab[4])(void);
static int n = 0;
static int reg(void (*f)(void)) { tab[n] = f; n = n + 1; return 0; }
int main(void)
{
    void (*p)(void);
    reg(a); reg(b);
    p = tab[1]; p();
    tab[0]();
    while (n > 0) { n = n - 1; tab[n](); }
    return 0;
}
