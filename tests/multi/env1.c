/* 0.0.31 H1'': one environ for the whole program.  This unit assigns it; env2.c's getenv must
   read the new array (C99 6.9.2 tentative definition merged across units, as cc links it). */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
const char *peek(void);
static char *mine[] = { "H1PROBE=from-env1", 0 };
int main(void) {
    printf("before %s\n", getenv("H1PROBE") ? getenv("H1PROBE") : "(none)");
    environ = mine;
    printf("env2 sees %s\n", peek());
    return 0;
}
