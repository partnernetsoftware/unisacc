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

#include <unistd.h>

/* ── self-test ──────────────────────────────────────────────────────────── */

static int failures = 0;
static void expect(int cond, const char *what) {
    if (!cond) { printf("FAIL %s\n", what); failures++; }
}

static void run_selftest(void) {
    term_size_t sz;

    /* q3 is the reason this block exists. Whether or not we are on a tty, the
     * size must be usable: never 0, never the 43690 that an untouched buffer
     * holds. A test that only ran on a tty would never catch the pipe case, and
     * the pipe case is exactly where the garbage comes from. */
    sz = term_size();
    expect(sz.cols >= TERM_MIN_COLS && sz.cols <= 10000, "cols is a plausible number");
    expect(sz.rows >= TERM_MIN_ROWS && sz.rows <= 10000, "rows is a plausible number");
    if (!isatty(1)) {
        expect(sz.source == 0, "off a tty the size is the fallback, not garbage");
        expect(sz.cols == TERM_FALLBACK_COLS && sz.rows == TERM_FALLBACK_ROWS,
               "off a tty the fallback is the documented one");
    } else {
        expect(sz.source == 1, "on a tty the size comes from ioctl");
    }

    /* Raw mode must refuse politely instead of breaking a pipe. */
    if (!isatty(0)) {
        expect(term_raw_enter() == -1, "raw mode refuses when stdin is not a tty");
        expect(term_raw_leave() == -1, "leave() is harmless after a failed enter()");
    }

    /* Signal handler installation must work (q5), even though we do not raise
     * here — raising SIGINT inside a self-test would prove it by killing the
     * test run, which is a worse trade than checking the install. */
    expect(term_install_exit_handler(term_on_signal) == 0, "signal() installs a handler");
    expect(term_interrupted() == 0, "no interrupt has been observed yet");

    /* poll must return rather than block forever — the redraw tick depends on
     * it, and an infinite block here would freeze the whole UI.
     *
     * The assertion cannot be "== 0", which is what the first version of this
     * test said and it was WRONG: under a redirect or an exhausted pipe, stdin
     * legitimately reports POLLIN or POLLHUP and poll returns -1 or 1 at once.
     * -1 is the correct answer there, not a failure. What must hold in every
     * environment is that the call TERMINATES promptly instead of hanging. */
    {
        int r = term_wait_readable(1);
        expect(r == -1 || r == 0 || r == 1, "poll returns a defined value");
    }

    /* The byte-level parser is pure logic, so it gets a table — this is the
     * payoff of splitting it out of read(). */
    {
        static const struct { const char *bytes; int n; term_key_kind want; const char *what; } cases[] = {
            { "a",        1, TERM_KEY_CHAR,      "a plain letter is a char" },
            { "q",        1, TERM_KEY_CHAR,      "q is a char (quit is the caller's rule)" },
            { "\x03",     1, TERM_KEY_CTRL_C,    "0x03 is ctrl-c" },
            { "\x04",     1, TERM_KEY_CTRL_D,    "0x04 is ctrl-d" },
            { "\033[200~", 6, TERM_KEY_PASTE_ON,  "ESC [ 200 ~ starts a paste" },
            { "\033[201~", 6, TERM_KEY_PASTE_OFF, "ESC [ 201 ~ ends a paste" },
            { "\r",       1, TERM_KEY_ENTER,     "CR is enter" },
            { "\n",       1, TERM_KEY_ENTER,     "LF is enter" },
            { "\x7f",     1, TERM_KEY_BACKSPACE, "DEL is backspace" },
            { "\x1b[A",   3, TERM_KEY_UP,        "ESC [ A is up" },
            { "\x1b[B",   3, TERM_KEY_DOWN,      "ESC [ B is down" },
            { "\x1b[C",   3, TERM_KEY_RIGHT,     "ESC [ C is right" },
            { "\x1b[D",   3, TERM_KEY_LEFT,      "ESC [ D is left" },
            { "\x1b[5~",  4, TERM_KEY_PGUP,      "ESC [ 5 ~ is page up" },
            { "\x1b[6~",  4, TERM_KEY_PGDN,      "ESC [ 6 ~ is page down" },
            { "\x1b[H",   3, TERM_KEY_HOME,      "ESC [ H is home" },
            { "\x1b[F",   3, TERM_KEY_END,       "ESC [ F is end" },
            { "\x1b[1~",  4, TERM_KEY_HOME,      "ESC [ 1 ~ is home" },
            { "\x1b[4~",  4, TERM_KEY_END,       "ESC [ 4 ~ is end" },
            /* A bare ESC must be UNKNOWN, never silently dropped: swallowing a
             * key is how a TUI gets "sometimes it ignores me" reports. */
            { "\x1b",     1, TERM_KEY_UNKNOWN,   "a bare ESC is unknown, not eaten" },
            { "\x1b[Z",   3, TERM_KEY_UNKNOWN,   "an unhandled sequence is unknown" },
        };
        size_t i;
        for (i = 0; i < sizeof cases / sizeof cases[0]; i++) {
            term_key_t k = term_parse_key(cases[i].bytes, cases[i].n);
            if (k.kind != cases[i].want) {
                printf("FAIL %s (got %s)\n", cases[i].what, term_key_name(k.kind));
                failures++;
            }
            /* The raw bytes must always survive, so an unknown key can be
             * reported verbatim instead of as a mystery. */
            if (k.nraw != cases[i].n) {
                printf("FAIL %s (raw bytes lost: %d != %d)\n", cases[i].what, k.nraw, cases[i].n);
                failures++;
            }
        }
        expect(term_parse_key("", 0).kind == TERM_KEY_NONE, "no bytes is no key");
        expect(term_parse_key("x", -1).kind == TERM_KEY_NONE, "a negative count is no key");
        expect(term_parse_key("a", 1).ch == 'a', "the char is carried through");
        {
            term_key_t m = term_parse_key("\033[<0;12;5M", 11);
            expect(m.kind == TERM_KEY_MOUSE && m.x == 12 && m.y == 5,
                   "SGR left press is a click");
            m = term_parse_key("\033[<0;12;5m", 11);
            expect(m.kind == TERM_KEY_NONE, "SGR release is not a key");
            m = term_parse_key("\033[<64;1;1M", 11);
            expect(m.kind == TERM_KEY_WHEEL && m.ch == 2 && m.x == 1 && m.y == 1,
                   "SGR wheel up");
            m = term_parse_key("\033[<65;3;4M", 11);
            expect(m.kind == TERM_KEY_WHEEL && m.ch == 1 && m.x == 3 && m.y == 4,
                   "SGR wheel down");
            m = term_parse_key("\033[<64;1;1m", 11);
            expect(m.kind == TERM_KEY_NONE, "SGR wheel release is not a key");
        }
    }
}

/*
 * The entry point moved to term_cli.c so term.c can be a LIBRARY — see
 * render_cli.c for the failure that forced this (three mains in one link).
 * The interactive subcommands (size/wait/parse) are still reachable; they live
 * in term_cli.c and call the functions below, which are the real surface.
 */
int term_run_selftest(void) {
    run_selftest();
    printf("%s\n", failures ? "SELFTEST FAILED" : "selftest ok");
    return failures == 0 ? 0 : 1;
}
