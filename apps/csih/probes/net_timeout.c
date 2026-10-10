/* net_timeout.c — C port of net_timeout.py.
 *
 * A private copy of the app tree gets NET_TOTAL_SEC 60 -> 1. The private agent
 * must hit the timeout branch (err=-6) against a localhost stub that sleeps 3s,
 * with exactly one request. The production 60s TUI selftest also runs, against
 * an unchanged private snapshot.
 *
 * Freeze: the python sha256 check is replaced by a byte-for-byte snapshot
 * compare of the same file set (every .c/.h/.inc/.cx under the app dir, plus
 * unisacc.com), taken before and after.
 *
 * Build:  /bin/sh unisacc.com probes/net_timeout.c -o OUT
 * Run from the app dir (apps/csih), or set CSIH_APP to it.
 * Temp base: CSIH_PROBE_TMP, else TMPDIR (one of them must be set).
 * Server: 127.0.0.1:19002 (forked child).
 */
#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PORT 19002
#define NET_NEEDLE "#define NET_TOTAL_SEC     60"
#define NET_REPL   "#define NET_TOTAL_SEC     1"
#define AGENT_MSG  "agent FAILED: model call timed out (60s) (err=-6)"
#define PASS_LINE  "PASS private timeout branch (err=-6), one localhost request; production60s TUI selftest"
#define RUN_TIMEOUT 14.0

static const char *TUI_SRC[] = {
    "tui.c", "render.c", "term.c", "chat.c", "clock.c", "tools.c", "cols.cx", "home.cx",
    "file.c", "shell.c", "edit.c", "gate.c", "json.cx", "session.c", "agent.c", "plugin.c",
    "net.c", "reload_state.c", "reload_session_decode.c", "reload_session_encode.c",
    "reload_io.c", "reload_load.c", "reload_consume.c", "journal_checkpoint.c",
    "reload_owner.c", "csih_message.c", "csih_message_io.c", "context_index.c", NULL};
static const char *CLI_SRC[] = {
    "agent.c", "agent_cli.c", "cols.cx", "home.cx", "file.c", "edit.c", "shell.c",
    "json.cx", "session.c", "net.c", "plugin.c", NULL};
static const char *INC_NAMES[] = {"csih_cols.h", "csih_home.h", "json.h", NULL};

static char APP[4096];   /* absolute app dir (apps/csih) */
static char PRIV[4096];  /* private temp dir */

/* ---------- snapshot ---------- */
typedef struct {
    char **path;
    unsigned char **data;
    size_t *len;
    size_t n, cap;
    int bad;
} Snap;

static void snap_add(Snap *s, const char *path, unsigned char *data, size_t len) {
    if (s->n == s->cap) {
        s->cap = s->cap ? s->cap * 2 : 64;
        s->path = realloc(s->path, s->cap * sizeof *s->path);
        s->data = realloc(s->data, s->cap * sizeof *s->data);
        s->len = realloc(s->len, s->cap * sizeof *s->len);
        if (!s->path || !s->data || !s->len) exit(1);
    }
    /* not strdup: unisacc's strdup result cannot be free()d (crashes) */
    {
        size_t pl = strlen(path);
        char *cp = malloc(pl + 1);
        if (!cp) exit(1);
        memcpy(cp, path, pl + 1);
        s->path[s->n] = cp;
    }
    s->data[s->n] = data;
    s->len[s->n] = len;
    s->n++;
}

static void snap_free(Snap *s) {
    size_t i;
    for (i = 0; i < s->n; i++) { free(s->path[i]); free(s->data[i]); }
    free(s->path); free(s->data); free(s->len);
    memset(s, 0, sizeof *s);
}

/* Whole file, NUL-terminated (length excludes the NUL). NULL on error. */
static unsigned char *read_file(const char *path, size_t *len) {
    FILE *f = fopen(path, "rb");
    size_t cap = 65536, n = 0, k;
    unsigned char *buf;
    if (!f) return NULL;
    buf = malloc(cap + 1);
    if (!buf) exit(1);
    for (;;) {
        if (n == cap) { cap *= 2; buf = realloc(buf, cap + 1); if (!buf) exit(1); }
        k = fread(buf + n, 1, cap - n, f);
        if (k == 0) break;
        n += k;
    }
    fclose(f);
    buf[n] = 0;
    if (len) *len = n;
    return buf;
}

