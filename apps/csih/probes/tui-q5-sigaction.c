/*
 * tui-q5-sigaction.c — TUI-INTERFACE.md §5.5, the route that does NOT work.
 *
 * EXPECTED TO BE RED UNDER UNISACC. That red is the answer, not a broken probe:
 * `struct sigaction` is an incomplete struct to unisacc, so it cannot be
 * declared at all, and the terminal-restore path has to use signal() instead.
 * This file is kept ONLY so that fact stays reproducible — when unisacc grows
 * the struct, this probe turns green and whoever sees it knows the constraint
 * in term.c can be revisited.
 *
 * Being its own file is mandatory: while this shared a file with the signal()
 * probe, this construct's compile-time rejection made that probe red too.
 *
 * CLI takes SUBCOMMANDS, not dash-options (unisacc reserves the dashes).
 */
#include <stdio.h>
#include <string.h>
#include <signal.h>

static volatile sig_atomic_t got = 0;
static void on_int(int s) { (void)s; got = 1; }

int main(int argc, char **argv) {
    struct sigaction sa;
    const char *q = argc > 1 ? argv[1] : "run";
    if (q[0] != 'r') { printf("usage: tui-q5-sigaction run\n"); return 64; }
    got = 0;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_int;
    if (sigaction(SIGINT, &sa, NULL) != 0) { printf("sigaction() FAILED to install\n"); return 1; }
    raise(SIGINT);
    printf("sigaction() handler ran: %d\n", (int)got);
    return got == 1 ? 0 : 1;
}
