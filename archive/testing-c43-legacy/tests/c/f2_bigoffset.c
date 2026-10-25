/* 0.0.28 F2: loads and stores far into a struct (offsets past 4 KB and 16 KB) at every width.  Before
   the copy loop these displacements came only from fully unrolled 20 KB struct copies.  cc prints
   1 2 3 4 5.5 6.25 7 8 9. */
#include <stdio.h>
struct Far { char pad[20000]; char c; short s; int i; long l; double d; float f; char tail[300]; long last; };
static struct Far g;
static long sum(struct Far *p) { return p->c + p->s + p->i + p->l + p->tail[299] + p->last; }
int main(void) {
    struct Far *p = &g; struct Far loc;
    p->c = 1; p->s = 2; p->i = 3; p->l = 4; p->d = 5.5; p->f = 6.25f; p->tail[299] = 7; p->last = 8;
    loc.pad[4100] = 9; loc.pad[16500] = loc.pad[4100];
    printf("%d %d %d %ld %g %g %d %ld %d\n", p->c, p->s, p->i, p->l, p->d, (double)p->f, p->tail[299], p->last, loc.pad[16500]);
    return sum(p) == 25 ? 0 : 1;
}
