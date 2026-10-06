/*
 * file_cli.c — the `main` for file.c, moved out so file.c can be a library.
 * Same rule as every other module here; see render_cli.c for the failure that
 * established it (three mains in one link).
 *
 * CLI takes SUBCOMMANDS, not dash-options (unisacc reserves the dashes).
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct { int ok; int err; long bytes; } file_result;

file_result file_read(const char *path, char *buf, size_t cap);
file_result file_write(const char *path, const char *text, size_t len);
file_result file_append_line(const char *path, const char *line);
file_result file_list(const char *dir, char *out, size_t cap);
int         file_run_selftest(void);

int main(int argc, char **argv) {
    const char *cmd = argc > 1 ? argv[1] : "";
    char buf[65536];
    file_result r;

    if (!strcmp(cmd, "selftest")) return file_run_selftest();

    if (!strcmp(cmd, "read")) {
        r = file_read(argc > 2 ? argv[2] : "", buf, sizeof buf);
        if (!r.ok) { printf("error %d bytes %ld\n", r.err, r.bytes); return 1; }
        fputs(buf, stdout);
        return 0;
    }
    if (!strcmp(cmd, "write")) {
        const char *path = argc > 2 ? argv[2] : "";
        const char *text = argc > 3 ? argv[3] : "";
        r = file_write(path, text, strlen(text));
        printf("%s %ld\n", r.ok ? "ok" : "error", r.bytes);
        return r.ok ? 0 : 1;
    }
    if (!strcmp(cmd, "append")) {
        r = file_append_line(argc > 2 ? argv[2] : "", argc > 3 ? argv[3] : "");
        printf("%s %ld\n", r.ok ? "ok" : "error", r.bytes);
        return r.ok ? 0 : 1;
    }
    if (!strcmp(cmd, "list")) {
        r = file_list(argc > 2 ? argv[2] : ".", buf, sizeof buf);
        if (!r.ok) { printf("error %d\n", r.err); return 1; }
        printf("%ld entries\n", r.bytes);
        fputs(buf, stdout);
        return 0;
    }
    printf("usage: file_cli selftest | read <path> | write <path> <text> | "
           "append <path> <line> | list <dir>\n");
    return 64;
}
