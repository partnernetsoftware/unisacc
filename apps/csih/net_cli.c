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

#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define NET_REFUSED_URL (-2002)
typedef struct {
    char host[256];
    char port[16];
    char path[1024];
    int  https;
} net_url;
int net_parse_url(const char *url, net_url *u);
void net_note_progress(const char *path, time_t t0);
int net_deepseek_forget(void);
void net_progress(int *sec, int *tokens, long *sent);
void net_recv(long *n);
void net_cache(int *hit, int *miss, int *seen);
extern int net_hit, net_miss, net_usage_seen;
void net_test_home(const char *home);

/* ── self-test ──────────────────────────────────────────────────────────── */

static int failures = 0;
static void expect(int cond, const char *what) {
    if (!cond) { printf("FAIL %s\n", what); failures++; }
}

/*
 * The self-test deliberately does NOT need the network.
 *
 * URL parsing and the https refusal are pure logic, and they are where the
 * dangerous mistakes are (a refused https that silently downgrades). Everything
 * that needs a socket lives in probes/net-cover.sh, which stands up a real
 * loopback listener — so `net selftest` runs on a plane, in CI, and on a
 * machine with no route, and still catches the bugs that matter most.
 */
static void run_selftest(void) {
    net_url u;
    int rc;

    rc = net_parse_url("http://example.com/path", &u);
    expect(rc == 0, "a plain http URL parses");
    expect(!strcmp(u.host, "example.com"), "the host is extracted");
    expect(!strcmp(u.port, "80"), "the port defaults to 80");
    expect(!strcmp(u.path, "/path"), "the path is extracted");

    rc = net_parse_url("http://example.com", &u);
    expect(rc == 0 && !strcmp(u.path, "/"), "a URL with no path gets /");

    rc = net_parse_url("http://127.0.0.1:8080/v1/chat", &u);
    expect(rc == 0, "an explicit port parses");
    expect(!strcmp(u.host, "127.0.0.1"), "an IP literal is a host");
    expect(!strcmp(u.port, "8080"), "the explicit port is kept");
    expect(!strcmp(u.path, "/v1/chat"), "a multi-segment path survives");

    /* https is parsed as https. Downgrading it to port 80 would send the
     * request in the clear. */
    rc = net_parse_url("https://example.com/secret", &u);
    expect(rc == 0, "https parses");
    expect(u.https == 1, "https stays https");
    expect(!strcmp(u.port, "443"), "https defaults to 443, not 80");
    expect(!strcmp(u.host, "example.com"), "https host is not rewritten");
    expect(!strcmp(u.path, "/secret"), "https path survives");

    rc = net_parse_url("ftp://example.com", &u);
    expect(rc == NET_REFUSED_URL, "an unsupported scheme is refused");
    rc = net_parse_url("example.com/path", &u);
    expect(rc == NET_REFUSED_URL, "a URL with no scheme is refused");
    rc = net_parse_url("http://", &u);
    expect(rc == NET_REFUSED_URL, "an empty host is refused");

    /* A URL whose host would overflow must be refused, not truncated into a
     * DIFFERENT host — a truncated hostname is a hostname that does not exist,
     * or worse, one that does. */
    {
        char big[600];
        int i;
        strcpy(big, "http://");
        for (i = 0; i < 500; i++) big[7 + i] = 'a';
        big[507] = '\0';
        rc = net_parse_url(big, &u);
        expect(rc == NET_REFUSED_URL, "an over-long host is refused, not truncated");
    }

    /* Bytes already on disk count before total_tokens exists. A new note
     * with no usage object drops the previous hit/miss. */
    {
        char path[] = "/tmp/cdsh-net-progress";
        FILE *f;
        int sec = -1, tokens = -1, hit = 9, miss = 9, seen = 9;
        long got = -1;
        net_hit = 3;
        net_miss = 4;
        net_usage_seen = 1;
        f = fopen(path, "w");
        if (!f) expect(0, "progress fixture opens");
        else {
            fputs("{\"abcd\"}", f);
            fclose(f);
            net_note_progress(path, time(0));
            net_progress(&sec, &tokens, 0);
            net_recv(&got);
            net_cache(&hit, &miss, &seen);
            expect(got == 8 && tokens == 2 && seen == 0, "partial body counts bytes and hides cache");
            f = fopen(path, "w");
            fputs("{\"usage\":{\"total_tokens\":11,\"prompt_cache_hit_tokens\":2,\"prompt_cache_miss_tokens\":3}}", f);
            fclose(f);
            net_note_progress(path, time(0));
            net_progress(&sec, &tokens, 0);
            net_cache(&hit, &miss, &seen);
            expect(tokens == 11 && seen == 1 && hit == 2 && miss == 3, "usage replaces the byte estimate");
            unlink(path);
        }
    }

    /* /reload asks this before it drops the cached key. A missing file must
     * leave the cache. DEEPSEEK_API_KEY is not touched. */
    {
        char saved[512];
        const char *home = getenv("HOME");
        int saw;
        saved[0] = 0;
        if (home) snprintf(saved, sizeof saved, "%s", home);
        net_test_home("/tmp/csih-reload-missing");
        saw = net_deepseek_forget();
        if (saved[0]) net_test_home(saved);
        expect(saw == 0, "a missing env.jsonl keeps the cached key");
        if (saved[0]) expect(net_deepseek_forget() == 1, "an open env.jsonl clears the cached key");
    }
}

int net_run_selftest(void) {
    run_selftest();
    printf("%s\n", failures ? "SELFTEST FAILED" : "selftest ok");
    return failures == 0 ? 0 : 1;
}
