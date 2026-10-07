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

/* term_cli.c owns the self-test. */
