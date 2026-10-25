/* W3 (D2): the process lifecycle one program can show -- exit statuses,
   atexit order, unflushed output at exit, abort/raise, a handled signal and
   a real crash.  The case is argv[1]; tests/lifecycle.sh runs every case
   built by cc, the C reference and (if given) the product, and compares the
   status and output the shell sees. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
static void h1(void) { printf("h1\n"); }
static void h2(void) { printf("h2\n"); }
static volatile int seen;
static void onterm(int s) { seen = s; }
int main(int argc, char **argv) {
    const char *c = argc > 1 ? argv[1] : "ret0";
    if (!strcmp(c, "ret0")) return 0;
    if (!strcmp(c, "ret7")) return 7;
    if (!strcmp(c, "ret255")) return 255;
    if (!strcmp(c, "ret256")) return 256;
    if (!strcmp(c, "exit3")) exit(3);
    if (!strcmp(c, "atexit")) { atexit(h1); atexit(h2); printf("main\n"); return 5; }
    if (!strcmp(c, "pending")) { printf("no newline"); exit(4); }
    if (!strcmp(c, "abort")) { printf("before\n"); fflush(stdout); abort(); }
    if (!strcmp(c, "raise")) { fflush(stdout); raise(SIGTERM); return 9; }
    if (!strcmp(c, "handled")) { signal(SIGTERM, onterm); raise(SIGTERM); printf("seen %d\n", seen); return 0; }
    if (!strcmp(c, "segv")) { fflush(stdout); *(volatile int *)0 = 1; return 8; }
    printf("unknown case %s\n", c);
    return 2;
}
