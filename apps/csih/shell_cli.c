/*
 * shell_cli.c — the `main` for shell.c, moved out so shell.c is a library that
 * the TUI can link. Same rule as every other module here.
 *
 * CLI takes SUBCOMMANDS, not dash-options (unisacc reserves the dashes).
 */
#include <stdio.h>
#include <string.h>

typedef struct {
    int ok; int err; int exited; int status; int signal; long bytes;
    char out[65536];
} shell_result;

shell_result shell_run(const char *command);
int          shell_run_selftest(void);

int main(int argc, char **argv) {
    const char *cmd = argc > 1 ? argv[1] : "";
    shell_result r;
    if (!strcmp(cmd, "selftest")) return shell_run_selftest();
    if (!strcmp(cmd, "run")) {
        r = shell_run(argc > 2 ? argv[2] : "");
        if (!r.ok) { printf("could not run (errno %d)\n", r.err); return 1; }
        fputs(r.out, stdout);
        if (!r.exited) printf("[signalled %d]\n", r.signal);
        else if (r.status) printf("[exit %d]\n", r.status);
        return r.exited ? r.status : 1;
    }
    printf("usage: shell_cli selftest | run \"<command>\"\n");
    return 64;
}
