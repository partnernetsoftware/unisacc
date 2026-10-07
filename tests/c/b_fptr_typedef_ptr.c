/* 0.0.34 L2 (7b4ee2cf): a pointer to a function-pointer typedef called through (*q)() and (**q)() */
#include <stdio.h>
static int implA(const char *s, int n) { return n + 1; }
static int (*const finderA)(const char *, int) = implA;
static int (*finderB)(const char *, int) = implA;
typedef int (*finder_type)(const char *, int);
struct V { int a; void *app; };
static struct V vs[] = { { 1, (void *)&finderA } };
int main(void) {
    finder_type *q = (finder_type *)vs[0].app; finder_type *qb = &finderB; void *ap = (void *)&finderA;
    printf("%d %d %d %d %d\n", (*q)("x", 2), (**q)("x", 3), (*qb)("x", 4),
           (**(finder_type *)ap)("x", 5), (**(finder_type *)vs[0].app)("x", 6));
    return 0;
}
