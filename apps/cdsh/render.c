/*
 * render.c — turn state into a screen, as a pure function.
 *
 * WHY THIS IS WRITTEN BEFORE termios EXISTS: a TUI's hardest part to test is
 * the part that talks to a terminal. The way out is not to test harder but to
 * make the talking unnecessary — `render()` takes state and returns a string,
 * with no tty anywhere in sight. Measured consequence: this file's self-test
 * runs under a pipe, under a redirect, in CI, and on six targets, because it
 * never asks what kind of terminal it is on.
 *
 * WHAT IT IS NOT: a reconciler. The JS side uses Ink/React, which diffs a
 * virtual tree; copying that brings React's whole complexity along for a UI
 * that has a handful of screens. Comparing rendered LINES as strings does the
 * same job at this size, and `frame_diff()` below is the entire mechanism.
 *
 * CLI takes SUBCOMMANDS, not dash-options (unisacc reserves the dashes).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render_api.h"   /* the interface lives in ONE place; see that file */

/* ── the state a screen renders from ────────────────────────────────────── */
/* r_state and r_frame are declared in render_api.h — including it here means
 * there is exactly one definition of each, so a change cannot land in the
 * header and silently not apply to the implementation. */

void r_frame_init(r_frame *f) { f->n = 0; memset(f->lines, 0, sizeof(f->lines)); }

/* Column width. Same ranges as tui_cp_cols: CJK and the box-drawing rule are
 * 2 and 1. A byte count is not a column count. */
static int r_next_cp(const char *s, unsigned int *cp) {
    unsigned char c;
    int need = 1, i;
    if (!s || !s[0]) return 0;
    c = (unsigned char)s[0];
    if (c < 0x80) { *cp = c; return 1; }
    if ((c & 0xe0) == 0xc0) need = 2;
    else if ((c & 0xf0) == 0xe0) need = 3;
    else if ((c & 0xf8) == 0xf0) need = 4;
    else return 0;
    *cp = c & (0xff >> (need + 1));
    for (i = 1; i < need; i++) {
        unsigned char d = (unsigned char)s[i];
        if ((d & 0xc0) != 0x80) return 0;
        *cp = (*cp << 6) | (d & 0x3f);
    }
    return need;
}

static int r_cp_cols(unsigned int cp) {
    if (cp < 0x1100) return 1;
    if (cp <= 0x115f) return 2;
    if (cp >= 0x2e80 && cp <= 0xa4cf) return 2;
    if (cp >= 0xac00 && cp <= 0xd7a3) return 2;
    if (cp >= 0xf900 && cp <= 0xfaff) return 2;
    if (cp >= 0xfe10 && cp <= 0xfe6f) return 2;
    if (cp >= 0xff00 && cp <= 0xff60) return 2;
    if (cp >= 0xffe0 && cp <= 0xffe6) return 2;
    if (cp >= 0x1f300 && cp <= 0x1f9ff) return 2;
    return 1;
}

/* Copy whole codepoints only. max_cols < 0 means no column cap. */
static int r_copy_fit(char *dst, int dstmax, const char *s, int max_cols, int *out_cols) {
    int n = 0, w = 0;
    if (!s) s = "";
    while (*s && n < dstmax) {
        unsigned int cp = 0;
        int need = r_next_cp(s, &cp);
        int cw;
        if (need <= 0) break;
        cw = r_cp_cols(cp);
        if (n + need > dstmax) break;
        if (max_cols >= 0 && w + cw > max_cols) break;
        memcpy(dst + n, s, (size_t)need);
        n += need;
        w += cw;
        s += need;
    }
    dst[n] = '\0';
    if (out_cols) *out_cols = w;
    return n;
}

static void r_put(r_frame *f, const char *s) {
    if (f->n >= R_MAX_LINES) return;
    if (!s) return;
    r_copy_fit(f->lines[f->n], R_LINE_MAX, s, -1, 0);
    f->n++;
}