static int write_file(const char *path, const unsigned char *data, size_t len) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    if (len && fwrite(data, 1, len, f) != len) { fclose(f); return -1; }
    return fclose(f) == 0 ? 0 : -1;
}

static int is_src(const char *name) {
    size_t n = strlen(name);
    return (n > 2 && strcmp(name + n - 2, ".c") == 0) ||
           (n > 2 && strcmp(name + n - 2, ".h") == 0) ||
           (n > 4 && strcmp(name + n - 4, ".inc") == 0) ||
           (n > 3 && strcmp(name + n - 3, ".cx") == 0);
}

static void walk(const char *dir, Snap *s) {
    DIR *d = opendir(dir);
    struct dirent *e;
    if (!d) { s->bad = 1; return; }
    while ((e = readdir(d)) != NULL) {
        char full[4200];
        struct stat st;
        if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
        snprintf(full, sizeof full, "%s/%s", dir, e->d_name);
        if (lstat(full, &st) != 0) { s->bad = 1; continue; }
        if (S_ISLNK(st.st_mode)) { if (is_src(e->d_name)) s->bad = 1; continue; }
        if (S_ISDIR(st.st_mode)) { walk(full, s); continue; }
        if (S_ISREG(st.st_mode) && is_src(e->d_name)) {
            size_t len = 0;
            unsigned char *data = read_file(full, &len);
            if (!data) { s->bad = 1; continue; }
            snap_add(s, full, data, len);
        }
    }
    closedir(d);
}

/* App sources + unisacc.com, sorted by path. Returns 0 ok, -1 unreadable. */
static int snapshot(Snap *s) {
    size_t len = 0, i, j;
    unsigned char *root;
    char rootpath[4200];
    memset(s, 0, sizeof *s);
    walk(APP, s);
    snprintf(rootpath, sizeof rootpath, "%s/../../unisacc.com", APP);
    root = read_file(rootpath, &len);
    if (!root) s->bad = 1; else snap_add(s, rootpath, root, len);
    for (i = 1; i < s->n; i++)
        for (j = i; j > 0 && strcmp(s->path[j - 1], s->path[j]) > 0; j--) {
            char *p = s->path[j]; s->path[j] = s->path[j - 1]; s->path[j - 1] = p;
            unsigned char *d = s->data[j]; s->data[j] = s->data[j - 1]; s->data[j - 1] = d;
            size_t l = s->len[j]; s->len[j] = s->len[j - 1]; s->len[j - 1] = l;
        }
    return s->bad ? -1 : 0;
}

static int snap_equal(const Snap *a, const Snap *b) {
    size_t i;
    if (a->n != b->n) return 0;
    for (i = 0; i < a->n; i++) {
        if (strcmp(a->path[i], b->path[i]) != 0) return 0;
        if (a->len[i] != b->len[i]) return 0;
        if (memcmp(a->data[i], b->data[i], a->len[i]) != 0) return 0;
    }
    return 1;
}

/* ---------- filesystem helpers ---------- */
static void mkdir_p(const char *path) {
    char tmp[4200];
    char *p;
    snprintf(tmp, sizeof tmp, "%s", path);
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') { *p = 0; mkdir(tmp, 0755); *p = '/'; }
    }
    mkdir(tmp, 0755);
}

static void rm_rf(const char *path) {
    struct stat st;
    if (lstat(path, &st) != 0) return;
    if (!S_ISDIR(st.st_mode)) { unlink(path); return; }
    {
        DIR *d = opendir(path);
        struct dirent *e;
        if (d) {
            while ((e = readdir(d)) != NULL) {
                char full[4200];
                if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
                snprintf(full, sizeof full, "%s/%s", path, e->d_name);
                rm_rf(full);
            }
            closedir(d);
        }
    }
    rmdir(path);
}

/* Whole seconds. unisacc's gettimeofday tv_usec is not usable, so sub-second
 * timing is not available; time() is 1s resolution. */
static double now_sec(void) {
    return (double)time(NULL);
}

