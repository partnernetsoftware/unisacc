/*
 * tools_cli.c — the `main` for tools.c, moved out so tools.c can be a library
 * linked into the TUI. Same rule as every other module here.
 *
 * CLI takes SUBCOMMANDS, not dash-options (unisacc reserves the dashes).
 */
#include <stdio.h>
#include <string.h>

typedef struct { int ok; char out[65536]; } tool_result;
tool_result tool_run(const char *line);
int         tool_run_selftest(void);

int main(int argc, char **argv) {
    const char *cmd = argc > 1 ? argv[1] : "";
    tool_result r;
    if (!strcmp(cmd, "selftest")) return tool_run_selftest();
    if (!strcmp(cmd, "run")) {
        /* usage: tools_cli run "<command line>" */
        r = tool_run(argc > 2 ? argv[2] : "");
        printf("%s\n", r.out);
        return r.ok ? 0 : 1;
    }
    printf("usage: tools_cli selftest | run \"<command line>\"\n");
    return 64;
}
