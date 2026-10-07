/*
 * render_api.h — render.c's public surface, declared once so other modules can
 * use it WITHOUT `#include "render.c"`.
 *
 * WHY THIS FILE EXISTS (a real mistake, recorded so it is not repeated):
 * tui.c first did `#include "render.c"` to reach render() and frame_diff(). That
 * pulled in the whole translation unit — including render.c's `main`, its
 * static `expect`, and its `failures` counter — and collided with term.c's
 * identically-named copies. The build failed with "redefinition of 'main'".
 *
 * That is the same trap this project has hit from the other direction before:
 * csih cannot combine two files that each define `main`, which is why the
 * self-tests live in *_cli.c and why check.sh asserts the rule still holds.
 * Including a .c was simply the same rule broken by a different route.
 *
 * RULE: a module's self-test lives in its *_cli.c; a module's interface is
 * declared here. Never `#include` a .c file.
 */
#ifndef CDSH_RENDER_API_H
#define CDSH_RENDER_API_H

#define R_MAX_LINES   200
#define R_MAX_COLS    200
/* One column can be a 3-byte rule or a CJK character. The column cap stays
 * R_MAX_COLS; the byte cap has to be wide enough that a full-width line is
 * not cut in the middle of a codepoint. */
#define R_LINE_MAX    (R_MAX_COLS * 3)
#define R_FRAME_BYTES (R_MAX_LINES * (R_LINE_MAX + 8))

/* The state a screen renders from. */
typedef struct {
    const char *title;      /* header line, may be NULL */
    const char *body[64];   /* body lines, may be fewer than 64 */
    int         nbody;
    const char *status;     /* footer, may be NULL */
    int         width;      /* usable columns */
} r_state;

/* The frame a screen produces. */
typedef struct {
    char  lines[R_MAX_LINES][R_LINE_MAX + 1];
    int   n;
} r_frame;

void     r_frame_init(r_frame *f);
r_frame  render(const r_state *st);
int      frame_diff(const r_frame *prev, const r_frame *next, char *out, size_t outlen);
int      render_run_selftest(void);

#endif
