/* agent_multi_action.c — C port of probes/agent_multi_action.py.
 *
 * Real default-native/local-HTTP multiple-action side effects, private only.
 * Copies the csih src files into a private temp tree, builds the agent there
 * with unisacc (-include globals + AGENT list), then runs each case against a
 * forked C HTTP stub. A rejected multi-exec followed by answer may return 0;
 * that row proves rejection/zero side effects, not honesty of completion.
 *
 *   /bin/sh <private compiler copy> <-include ...> -o OUT <AGENT>   (cwd: private source)
 *   OUT agent PROMPT                                                (cwd: case dir)
 *
 * Verdict: last line "PASS agent multi-action" or "FAIL agent multi-action"; rc 0/1.
 * Optional argv: case names. Temp dirs use $TMPDIR (fallback /tmp).
 */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include "csih_flat.h"

#define MA_UNISACC CSIH_ROOT "/unisacc.com"
#define MA_SELF CSIH_APP "/probes/agent_multi_action.c"
#define MA_PORT_LO 19150
#define MA_PORT_HI 19199

/* ---------- constants (json.dumps-equivalent action text) ---------- */
#define W1 "{\"act\": \"file\", \"op\": \"write\", \"path\": \"first.txt\", \"text\": \"第一份中文\\n\"}"
#define W2 "{\"act\": \"file\", \"op\": \"write\", \"path\": \"second.txt\", \"text\": \"第二份中文\\n\"}"
#define DW W1 "\n" W2
#define DE "{\"act\": \"exec\", \"cmd\": \"touch first.txt\", \"why\": \"private first\"}\n" \
           "{\"act\": \"exec\", \"cmd\": \"touch second.txt\", \"why\": \"private second\"}"
#define ANS "{\"act\": \"answer\", \"text\": \"done\"}"
#define STP "{\"go\": \"stop\"}"
#define SF "{\"act\": \"file\", \"op\": \"write\", \"path\": \"first.txt\", \"text\": \"字符串内 {中文} 与 \\\"引号\\\"\\n\"}"

/* ---------- heap / files ---------- */

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
    struct stat st;
    unsigned char *buf;
    size_t got = 0;
    int fd = open(path, O_RDONLY);
    if (fd < 0) return NULL;
    if (fstat(fd, &st) != 0) { close(fd); return NULL; }
    buf = xmalloc((size_t)st.st_size);
    while (got < (size_t)st.st_size) {
        ssize_t k = read(fd, buf + got, (size_t)st.st_size - got);
        if (k < 0) { if (errno == EINTR) continue; break; }
        if (k == 0) break;
        got += (size_t)k;
    }
    close(fd);
    if (got != (size_t)st.st_size) { free(buf); return NULL; }
    *len = got;
    return buf;
}

static int write_file(const char *path, const void *data, size_t n, mode_t mode) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, mode);
    size_t off = 0;
    if (fd < 0) return -1;
    while (off < n) {
        ssize_t k = write(fd, (const char *)data + off, n - off);
        if (k < 0) { if (errno == EINTR) continue; close(fd); return -1; }
        off += (size_t)k;
    }
    close(fd);
    chmod(path, mode);
    return 0;
}

static int ends(const char *s, const char *suf) {
    size_t a = strlen(s), b = strlen(suf);
    return a >= b && !strcmp(s + a - b, suf);
}

static int is_src(const char *name) {
    return ends(name, ".c") || ends(name, ".h") || ends(name, ".inc") || ends(name, ".cx");
}

/* ---------- snapshot (byte-for-byte replacement for sha256 freeze) ---------- */

typedef struct {
    char **path;
    unsigned char **data;
    size_t *len;
    size_t n, cap;
} snap_t;

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

/* srconly: only .c/.h/.inc/.cx; otherwise every regular file */
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
    snap_add(s, MA_UNISACC);
    snap_add(s, MA_SELF);
}

