/*
 * net_cli.c — the `main` for net.c, moved out so net.c can be a library (with
 * shell.c and everything else, per unisacc's one-main-per-program rule).
 *
 * CLI takes SUBCOMMANDS, not dash-options.
 */
#include <stdio.h>
#include <string.h>

typedef struct {
    int ok; int err; int status; long body_bytes; int chunked;
    char header[16384];
    char body[65536];
} net_response;

net_response net_http(const char *method, const char *url,
                      const char *content_type, const char *body);
int          net_run_selftest(void);

int main(int argc, char **argv) {
    const char *cmd = argc > 1 ? argv[1] : "";
    if (!strcmp(cmd, "selftest")) return net_run_selftest();
    if (!strcmp(cmd, "get") || !strcmp(cmd, "post")) {
        const char *url = argc > 2 ? argv[2] : "";
        const char *body = argc > 3 ? argv[3] : NULL;
        net_response r = net_http(!strcmp(cmd, "post") ? "POST" : "GET", url,
                                  "application/json", body);
        if (!r.ok) {
            printf("failed (err %d)", r.err);
            if (r.err == -2003) printf(": libcurl did not load");
            if (r.err == -2002) printf(": malformed or unsupported URL");
            printf("\n");
            return 1;
        }
        printf("status %d, %ld body bytes%s\n", r.status, r.body_bytes,
               r.chunked ? " (CHUNKED, not decoded)" : "");
        fputs(r.body, stdout);
        return r.status >= 200 && r.status < 300 ? 0 : 2;
    }
    printf("usage: net_cli selftest | get <url> | post <url> <body>\n");
    return 64;
}
