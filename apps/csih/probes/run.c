/* run.c — run the leaf fixture .c files under unisacc. No gcc, no -o.
 *   unisacc probes/run.c probes/u_run.c
 * Drivers (this file, u_run, the suite probes) are not fixtures.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int u_spawn(const char *bin, char **args, int nargs, const char *cwd,
            char *out, int cap, int *rc);

static const char *leaf[] = {
    "probes/fs-open.c", "probes/fs-stat.c", "probes/tty-raw.c", "probes/tty-size.c",
    "probes/poll-mux.c", "probes/dns.c", "probes/net-tcp.c",
    "probes/tui-q5-signal.c", "probes/tui-terms.c", "probes/tui-q5-sigaction.c",
    "probes/net/alone.c", "probes/net/withwait.c", "probes/net/roundtrip.c", "probes/net/dns.c",
};

int main(void) {
    const char *u = getenv("UNISACC");
    char out[400], line[120];
    int i, rc;
    if (!u || !u[0]) u = "/Users/wjc/repos/unisacc/unisacc.com";
    for (i = 0; i < (int)(sizeof leaf / sizeof leaf[0]); i++) {
        char *args[1];
        size_t k = 0;
        const char *name = strrchr(leaf[i], '/');
        name = name ? name + 1 : leaf[i];
        args[0] = (char *)leaf[i];
        if (access(leaf[i], 0) != 0) { printf("%-24s missing\n", name); continue; }
        u_spawn(u, args, 1, NULL, out, (int)sizeof out, &rc);
        while (out[k] && out[k] != '\n' && k + 1 < sizeof line) { line[k] = out[k]; k++; }
        line[k] = 0;
        printf("%-24s rc=%-3d %s\n", name, rc, line[0] ? line : "<no output>");
    }
    return 0;
}
