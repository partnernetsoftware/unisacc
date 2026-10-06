/*
 * term.c — the thin layer that talks to a terminal, and nothing else.
 *
 * WHY SO THIN: every line here is a line that cannot be tested without a tty.
 * TUI-INTERFACE.md §2 put the split this way — render() is a pure function and
 * frame_diff() is the only place that knows ANSI, so this file holds ONLY the
 * five syscalls that genuinely need a terminal: raw mode in, raw mode out,
 * window size, poll-for-input, and the signal that puts the terminal back.
 * Everything that could be decided without a terminal was decided elsewhere.
 *
 * THE FIVE R18-10 QUESTIONS WERE ANSWERED BY MEASUREMENT, NOT ASSUMED
 * (probes/tui-terms.c, both backends; results in probes/STATUS.md):
 *
 *   q1  cfmakeraw           EXISTS. Called it; c_lflag changed. So raw mode is
 *                           one call, not a hand-rolled flag dance — and the
 *                           hand-rolled version would have been wrong on one of
 *                           the three targets.
 *   q2  struct termios      sizeof=72, and c_iflag/oflag/cflag/lflag/cc/ispeed/
 *                           ospeed sit at the SAME byte offsets under unisacc
 *                           and gcc. So NO per-target layout fork is needed.
 *                           (TUI-INTERFACE.md §5.2 said "must verify" — verified,
 *                           and the answer was the boring one.)
 *   q3  TIOCGWINSZ off-tty  rc=-1 AND THE BUFFER IS NOT TOUCHED: a struct
 *                           pre-filled with 0xAA still read 43690/43690
 *                           afterwards. THIS IS A LIVE MINE. 43690 is not a
 *                           crash, it is a frame 43690 columns wide — a
 *                           corrupted screen that looks like a rendering bug
 *                           and is actually a missing memset. Hence
 *                           term_size() zeroes the struct and refuses any
 *                           value outside a sane range.
 *   q4  poll timeout         milliseconds, and POLLIN/POLLOUT behave the same
 *                           on both backends. So the redraw timer is just the
 *                           poll timeout — no separate clock.
 *   q5  SIGINT restore      signal() installs a handler and it really runs
 *                           (raise → handler ran 1). struct sigaction does NOT:
 *                           "not covered: incomplete struct". So the restore
 *                           path uses signal(), and a later move to sigaction
 *                           has to wait for that struct.
 *
 * WHY term_size() DOES NOT TRUST rc ALONE: a caller that only checks the
 * return code still has a struct full of garbage to render from. The guard has
 * to be on the VALUE, so this returns a fallback size rather than a failure the
 * caller might ignore.
 *
 * CLI takes SUBCOMMANDS, not dash-options (unisacc reserves the dashes), and a
 * subcommand that needs a terminal reports UNAVAILABLE instead of failing — a
 * probe that is red on a pipe is indistinguishable from a probe that is broken.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <signal.h>
#include <errno.h>

#include "term_api.h"   /* the interface lives in ONE place; see that file */

/* ── raw mode ───────────────────────────────────────────────────────────── */

static struct termios term_saved;
static int            term_saved_ok = 0;
static volatile sig_atomic_t term_got_int = 0;

/* Not static: tui.c installs this as the exit handler, so it is part of the
 * module's surface and declared in term_api.h. A `static` here and a plain
 * declaration there is a link error waiting to happen. */
void term_on_signal(int sig) { (void)sig; term_got_int = 1; }

/*
 * Enter raw mode, remembering the previous settings so leave() can restore them
 * even if the program exits from a signal handler.
 *
 * Returns 0 on success, -1 if stdin is not a terminal — which is NOT an error
 * worth aborting on: csih must still run when its output is a pipe (that is how
 * check.sh tests it).
 */
