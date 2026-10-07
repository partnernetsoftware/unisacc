/*
 * render_cli.c — the `main` for render.c, moved out so render.c is a library.
 *
 * Same reason as gate_cli.c: unisacc runs a source as a program and a program
 * has one `main`, so a module that keeps its own `main` cannot be linked with
 * any other. Combining render.c + term.c + tui.c pulled in THREE mains and the
 * link failed with `duplicate symbol '_main'` (gcc) and
 * `arm64: main:` (unisacc) — the identical trap the rest of csih already avoids.
 *
 * CLI takes SUBCOMMANDS, not dash-options (unisacc reserves the dashes).
 */
#include <stdio.h>
#include <string.h>
#include "render_api.h"

int main(int argc, char **argv) {
    const char *cmd = argc > 1 ? argv[1] : "";
    if (!strcmp(cmd, "selftest")) return render_run_selftest();
    printf("usage: render_cli selftest\n");
    return 64;
}

#include <stdlib.h>

void render_test_home(const char *home);

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
    render_test_home("/Users/cdshuser");
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
    render_test_home("/home/cdshuser");
    st.title = "cwd=/home/cdshuser/repos";
    b = render(&st);
    expect(strstr(b.lines[0], "cwd=~/repos") && !strstr(b.lines[0], "cdshuser"),
           "/home/<user> displays as ~/");
}

/* Public render() and frame_diff() only. */
int render_run_selftest(void) {
    run_selftest();
    printf("%s\n", failures ? "SELFTEST FAILED" : "selftest ok");
    return failures == 0 ? 0 : 1;
}
