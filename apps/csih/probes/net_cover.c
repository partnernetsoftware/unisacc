/* net_cover.c — headers, the wait.h trap, loopback, localhost DNS.
 *   unisacc probes/net_cover.c probes/u_run.c
 * No gcc. Run from the cdsh directory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int u_spawn(const char *bin, char **args, int nargs, const char *cwd,
            char *out, int cap, int *rc);

static const char *pick(const char *a, const char *b) {
    if (access(a, 0) == 0) return a;
    return b;
}

static int one(const char *u, const char *path, char *out, int cap, int *rc) {
    char *args[1];
    args[0] = (char *)path;
    return u_spawn(u, args, 1, NULL, out, cap, rc);
}

int main(void) {
    const char *u = getenv("UNISACC");
    const char *headers[] = { "sys/socket.h", "netdb.h", "arpa/inet.h", "netinet/in.h", "sys/select.h" };
    char dir[64], src[80], body[80], out[2048];
    int i, rc, fail = 0;
    FILE *f;
    if (!u || !u[0]) u = "/Users/wjc/repos/unisacc/unisacc.com";
    snprintf(dir, sizeof dir, "/tmp/cdsh-net-%d", (int)getpid());
    if (mkdir(dir, 0700) != 0) { printf("no temp\n"); return 2; }
    snprintf(src, sizeof src, "%s/h.c", dir);
    printf("headers\n");
    for (i = 0; i < 5; i++) {
        char *args[1];
        snprintf(body, sizeof body, "#include <%s>\nint main(void){ return 0; }\n", headers[i]);
        f = fopen(src, "w"); if (!f) { fail = 1; continue; }
        fputs(body, f); fclose(f);
        args[0] = src;
        u_spawn(u, args, 1, NULL, out, (int)sizeof out, &rc);
        if (rc == 0 && out[0] == 0) printf("  ok    %s\n", headers[i]);
        else { printf("  MISS  %s %s\n", headers[i], out); fail = 1; }
    }
    printf("wait.h trap\n");
    one(u, pick("probes/net/alone.c", "net/alone.c"), out, (int)sizeof out, &rc);
    printf("  socket() alone        : %s\n", out[0] ? out : "ok");
    if (strstr(out, "_unisa_ret")) printf("  STATUS: trap still present\n");
    else if (rc == 0 && out[0] == 0) printf("  STATUS: FIXED — socket() needs no extra include\n");
    else printf("  STATUS: changed: %s\n", out);
    one(u, pick("probes/net/withwait.c", "net/withwait.c"), out, (int)sizeof out, &rc);
    printf("  socket() + sys/wait.h : %s\n", out[0] ? out : "ok");

    printf("loopback\n");
    one(u, pick("probes/net/roundtrip.c", "net/roundtrip.c"), out, (int)sizeof out, &rc);
    printf("  unisacc: %s\n", out);
    if (strstr(out, "client got: ping") && strstr(out, "server got: pong"))
        printf("  ok    round trip\n");
    else { printf("  FAIL  round trip\n"); fail = 1; }

    printf("dns\n");
    one(u, pick("probes/net/dns.c", "net/dns.c"), out, (int)sizeof out, &rc);
    printf("  unisacc: %s\n", out);
    if (strstr(out, "127.0.0.1")) printf("  ok    localhost\n");
    else { printf("  FAIL  localhost DNS\n"); fail = 1; }

    remove(src); rmdir(dir);
    return fail ? 1 : 0;
}