int term_raw_enter(void) {
    struct termios raw;
    if (!isatty(0)) return -1;
    if (tcgetattr(0, &term_saved) != 0) return -1;
    term_saved_ok = 1;
    raw = term_saved;
    cfmakeraw(&raw);            /* q1: exists, so no flag dance */
    if (tcsetattr(0, TCSANOW, &raw) != 0) { term_saved_ok = 0; return -1; }
    /* SGR clicks. tmux with mouse on still delivers these once the pane asks. */
    if (isatty(1)) {
        const char on[] = "\033[?1000h\033[?1006h\033[?2004h";
        if (write(1, on, sizeof on - 1) < 0) { /* still in raw mode */ }
    }
    return 0;
}

/* Restore. Safe to call twice, and safe to call after a failed enter(). */
int term_raw_leave(void) {
    if (isatty(1)) {
        const char off[] = "\033[?1000l\033[?1006l\033[?2004l";
        if (write(1, off, sizeof off - 1) < 0) { /* restore anyway */ }
    }
    if (!term_saved_ok) return -1;
    term_saved_ok = 0;
    return tcsetattr(0, TCSANOW, &term_saved);
}

/* Install the restore-on-exit handler. signal(), not sigaction — see q5. */
int term_install_exit_handler(void (*fn)(int)) {
    return signal(SIGINT, fn ? fn : term_on_signal) == SIG_ERR ? -1 : 0;
}
int term_interrupted(void) { return (int)term_got_int; }

/*
 * q3 in code: the buffer is zeroed FIRST, then a value outside a usable range is
 * rejected even when ioctl claimed success. 43690 has never been a terminal.
 */
term_size_t term_size(void) {
    struct winsize ws;
    term_size_t out;
    out.cols = TERM_FALLBACK_COLS;
    out.rows = TERM_FALLBACK_ROWS;
    out.source = 0;
    memset(&ws, 0, sizeof ws);                    /* the memset q3 demands */
    if (ioctl(1, TIOCGWINSZ, &ws) != 0) return out;
    if (ws.ws_col < TERM_MIN_COLS || ws.ws_row < TERM_MIN_ROWS) return out;
    out.cols = (int)ws.ws_col;
    out.rows = (int)ws.ws_row;
    out.source = 1;
    return out;
}

/* ── input ──────────────────────────────────────────────────────────────── */

/*
 * Wait up to `ms` for a byte on stdin. Returns 1 = byte ready, 0 = timeout,
 * -1 = stdin is gone.
 *
 * The whole redraw timer lives here: q4 says poll's timeout is milliseconds, so
 * "call this with the frame interval" IS the periodic tick. A separate timer
 * would be a second clock to keep honest.
 */
int term_wait_readable(int ms) {
    struct pollfd p;
    int rc;
    p.fd = 0;
    p.events = POLLIN;
    p.revents = 0;
    do {
        rc = poll(&p, 1, ms);
    } while (rc < 0 && errno == EINTR);
    if (rc < 0) return -1;
    if (rc == 0) return 0;
    if (p.revents & (POLLERR | POLLHUP | POLLNVAL)) return -1;
    return (p.revents & POLLIN) ? 1 : 0;
}

/*
 * Escape sequences arrive as several bytes. Reading one byte at a time would
 * turn an arrow key into three separate keypresses, so this reads what is
 * already buffered in one go and parses the common cases.
 *
 * Unrecognised sequences are returned as TERM_KEY_UNKNOWN with the raw bytes
 * kept in `raw`, rather than being dropped: silently eating a key is how a TUI
 * gets "unresponsive" bug reports that cannot be reproduced.
 */
static int term_read_some(char *buf, int cap) {
    int n = (int)read(0, buf, (size_t)cap);
    return n < 0 ? 0 : n;
}

/*
 * The parser, split out from the read ON PURPOSE.
 *
 * `read()` needs a terminal; parsing bytes does not. Keeping them in one
 * function would put the key table in the untestable half of the file — and the
 * key table is exactly where bugs hide (an arrow key read as three keypresses,
 * a bare ESC swallowed). Split, the table below can be driven with literal byte
 * strings from a self-test on any machine, tty or not.
 */
