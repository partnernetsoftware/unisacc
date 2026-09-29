/* R13-0b: external trial of 0.0.12, real-world batch.  Source: ~/unisacc-feedback-0.0.12-realworld/16-assign-through-deref-as-expr.c (+ .out.txt for the gcc-vs-unisacc record). */
/* An assignment whose left side is a dereference (*p = v) cannot be used
   as a sub-expression: `(*p = 5)` in a condition or initializer, chained
   `r = *p = 5`, and the classic copy loop `while ((*d++ = *s++))` are all
   rejected ("expected ')'" / "expected ';'").  As a statement *p = 5; works,
   and (a[1] = 5) works.  Hit by Lua (lvm.h: (*(&i1) = x, 1)). */
#include <stdio.h>
int main(void) {
    long i1, r, *p = &i1;
    char buf[8], *d = buf;
    const char *s = "copy";
    r = (*p = 5);
    if ((*p = 6)) r = r + *p;
    while ((*d++ = *s++))
        ;
    printf("%ld %ld %s\n", i1, r, buf);
    return 0;
}
