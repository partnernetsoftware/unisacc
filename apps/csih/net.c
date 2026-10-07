/*
 * net.c — talk HTTP/1.1 over a TCP socket. This is the layer csih needs to
 * reach a model, and it is deliberately the smallest thing that can do it.
 *
 * http:// is hand-written sockets. The host is resolved with getaddrinfo
 * (unisacc 0.0.25 forwards that to the system resolver, including from a
 * second source file as of 0.0.26). https:// calls libcurl in this file:
 * dlopen RTLD_GLOBAL, then the forwarded curl_* names. libcurl's body is
 * flushed from the host libc stdout into a pipe — unisacc fflush is a no-op,
 * and a unisacc FILE* or function must not be handed to curl. Redirects,
 * pooling, and chunked decoding are still not implemented.
 *
 * WHAT THIS DOES NOT DO, explicitly, because each is a silent wrong answer if
 * assumed away:
 *   - https:// is not rewritten as http://. Certificate checks are libcurl's.
 *   - NO REDIRECTS. A 3xx is returned to the caller as a 3xx.
 *   - NO CHUNKED TRANSFER DECODING. Content-Length is honoured; a chunked
 *     response is returned raw and flagged.
 *   - NO CONNECTION REUSE. One request, one socket, closed after.
 *
 * http:// stays a hand-written socket. That is what the loopback probes use.
 *
 * THE sys/wait.h TRAP — FIXED (unisacc, 2026-10-02). Until recently `socket()`
 * failed to build with `undefined function '_unisa_ret'` unless <sys/wait.h> was
 * also included, even though nothing here uses wait(). An unexplained include is
 * indistinguishable from cargo cult, so it was carried with a note and pinned by
 * probes/net-cover.sh. The probe flipped to FIXED, so the include and its note
 * are gone — verified by building net.c without it (probe reports ok, and
 * check.sh asserts this file no longer mentions the header). If the trap ever
 * returns, check.sh goes red and names it; do not re-add the include blind.
 *
 * unisacc limits honoured (SKILL.md §2): structs restated so each module builds
 * alone; no `return f()` of a struct from a non-main function; stdio owned by
 * the CLI file.
 *
 * CLI takes SUBCOMMANDS, not dash-options.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unisacc_ffi.h>

void *curl_easy_init(void);
int curl_easy_perform(void *h);
void curl_easy_cleanup(void *h);
void *curl_multi_init(void);
int curl_multi_add_handle(void *m, void *e);
int curl_multi_perform(void *m, int *running);
int curl_multi_wait(void *m, void *extra, unsigned extra_nfds, int timeout_ms, int *numfds);
int curl_multi_remove_handle(void *m, void *e);
void curl_multi_cleanup(void *m);
#define CURLOPT_NOSIGNAL 99
void *curl_slist_append(void *list, const char *s);
void curl_slist_free_all(void *list);
#define CURLOPT_TIMEOUT    13
#define CURLOPT_CONNECTTIMEOUT 78
#define CURLOPT_WRITEDATA  10001
#define CURLOPT_URL        10002
#define CURLOPT_POSTFIELDS 10015
#define CURLOPT_HTTPHEADER 10023
#define CURLINFO_RESPONSE_CODE 0x200002

/* ── results ────────────────────────────────────────────────────────────── */

#define NET_BODY_MAX  65536
#define NET_HDR_MAX   16384

typedef struct {
    int  ok;                    /* 1 = a complete HTTP response was parsed */
    int  err;                   /* errno when ok == 0 */
    int  status;                /* HTTP status code */
    long body_bytes;            /* bytes of body captured; -1 = truncated */
    int  chunked;               /* 1 = the response was chunked (NOT decoded) */
    char header[NET_HDR_MAX];   /* the raw status line + headers */
    char body[NET_BODY_MAX];
} net_response;