/* Runs `sh -c 'cd DIR && ENVS exec "$0" "$@"' BIN ARGS...` in its own session.
 * stdout/stderr go to files. Returns 0 ok (*rc set), -1 spawn error,
 * -2 timed out (child killed). */
static int run_timed(const char *dir, const char *envs, const char *bin,
                     char **args, int nargs, const char *out, const char *err, int *rc) {
    char script[8192];
    char *argv[96];
    int i, n = 0, st;
    pid_t pid;
    double start;
    if (nargs + 6 > 96) return -1;
    snprintf(script, sizeof script, "cd '%s' && %s exec \"$0\" \"$@\"", dir, envs);
    argv[n++] = (char *)"sh";
    argv[n++] = (char *)"-c";
    argv[n++] = script;
    argv[n++] = (char *)bin;
    for (i = 0; i < nargs; i++) argv[n++] = args[i];
    argv[n] = NULL;
    pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        int fi = open("/dev/null", O_RDONLY);
        int fo = open(out, O_WRONLY | O_CREAT | O_TRUNC, 0600);
        int fe = open(err, O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (fi >= 0) dup2(fi, 0);
        if (fo >= 0) dup2(fo, 1);
        if (fe >= 0) dup2(fe, 2);
        execv("/bin/sh", argv);
        _exit(127);
    }
    start = now_sec();
    for (;;) {
        pid_t w = waitpid(pid, &st, WNOHANG);
        if (w == pid) { *rc = WIFEXITED(st) ? WEXITSTATUS(st) : 128; return 0; }
        if (w < 0) return -1;
        if (now_sec() - start > RUN_TIMEOUT) {
            kill(pid, SIGKILL); /* sh exec()s the target, so pid is the program */
            waitpid(pid, &st, 0);
            return -2;
        }
        usleep(20000);
    }
}

/* env for every private run; `extra` is appended (e.g. CSIH_ENDPOINT). */
static void build_env(char *envs, size_t cap, const char *extra) {
    snprintf(envs, cap,
             "export HOME='%s/home'; export DEEPSEEK_API_KEY=LOCAL_STUB_ONLY; "
             "export CSIH_ROLE=; export CSIH_PEER=; export CSIH_CWD='%s'; "
             "export CSIH_TRANSCRIPT='%s/journal.jsonl'; export CSIH_MODEL=local-stub; "
             "export NO_PROXY=127.0.0.1,localhost; export no_proxy=127.0.0.1,localhost; "
             "unset OPENAI_API_KEY http_proxy https_proxy HTTP_PROXY HTTPS_PROXY all_proxy "
             "ALL_PROXY CDSH_ROLE CDSH_PEER CDSH_ENDPOINT CDSH_MODEL CDSH_CWD; %s",
             PRIV, PRIV, PRIV, extra ? extra : "");
}

/* sh <PRIV>/unisacc.com -include ABS... SRC... -o OUT, cwd = app copy. */
static int build_unisacc(const char *envs, const char *out_bin, const char **srcs,
                         const char *tag, int *rc) {
    char *args[96];
    char inc[3][4200];
    char compiler[4200], out_f[4200], err_f[4200];
    int n = 0, i, st;
    char app_copy[4200];
    snprintf(app_copy, sizeof app_copy, "%s/app", PRIV);
    snprintf(compiler, sizeof compiler, "%s/unisacc.com", PRIV);
    snprintf(out_f, sizeof out_f, "%s/%s.out", PRIV, tag);
    snprintf(err_f, sizeof err_f, "%s/%s.err", PRIV, tag);
    args[n++] = compiler;
    for (i = 0; INC_NAMES[i]; i++) {
        snprintf(inc[i], sizeof inc[i], "%s/%s", APP, INC_NAMES[i]);
        args[n++] = (char *)"-include";
        args[n++] = inc[i];
    }
    for (i = 0; srcs[i]; i++) args[n++] = (char *)srcs[i];
    args[n++] = (char *)"-o";
    args[n++] = (char *)out_bin;
    st = run_timed(app_copy, envs, "/bin/sh", args, n, out_f, err_f, rc);
    return st;
}

