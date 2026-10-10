/* agent_loop_bound.c — C port of probes/agent_loop_bound.py (working-tree version).
 *
 * Builds the csih agent once with unisacc (-include globals + AGENT list), then
 * runs it per case against a forked C HTTP stub that serves canned chat
 * completions. Blocked by decision A: the stubs still use the old answer format
 * (no claims/observation_refs); expectations are unchanged.
 *
 *   /bin/sh /Users/wjc/repos/unisacc/unisacc.com <-include ...> -o OUT <AGENT>
 *   OUT agent PROMPT
 *
 * Verdict: last line "TOTAL PASS n" or "TOTAL FAIL n"; rc 0 / 1.
 * Optional argv: case names to run (same selector as the Python).
 * Temp dirs use $TMPDIR (fallback /tmp), like Python tempfile.
 */
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "csih_flat.h"

#define LB_UNISACC CSIH_ROOT "/unisacc.com"
#define LB_PORT_LO 19100
#define LB_PORT_HI 19149
#define CONT "{\"go\":\"continue\"}"
#define EXEC_A "{\"act\":\"exec\",\"cmd\":\"true\",\"why\":\"x\"}"
#define ANSWER_A "{\"act\":\"answer\",\"outcome\":\"completed\",\"text\":\"done\"}"
#define STOP_A "{\"go\":\"stop\"}"
#define FAILED_A "{\"act\": \"answer\", \"outcome\": \"failed\", \"text\": \"FAILED_ARTIFACT\"}"
#define PARTIAL_A "{\"act\": \"answer\", \"outcome\": \"partial\", \"text\": \"PARTIAL_ARTIFACT\"}"
#define LEGACY_A "{\"act\": \"answer\", \"text\": \"LEGACY_ARTIFACT\"}"
#define RED_WRITE "{\"act\":\"file\",\"op\":\"write\",\"path\":\"apps/csih/probe.c\",\"text\":\"x\"}"

/* ---------- snapshot (byte-for-byte replacement for sha256 freeze) ---------- */

typedef struct {
    char **path;
    unsigned char **data;
    size_t *len;
    size_t n, cap;
} snap_t;

static void die(const char *msg) {
    printf("FAIL %s\n", msg);
    exit(1);
}

static void *xmalloc(size_t n) {
    void *p = malloc(n ? n : 1);
    if (!p) die("oom");
    return p;
}

static unsigned char *read_file(const char *path, size_t *len) {
    FILE *f = fopen(path, "rb");
    unsigned char *buf;
    long sz;
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = xmalloc((size_t)sz);
    if (sz > 0 && fread(buf, 1, (size_t)sz, f) != (size_t)sz) { fclose(f); free(buf); return NULL; }
    fclose(f);
    *len = (size_t)sz;
    return buf;
}

static int ends(const char *s, const char *suf) {
    size_t a = strlen(s), b = strlen(suf);
    return a >= b && !strcmp(s + a - b, suf);
}

static int is_src(const char *name) {
    return ends(name, ".c") || ends(name, ".h") || ends(name, ".inc") || ends(name, ".cx");
}

static void snap_add(snap_t *s, const char *path) {
    size_t len = 0;
    unsigned char *d = read_file(path, &len);
    if (!d) die("snapshot read");
    if (s->n == s->cap) {
        s->cap = s->cap ? s->cap * 2 : 64;
        s->path = realloc(s->path, s->cap * sizeof *s->path);
        s->data = realloc(s->data, s->cap * sizeof *s->data);
        s->len = realloc(s->len, s->cap * sizeof *s->len);
        if (!s->path || !s->data || !s->len) die("oom");
    }
    /* not strdup: unisacc strdup yields blocks that corrupt the heap on free() */
    s->path[s->n] = xmalloc(strlen(path) + 1);
    memcpy(s->path[s->n], path, strlen(path) + 1);
    s->data[s->n] = d;
    s->len[s->n] = len;
    s->n++;
}