/* Username from $HOME when it is /Users/<name> or /home/<name>. */
static int r_home_user(char *user, int n) {
    const char *home = getenv("HOME");
    const char *p;
    int len;
    if (!home) return 0;
    if (!strncmp(home, "/Users/", 7)) p = home + 7;
    else if (!strncmp(home, "/home/", 6)) p = home + 6;
    else return 0;
    while (*p == '/') p++;
    len = 0;
    while (p[len] && p[len] != '/' && len + 1 < n) {
        user[len] = p[len];
        len++;
    }
    if (len < 1 || (p[len] && p[len] != '/')) return 0;
    user[len] = '\0';
    return 1;
}

/* Replace /Users/<user> and /home/<user> with ~. A following slash stays, so
 * the screen shows ~/. A longer name such as <user>2 is left alone. */
static void r_hide_user(char *s) {
    char user[96];
    char pat[2][128];
    int pi;
    if (!s || !r_home_user(user, (int)sizeof user)) return;
    snprintf(pat[0], sizeof pat[0], "/Users/%s", user);
    snprintf(pat[1], sizeof pat[1], "/home/%s", user);
    for (pi = 0; pi < 2; pi++) {
        int plen = (int)strlen(pat[pi]);
        char *p = s;
        if (plen < 2) continue;
        while ((p = strstr(p, pat[pi])) != NULL) {
            char next = p[plen];
            /* '/' keeps the slash (so ~/...). End, space, or punctuation ends the path. */
            if (next != '\0' && next != '/'
                && ((next >= 'a' && next <= 'z') || (next >= 'A' && next <= 'Z')
                    || (next >= '0' && next <= '9') || next == '_' || next == '.' || next == '-')) {
                p += plen;
                continue;
            }
            memmove(p + 1, p + plen, strlen(p + plen) + 1);
            *p = '~';
            p += 1;
        }
    }
}

/* Pad to `width` terminal columns so a redraw covers the previous tail.
 * Padding by bytes cuts a ─ rule and a CJK line in half, and the next column
 * keeps the old glyph. Home paths are folded here, on the way to the glass,
 * so a cwd= line cannot print the account name. */
static void r_put_padded(r_frame *f, const char *s, int width) {
    char raw[R_LINE_MAX + 1];
    char buf[R_LINE_MAX + 1];
    int n, w;
    if (width > R_MAX_COLS) width = R_MAX_COLS;
    if (width < 0) width = 0;
    r_copy_fit(raw, R_LINE_MAX, s ? s : "", -1, 0);
    r_hide_user(raw);
    n = r_copy_fit(buf, R_LINE_MAX, raw, width, &w);
    while (w < width && n < R_LINE_MAX) {
        buf[n++] = ' ';
        w++;
    }
    buf[n] = '\0';
    r_put(f, buf);
}

r_frame render(const r_state *st) {
    r_frame f;
    int i;
    r_frame_init(&f);
    if (st->title) r_put_padded(&f, st->title, st->width);
    for (i = 0; i < st->nbody && i < 64; i++) r_put_padded(&f, st->body[i], st->width);
    if (st->status) r_put_padded(&f, st->status, st->width);
    return f;
}

/* ── the whole redraw mechanism ─────────────────────────────────────────── */

/*
 * Compare two frames and write the escape sequences that turn `prev` into
 * `next`. Returns the number of lines actually rewritten.
 *
 * This is deliberately the ONLY place that knows about ANSI. Everything above
 * is plain strings, so the logic that decides WHAT to draw stays testable, and
 * the part that cannot be tested without a terminal stays tiny.
 */