/* Replace the single NET needle in buf (length len). Caller frees result. */
static unsigned char *replace_net(const unsigned char *buf, size_t len, size_t *outlen) {
    const char *hit = strstr((const char *)buf, NET_NEEDLE);
    size_t pre = (size_t)(hit - (const char *)buf);
    size_t nl = strlen(NET_REPL), ol = strlen(NET_NEEDLE);
    unsigned char *o = malloc(len - ol + nl + 1);
    if (!o) exit(1);
    memcpy(o, buf, pre);
    memcpy(o + pre, NET_REPL, nl);
    memcpy(o + pre + nl, buf + pre + ol, len - pre - ol);
    *outlen = len - ol + nl;
    return o;
}

static int count_sub(const char *hay, const char *needle) {
    int n = 0;
    const char *p = hay;
    size_t l = strlen(needle);
    while ((p = strstr(p, needle)) != NULL) { n++; p += l; }
    return n;
}

/* Stub server child: same reply as the python handler. Requests are logged
 * to <PRIV>/requests.log after being read, then sleep 3s before answering. */
static void serve_child(int lfd) {
    static char req[65536];
    char resp[256];
    const char *body = "{\"choices\":[{\"message\":{\"content\":\"unused\"}}]}";
    for (;;) {
        int c = accept(lfd, NULL, NULL);
        size_t have = 0, hdr = 0, want = 0;
        int got = 0;
        char *e;
        if (c < 0) continue;
        for (;;) {
            ssize_t k;
            if (have >= sizeof req - 1) break;
            k = read(c, req + have, sizeof req - 1 - have);
            if (k <= 0) break;
            have += (size_t)k;
            req[have] = 0;
            if (!got && (e = strstr(req, "\r\n\r\n")) != NULL) {
                char *cl;
                got = 1;
                hdr = (size_t)(e - req) + 4;
                cl = strstr(req, "Content-Length:");
                if (!cl) cl = strstr(req, "content-length:");
                if (cl) want = (size_t)strtoul(cl + 15, NULL, 10);
            }
            if (got && have >= hdr + want) break;
        }
        if (got) {
            char lp[4200];
            int fd;
            snprintf(lp, sizeof lp, "%s/requests.log", PRIV);
            fd = open(lp, O_WRONLY | O_CREAT | O_APPEND, 0600);
            if (fd >= 0) { if (write(fd, "request\n", 8) < 0) { /* best effort */ } close(fd); }
        }
        sleep(3);
        snprintf(resp, sizeof resp,
                 "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nConnection: close\r\n\r\n",
                 (int)strlen(body));
        if (write(c, resp, strlen(resp)) < 0) { /* client gone */ }
        if (write(c, body, strlen(body)) < 0) { /* client gone */ }
        close(c);
    }
}

