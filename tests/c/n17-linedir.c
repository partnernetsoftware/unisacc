/* R14-3: #line N ["file"] (C99 6.10.4) changes what __LINE__ and __FILE__
   report for the lines that follow it, and so what diagnostics say. */
#include <stdio.h>
int a = __LINE__;
#line 100
int b = __LINE__;
int c = __LINE__;
#line 200 "renamed.c"
const char *f = __FILE__;
int d = __LINE__;
#define L __LINE__
int e = L;
int main(void) {
    printf("%d %d %d %s %d %d %d\n", a, b, c, f, d, e, __LINE__);
    return 0;
}
