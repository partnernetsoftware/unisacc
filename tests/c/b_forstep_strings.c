#include <stdio.h>
#include <string.h>
static int stop(const char *s) { puts(s); return 0; }
int main(void) {
    int i, j;
    char b[] = "aa bb", out[32];
    char *w;
    for (w = strtok(b, " "); w; w = strtok(0, " ")) {
        sprintf(out, " -%s", w);
        puts(out);
    }
    for (i = 1; i; i = stop("outer-step")) {
        puts("outer-body");
        for (j = 1; j; j = stop("inner-step")) puts("inner-body");
        puts("outer-tail");
    }
    for (i = 1; i; printf("step-%s\n", "adj" "acent"), i = 0)
        printf("body-%s\n", __func__);
    for (i = 1; i; (void)sizeof("not-emitted"), i = 0) puts("sizeof-body");
    for (i = 1; i;) { puts("empty-step"); i = 0; }
    for (i = 1; i; i = stop("bare-outer-step"))
        for (j = 1; j; j = stop("bare-inner-step")) puts("bare-body");
    return 0;
}