/* A refusal that is ours, not the OS's — same pattern as file.c's FILE_TOO_BIG. */
#define NET_REFUSED_HTTPS (-2001)   /* kept: curl missing, not "https is forbidden" */
#define NET_REFUSED_URL   (-2002)
#define NET_NO_CURL       (-2003)
#define NET_CANCELLED     (-2004)
#define NET_TIMEOUT       (-2005)
#define NET_TOTAL_SEC     120

/*
 * A tiny local case-insensitive compare.
 *
 * Declared and defined HERE, above its use, rather than after it: the first
 * version defined it at the bottom of the file with no prototype, which is a
 * call to an undeclared function — a constraint violation in C99 that gcc only
 * warns about and unisacc reports as a confusing "undefined function". Header
 * folding is not something this file should depend on either; five lines is
 * cheaper than the question.
 */
static int strncasecmp_local(const char *a, const char *b, size_t n) {
    size_t i;
    for (i = 0; i < n; i++) {
        char ca = a[i], cb = b[i];
        if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
        if (ca != cb) return (int)(unsigned char)ca - (int)(unsigned char)cb;
        if (ca == '\0') return 0;
    }
    return 0;
}

/* ── URL splitting ──────────────────────────────────────────────────────── */

typedef struct {
    char host[256];
    char port[16];
    char path[1024];
    int  https;
} net_url;

/*
 * Split `http://host[:port]/path` into its parts.
 *
 * https:// is parsed and later handed to libcurl. It is not rewritten as http://.
 */
int net_parse_url(const char *url, net_url *u) {
    const char *p, *slash;
    memset(u, 0, sizeof *u);

    if (!strncmp(url, "https://", 8)) { u->https = 1; p = url + 8; }
    else if (!strncmp(url, "http://", 7)) p = url + 7;
    else return NET_REFUSED_URL;

    slash = strchr(p, '/');
    {
        const char *colon = strchr(p, ':');
        const char *hostend = slash ? slash : p + strlen(p);
        if (colon && colon < hostend) {
            size_t hl = (size_t)(colon - p);
            size_t pl;
            if (hl >= sizeof u->host) return NET_REFUSED_URL;
            memcpy(u->host, p, hl);
            u->host[hl] = '\0';
            pl = (size_t)(hostend - colon - 1);
            if (pl >= sizeof u->port) return NET_REFUSED_URL;
            memcpy(u->port, colon + 1, pl);
            u->port[pl] = '\0';
        } else {
            size_t hl = (size_t)(hostend - p);
            if (hl >= sizeof u->host) return NET_REFUSED_URL;
            memcpy(u->host, p, hl);
            u->host[hl] = '\0';
            strcpy(u->port, u->https ? "443" : "80");
        }
    }
    if (slash) {
        if (strlen(slash) >= sizeof u->path) return NET_REFUSED_URL;
        strcpy(u->path, slash);
    } else {
        strcpy(u->path, "/");
    }
    if (u->host[0] == '\0') return NET_REFUSED_URL;
    return 0;
}

/* ── connect ────────────────────────────────────────────────────────────── */