/* Returns NULL on pass, else the failure reason (static text). */
static const char *probe(void) {
    static char msg[512];
    Snap before;
    char envs[8192], agent_envs[8192], tbase_buf[4200];
    char tmpl[4200], tui_bin[4200], agent_bin[4200], out_f[4200], err_f[4200];
    char netc[4200], app_copy[4200], log_p[4200];
    const char *tbase;
    const char *err = NULL;
    unsigned char *netdata = NULL;
    size_t len = 0, i;
    int rc = 0, lfd = -1, srv = -1, one = 1, st;
    struct sockaddr_in a;
    double t0, t1;
    char *args[4];
    unsigned char *o; size_t ol = 0;

    memset(&before, 0, sizeof before);
    if (snapshot(&before) != 0) { snap_free(&before); return "snapshot before failed"; }

    snprintf(netc, sizeof netc, "%s/net.c", APP);
    netdata = read_file(netc, &len);
    if (!netdata) { err = "cannot read app net.c"; goto out; }
    if (count_sub((const char *)netdata, NET_NEEDLE) != 1) { err = "app net.c: NET_TOTAL_SEC 60 count != 1"; goto out; }

    tbase = getenv("CSIH_PROBE_TMP");
    if (!tbase || !tbase[0]) tbase = getenv("TMPDIR");
    if (!tbase || !tbase[0]) { err = "set CSIH_PROBE_TMP or TMPDIR"; goto out; }
    snprintf(tbase_buf, sizeof tbase_buf, "%s", tbase);
    snprintf(tmpl, sizeof tmpl, "%s/csih-net-timeout-XXXXXX", tbase_buf);
    if (!mkdtemp(tmpl)) { err = "mkdtemp failed"; goto out; }
    snprintf(PRIV, sizeof PRIV, "%s", tmpl);
    snprintf(app_copy, sizeof app_copy, "%s/app", PRIV);
    snprintf(tui_bin, sizeof tui_bin, "%s/tui", PRIV);
    snprintf(agent_bin, sizeof agent_bin, "%s/agent", PRIV);
    snprintf(log_p, sizeof log_p, "%s/requests.log", PRIV);

    /* private app copy (all sources) + private compiler + private home */
    mkdir(app_copy, 0755);
    for (i = 0; i < before.n; i++) {
        const char *p = before.path[i];
        size_t al = strlen(APP);
        char dst[4400], dir[4400];
        if (strncmp(p, APP, al) != 0 || p[al] != '/' || !is_src(p)) continue;
        snprintf(dst, sizeof dst, "%s/%s", app_copy, p + al + 1);
        snprintf(dir, sizeof dir, "%s", dst);
        *strrchr(dir, '/') = 0;
        mkdir_p(dir);
        if (write_file(dst, before.data[i], before.len[i]) != 0) { err = "copy failed"; goto out; }
    }
    for (i = 0; i < before.n; i++) {
        const char *p = before.path[i];
        size_t pl = strlen(p);
        if (pl > 12 && strcmp(p + pl - 12, "/unisacc.com") == 0) {
            char cp[4300];
            snprintf(cp, sizeof cp, "%s/unisacc.com", PRIV);
            if (write_file(cp, before.data[i], before.len[i]) != 0) { err = "compiler copy failed"; goto out; }
        }
    }
    {
        char hd[4300];
        snprintf(hd, sizeof hd, "%s/home", PRIV);
        mkdir(hd, 0700);
        snprintf(hd, sizeof hd, "%s/home/env.jsonl", PRIV);
        if (write_file(hd, (const unsigned char *)"{\"DEEPSEEK_API_KEY\":\"LOCAL_STUB_ONLY\"}\n", 39) != 0) { err = "env write failed"; goto out; }
    }

    build_env(envs, sizeof envs, NULL);

    /* production-style TUI build; headers come from the production app dir */
    {
        const char *srcs[64];
        int k = 0;
        for (i = 0; TUI_SRC[i]; i++) srcs[k++] = TUI_SRC[i];
        srcs[k] = NULL;
        st = build_unisacc(envs, tui_bin, srcs, "tui_build", &rc);
        if (st != 0 || rc != 0) { err = "tui build rc != 0"; goto out; }
    }

    /* selftest: unchanged private snapshot, 60s threshold */
    snprintf(out_f, sizeof out_f, "%s/tui_self.out", PRIV);
    snprintf(err_f, sizeof err_f, "%s/tui_self.err", PRIV);
    args[0] = (char *)"selftest";
    st = run_timed(app_copy, envs, tui_bin, args, 1, out_f, err_f, &rc);
    if (st != 0) { err = "tui selftest timed out or spawn failed"; goto out; }
    {
        unsigned char *so = read_file(out_f, NULL), *se = read_file(err_f, NULL);
        const char *s = so ? (const char *)so : "";
        const char *e2 = se ? (const char *)se : "";
        size_t sl = strlen(s);
        const char *end = s + sl;
        int ok;
        while (end > s && (end[-1] == '\n' || end[-1] == '\r')) end--;
        {
            const char *q = end;
            while (q > s && q[-1] != '\n') q--;
            ok = rc == 0 && end - q == 11 && memcmp(q, "selftest ok", 11) == 0 &&
                 strstr(s, "FAIL") == NULL && e2[0] == 0 && end > s;
        }
        free(so); free(se);
        if (!ok) { err = "tui selftest gate failed (need rc 0, last line 'selftest ok', no FAIL, empty stderr)"; goto out; }
    }

    /* private threshold: NET_TOTAL_SEC 60 -> 1 in the private copy only */
    {
        char netp[4300];
        size_t nl = 0;
        unsigned char *nb;
        snprintf(netp, sizeof netp, "%s/net.c", app_copy);
        nb = read_file(netp, &nl);
        if (!nb || count_sub((const char *)nb, NET_NEEDLE) != 1) { free(nb); err = "private net.c count != 1"; goto out; }
        o = replace_net(nb, nl, &ol);
        free(nb);
        if (write_file(netp, o, ol) != 0) { free(o); err = "private net.c write failed"; goto out; }
        free(o);
    }

    /* private agent build, same production headers */
    {
        const char *srcs[64];
        int k = 0;
        for (i = 0; CLI_SRC[i]; i++) srcs[k++] = CLI_SRC[i];
        srcs[k] = NULL;
        st = build_unisacc(envs, agent_bin, srcs, "agent_build", &rc);
        if (st != 0 || rc != 0) { err = "agent build rc != 0"; goto out; }
    }

    /* localhost stub on 127.0.0.1:PORT, forked child */
    lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd < 0) { err = "stub socket failed"; goto out; }
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_port = htons(PORT);
    a.sin_addr.s_addr = htonl(0x7f000001);
    if (bind(lfd, (struct sockaddr *)&a, sizeof a) != 0) { err = "stub bind failed"; goto out; }
    if (listen(lfd, 8) != 0) { err = "stub listen failed"; goto out; }
    srv = fork();
    if (srv < 0) { err = "stub fork failed"; goto out; }
    if (srv == 0) {
        serve_child(lfd);
        _exit(0);
    }
    close(lfd);
    lfd = -1;

    /* agent run against the stub */
    snprintf(out_f, sizeof out_f, "%s/agent.out", PRIV);
    snprintf(err_f, sizeof err_f, "%s/agent.err", PRIV);
    {
        char extra[256];
        snprintf(extra, sizeof extra, "export CSIH_ENDPOINT=http://127.0.0.1:%d/v1/chat/completions;", PORT);
        build_env(agent_envs, sizeof agent_envs, extra);
    }
    args[0] = (char *)"agent";
    args[1] = (char *)"TIMEOUT_PROBE";
    t0 = now_sec();
    st = run_timed(app_copy, agent_envs, agent_bin, args, 2, out_f, err_f, &rc);
    t1 = now_sec();
    if (st != 0) { err = "agent run timed out or spawn failed"; goto out; }
    {
        unsigned char *lg = read_file(log_p, NULL);
        int requests = lg ? count_sub((const char *)lg, "request\n") : 0;
        unsigned char *ao = read_file(out_f, NULL);
        free(lg);
        if (rc != 1 || requests != 1) {
            snprintf(msg, sizeof msg, "agent rc=%d requests=%d (want rc 1, 1 request)", rc, requests);
            free(ao);
            err = msg;
            goto out;
        }
        if (!ao || !strstr((const char *)ao, AGENT_MSG)) {
            free(ao);
            err = "agent stdout missing timeout message (err=-6)";
            goto out;
        }
        free(ao);
    }
    if (t1 - t0 >= 5.0) { err = "agent run took 5s or more"; goto out; }

out:
    if (srv > 0) { kill(srv, SIGKILL); waitpid(srv, &st, 0); }
    if (lfd >= 0) close(lfd);
    if (PRIV[0]) rm_rf(PRIV);
    free(netdata);
    {
        Snap after;
        memset(&after, 0, sizeof after);
        if (snapshot(&after) != 0 || !snap_equal(&before, &after)) {
            snap_free(&after);
            snap_free(&before);
            return err ? err : "production source/compiler changed";
        }
        snap_free(&after);
    }
    snap_free(&before);
    return err;
}

int main(void) {
    const char *cwd_env = getenv("CSIH_APP");
    const char *reason;
    if (cwd_env && cwd_env[0]) snprintf(APP, sizeof APP, "%s", cwd_env);
    else if (!getcwd(APP, sizeof APP)) { printf("FAIL getcwd\n"); return 1; }
    if (APP[0] != '/') { printf("FAIL app dir must be absolute\n"); return 1; }
    reason = probe();
    if (reason) { printf("FAIL %s\n", reason); return 1; }
    printf("%s\n", PASS_LINE);
    return 0;
}
