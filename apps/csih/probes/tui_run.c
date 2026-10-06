/* tui_run.c — run each TUI question probe under unisacc only.
 *   unisacc probes/tui_run.c probes/u_run.c
 * sigaction is the one expected red. Any other non-zero is a failure.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int u_spawn(const char *bin, char **args, int nargs, const char *cwd,
            char *out, int cap, int *rc);

static const char *src[] = {
    "probes/tui-terms.c", "probes/tui-terms.c", "probes/tui-terms.c",
    "probes/tui-terms.c", "probes/tui-q5-signal.c", "probes/tui-q5-sigaction.c",
};
static const char *sub[] = {
    "q1-cfmakeraw", "q2-layout", "q3-winsize", "q4-poll", "run", "run",
};
static int expect_red[] = { 0, 0, 0, 0, 0, 1 };

int main(void) {
    const char *u = getenv("UNISACC");
    char out[1024], line[200];
    int i, rc, bad = 0;
    if (!u || !u[0]) u = "/Users/wjc/repos/unisacc/unisacc.com";
    printf("%-28s %-40s %s\n", "question", "unisacc", "note");
    for (i = 0; i < 6; i++) {
        char *args[2];
        size_t k = 0;
        const char *path = src[i];
        if (access(path, 0) != 0) path = src[i] + 7; /* probes/ stripped */
        args[0] = (char *)path;
        args[1] = (char *)sub[i];
        u_spawn(u, args, 2, NULL, out, (int)sizeof out, &rc);
        while (out[k] && out[k] != '\n' && k + 1 < sizeof line) { line[k] = out[k]; k++; }
        line[k] = 0;
        if (expect_red[i]) {
            printf("%-28s %-40s %s\n", sub[i], line, "expected-red");
            if (rc == 0 && !strstr(out, "not covered") && !strstr(out, "undefined") && !strstr(out, "error")) {
                printf("  FAIL  sigaction unexpectedly succeeded\n");
                bad++;
            }
        } else if (rc == 0) {
            printf("%-28s %-40s %s\n", sub[i], line, "unisacc-ok");
        } else {
            printf("%-28s %-40s %s\n", sub[i], line, "UNISACC RED");
            bad++;
        }
    }
    printf("\nexpected red: sigaction only\n");
    return bad ? 1 : 0;
}