/* A Winsock SOCKET is pointer-width and must never enter the POSIX fd layer. */
#define NET_INVALID_SOCKET (~0UL)
static void net_socket_error(void) {
#ifdef _WIN32
    int e = WSAGetLastError();
    errno = e == 10004 ? EINTR : e;
#endif
}
static void net_socket_close(unsigned long fd) {
#ifdef _WIN32
    closesocket(fd);
#else
    close((int)fd);
#endif
}
static long net_socket_send(unsigned long fd, const void *buf, size_t len) {
#ifdef _WIN32
    int n = (int)(len > 0x7fffffffUL ? 0x7fffffffUL : len);
    int r = send(fd, (const char *)buf, n, 0);
    if (r < 0) net_socket_error();
    return r;
#else
    return send((int)fd, buf, len, 0);
#endif
}
static long net_socket_recv(unsigned long fd, void *buf, size_t len) {
#ifdef _WIN32
    int n = (int)(len > 0x7fffffffUL ? 0x7fffffffUL : len);
    int r = recv(fd, (char *)buf, n, 0);
    if (r < 0) net_socket_error();
    return r;
#else
    return recv((int)fd, buf, len, 0);
#endif
}
/* Any name getaddrinfo can resolve, not only numeric IPv4 and localhost. */
static unsigned long net_connect(const char *host, const char *port) {
    struct addrinfo hints, *res = 0, *p;
    unsigned long fd = NET_INVALID_SOCKET;
    int rc;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    rc = getaddrinfo(host, port, &hints, &res);
    if (rc != 0) return -1;
    for (p = res; p; p = p->ai_next) {
        fd = (unsigned long)socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd == NET_INVALID_SOCKET) { net_socket_error(); continue; }
        if (connect(fd, p->ai_addr, (int)p->ai_addrlen) == 0) break;
        net_socket_error();
        net_socket_close(fd);
        fd = NET_INVALID_SOCKET;
    }
    freeaddrinfo(res);
    return fd;
}

#ifdef __APPLE__
static int curl_setopt(void *h, int opt, void *val) {
    static void *fn; int kinds[3]; void *vals[3]; int r = 0;
    if (!fn) fn = uffi_dlsym((void *)UFFI_RTLD_DEFAULT, "curl_easy_setopt");
    kinds[0] = UFFI_POINTER; kinds[1] = UFFI_INT; kinds[2] = UFFI_POINTER;
    vals[0] = &h; vals[1] = &opt; vals[2] = &val;
    uffi_call(fn, UFFI_INT, kinds, vals, 3, 2, &r);
    return r;
}
static int curl_setopt_long(void *h, int opt, long val) {
    static void *fn; int kinds[3]; void *vals[3]; int r = 0;
    if (!fn) fn = uffi_dlsym((void *)UFFI_RTLD_DEFAULT, "curl_easy_setopt");
    kinds[0] = UFFI_POINTER; kinds[1] = UFFI_INT; kinds[2] = UFFI_LONG;
    vals[0] = &h; vals[1] = &opt; vals[2] = &val;
    uffi_call(fn, UFFI_INT, kinds, vals, 3, 2, &r);
    return r;
}
static int curl_getinfo(void *h, int info, long *out) {
    static void *fn; int kinds[3]; void *vals[3]; int r = 0;
    if (!fn) fn = uffi_dlsym((void *)UFFI_RTLD_DEFAULT, "curl_easy_getinfo");
    kinds[0] = UFFI_POINTER; kinds[1] = UFFI_INT; kinds[2] = UFFI_POINTER;
    vals[0] = &h; vals[1] = &info; vals[2] = &out;
    uffi_call(fn, UFFI_INT, kinds, vals, 3, 2, &r);
    return r;
}
static void flush_host_stdout(void) {
    void *ff, *slot, *file;
    int kinds[1]; void *vals[1]; int r = 0;
    ff = uffi_dlsym((void *)UFFI_RTLD_DEFAULT, "fflush");
    slot = uffi_dlsym((void *)UFFI_RTLD_DEFAULT, "__stdoutp");
    if (!ff || !slot) return;
    file = *(void **)slot;
    kinds[0] = UFFI_POINTER; vals[0] = &file;
    uffi_call(ff, UFFI_INT, kinds, vals, 1, 1, &r);
}
#else
int curl_easy_setopt(void *h, int opt, void *val);
int curl_easy_getinfo(void *h, int info, long *out);
#define curl_setopt curl_easy_setopt
#define curl_getinfo curl_easy_getinfo
static int curl_setopt_long(void *h, int opt, long val) {
    return curl_easy_setopt(h, opt, (void *)val);
}
static void flush_host_stdout(void) {
    void *ff, *slot, *file;
    int kinds[1]; void *vals[1]; int r = 0;
    ff = uffi_dlsym((void *)UFFI_RTLD_DEFAULT, "fflush");
    slot = uffi_dlsym((void *)UFFI_RTLD_DEFAULT, "stdout");
    if (!ff || !slot) return;
    file = *(void **)slot;
    kinds[0] = UFFI_POINTER; vals[0] = &file;
    uffi_call(ff, UFFI_INT, kinds, vals, 1, 1, &r);
}
#endif

