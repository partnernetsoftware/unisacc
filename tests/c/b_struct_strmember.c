/* R17-8: a character-array MEMBER initialised by a bare string literal (C99 6.7.8p14),
   global, array-of-struct, local and designated.  The reference stored the
   literal's ADDRESS into the first byte (cdx probes tests/r17probes/structarg_*.c). */
#include <stdio.h>
struct a { char x[3]; int n; char y[4]; };
struct a g = { "ab", 7, "xyz" };
struct a h[2] = { { "p", 1, "q" }, { "rs", 2, "tuv" } };
int main(void) {
    struct a l = { "cd", 9, "uvw" };
    struct a m = { .y = "zz", .x = "k" };
    printf("%s %d %s|%s %d %s|%s %s %s %s|%s %d %s\n", g.x, g.n, g.y, l.x, l.n, l.y, h[0].x, h[0].y, h[1].x, h[1].y, m.x, m.n, m.y);
    return 0;
}
