#include <string.h>
#include <stdio.h>

static int helper_loop(int n) {
    int i;
    int s;
    s = 0;
    for (i = 0; i < n; i = i + 1) {
        if (i == 2) goto skip;
        s = s + i;
    skip:
        ;
    }
    return s;
}

static int helper_branch(int x) {
    if (x > 0) return helper_loop(x);
    return 0;
}

int main(void) {
    char *s = strdup("hi");
    int v;
    if (!s) return 1;
    v = helper_branch(4);
    if (v != 4) return 2;
    puts(s);
    return 0;
}