/* Called from our own poll loop, never from libcurl. A host callback into
 * unisacc still segfaults. NULL means the request stays quiet. */
typedef void (*net_tick_fn)(void);
static net_tick_fn net_tick_cb;
static int net_stop;
static int net_elapsed;
static time_t net_turn0;
static int net_tokens;
static long net_sent;
static long net_got;
int net_hit, net_miss, net_usage_seen;
void net_set_tick(net_tick_fn fn) { net_tick_cb = fn; }
void net_cancel(void) { net_stop = 1; }
void net_reset(void) { net_stop = 0; }
/* One clock for the user turn. net_async_begin must not touch it. */
void net_turn_clock(void) {
    net_turn0 = time(0);
    net_elapsed = 0;
    net_got = 0;
    net_hit = net_miss = net_usage_seen = 0;
}
void net_progress(int *sec, int *tokens, long *sent) {
    if (sec) *sec = net_elapsed;
    if (tokens) *tokens = net_tokens;
    if (sent) *sent = net_sent;
}
void net_recv(long *n) {
    if (n) *n = net_got;
}
void net_cache(int *hit, int *miss, int *seen) {
    if (hit) *hit = net_hit;
    if (miss) *miss = net_miss;
    if (seen) *seen = net_usage_seen;
}

/* Last closed usage object in the tail. Absent cache fields become 0.
 * No usage object in this body: drop any previous hit/miss. */
static void net_note_usage(const char *tail) {
    const char *u = 0, *p, *brace, *end, *k;
    int hit = 0, miss = 0;
    for (p = tail; (p = strstr(p, "\"usage\"")); p++) u = p;
    if (!u) { net_hit = net_miss = net_usage_seen = 0; return; }
    brace = strchr(u, '{');
    if (!brace) { net_hit = net_miss = net_usage_seen = 0; return; }
    end = strchr(brace, '}');
    if (!end) { net_hit = net_miss = net_usage_seen = 0; return; }
    k = strstr(brace, "\"prompt_cache_hit_tokens\"");
    if (k && k < end) {
        k = strchr(k, ':');
        if (k && k < end) hit = atoi(k + 1);
    }
    k = strstr(brace, "\"prompt_cache_miss_tokens\"");
    if (k && k < end) {
        k = strchr(k, ':');
        if (k && k < end) miss = atoi(k + 1);
    }
    net_hit = hit;
    net_miss = miss;
    net_usage_seen = 1;
}

/* Prefer the API's total_tokens once it is on disk. Until then, bytes/4. */
void net_note_progress(const char *path, time_t t0) {
    struct stat st;
    char tail[4096];
    int fd, n;
    char *p;
    net_elapsed = (int)(time(0) - (net_turn0 ? net_turn0 : t0));
    if (net_elapsed < 0) net_elapsed = 0;
    if (stat(path, &st) != 0 || st.st_size <= 0) {
        net_tokens = 0;
        net_got = 0;
        return;
    }
    net_got = (long)st.st_size;
    net_tokens = (int)(st.st_size / 4);
    fd = open(path, O_RDONLY);
    if (fd < 0) return;
    if (st.st_size > (off_t)sizeof tail) lseek(fd, st.st_size - (off_t)sizeof tail, SEEK_SET);
    n = (int)read(fd, tail, sizeof tail - 1);
    close(fd);
    if (n < 0) return;
    tail[n] = 0;
    p = strstr(tail, "\"total_tokens\":");
    if (p) net_tokens = atoi(p + 15);
    net_note_usage(tail);
}