/* Read a decimal at *p, stopping before end. Advances *p past the digits. */
static int term_num(const char **p, const char *end, int *out) {
    int v = 0, any = 0;
    while (*p < end && **p >= '0' && **p <= '9') {
        v = v * 10 + (**p - '0');
        any = 1;
        (*p)++;
    }
    if (!any) return 0;
    *out = v;
    return 1;
}

term_key_t term_parse_key(const char *buf, int got) {
    term_key_t k;
    memset(&k, 0, sizeof k);
    k.kind = TERM_KEY_NONE;
    if (got <= 0) return k;
    /* ESC [ < btn ; x ; y M   left press is button 0 and ends in 'M'.
     * Wheel up is button 64, wheel down is 65. Release ('m') and other
     * buttons are not keys. A wheel must not be reported as a click. */
    if (got >= 6 && buf[0] == 0x1b && buf[1] == '[' && buf[2] == '<') {
        const char *p = buf + 3;
        const char *end = buf + got;
        int b = 0, x = 0, y = 0;
        if (term_num(&p, end, &b) && p < end && *p == ';') {
            p++;
            if (term_num(&p, end, &x) && p < end && *p == ';') {
                p++;
                if (term_num(&p, end, &y) && p < end && (*p == 'M' || *p == 'm')) {
                    if (*p == 'M' && x > 0 && y > 0 && (b == 0 || b == 64 || b == 65)) {
                        k.kind = (b == 0) ? TERM_KEY_MOUSE : TERM_KEY_WHEEL;
                        k.ch = (b == 64) ? 2 : (b == 65) ? 1 : 0;
                        k.x = x;
                        k.y = y;
                    }
                    return k;
                }
            }
        }
    }
    if (got > (int)sizeof k.raw - 1) got = (int)sizeof k.raw - 1;
    memcpy(k.raw, buf, (size_t)got);
    k.raw[got] = '\0';
    k.nraw = got;

    /* ESC [ <digits> ~  is one key: paste, page, home, or end. */
    if (got >= 4 && buf[0] == 0x1b && buf[1] == '[' && buf[got - 1] == '~') {
        const char *p = buf + 2;
        const char *end = buf + got - 1;
        int num = 0;
        if (term_num(&p, end, &num) && p == end) {
            if (num == 200) k.kind = TERM_KEY_PASTE_ON;
            else if (num == 201) k.kind = TERM_KEY_PASTE_OFF;
            else if (num == 5) k.kind = TERM_KEY_PGUP;
            else if (num == 6) k.kind = TERM_KEY_PGDN;
            else if (num == 1) k.kind = TERM_KEY_HOME;
            else if (num == 4) k.kind = TERM_KEY_END;
            else k.kind = TERM_KEY_UNKNOWN;
            return k;
        }
    }
    if (got == 1) {
        unsigned char c = (unsigned char)buf[0];
        if (c == 3)  { k.kind = TERM_KEY_CTRL_C;    return k; }
        if (c == 4)  { k.kind = TERM_KEY_CTRL_D;    return k; }
        if (c == 10 || c == 13) { k.kind = TERM_KEY_ENTER; return k; }
        if (c == 127 || c == 8) { k.kind = TERM_KEY_BACKSPACE; return k; }
        if (c == 0x1b) { k.kind = TERM_KEY_UNKNOWN; return k; }  /* bare ESC */
        k.kind = TERM_KEY_CHAR;
        k.ch = buf[0];
        return k;
    }
    if (got == 3 && buf[0] == 0x1b && buf[1] == '[') {
        switch (buf[2]) {
        case 'A': k.kind = TERM_KEY_UP;    return k;
        case 'B': k.kind = TERM_KEY_DOWN;  return k;
        case 'C': k.kind = TERM_KEY_RIGHT; return k;
        case 'D': k.kind = TERM_KEY_LEFT;  return k;
        case 'H': k.kind = TERM_KEY_HOME;  return k;
        case 'F': k.kind = TERM_KEY_END;   return k;
        default: break;
        }
    }
    k.kind = TERM_KEY_UNKNOWN;
    return k;
}

