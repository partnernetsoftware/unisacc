/*
 * render_cli.c — the `main` for render.c, moved out so render.c is a library.
 *
 * Same reason as gate_cli.c: unisacc runs a source as a program and a program
 * has one `main`, so a module that keeps its own `main` cannot be linked with
 * any other. Combining render.c + term.c + tui.c pulled in THREE mains and the
 * link failed with `duplicate symbol '_main'` (gcc) and
 * `arm64: main:` (unisacc) — the identical trap the rest of cdsh already avoids.
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
