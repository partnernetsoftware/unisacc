/*
 * tui-terms.c — the R18-10 questions from TUI-INTERFACE.md §5 that build on
 * BOTH backends.
 *
 * WHY q5 IS NOT HERE — this file's history is the reason, and it matters:
 * the first version put all five questions in one file, and question 5 used
 * `struct sigaction`. unisacc rejects that struct at COMPILE time, so the file
 * never built, and ALL FIVE questions became unanswerable at once. One
 * unsupported construct took the other four answers down with it — and the four
 * it took down were the ones that worked.
 *
 * So the rule for this directory is: a probe that a backend might reject lives
 * in its OWN FILE. `struct sigaction` is such a construct; it is in tui-q5.c.
 * Do not merge it back in — merging is what broke it the first time.
 *
 * VERIFIED BY: probes/tui-run.sh, which runs every tui-q*.c against unisacc and
 * gcc and reports per-question, so one red question cannot hide the others.
 *
 * CLI takes SUBCOMMANDS, not dash-options (unisacc reserves the dashes).
 */
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <poll.h>

int main(int argc, char **argv) {
    const char *q = argc > 1 ? argv[1] : "";
    struct termios t;

    if (!strcmp(q, "q1-cfmakeraw")) {
        tcgetattr(0, &t);
        cfmakeraw(&t);
        printf("q1 cfmakeraw: callable, c_lflag changed\n");
        return 0;
    }
    if (!strcmp(q, "q2-layout")) {
        printf("q2 sizeof=%d lflag@%d cc@%d ispeed@%d isatty=%d\n",
               (int)sizeof(struct termios),
               (int)((char *)&t.c_lflag - (char *)&t),
               (int)((char *)&t.c_cc - (char *)&t),
               (int)((char *)&t.c_ispeed - (char *)&t),
               isatty(0));
        return 0;
    }
    if (!strcmp(q, "q3-winsize")) {
        /* Fill with 0xAA FIRST, so the output proves whether ioctl wrote
         * anything at all. That is how the 43690 mine was found: ioctl failed
         * and left the caller's garbage in place, so a program that trusts the
         * struct renders a 43690-column frame. */
        struct winsize ws;
        int rc;
        memset(&ws, 0xAA, sizeof ws);
        rc = ioctl(1, TIOCGWINSZ, &ws);
        printf("q3 ioctl rc=%d rows=%u cols=%u\n",
               rc, (unsigned)ws.ws_row, (unsigned)ws.ws_col);
        return 0;
    }
    if (!strcmp(q, "q4-poll")) {
        struct pollfd p;
        int rc;
        p.fd = 0; p.events = POLLIN; p.revents = 0;
        rc = poll(&p, 1, 5);
        printf("q4 poll(5ms) rc=%d revents=0x%x\n", rc, (unsigned)p.revents);
        p.fd = 1; p.events = POLLOUT; p.revents = 0;
        rc = poll(&p, 1, 0);
        printf("q4 poll(stdout,0) rc=%d POLLOUT=%s\n", rc, (p.revents & POLLOUT) ? "set" : "clear");
        return 0;
    }
    printf("usage: tui-terms q1-cfmakeraw|q2-layout|q3-winsize|q4-poll\n");
    printf("q5 (signals) is in tui-q5.c: struct sigaction does not compile under\n");
    printf("unisacc, so keeping it in this file would break q1..q4 as well.\n");
    return 64;
}
