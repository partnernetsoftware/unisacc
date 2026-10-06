/*
 * tui-q5-signal.c — TUI-INTERFACE.md §5.5, the route that WORKS.
 *
 * WHY ITS OWN FILE: this began life as a branch inside tui-q5.c, next to a
 * `sigaction` branch. unisacc compiles the whole file, so the sigaction branch
 * — a construct unisacc rejects outright — took THIS branch down with it even
 * though it is never called. The probe went red for a reason that had nothing
 * to do with signals.
 *
 * That is the same failure that started this directory's one-file-one-question
 * rule, which makes it worth stating plainly: `#if`-style reasoning ("it is not
 * even called") does not apply to a compiler that rejects at parse time. If one
 * backend may refuse a construct, the construct gets its own file. No merging.
 *
 * CLI takes SUBCOMMANDS, not dash-options (unisacc reserves the dashes).
 */
#include <stdio.h>
#include <signal.h>

static volatile sig_atomic_t got = 0;
static void on_int(int s) { (void)s; got = 1; }

int main(int argc, char **argv) {
    const char *q = argc > 1 ? argv[1] : "run";
    if (q[0] != 'r') { printf("usage: tui-q5-signal run\n"); return 64; }
    got = 0;
    if (signal(SIGINT, on_int) == SIG_ERR) { printf("signal() FAILED to install\n"); return 1; }
    raise(SIGINT);
    printf("signal() handler ran: %d\n", (int)got);
    return got == 1 ? 0 : 1;
}