int frame_diff(const r_frame *prev, const r_frame *next, char *out, size_t outlen) {
    size_t used = 0;
    int rewritten = 0, i, lines;
    lines = next->n > prev->n ? next->n : prev->n;
    for (i = 0; i < lines; i++) {
        const char *a = i < prev->n ? prev->lines[i] : "";
        const char *b = i < next->n ? next->lines[i] : "";
        if (!strcmp(a, b)) continue;          /* unchanged: do not touch it */
        rewritten++;
        used += (size_t)snprintf(out + used, outlen > used ? outlen - used : 0,
                                 "\x1b[%d;1H%s\x1b[K", i + 1, b);
        if (used >= outlen) break;
    }
    return rewritten;
}

/* ── self-test ──────────────────────────────────────────────────────────── */

static int failures = 0;
static void expect(int cond, const char *what) {
    if (!cond) { printf("FAIL %s\n", what); failures++; }
}

static void run_selftest(void) {
    r_state st;
    r_frame a, b;
    char out[4096];

    memset(&st, 0, sizeof(st));
    st.title = "SGI";
    st.body[0] = "line one";
    st.body[1] = "line two";
    st.nbody = 2;
    st.status = "ready";
    st.width = 20;

    a = render(&st);
    expect(a.n == 4, "title + 2 body + status = 4 lines");
    expect((int)strlen(a.lines[0]) == 20, "every line is padded to the width");
    expect(!strcmp(a.lines[1], "line one            "), "body line is padded");

    /* Rendering is deterministic: same state, same frame. Without this the
     * diff below would rewrite the world on every tick. */
    b = render(&st);
    expect(a.n == b.n && !strcmp(a.lines[0], b.lines[0]), "render is deterministic");

    /* The point of the diff: an unchanged frame writes NOTHING. */
    expect(frame_diff(&a, &b, out, sizeof(out)) == 0, "identical frames rewrite nothing");

    /* Change one body line: exactly one line is rewritten. */
    st.body[1] = "CHANGED";
    b = render(&st);
    expect(frame_diff(&a, &b, out, sizeof(out)) == 1, "one changed line rewrites one line");
    expect(strstr(out, "CHANGED") != NULL, "the change is in the output");
    expect(strstr(out, "line one") == NULL, "unchanged lines are not re-emitted");

    /* Shrinking must not leave stale text: the shortened frame pads to width. */
    st.nbody = 1;
    b = render(&st);
    expect((int)strlen(b.lines[1]) == 20, "a shorter frame still pads to width");

    /* Empty state must not crash or emit garbage. */
    memset(&st, 0, sizeof(st));
    st.width = 10;
    b = render(&st);
    expect(b.n == 0, "an empty state renders zero lines");

    /* The account name must not reach the glass. Both home layouts fold. */
    setenv("HOME", "/Users/cdshuser", 1);
    memset(&st, 0, sizeof st);
    st.width = 48;
    st.title = "cwd=/Users/cdshuser/repos /home/cdshuser/x end=/Users/cdshuser";
    b = render(&st);
    expect(b.n == 1 && strstr(b.lines[0], "cwd=~/repos")
           && strstr(b.lines[0], "~/x") && strstr(b.lines[0], "end=~"),
           "home paths display as ~/");
    expect(!strstr(b.lines[0], "cdshuser"), "username stays off the frame");
    st.title = "keep /Users/cdshuser2/no";
    b = render(&st);
    expect(strstr(b.lines[0], "/Users/cdshuser2/no"),
           "a longer username is not the home user");
    setenv("HOME", "/home/cdshuser", 1);
    st.title = "cwd=/home/cdshuser/repos";
    b = render(&st);
    expect(strstr(b.lines[0], "cwd=~/repos") && !strstr(b.lines[0], "cdshuser"),
           "/home/<user> displays as ~/");
}

/* The entry point moved to render_cli.c so render.c can be a LIBRARY. The
 * self-test stays here, as a non-static function, because it tests this file's
 * internals and because that is the pattern gate.c/json.c/session.c follow. */
int render_run_selftest(void) {
    run_selftest();
    printf("%s\n", failures ? "SELFTEST FAILED" : "selftest ok");
    return failures == 0 ? 0 : 1;
}
