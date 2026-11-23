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

struct Big {
    long a[64];
};

static int helper_big(struct Big b, int n) {
    int i;
    long acc;
    acc = 0;
    for (i = 0; i < n; i = i + 1) {
        if (i == 1) goto next_it;
        acc = acc + b.a[i];
    next_it:
        ;
    }
    return (int)acc;
}

int main(void) {
    char *s = strdup("hi");
    struct Big b;
    int i;
    int v;
    if (!s) return 1;
    for (i = 0; i < 64; i = i + 1) b.a[i] = i + 1;
    v = helper_branch(4) + helper_big(b, 4);
    if (v != 12) return 2;
    puts(s);
    return 0;
}
