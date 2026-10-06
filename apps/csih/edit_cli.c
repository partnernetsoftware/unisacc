/*
 * edit_cli.c — the `main` for edit.c, moved out so edit.c can be a library.
 * Same rule as every other module here; see render_cli.c for the failure that
 * established it (three mains in one link).
 *
 * CLI takes SUBCOMMANDS, not dash-options (unisacc reserves the dashes).
 */
#include <stdio.h>
#include <string.h>

typedef struct { int ok; int err; long count; long bytes; } edit_result;

edit_result edit_replace(const char *path, const char *old_text,
                         const char *new_text);
int         edit_run_selftest(void);

int main(int argc, char **argv) {
    const char *cmd = argc > 1 ? argv[1] : "";
    edit_result r;

    if (!strcmp(cmd, "selftest")) return edit_run_selftest();

    if (!strcmp(cmd, "replace")) {
        /* usage: edit_cli replace <path> <old> <new> */
        r = edit_replace(argc > 2 ? argv[2] : "",
                         argc > 3 ? argv[3] : "",
                         argc > 4 ? argv[4] : "");
        if (r.ok) {
            printf("ok 1 replacement\n");
            return 0;
        }
        /* The two interesting refusals are NAMED with their counts, because a
         * caller that only sees an exit code cannot tell "your context was
         * wrong" from "the file is genuinely ambiguous" — and those want
         * different next moves. */
        if (r.err == -2001) {
            printf("error not found\n");
        } else if (r.err == -2002) {
            printf("error appears %ld times, need more context\n", r.count);
        } else {
            printf("error %d\n", r.err);
        }
        return 1;
    }

    printf("usage: edit_cli selftest | replace <path> <old> <new>\n");
    return 64;
}