static void net_take_body(int fd, char *buf, size_t cap, size_t *off) {
    for (;;) {
        ssize_t k;
        if (*off + 1 >= cap) return;
        k = read(fd, buf + *off, cap - 1 - *off);
        if (k < 0) { if (errno == EINTR) continue; return; }
        if (k == 0) return;
        *off += (size_t)k;
        buf[*off] = 0;
    }
}

/* Key read from ~/env.jsonl. DEEPSEEK_API_KEY, when set, is used first and
 * is not stored here. /reload clears only this buffer. */
static char net_key[200];

/* unisacc re-execs into its cache and drops DEEPSEEK_API_KEY. Read the
 * deepseek object in ~/env.jsonl. The first api_key in that file is not this one. */
static const char *net_deepseek_key(void) {
    const char *env = getenv("DEEPSEEK_API_KEY");
    const char *home;
    char path[512], line[4096];
    FILE *f;
    if (env && env[0]) return env;
    if (net_key[0]) return net_key;
    home = getenv("HOME");
    if (!home) return 0;
    snprintf(path, sizeof path, "%s/env.jsonl", home);
    f = fopen(path, "r");
    if (!f) return 0;
    while (fgets(line, sizeof line, f)) {
        char *p, *q;
        int i = 0;
        if (!strstr(line, "\"deepseek\"")) continue;
        p = strstr(line, "\"api_key\"");
        if (!p) continue;
        p = strchr(p, ':');
        if (!p) continue;
        q = strchr(p, '"');
        if (!q) continue;
        q++;
        while (*q && *q != '"' && i < (int)sizeof net_key - 1) net_key[i++] = *q++;
        net_key[i] = 0;
        break;
    }
    fclose(f);
    return net_key[0] ? net_key : 0;
}

/* unisacc setenv() stays in the file that called it. net_cli.c cannot
 * change this file's getenv, so the self-test asks this function to do it. */
void net_test_home(const char *home) {
    if (home && home[0]) setenv("HOME", home, 1);
}

/* Drop the cached env.jsonl key when that file can be opened. Returns 1
 * then. A missing file leaves the cache, so the next call still has it.
 * Does not unset DEEPSEEK_API_KEY and does not rewrite env.jsonl. */
int net_deepseek_forget(void) {
    const char *home = getenv("HOME");
    char path[512];
    FILE *f;
    if (!home || !home[0]) return 0;
    snprintf(path, sizeof path, "%s/env.jsonl", home);
    f = fopen(path, "r");
    if (!f) return 0;
    fclose(f);
    net_key[0] = 0;
    return 1;
}

/* Host libc FILE*. curl's default writer is fwrite; a unisacc FILE* or
 * callback must not be passed in. */
static void *host_fopen(const char *path, const char *mode) {
    static void *fn;
    int kinds[2];
    void *vals[2];
    void *r = 0;
    if (!fn) fn = uffi_dlsym((void *)UFFI_RTLD_DEFAULT, "fopen");
    if (!fn) return 0;
    kinds[0] = UFFI_POINTER; kinds[1] = UFFI_POINTER;
    vals[0] = &path; vals[1] = &mode;
    if (uffi_call(fn, UFFI_POINTER, kinds, vals, 2, 2, &r) != 0) return 0;
    return r;
}
static void host_fflush_file(void *file) {
    static void *fn;
    int kinds[1]; void *vals[1]; int r = 0;
    if (!file) return;
    if (!fn) fn = uffi_dlsym((void *)UFFI_RTLD_DEFAULT, "fflush");
    if (!fn) return;
    kinds[0] = UFFI_POINTER; vals[0] = &file;
    uffi_call(fn, UFFI_INT, kinds, vals, 1, 1, &r);
}
static void host_fclose(void *file) {
    static void *fn;
    int kinds[1]; void *vals[1]; int r = 0;
    if (!file) return;
    if (!fn) fn = uffi_dlsym((void *)UFFI_RTLD_DEFAULT, "fclose");
    if (!fn) return;
    kinds[0] = UFFI_POINTER; vals[0] = &file;
    uffi_call(fn, UFFI_INT, kinds, vals, 1, 1, &r);
}