static const char *term_key_name(term_key_kind k) {
    switch (k) {
    case TERM_KEY_NONE:      return "none";
    case TERM_KEY_CTRL_C:    return "ctrl-c";
    case TERM_KEY_CTRL_D:    return "ctrl-d";
    case TERM_KEY_PASTE_ON:  return "paste-on";
    case TERM_KEY_PASTE_OFF: return "paste-off";
    case TERM_KEY_ENTER:     return "enter";
    case TERM_KEY_BACKSPACE: return "backspace";
    case TERM_KEY_UP:        return "up";
    case TERM_KEY_DOWN:      return "down";
    case TERM_KEY_RIGHT:     return "right";
    case TERM_KEY_LEFT:      return "left";
    case TERM_KEY_PGUP:      return "pgup";
    case TERM_KEY_PGDN:      return "pgdn";
    case TERM_KEY_HOME:      return "home";
    case TERM_KEY_END:       return "end";
    case TERM_KEY_CHAR:      return "char";
    case TERM_KEY_MOUSE:     return "mouse";
    case TERM_KEY_WHEEL:     return "wheel";
    default:                 return "unknown";
    }
}

/*
 * Read everything available and split it into keys, one entry per key.
 *
 * WHY THIS REPLACES term_read_key() IN THE LOOP, and the bug it fixes:
 * term_read_key() read up to 16 bytes and called term_parse_key() ONCE on that
 * whole buffer. term_parse_key's contract is "got == 1 means one byte, got == 3
 * means one escape sequence" — so a fifteen-byte read of `hello from tui\r`
 * matched neither branch and came back as a SINGLE TERM_KEY_UNKNOWN. Fourteen
 * characters of a typed sentence were destroyed by a parser being handed more
 * than one key's worth of input.
 *
 * The observable symptom was a transcript line reading
 *     {"role":"user","text":""}
 * after visibly typing a sentence: data loss that presents as an empty box.
 *
 * Parsing is per-key, so this feeds the parser one key's worth at a time:
 * a lone ESC is held back to see whether a sequence follows, every other byte
 * is one key. Bytes are never dropped — only the ARRIVAL granularity changes.
 *
 * Returns the number of keys written to `out`.
 */
