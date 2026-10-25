/* C99 6.7.5.2 variable-length arrays.  Their storage cannot be in the fixed
   frame, so it comes off the tape stack where the declaration is; the
   enclosing block puts the stack pointer back when it ends, and so do
   `break` and `continue`, or a loop would eat the stack.  `sizeof` on one is
   a runtime load.  `int a[1 && 1]` is NOT one -- which is why the constant
   folder has to know the whole conditional ladder. */
#include <stdio.h>

int take(int n, int a[n]) { return a[n - 1]; }

int loop(int n) {
    int i, t;
    t = 0;
    for (i = 1; i <= n; i++) {
        char buf[i * 8];
        int k;
        for (k = 0; k < i * 8; k++) buf[k] = (char) k;
        t += buf[i] + (int) sizeof buf;
        if (i == 3) continue;
        if (i == 5) break;
    }
    return t;
}

int churn(int n) {
    int i, t;
    t = 0;
    for (i = 0; i < n; i++) { int v[64]; v[0] = i; t += v[0]; }
    for (i = 0; i < n; i++) { int v[n]; v[0] = i; t += v[0]; }
    return t;
}

int main(void) {
    int n = 5;
    int fixed[1 && 1];
    int cond[1 ? 3 : 9];
    int a[n];
    int i;
    for (i = 0; i < n; i++) a[i] = i * 3;
    fixed[0] = 7;
    cond[2] = 8;
    printf("%d %d %d\n", (int) sizeof a, take(n, a), loop(7));
    printf("%d %d %d %d\n", fixed[0], cond[2], (int) sizeof cond, churn(2000));
    return 0;
}