/* One HTTPS transfer, driven one slice at a time.
 * The caller keeps `body` alive until net_async_pump returns 0.
 * That is the whole async story: no thread, no stack switch. */
static int net_on, net_running, net_rc;
static void *net_easy, *net_multi, *net_hdr, *net_outf;
static char net_capture[64];
static time_t net_t0;
static net_response net_res;

int net_async_begin(const char *method, const char *url,
                    const char *content_type, const char *body) {
    void *h, *hdr = 0;
    char chead[180], auth[600];
    const char *key;
    if (net_on) return -1;
    net_on = net_running = net_rc = 0;
    net_easy = net_multi = net_hdr = net_outf = 0;
    net_hdr = 0;
    net_capture[0] = 0;
    net_t0 = 0;
    memset(&net_res, 0, sizeof net_res);
    if (net_stop) { net_res.err = NET_CANCELLED; return -1; }
    net_sent = body ? (long)strlen(body) : 0;
    {
        static int curl_loaded;
        if (!curl_loaded) {
#ifdef __APPLE__
            if (!uffi_dlopen("/usr/lib/libcurl.4.dylib", 2 | 8)) { net_res.err = NET_NO_CURL; return -1; }
#else
            if (!uffi_dlopen("libcurl.so.4", 2 | 0x100)) { net_res.err = NET_NO_CURL; return -1; }
#endif
            curl_loaded = 1;
        }
    }
    h = curl_easy_init();
    if (!h) { net_res.err = NET_NO_CURL; return -1; }
    hdr = curl_slist_append(0, "Accept: application/json");
    if (content_type && content_type[0]) {
        snprintf(chead, sizeof chead, "Content-Type: %s", content_type);
        hdr = curl_slist_append(hdr, chead);
    }
    key = net_deepseek_key();
    if (key && key[0] && strstr(url, "api.deepseek.com") && strlen(key) < 500) {
        snprintf(auth, sizeof auth, "Authorization: Bearer %s", key);
        hdr = curl_slist_append(hdr, auth);
    }
    curl_setopt_long(h, CURLOPT_NOSIGNAL, 1);
    curl_setopt_long(h, CURLOPT_CONNECTTIMEOUT, 20);
    curl_setopt_long(h, CURLOPT_TIMEOUT, NET_TOTAL_SEC);
    curl_setopt(h, CURLOPT_URL, (void *)url);
    curl_setopt(h, CURLOPT_HTTPHEADER, hdr);
    if (!strcmp(method, "POST")) curl_setopt(h, CURLOPT_POSTFIELDS, (void *)(body ? body : ""));
    snprintf(net_capture, sizeof net_capture, "/tmp/cdsh-net-%d", (int)getpid());
    net_outf = host_fopen(net_capture, "w+");
    if (!net_outf) { curl_easy_cleanup(h); net_res.err = NET_NO_CURL; return -1; }
    curl_setopt(h, CURLOPT_WRITEDATA, net_outf);
    net_multi = curl_multi_init();
    if (!net_multi) {
        host_fclose(net_outf);
        unlink(net_capture);
        curl_easy_cleanup(h);
        net_res.err = NET_NO_CURL;
        return -1;
    }
    curl_multi_add_handle(net_multi, h);
    net_easy = h;
    net_hdr = hdr;
    net_running = 1;
    net_t0 = time(0);
    net_on = 1;
    net_tokens = 0;
    net_got = 0;
    /* This call has no usage yet. Do not keep the previous hit/miss on screen. */
    net_hit = net_miss = net_usage_seen = 0;
    return 0;
}

