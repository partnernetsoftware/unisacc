/*
 * render.c — turn state into a screen, as a pure function.
 *
 * WHY THIS IS WRITTEN BEFORE termios EXISTS: a TUI's hardest part to test is
 * the part that talks to a terminal. The way out is not to test harder but to
 * make the talking unnecessary — `render()` takes state and returns a string,
 * with no tty anywhere in sight. Measured consequence: the self-test in render_cli.c
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
#include "csih_cols.h"

#define r_next_cp csih_next_cp
#define r_cp_cols csih_cp_cols

/* ── the state a screen renders from ────────────────────────────────────── */
/* r_state and r_frame are declared in render_api.h — including it here means
 * there is exactly one definition of each, so a change cannot land in the
 * header and silently not apply to the implementation. */

void r_frame_init(r_frame *f) { f->n = 0; memset(f->lines, 0, sizeof(f->lines)); }

/* Copy whole codepoints only. max_cols < 0 means no column cap.
 * Column width comes from csih_cols.h, the same measure tui.c uses. */
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

/* unisacc setenv() stays in the file that called it. render_cli.c cannot
 * change this file's getenv, so the self-test asks this function to do it. */
void render_test_home(const char *home) {
    if (home) setenv("HOME", home, 1);
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

/* render_cli.c owns the self-test. */