/* Copy src files of tree src into dst (dirs created as needed). Symlinked src files are fatal. */
static void copy_tree(const char *src, const char *dst) {
    DIR *d = opendir(src);
    struct dirent *e;
    if (!d) die("copy opendir");
    while ((e = readdir(d)) != NULL) {
        char cs[4096], cd[4096];
        struct stat st;
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        snprintf(cs, sizeof cs, "%s/%s", src, e->d_name);
        snprintf(cd, sizeof cd, "%s/%s", dst, e->d_name);
        if (lstat(cs, &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) {
            mkdir(cd, 0700);
            copy_tree(cs, cd);
        } else if (S_ISLNK(st.st_mode)) {
            if (is_src(e->d_name)) die("symlink in source");
        } else if (S_ISREG(st.st_mode) && is_src(e->d_name)) {
            size_t len = 0;
            unsigned char *b = read_file(cs, &len);
            if (!b) die("copy read");
            if (write_file(cd, b, len, 0644) != 0) die("copy write");
            free(b);
        }
    }
    closedir(d);
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

static double now_s(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1e6;
}

/* ---------- stub HTTP server (forked child) ---------- */

typedef struct {
    int lsock;
    int nresp;
    const char *resp[4];
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
    for (p = MA_PORT_LO; p <= MA_PORT_HI; p++) {
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
    const char *resp[6];
    int nresp;
    const char *prompt;
    int reqs, rc;
    int nfiles;               /* expected side-effect files: first.txt / second.txt */
    const char *f1, *f2;      /* expected contents (NULL = must not exist) */
    const char *cause;        /* expected "unfinished" cause, or NULL */
    const char *limit;
} case_t;

static const case_t CASES[] = {
    {"double-write-repeat", {DW, DW, DW}, 3, "write the files", 3, 1, 0, NULL, NULL, "too many unparseable steps", NULL},
    {"double-fenced-leading", {"说明：只发一件。\n```json\n" DW "\n```", "说明：只发一件。\n```json\n" DW "\n```", "说明：只发一件。\n```json\n" DW "\n```"},
     3, "write the files", 3, 1, 0, NULL, NULL, "too many unparseable steps", NULL},
    {"double-after-fence", {"```json\n" W1 "\n```\n" W2, "```json\n" W1 "\n```\n" W2, "```json\n" W1 "\n```\n" W2},
     3, "write the files", 3, 1, 0, NULL, NULL, "too many unparseable steps", NULL},
    {"double-english-leading", {"next step\n" DW, "next step\n" DW, "next step\n" DW},
     3, "write the files", 3, 1, 0, NULL, NULL, "too many unparseable steps", NULL},
    {"malformed-array-shell", {"[ BROKEN " W1 " ]", "[ BROKEN " W1 " ]", "[ BROKEN " W1 " ]"},
     3, "write one file", 3, 1, 0, NULL, NULL, "too many unparseable steps", NULL},
    {"double-write-recovery", {DW, W1, W2, ANS, STP}, 5, "write the files", 5, 0, 2, "第一份中文\n", "第二份中文\n", NULL, NULL},
    {"double-exec-rejected", {DE, ANS, STP}, 3, "do the actions", 3, 0, 0, NULL, NULL, NULL,
     "rc0 after a separate answer is not proof that rejected work completed"},
    {"single-fenced-braces", {"说明：只执行一个对象。\n```json\n" SF "\n```", ANS, STP}, 3, "write one file", 3, 0, 1,
     "字符串内 {中文} 与 \"引号\"\n", NULL, NULL, NULL},
    {"no-tools-braces", {"中文说明，示例代码 if (x) { return; }，只需直接答复。", STP}, 2, "不用工具，直接回答测试", 2, 0, 0,
     NULL, NULL, NULL, NULL},
};
#define NCASES ((int)(sizeof CASES / sizeof CASES[0]))

typedef struct {
    char *cwd;
    char *source;
    char *binary;
    char *root;
    double deadline;
} ctx_t;

static int run_case(const case_t *c, ctx_t *x) {
    char cwd[700], home[760], trace[760], countp[760], script[3072], ep[160], line[512];
    static char out[1 << 16];
    char reasons[1024] = "";
    int lsock, port = 0, rc = 0, timed = 0, reqs = 0, ok = 1, i;
    pid_t stub;
    char *args[4];
    char *tbuf;
    size_t tlen = 0;
    double remaining;
    int tmo;
    char path1[800], path2[800];

#define MADDR(...) do { size_t _l = strlen(reasons); snprintf(reasons + _l, sizeof reasons - _l, __VA_ARGS__); ok = 0; } while (0)

    snprintf(cwd, sizeof cwd, "%s/%s", x->root, c->name);
    mkdir(cwd, 0700);
    snprintf(home, sizeof home, "%s/home", cwd);
    snprintf(trace, sizeof trace, "%s/trace.jsonl", cwd);
    snprintf(countp, sizeof countp, "%s/reqs.txt", cwd);
    mkdir(home, 0700);
    snprintf(line, sizeof line, "%s/env.jsonl", home);
    write_file(line, "{\"DEEPSEEK_API_KEY\":\"LOCAL_STUB_ONLY\"}\n", 36, 0600);

    lsock = listen_local(&port);
    if (lsock < 0) die("stub listen");
    {
        stub_t st;
        memset(&st, 0, sizeof st);
        st.lsock = lsock;
        st.nresp = c->nresp;
        for (i = 0; i < c->nresp; i++) st.resp[i] = c->resp[i];
        st.countpath = countp;
        stub = fork();
        if (stub == 0) stub_serve(&st);
    }
    close(lsock);
    snprintf(ep, sizeof ep, "http://127.0.0.1:%d/v1/chat/completions", port);

    remaining = x->deadline - now_s();
    if (remaining <= 0) MADDR("selector deadline before run; ");
    tmo = remaining < 9 ? (int)remaining + 1 : 9;
    snprintf(script, sizeof script,
             "unset CSIH_ROLE CSIH_PEER OPENAI_API_KEY HTTP_PROXY HTTPS_PROXY ALL_PROXY http_proxy https_proxy all_proxy; "
             "export NO_PROXY='127.0.0.1,localhost' no_proxy='127.0.0.1,localhost' "
             "HOME='%s' DEEPSEEK_API_KEY='LOCAL_STUB_ONLY' CSIH_ENDPOINT='%s' CSIH_MODEL='multi-action-stub' "
             "CSIH_CWD='%s' CSIH_TRANSCRIPT='%s'; cd '%s' && exec \"$0\" \"$@\"",
             home, ep, cwd, trace, cwd);
    args[0] = (char *)"agent";
    args[1] = (char *)c->prompt;
    spawn(script, x->binary, args, 2, tmo, out, sizeof out, &rc, &timed);
    if (timed) rc = -1;
    kill(stub, SIGKILL);
    waitpid(stub, NULL, 0);
    reqs = read_int(countp);

    if (timed) MADDR("timed out; ");
    if (rc != c->rc || reqs != c->reqs) MADDR("rc/reqs %d/%d != %d/%d; ", rc, reqs, c->rc, c->reqs);

    /* private side effects: exact bytes for first.txt / second.txt */
    snprintf(path1, sizeof path1, "%s/first.txt", cwd);
    snprintf(path2, sizeof path2, "%s/second.txt", cwd);
    {
        size_t l1 = 0, l2 = 0;
        unsigned char *b1 = read_file(path1, &l1), *b2 = read_file(path2, &l2);
        int want1 = c->nfiles >= 1 && c->f1 != NULL, want2 = c->nfiles >= 2 && c->f2 != NULL;
        if (c->nfiles == 1 && c->f1 == NULL) want1 = 0;
        if (c->nfiles == 0) { want1 = 0; want2 = 0; }
        if (!strcmp(c->name, "single-fenced-braces")) { want1 = 1; want2 = 0; }
        if (!strcmp(c->name, "double-write-recovery")) { want1 = 1; want2 = 1; }
        if ((b1 != NULL) != want1) MADDR("first.txt existence mismatch; ");
        if ((b2 != NULL) != want2) MADDR("second.txt existence mismatch; ");
        if (b1 && want1 && (l1 != strlen(c->f1) || memcmp(b1, c->f1, l1))) MADDR("first.txt content mismatch; ");
        if (b2 && want2 && (l2 != strlen(c->f2) || memcmp(b2, c->f2, l2))) MADDR("second.txt content mismatch; ");
        free(b1);
        free(b2);
    }
    if (c->cause && (!strstr(out, "unfinished") || !strstr(out, c->cause))) MADDR("missing unfinished cause; ");

    tbuf = (char *)read_file(trace, &tlen);
    if (!tbuf) { tbuf = xmalloc(1); tlen = 0; }
    {
        char *t2 = xmalloc(tlen + 1);
        memcpy(t2, tbuf, tlen);
        t2[tlen] = 0;
        free(tbuf);
        tbuf = t2;
    }
    if (!strncmp(c->name, "double-", 7) && !trace_count(tbuf, "name", "error", NULL, NULL, NULL, NULL))
        MADDR("no actual rejected-action error trace; ");
    if (!strcmp(c->name, "no-tools-braces") && !trace_count(tbuf, "role", "assistant", "text", c->resp[0], NULL, NULL))
        MADDR("no-tools assistant trace missing; ");
    free(tbuf);

    printf("case %-24s %s%s%s\n", c->name, ok ? "PASS" : "FAIL", reasons[0] ? "  reasons: " : "", reasons);
    if (!ok && out[0]) printf("%.1200s\n", out);
    (void)path1;
    (void)path2;
    return ok;
#undef MADDR
}

int main(int argc, char **argv) {
    static char root[700], source[760], compiler[800], binary[800], errp[800];
    snap_t before, after, priv_before, priv_after;
    ctx_t x;
    int i, all = 1, nsel = 0, j;
    int frozen, private_same;
    char script[1024];
    char *args[64];
    int n = 0, rc = -1, timed = 0;
    char ebuf[256];
    size_t cl = 0;
    unsigned char *e;

    snap_hashes(&before);
    snprintf(root, sizeof root, "%s/csih-agent-multi-XXXXXX", tmpbase());
    if (!mkdtemp(root)) die("mkdtemp root");
    snprintf(source, sizeof source, "%s/source", root);
    mkdir(source, 0700);
    copy_tree(CSIH_APP, source);
    snprintf(compiler, sizeof compiler, "%s/compiler.com", root);
    {
        size_t len = 0;
        unsigned char *b = read_file(MA_UNISACC, &len);
        if (!b) die("read unisacc");
        if (write_file(compiler, b, len, 0600) != 0) die("write compiler");
        /* compiler copy must be byte-identical to the frozen unisacc.com */
        for (j = 0; (size_t)j < before.n; j++) if (!strcmp(before.path[j], MA_UNISACC)) break;
        if ((size_t)j >= before.n || before.len[j] != len || memcmp(before.data[j], b, len)) die("compiler copy differs");
        free(b);
    }
    snprintf(binary, sizeof binary, "%s/agent-native", root);
    snprintf(errp, sizeof errp, "%s/build-err.txt", root);
    snprintf(script, sizeof script, "cd '%s' && exec /bin/sh \"$0\" \"$@\" 2>'%s'", source, errp);
    for (i = 0; csih_global_include_args[i]; i++) args[n++] = (char *)csih_global_include_args[i];
    args[n++] = (char *)"-o";
    args[n++] = binary;
    for (i = 0; csih_agent[i]; i++) args[n++] = (char *)csih_agent[i];
    spawn(script, compiler, args, n, 14, ebuf, sizeof ebuf, &rc, &timed);
    if (rc != 0 || timed) die("multi-action build failed");
    e = read_file(errp, &cl);
    if (e && cl) die("multi-action build wrote stderr");
    free(e);
    unlink(errp);

    memset(&priv_before, 0, sizeof priv_before);
    walk(&priv_before, source, 0);

    x.root = root;
    x.source = source;
    x.binary = binary;
    x.cwd = NULL;
    x.deadline = now_s() + 55;

    for (i = 1; i < argc; i++) {
        int found = 0;
        for (j = 0; j < NCASES; j++) if (!strcmp(argv[i], CASES[j].name)) found = 1;
        if (!found) { printf("FAIL unknown case %s\n", argv[i]); rm_rf(root); return 1; }
    }
    for (j = 0; j < NCASES; j++) {
        int keep = argc == 1, k;
        for (k = 1; k < argc; k++) if (!strcmp(argv[k], CASES[j].name)) keep = 1;
        if (!keep) continue;
        nsel++;
        if (!run_case(&CASES[j], &x)) all = 0;
    }

    memset(&priv_after, 0, sizeof priv_after);
    walk(&priv_after, source, 0);
    private_same = snap_same(&priv_before, &priv_after);
    snap_hashes(&after);
    frozen = snap_same(&before, &after);
    if (!frozen) printf("production input changed during run\n");
    if (!private_same) printf("private sources changed during run\n");
    {
        int passed = all && nsel > 0 && frozen && private_same;
        printf("%s agent multi-action\n", passed ? "PASS" : "FAIL");
        rm_rf(root);
        return passed ? 0 : 1;
    }
}