/* 1 = still waiting. 0 = finished; call net_async_end. wait_ms 0 does not block. */
int net_async_pump(int wait_ms) {
    int nfds = 0;
    if (!net_on) return 0;
    curl_multi_perform(net_multi, &net_running);
    host_fflush_file(net_outf);
    net_note_progress(net_capture, net_t0);
    if (!net_running) return 0;
    if ((int)(time(0) - net_t0) >= NET_TOTAL_SEC) { net_rc = NET_TIMEOUT; return 0; }
    if (wait_ms > 0) curl_multi_wait(net_multi, 0, 0, wait_ms, &nfds);
    if (net_tick_cb) net_tick_cb();
    if (net_stop) { net_rc = NET_CANCELLED; return 0; }
    if ((int)(time(0) - net_t0) >= NET_TOTAL_SEC) { net_rc = NET_TIMEOUT; return 0; }
    return 1;
}

net_response net_async_end(void) {
    int fd;
    ssize_t nread;
    long code = 0;
    net_response r;
    if (!net_on && net_res.err) { net_response early = net_res; return early; }
    if (net_multi && net_easy) {
        curl_multi_remove_handle(net_multi, net_easy);
        curl_multi_cleanup(net_multi);
        net_multi = 0;
    }
    if (net_outf) {
        host_fflush_file(net_outf);
        host_fclose(net_outf);
        net_outf = 0;
        fd = open(net_capture, O_RDONLY);
        if (fd >= 0) {
            nread = read(fd, net_res.body, sizeof net_res.body - 1);
            if (nread < 0) nread = 0;
            net_res.body[nread] = 0;
            net_res.body_bytes = (long)nread;
            close(fd);
        }
        unlink(net_capture);
    }
    if (net_easy) {
        curl_getinfo(net_easy, CURLINFO_RESPONSE_CODE, &code);
        curl_easy_cleanup(net_easy);
        net_easy = 0;
    }
    if (net_hdr) { curl_slist_free_all(net_hdr); net_hdr = 0; }
    r = net_res;
    if (net_rc == NET_CANCELLED) r.err = NET_CANCELLED;
    else if (net_rc == NET_TIMEOUT) r.err = NET_TIMEOUT;
    else if (net_rc != 0) r.err = NET_NO_CURL;
    else {
        r.status = (int)code;
        if (r.status <= 0) r.err = EPROTO;
        else r.ok = 1;
    }
    net_on = net_running = net_rc = 0;
    net_easy = net_multi = net_hdr = net_outf = 0;
    net_capture[0] = 0;
    net_t0 = 0;
    memset(&net_res, 0, sizeof net_res);
    return r;
}

/* In-process libcurl. The body goes to a host FILE*, not a pipe. */
static net_response net_https(const char *method, const char *url,
                              const char *content_type, const char *body) {
    net_response r;
    if (net_async_begin(method, url, content_type, body) != 0) {
        r = net_async_end();
        return r;
    }
    while (net_async_pump(200)) ;
    r = net_async_end();
    return r;
}

/* ── the request ────────────────────────────────────────────────────────── */

/*
 * Send one request and read the response.
 *
 * `body` may be NULL for a GET. Content-Length is always sent, including 0,
 * because a server that guesses the body length from the connection close is a
 * server that will wait for a body that is not coming.
 */