int term_read_keys(term_key_t *out, int cap) {
    char buf[256];
    int  n, i, nk = 0;
    int  want;
    /* A half escape sequence is held here until the next call completes it.
     * Without this, `ESC [` split across two reads reaches the parser as
     * garbage and is emitted as UNKNOWN instead of an arrow key. */
    static char pend[32];
    static int  npend = 0;
    /* Read at most `cap` bytes. A larger read drops the tail: those bytes are
     * already taken from the tty, and this function only returns `cap` keys. */
    if (cap < 1) return 0;
    want = cap < (int)sizeof buf ? cap : (int)sizeof buf;
    n = term_read_some(buf, want);
    if (n <= 0 && npend == 0) return 0;
    /* Prepend any bytes left over from the previous call. */
    if (npend > 0) {
        char joined[260];
        int  j = 0, p;
        for (p = 0; p < npend && j < (int)sizeof joined; p++) joined[j++] = pend[p];
        for (p = 0; p < n && j < (int)sizeof joined; p++)       joined[j++] = buf[p];
        n = j;
        memcpy(buf, joined, (size_t)n);
        npend = 0;
    }
    for (i = 0; i < n && nk < cap; ) {
        if (buf[i] == 0x1b) {
            int avail = n - i;
            /* SGR mouse is `ESC [ < ... M`. Longer than an arrow, and it must
             * not be split into a 3-byte unknown plus leftover digits. */
            if (avail >= 3 && buf[i + 1] == '[' && buf[i + 2] == '<') {
                int kk, end = -1, lim = avail > 24 ? 24 : avail;
                for (kk = 3; kk < lim; kk++) {
                    char c = buf[i + kk];
                    if (c == 'M' || c == 'm') { end = i + kk; break; }
                }
                if (end < 0 && avail < 24) {
                    int p;
                    npend = 0;
                    for (p = i; p < n && npend < (int)sizeof pend; p++)
                        pend[npend++] = buf[p];
                    i = n;
                } else if (end < 0) {
                    out[nk++] = term_parse_key(&buf[i], 1);
                    i += 1;
                } else {
                    term_key_t mk = term_parse_key(&buf[i], end - i + 1);
                    if (mk.kind != TERM_KEY_NONE) out[nk++] = mk;
                    i = end + 1;
                }
            } else if (avail >= 3 && buf[i + 1] == '['
                       && buf[i + 2] >= '0' && buf[i + 2] <= '9') {
                /* ESC [ <digits> ~ : paste, page, home, end. Longer than an arrow. */
                int kk, end = -1, lim = avail > 8 ? 8 : avail;
                for (kk = 3; kk < lim; kk++)
                    if (buf[i + kk] == '~') { end = i + kk; break; }
                if (end < 0 && avail < 8) {
                    int p;
                    npend = 0;
                    for (p = i; p < n && npend < (int)sizeof pend; p++)
                        pend[npend++] = buf[p];
                    i = n;
                } else if (end < 0) {
                    out[nk++] = term_parse_key(&buf[i], 1);
                    i += 1;
                } else {
                    out[nk++] = term_parse_key(&buf[i], end - i + 1);
                    i = end + 1;
                }
            } else if (avail >= 3 && buf[i + 1] == '[') {
                /* `ESC [ X` is an arrow. Hold a short tail for the next read. */
                out[nk++] = term_parse_key(&buf[i], 3);
                i += 3;
            } else if (avail < 3 && avail == 2 && buf[i + 1] != '[') {
                /* `ESC x` (x not '[') is two complete keys: a bare ESC and
                 * the char. Only a trailing `ESC [` or lone `ESC` waits. */
                out[nk++] = term_parse_key(&buf[i], 1);
                out[nk++] = term_parse_key(&buf[i + 1], 1);
                i += 2;
            } else if (avail < 3) {
                int p;
                npend = 0;
                for (p = i; p < n && npend < (int)sizeof pend; p++)
                    pend[npend++] = buf[p];
                i = n;
            } else {
                out[nk++] = term_parse_key(&buf[i], 1);
                i += 1;
            }
        } else {
            out[nk++] = term_parse_key(&buf[i], 1);
            i += 1;
        }
    }
    return nk;
}

term_key_t term_read_key(void) {
    char buf[16];
    int  n, got;
    /* A local, then return it — unisacc rejects `return f()` when f returns a
     * struct ("unknown identifier"), so the one-line form is a portability bug
     * even though it is valid C99. */
    term_key_t k;
    n = term_read_some(buf, (int)sizeof buf);
    if (n <= 0) { memset(&k, 0, sizeof k); k.kind = TERM_KEY_NONE; return k; }
    got = n;
    /* Pull in the rest of an escape sequence that is still in flight. A human
     * cannot type the tail of ESC [ A within one poll tick, but a paste or a
     * fast terminal can, and a half-read sequence is a wrong key. */
    if (buf[0] == 0x1b && got < 3) {
        while (got < 3 && term_wait_readable(1) == 1) {
            int m = term_read_some(&buf[got], (int)sizeof buf - got);
            if (m <= 0) break;
            got += m;
        }
    }
    k = term_parse_key(buf, got);
    return k;
}

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