/* srconly: only .c/.h/.inc/.cx (Python hashes() suffix filter) */
static void walk(snap_t *s, const char *dir, int srconly) {
    DIR *d = opendir(dir);
    struct dirent *e;
    if (!d) return;
    while ((e = readdir(d)) != NULL) {
        char child[4096];
        struct stat st;
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        snprintf(child, sizeof child, "%s/%s", dir, e->d_name);
        if (lstat(child, &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) walk(s, child, srconly);
        else if (S_ISREG(st.st_mode) && (!srconly || is_src(e->d_name))) snap_add(s, child);
    }
    closedir(d);
}

static void snap_free(snap_t *s) {
    size_t i;
    for (i = 0; i < s->n; i++) { free(s->path[i]); free(s->data[i]); }
    free(s->path); free(s->data); free(s->len);
    memset(s, 0, sizeof *s);
}

static int snap_same(const snap_t *a, const snap_t *b) {
    size_t i, j;
    if (a->n != b->n) return 0;
    for (i = 0; i < a->n; i++) {
        int found = 0;
        for (j = 0; j < b->n; j++) {
            if (strcmp(a->path[i], b->path[j])) continue;
            found = 1;
            if (a->len[i] != b->len[j] || memcmp(a->data[i], b->data[j], a->len[i])) return 0;
            break;
        }
        if (!found) return 0;
    }
    return 1;
}

/* hashes(): APP src files + unisacc.com + this probe source */
static void snap_hashes(snap_t *s) {
    memset(s, 0, sizeof *s);
    walk(s, CSIH_APP, 1);
    snap_add(s, LB_UNISACC);
    snap_add(s, CSIH_APP "/probes/agent_loop_bound.c");
}

/* ---------- process helpers ---------- */

/* Run /bin/sh -c SCRIPT BIN ARGS... with stdout+stderr merged into out.
 * timeout>0: SIGKILL the process group after timeout seconds (*timed=1).
 * *rc is the exit status, or -1 when signaled/timed out. */
static void spawn(const char *script, const char *bin, char **args, int nargs, int timeout,
                  char *out, size_t cap, int *rc, int *timed) {
    int p[2], st = 0, i;
    pid_t pid, wd = -1;
    size_t off = 0;
    char *av[64];
    out[0] = 0;
    *rc = -1;
    *timed = 0;
    if (pipe(p) != 0) return;
    pid = fork();
    if (pid == 0) {
        int n = 0, dn;
        setpgid(0, 0);
        av[n++] = (char *)"sh";
        av[n++] = (char *)"-c";
        av[n++] = (char *)script;
        av[n++] = (char *)bin;
        for (i = 0; i < nargs; i++) av[n++] = args[i];
        av[n] = NULL;
        dn = open("/dev/null", O_RDONLY);
        if (dn >= 0) { dup2(dn, 0); close(dn); }
        dup2(p[1], 1);
        dup2(p[1], 2);
        close(p[0]);
        close(p[1]);
        execv("/bin/sh", av);
        _exit(127);
    }
    setpgid(pid, pid);
    close(p[1]);
    if (timeout > 0) {
        wd = fork();
        if (wd == 0) {
            close(p[0]);
            sleep((unsigned)timeout);
            kill(-pid, SIGKILL);
            _exit(42);
        }
    }
    for (;;) {
        char tmp[4096];
        ssize_t k = read(p[0], tmp, sizeof tmp);
        if (k < 0) { if (errno == EINTR) continue; break; }
        if (k == 0) break;
        if (off + 1 < cap) {
            size_t take = cap - 1 - off;
            if ((size_t)k < take) take = (size_t)k;
            memcpy(out + off, tmp, take);
            off += take;
        }
    }
    out[off] = 0;
    close(p[0]);
    waitpid(pid, &st, 0);
    if (wd > 0) {
        int ws = 0;
        kill(wd, SIGKILL);
        waitpid(wd, &ws, 0);
        *timed = WIFEXITED(ws) && WEXITSTATUS(ws) == 42;
    }
    if (*timed || WIFSIGNALED(st)) *rc = -1;
    else *rc = WEXITSTATUS(st);
}

static const char *tmpbase(void) {
    const char *t = getenv("TMPDIR");
    return (t && t[0]) ? t : "/tmp";
}

static void rm_rf(const char *path) {
    struct stat st;
    if (lstat(path, &st) != 0) return;
    if (S_ISDIR(st.st_mode)) {
        DIR *d = opendir(path);
        struct dirent *e;
        while (d && (e = readdir(d)) != NULL) {
            char child[4096];
            if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
            snprintf(child, sizeof child, "%s/%s", path, e->d_name);
            rm_rf(child);
        }
        if (d) closedir(d);
        rmdir(path);
    } else {
        unlink(path);
    }
}

static int read_int(const char *path) {
    FILE *f = fopen(path, "r");
    char b[32];
    int v = 0;
    if (!f) return 0;
    if (fgets(b, sizeof b, f)) v = atoi(b);
    fclose(f);
    return v;
}

/* ---------- csih agent build ---------- */

static int build_bin(const char *out_bin) {
    char script[1024], errp[1024], *args[64];
    char ebuf[256];
    int n = 0, i, rc = -1, timed = 0;
    size_t elen = 0;
    unsigned char *e;
    snprintf(errp, sizeof errp, "%s/build-err.txt", tmpbase());
    snprintf(script, sizeof script, "cd '%s' && exec /bin/sh \"$0\" \"$@\" 2>'%s'", CSIH_APP, errp);
    for (i = 0; csih_global_include_args[i]; i++) args[n++] = (char *)csih_global_include_args[i];
    args[n++] = (char *)"-o";
    args[n++] = (char *)out_bin;
    for (i = 0; csih_agent[i]; i++) args[n++] = (char *)csih_agent[i];
    spawn(script, LB_UNISACC, args, n, 180, ebuf, sizeof ebuf, &rc, &timed);
    if (rc != 0 || timed) return -1;
    e = read_file(errp, &elen);
    if (e) { free(e); if (elen) return -1; }
    unlink(errp);
    return 0;
}

/* ---------- stub HTTP server (forked child) ---------- */

typedef struct {
    int lsock;
    int nresp;
    const char *resp[10];
    const char *countpath;
} stub_t;

static int ci_eq(const char *a, const char *b, size_t n) {
    size_t i;
    for (i = 0; i < n; i++) {
        char x = a[i], y = b[i];
        if (x >= 'A' && x <= 'Z') x = (char)(x - 'A' + 'a');
        if (x != y) return 0;
    }
    return 1;
}

static long content_length(const char *h, const char *end) {
    const char *p = h;
    while (p < end) {
        const char *eol = strstr(p, "\r\n");
        if (!eol || eol > end) eol = end;
        if (eol - p > 15 && ci_eq(p, "content-length:", 15)) return atol(p + 15);
        if (eol >= end) break;
        p = eol + 2;
    }
    return 0;
}

/* JSON-wrap a chat completion; escapes the content like json.dumps. */
static char *json_wrap(const char *content) {
    size_t i, o = 0, cap = strlen(content) * 6 + 128;
    char *b = xmalloc(cap);
    const char *head = "{\"choices\": [{\"message\": {\"content\": \"";
    const char *tail = "\"}}]}";
    memcpy(b, head, strlen(head));
    o = strlen(head);
    for (i = 0; content[i]; i++) {
        unsigned char c = (unsigned char)content[i];
        if (c == '"' || c == '\\') { b[o++] = '\\'; b[o++] = (char)c; }
        else if (c == '\n') { b[o++] = '\\'; b[o++] = 'n'; }
        else if (c == '\r') { b[o++] = '\\'; b[o++] = 'r'; }
        else if (c == '\t') { b[o++] = '\\'; b[o++] = 't'; }
        else if (c < 0x20) { o += (size_t)sprintf(b + o, "\\u%04x", c); }
        else b[o++] = (char)c;
    }
    memcpy(b + o, tail, strlen(tail));
    o += strlen(tail);
    b[o] = 0;
    return b;
}

static void send_all(int fd, const char *b, size_t n) {
    while (n > 0) {
        ssize_t k = write(fd, b, n);
        if (k < 0) { if (errno == EINTR) continue; return; }
        b += k;
        n -= (size_t)k;
    }
}

static void stub_one(int c, const stub_t *st, int *count) {
    static char buf[1 << 16];
    size_t len = 0, need = 0;
    char *hend = NULL;
    for (;;) {
        ssize_t k;
        if (hend && len >= need) break;
        if (len + 1 >= sizeof buf) return;
        k = recv(c, buf + len, sizeof buf - 1 - len, 0);
        if (k < 0 && errno == EINTR) continue;
        if (k <= 0) return;
        len += (size_t)k;
        buf[len] = 0;
        if (!hend) {
            hend = strstr(buf, "\r\n\r\n");
            if (hend) need = (size_t)(hend - buf) + 4 + (size_t)content_length(buf, hend);
        }
    }
    {
        int idx = *count;
        const char *content = st->resp[idx < st->nresp ? idx : st->nresp - 1];
        char *body, hdr[256];
        size_t bl, hl;
        FILE *f;
        (*count)++;
        f = fopen(st->countpath, "w");
        if (f) { fprintf(f, "%d\n", *count); fclose(f); }
        body = json_wrap(content);
        bl = strlen(body);
        hl = (size_t)snprintf(hdr, sizeof hdr,
              "HTTP/1.0 200 OK\r\nContent-Type: application/json\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n", bl);
        send_all(c, hdr, hl);
        send_all(c, body, bl);
        free(body);
    }
}

static void stub_serve(const stub_t *st) {
    int count = 0;
    for (;;) {
        int c = accept(st->lsock, NULL, NULL);
        if (c < 0) { if (errno == EINTR) continue; _exit(0); }
        stub_one(c, st, &count);
        close(c);
    }
}

static int listen_local(int *port) {
    int p;
    for (p = LB_PORT_LO; p <= LB_PORT_HI; p++) {
        struct sockaddr_in a;
        int one = 1;
        int s = socket(AF_INET, SOCK_STREAM, 0);
        if (s < 0) return -1;
        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
        memset(&a, 0, sizeof a);
        a.sin_family = AF_INET;
        a.sin_port = htons((unsigned short)p);
        a.sin_addr.s_addr = htonl(0x7f000001);
        if (bind(s, (struct sockaddr *)&a, sizeof a) == 0 && listen(s, 16) == 0) {
            *port = p;
            return s;
        }
        close(s);
    }
    return -1;
}

/* ---------- minimal JSON scanning for trace records ---------- */

static void put_utf8(char *dst, size_t *o, size_t cap, unsigned cp) {
    char tmp[4];
    int n = 0, i;
    if (cp < 0x80) tmp[n++] = (char)cp;
    else if (cp < 0x800) { tmp[n++] = (char)(0xC0 | (cp >> 6)); tmp[n++] = (char)(0x80 | (cp & 0x3F)); }
    else if (cp < 0x10000) { tmp[n++] = (char)(0xE0 | (cp >> 12)); tmp[n++] = (char)(0x80 | ((cp >> 6) & 0x3F)); tmp[n++] = (char)(0x80 | (cp & 0x3F)); }
    else { tmp[n++] = (char)(0xF0 | (cp >> 18)); tmp[n++] = (char)(0x80 | ((cp >> 12) & 0x3F)); tmp[n++] = (char)(0x80 | ((cp >> 6) & 0x3F)); tmp[n++] = (char)(0x80 | (cp & 0x3F)); }
    for (i = 0; i < n && *o + 1 < cap; i++) dst[(*o)++] = tmp[i];
}

static unsigned hex4(const char *p) {
    unsigned v = 0;
    int i;
    for (i = 0; i < 4; i++) {
        char c = p[i];
        v <<= 4;
        if (c >= '0' && c <= '9') v |= (unsigned)(c - '0');
        else if (c >= 'a' && c <= 'f') v |= (unsigned)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') v |= (unsigned)(c - 'A' + 10);
    }
    return v;
}

/* Decode string value of "key" in a JSON line. Returns 1 if it is a string. */
static int jget(const char *line, const char *key, char *dst, size_t cap) {
    char pat[96];
    const char *p = line, *q = NULL;
    size_t o = 0;
    snprintf(pat, sizeof pat, "\"%s\"", key);
    while ((p = strstr(p, pat)) != NULL) {
        q = p + strlen(pat);
        while (*q == ' ' || *q == '\t') q++;
        if (*q == ':') break;
        p++;
    }
    if (!p) return 0;
    q++;
    while (*q == ' ' || *q == '\t') q++;
    if (*q != '"') return 0;
    q++;
    dst[0] = 0;
    while (*q && *q != '"') {
        if (*q == '\\') {
            unsigned cp;
            q++;
            switch (*q) {
            case 'n': if (o + 1 < cap) dst[o++] = '\n'; break;
            case 'r': if (o + 1 < cap) dst[o++] = '\r'; break;
            case 't': if (o + 1 < cap) dst[o++] = '\t'; break;
            case 'b': if (o + 1 < cap) dst[o++] = '\b'; break;
            case 'f': if (o + 1 < cap) dst[o++] = '\f'; break;
            case 'u':
                cp = hex4(q + 1);
                q += 4;
                if (cp >= 0xD800 && cp <= 0xDBFF && q[1] == '\\' && q[2] == 'u') {
                    unsigned lo = hex4(q + 3);
                    cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                    q += 6;
                }
                put_utf8(dst, &o, cap, cp);
                break;
            default:
                if (o + 1 < cap) dst[o++] = *q;
            }
        } else {
            if (o + 1 < cap) dst[o++] = *q;
        }
        q++;
    }
    dst[o] = 0;
    return 1;
}

/* Count trace lines where (k1==v1) and (k2==v2) and (ksub contains sub); NULL = skip. */
static int trace_count(const char *buf, const char *k1, const char *v1, const char *k2, const char *v2,
                       const char *ksub, const char *sub) {
    const char *p = buf;
    int count = 0;
    while (p && *p) {
        const char *e = strchr(p, '\n');
        size_t len = e ? (size_t)(e - p) : strlen(p);
        char *lb = xmalloc(len + 1);
        char *tmp = xmalloc(len + 1);
        int ok = 1;
        memcpy(lb, p, len);
        lb[len] = 0;
        if (k1) { if (!jget(lb, k1, tmp, len + 1) || strcmp(tmp, v1)) ok = 0; }
        if (ok && k2) { if (!jget(lb, k2, tmp, len + 1) || strcmp(tmp, v2)) ok = 0; }
        if (ok && ksub) { if (!jget(lb, ksub, tmp, len + 1) || !strstr(tmp, sub)) ok = 0; }
        if (ok) count++;
        free(lb);
        free(tmp);
        p = e ? e + 1 : NULL;
    }
    return count;
}

/* ---------- cases ---------- */

typedef struct {
    const char *name;
    const char *resp[10];
    int nresp;
    const char *prompt;
    int reqs, rc;
    int red;
    const char *extra_env;
} row_t;

static int pick(const char *s, const char *sub) { return strstr(s, sub) != NULL; }

static const char *expected_cause(const char *name) {
    if (!strcmp(name, "parsefail")) return "too many unparseable steps";
    if (!strcmp(name, "stop_no_answer") || !strcmp(name, "direct_stop_no_answer")) return "go=stop without answer";
    if (!strcmp(name, "action_budget")) return "MAX_ACTIONS";
    if (!strcmp(name, "round_budget") || !strcmp(name, "prior_answer_budget")) return "MAX_ROUNDS after continue";
    if (!strcmp(name, "invalid_judge")) return "invalid round-end judgment";
    return NULL;
}

static int run_case(const row_t *r, const char *bin) {
    char cwd[600], home[700], trace[700], countp[700], script[3072], extra[320], ep[160], tmpl[600];
    static char out[1 << 16];
    char reasons[1200] = "";
    char *tbuf = NULL;
    size_t tlen = 0;
    int lsock, port = 0, rc = 0, timed = 0, reqs = 0, ok, i;
    pid_t stub;
    char *args[4];
    char *s;
    int watch = pick(r->name, "watch") && !strncmp(r->name, "watch", 5);

#define ADDR(...) do { size_t _l = strlen(reasons); snprintf(reasons + _l, sizeof reasons - _l, __VA_ARGS__); } while (0)

    snprintf(tmpl, sizeof tmpl, "%s/csih-probe-XXXXXX", tmpbase());
    if (!mkdtemp(tmpl)) die("mkdtemp case");
    snprintf(cwd, sizeof cwd, "%s", tmpl);
    snprintf(home, sizeof home, "%s/home", cwd);
    snprintf(trace, sizeof trace, "%s/trace.jsonl", cwd);
    snprintf(countp, sizeof countp, "%s/reqs.txt", cwd);
    mkdir(home, 0700);
    if (r->red) {
        char apps[700];
        snprintf(apps, sizeof apps, "%s/apps", cwd);
        mkdir(apps, 0700);
        snprintf(apps, sizeof apps, "%s/apps/csih", cwd);
        mkdir(apps, 0700);
    }

    lsock = listen_local(&port);
    if (lsock < 0) die("stub listen");
    {
        stub_t st;
        memset(&st, 0, sizeof st);
        st.lsock = lsock;
        st.nresp = r->nresp;
        for (i = 0; i < r->nresp; i++) st.resp[i] = r->resp[i];
        st.countpath = countp;
        stub = fork();
        if (stub == 0) stub_serve(&st);
    }
    close(lsock);
    snprintf(ep, sizeof ep, "http://127.0.0.1:%d/v1/chat/completions", port);

    extra[0] = 0;
    if (watch) snprintf(extra, sizeof extra, "export CSIH_ROLE='watch' CSIH_PEER='0:csih-x'; ");
    else extra[0] = 0;
    if (r->extra_env) {
        size_t l = strlen(extra);
        snprintf(extra + l, sizeof extra - l, "export %s; ", r->extra_env);
    }
    snprintf(script, sizeof script,
             "unset OPENAI_API_KEY http_proxy https_proxy HTTP_PROXY HTTPS_PROXY all_proxy ALL_PROXY CSIH_ROLE CSIH_PEER; "
             "export HOME='%s' DEEPSEEK_API_KEY='LOCAL_STUB_ONLY' NO_PROXY='127.0.0.1,localhost' no_proxy='127.0.0.1,localhost' "
             "CSIH_ENDPOINT='%s' CSIH_MODEL='stub' CSIH_CWD='%s' CSIH_TRANSCRIPT='%s'; %s"
             "cd '%s' && exec \"$0\" \"$@\"",
             home, ep, cwd, trace, extra, CSIH_APP);
    args[0] = (char *)"agent";
    args[1] = (char *)r->prompt;
    spawn(script, bin, args, 2, 9, out, sizeof out, &rc, &timed);
    if (timed) rc = -1;
    kill(stub, SIGKILL);
    waitpid(stub, NULL, 0);
    reqs = read_int(countp);

    ok = (rc == r->rc) && (reqs == r->reqs);
    if (rc != r->rc) ADDR("rc %d != %d; ", rc, r->rc);
    if (reqs != r->reqs) ADDR("reqs %d != %d; ", reqs, r->reqs);
    s = out;
    if (!strcmp(r->name, "red")) {
        if (!strstr(s, "unfinished")) ADDR("red missing 'unfinished'; ");
        if (!strstr(s, "slice row")) ADDR("red missing 'slice row'; ");
    }
    if (watch && strcmp(r->name, "watch_no_tools")) {
        if (!strstr(s, "unfinished")) ADDR("%s missing 'unfinished'; ", r->name);
        if (!strstr(s, "peer mail not delivered")) ADDR("%s missing 'peer mail not delivered'; ", r->name);
    }
    if (!strcmp(r->name, "normal") || !strcmp(r->name, "watch_no_tools")) {
        if (strstr(s, "agent FAILED")) ADDR("%s x 'agent FAILED'; ", r->name);
    }
    if ((!strcmp(r->name, "outcome_failed") || !strcmp(r->name, "outcome_failed_last") || !strcmp(r->name, "watch_failed"))
        && !strstr(s, "FAILED_ARTIFACT")) ADDR("failed body missing; ");
    if ((!strcmp(r->name, "outcome_partial") || !strcmp(r->name, "watch_partial")) && !strstr(s, "PARTIAL_ARTIFACT"))
        ADDR("partial body missing; ");
    if (!strcmp(r->name, "outcome_legacy") && (!strstr(s, "LEGACY_ARTIFACT") || !strstr(s, "unverified")))
        ADDR("legacy outcome missing; ");
    {
        const char *cause = expected_cause(r->name);
        if (cause && (!strstr(s, "unfinished") || !strstr(s, cause))) ADDR("missing unfinished cause %s; ", cause);
    }

    tbuf = read_file(trace, &tlen);
    if (!tbuf) { tbuf = xmalloc(1); tbuf[0] = 0; tlen = 0; }
    {
        /* NUL-terminate the trace buffer (read_file gives raw bytes). */
        char *t2 = xmalloc(tlen + 1);
        memcpy(t2, tbuf, tlen);
        t2[tlen] = 0;
        free(tbuf);
        tbuf = t2;
    }
    if (!strcmp(r->name, "invalid_judge") || !strcmp(r->name, "invalid_judge_recovery")) {
        int stops;
        if (!trace_count(tbuf, "role", "assistant", "text", "INVALID_JUDGMENT", NULL, NULL))
            ADDR("invalid judgment actual content missing; ");
        if (!trace_count(tbuf, "name", "error", NULL, NULL, "text", "invalid round-end judgment"))
            ADDR("invalid judgment error record missing; ");
        stops = trace_count(tbuf, "role", "decision", "go", "stop", NULL, NULL);
        if (stops != (!strcmp(r->name, "invalid_judge_recovery") ? 1 : 0))
            ADDR("invalid token synthesized a stop; ");
    }
    free(tbuf);
    ok = ok && reasons[0] == 0;
    printf("%-14s reqs=%d rc=%d expect=(%d,%d) %s\n", r->name, reqs, rc, r->reqs, r->rc, ok ? "OK" : "FAIL");
    if (reasons[0]) printf("   reasons: %s\n", reasons);
    if (!ok && out[0]) printf("%.1500s\n", out);
    rm_rf(cwd);
    return ok;
#undef ADDR
}

int main(int argc, char **argv) {
    static char bindir[600], bin[700];
    static char fake_act[6][320];
    static const char *fake_cmd[6] = {
        "false # envelope 0:csih-x",
        "echo 'envelope → 0:csih-x.0: 80 chars' # envelope 0:csih-x",
        "false; true # envelope 0:csih-x",
        "echo 'envelope → 0:csih-x2.0: 80 chars' # envelope 0:csih-x2",
        "echo 'envelope → 0:csih-x.0: 80 chars'; exit 1 # envelope 0:csih-x",
        "sleep 2 # envelope 0:csih-x",
    };
    static const char *fake_name[6] = {
        "watch_false_comment", "watch_echo_receipt", "watch_false_true",
        "watch_wrong_peer", "watch_exit1", "watch_timeout",
    };
    static row_t rows[40];
    int nrows = 0, i, selected_n = 0, all_ok = 1;
    snap_t before, after;

#define ROW(nm, np, pr, rq, rcode, rd, ex, ...) do { \
        const char *_r[] = { __VA_ARGS__ }; \
        int _k; \
        rows[nrows].name = nm; rows[nrows].nresp = np; \
        for (_k = 0; _k < (np); _k++) rows[nrows].resp[_k] = _r[_k]; \
        rows[nrows].prompt = pr; rows[nrows].reqs = rq; rows[nrows].rc = rcode; \
        rows[nrows].red = rd; rows[nrows].extra_env = ex; nrows++; } while (0)

    snprintf(bindir, sizeof bindir, "%s/csih-loop-bin-XXXXXX", tmpbase());
    if (!mkdtemp(bindir)) die("mkdtemp bin");
    snprintf(bin, sizeof bin, "%s/agent-bin", bindir);
    if (build_bin(bin) != 0) die("agent build (no -include leak, rc/stderr)");

    ROW("outcome_failed", 2, "do it", 1, 1, 0, NULL, FAILED_A, STOP_A);
    ROW("outcome_partial", 2, "do it", 1, 1, 0, NULL, PARTIAL_A, STOP_A);
    ROW("outcome_legacy", 2, "do it", 1, 1, 0, NULL, LEGACY_A, STOP_A);
    ROW("outcome_failed_last", 8, "do it", 8, 1, 0, NULL, CONT, CONT, CONT, CONT, CONT, CONT, CONT, FAILED_A);
    ROW("outcome_last_continue", 9, "do it", 9, 1, 0, NULL, CONT, CONT, CONT, CONT, CONT, CONT, CONT, ANSWER_A, CONT);
    ROW("outcome_invalid", 1, "do it", 3, 1, 0, NULL, "{\"act\":\"answer\",\"outcome\":1,\"text\":\"bad\"}");
    ROW("outcome_duplicate", 1, "do it", 3, 1, 0, NULL, "{\"act\":\"answer\",\"outcome\":\"failed\",\"outcome\":\"completed\",\"text\":\"bad\"}");
    ROW("outcome_nul", 1, "do it", 3, 1, 0, NULL, "{\"act\":\"answer\",\"outcome\":\"completed\\u0000failed\",\"text\":\"bad\"}");
    ROW("watch_failed", 1, "do watch", 1, 1, 0, NULL, FAILED_A);
    ROW("watch_partial", 1, "do watch", 1, 1, 0, NULL, PARTIAL_A);
    ROW("normal", 3, "do it", 3, 0, 0, NULL, EXEC_A, ANSWER_A, STOP_A);
    ROW("parsefail", 1, "do it", 3, 1, 0, NULL, "UNPARSEABLE");
    ROW("stop_no_answer", 1, "do it", 3, 1, 0, NULL, STOP_A);
    ROW("direct_stop_no_answer", 3, "do it", 3, 1, 0, NULL, CONT, CONT, STOP_A);
    ROW("last_round_answer", 9, "do it", 9, 0, 0, NULL, CONT, CONT, CONT, CONT, CONT, CONT, CONT, ANSWER_A, STOP_A);
    ROW("action_budget", 1, "do it", 16, 1, 0, NULL, EXEC_A);
    ROW("round_budget", 1, "do it", 8, 1, 0, NULL, CONT);
    ROW("prior_answer_budget", 9, "do it", 9, 1, 0, NULL, ANSWER_A, CONT, CONT, CONT, CONT, CONT, CONT, CONT, CONT);
    ROW("invalid_judge", 2, "do it", 4, 1, 0, NULL, ANSWER_A, "INVALID_JUDGMENT");
    ROW("invalid_judge_recovery", 3, "do it", 3, 0, 0, NULL, ANSWER_A, "INVALID_JUDGMENT", STOP_A);
    ROW("no_tools_prose", 2, "不用工具，直接回答测试", 1, 1, 0, NULL, "直接答复", STOP_A);
    ROW("red", 2, "write probe", 2, 1, 1, NULL, RED_WRITE, STOP_A);
    ROW("watch_answer", 2, "do watch", 2, 1, 0, NULL, ANSWER_A, ANSWER_A);
    ROW("watch_stop", 1, "do watch", 1, 1, 0, NULL, STOP_A);
    ROW("watch_no_tools", 2, "不用工具，直接回答测试", 2, 0, 0, NULL, ANSWER_A, STOP_A);
    for (i = 0; i < 6; i++) {
        snprintf(fake_act[i], sizeof fake_act[i], "{\"act\": \"exec\", \"cmd\": \"%s\", \"why\": \"regression\"}", fake_cmd[i]);
        ROW(fake_name[i], 3, "do watch", 3, 1, 0,
            i == 5 ? "CSIH_EXEC_TIMEOUT_SEC='1'" : NULL,
            fake_act[i], ANSWER_A, STOP_A);
    }

    /* case selection (argv names) */
    {
        int chosen = 0;
        for (i = 1; i < argc; i++) {
            int j, found = 0;
            for (j = 0; j < nrows; j++) if (!strcmp(argv[i], rows[j].name)) found = 1;
            if (!found) { printf("FAIL unknown case %s\n", argv[i]); return 1; }
        }
        selected_n = 0;
        for (i = 0; i < nrows; i++) {
            int keep = argc == 1, j;
            for (j = 1; j < argc; j++) if (!strcmp(argv[j], rows[i].name)) keep = 1;
            if (keep) { rows[chosen++] = rows[i]; selected_n++; }
        }
        nrows = chosen;
    }
    if (nrows == 0) die("no rows");

    snap_hashes(&before);
    for (i = 0; i < nrows; i++) {
        if (!run_case(&rows[i], bin)) all_ok = 0;
    }
    snap_hashes(&after);
    {
        int passed = all_ok && snap_same(&before, &after);
        printf("TOTAL %s %d\n", passed ? "PASS" : "FAIL", nrows);
        snap_free(&before);
        snap_free(&after);
        rm_rf(bindir);
        return passed ? 0 : 1;
    }
#undef ROW
}