net_response net_http(const char *method, const char *url,
                      const char *content_type, const char *body) {
    net_response r;
    net_url u;
    unsigned long fd;
    int prc;
    char req[8192];
    size_t reqlen;
    size_t blen = body ? strlen(body) : 0;

    memset(&r, 0, sizeof r);
    r.ok = 0;
    r.body_bytes = 0;

    prc = net_parse_url(url, &u);
    if (prc != 0) { r.err = prc; return r; }
    if (strcmp(method, "GET") && strcmp(method, "POST")) { r.err = NET_REFUSED_URL; return r; }
    if (u.https) { r = net_https(method, url, content_type, body); return r; }

    fd = net_connect(u.host, u.port);
    if (fd == NET_INVALID_SOCKET) { r.err = errno ? errno : ECONNREFUSED; return r; }

    reqlen = (size_t)snprintf(req, sizeof req,
        "%s %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: csih/0.1\r\n"
        "Accept: */*\r\n"
        "Connection: close\r\n"
        "Content-Length: %lu\r\n"
        "%s%s\r\n"
        "\r\n"
        "%s",
        method, u.path, u.host,
        (unsigned long)blen,
        blen ? "Content-Type: " : "",
        blen ? (content_type ? content_type : "application/octet-stream") : "",
        body ? body : "");

    if (reqlen >= sizeof req) { net_socket_close(fd); r.err = NET_REFUSED_URL; return r; }

    {
        size_t off = 0;
        while (off < reqlen) {
            long w = net_socket_send(fd, req + off, reqlen - off);
            if (w < 0) { if (errno == EINTR) continue; r.err = errno; net_socket_close(fd); return r; }
            off += (size_t)w;
        }
    }

    /* Read everything until the peer closes. "Connection: close" means the end
     * of the body IS the end of the stream, so no length arithmetic is needed
     * for the read loop — which is exactly why that header is sent. */
    {
        char *all = (char *)malloc(NET_HDR_MAX + NET_BODY_MAX);
        size_t off = 0, cap = NET_HDR_MAX + NET_BODY_MAX;
        int truncated = 0;
        if (!all) { net_socket_close(fd); r.err = ENOMEM; return r; }
        for (;;) {
            long n;
            if (off >= cap) { truncated = 1; break; }
            n = net_socket_recv(fd, all + off, cap - off);
            if (n < 0) { if (errno == EINTR) continue; break; }
            if (n == 0) break;
            off += (size_t)n;
        }
        net_socket_close(fd);
        all[off < cap ? off : cap - 1] = '\0';

        /* Split headers from body on the first blank line. A response with no
         * blank line is malformed and is reported as such rather than guessed
         * at — returning the whole thing as "body" would hide the problem. */
        {
            char *sep = strstr(all, "\r\n\r\n");
            size_t hlen;
            if (!sep) { free(all); r.err = EPROTO; return r; }
            hlen = (size_t)(sep - all) + 2;             /* keep the final CRLF */
            if (hlen >= sizeof r.header) hlen = sizeof r.header - 1;
            memcpy(r.header, all, hlen);
            r.header[hlen] = '\0';

            {
                const char *bodyp = sep + 4;
                size_t bodylen = off > (size_t)(bodyp - all) ? off - (size_t)(bodyp - all) : 0;
                if (bodylen >= sizeof r.body) { bodylen = sizeof r.body - 1; truncated = 1; }
                memcpy(r.body, bodyp, bodylen);
                r.body[bodylen] = '\0';
                r.body_bytes = truncated ? -1 : (long)bodylen;
            }
        }
        free(all);
    }

    /* Parse the status line: "HTTP/1.1 200 OK" */
    {
        const char *sp = strchr(r.header, ' ');
        if (!sp) { r.err = EPROTO; return r; }
        r.status = atoi(sp + 1);
        if (r.status <= 0) { r.err = EPROTO; return r; }
    }

    /* Chunked responses are NOT decoded. Detect and say so, because a caller
     * handed raw chunk frames and told they are the body would send chunk
     * sizes to a model as if they were content. */
    {
        const char *cl = r.header;
        int i;
        for (i = 0; cl[i]; i++) {
            if ((cl[i] == 'T' || cl[i] == 't') &&
                !strncasecmp_local(cl + i, "transfer-encoding:", 18)) {
                const char *v = cl + i + 18;
                while (*v == ' ') v++;
                if (!strncasecmp_local(v, "chunked", 7)) r.chunked = 1;
            }
        }
    }
    r.ok = 1;
    return r;
}

/* net_cli.c owns the self-test. */
