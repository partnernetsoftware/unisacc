/*
 * term_cli.c — the `main` for term.c, moved out so term.c is a library.
 *
 * Same reason as render_cli.c: each module used to carry its own `main`, and
 * linking them together produced `duplicate symbol '_main'`. The rest of csih
 * already split library from CLI (gate/json/session); render/term/tui had not,
 * because until now nothing needed them combined.
 *
 * This file OWNS the stdio, exactly as gate_cli.c does, so term.c stays a
 * library with no `FILE *` in its entry points. An earlier note blamed a
 * cross-file `FILE *` prototype for mis-lowering printf; unisacc 0.0.23 does not
 * do that (measured 2026-10-04), so the claim is gone.
 *
 * CLI takes SUBCOMMANDS, not dash-options (unisacc reserves the dashes).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "term_api.h"

int main(int argc, char **argv) {
    const char *cmd = argc > 1 ? argv[1] : "";
    if (!strcmp(cmd, "selftest")) return term_run_selftest();
    if (!strcmp(cmd, "size")) {
        term_size_t s = term_size();
        printf("cols=%d rows=%d source=%s\n", s.cols, s.rows, s.source ? "tty" : "fallback");
        return 0;
    }
    if (!strcmp(cmd, "wait")) {
        int ms = argc > 2 ? atoi(argv[2]) : 100;
        printf("wait %dms -> %d\n", ms, term_wait_readable(ms));
        return 0;
    }
    if (!strcmp(cmd, "parse")) {
        char buf[32];
        int  n = 0, i;
        const char *s = argc > 2 ? argv[2] : "";
        for (i = 0; s[i] && n < (int)sizeof buf; i++) {
            if (s[i] == '\\' && s[i + 1]) {
                i++;
                if (s[i] == 'e') buf[n++] = 0x1b;
                else if (s[i] == 'r') buf[n++] = '\r';
                else if (s[i] == 'n') buf[n++] = '\n';
                else buf[n++] = s[i];
            } else {
                buf[n++] = s[i];
            }
        }
        {
            term_key_t k = term_parse_key(buf, n);
            printf("kind=%d bytes=%d nraw=%d\n", (int)k.kind, n, k.nraw);
        }
        return 0;
    }
    printf("usage: term_cli selftest | size | wait <ms> | parse <escaped-bytes>\n");
    return 64;
}
