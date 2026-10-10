/* reload_candidate.c -- C port of reload_candidate.py (same CLI, same exit codes).
 *
 * Trusted-private-directory candidate builder/verifier, not authentication or handoff.
 *
 * CLI:
 *   reload_candidate build SOURCE_APP COMPILER PRIVATE_ROOT   -> prints candidate JSON, rc 0
 *   reload_candidate verify CANDIDATE_DIR                     -> prints candidate JSON, rc 0
 *   on failure prints {"error": "..."} and exits 1.
 *
 * SOURCES / AGENT / GATES tables are the same file table csih.sh uses.
 * Build with unisacc (0.0.38):
 *   /bin/sh unisacc.com -include ABS/json.h ABS/json.cx ABS/reload_candidate.c -o OUT
 *
 * Deviations from the Python original (all observable only on error paths or edge inputs):
 *   - no setsid(): the command runs in its own process group (setpgid), killed as a group;
 *   - exec failure is reported as rc 127 by the child, not as a Python exception;
 *   - error text is C-side wording (exit codes are unchanged);
 *   - include resolution uses realpath(); a missing include target is reported as
 *     "outside supported inputs" (the Python reports that too after resolve()).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <poll.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <cx.h>

#define SCHEMA "csih-candidate-v1"
#define PATHCAP 4096
#define MODEBITS(m) ((m) & 07777)
#define TIMEOUT_SECS 14

/* ---- file table (same as reload_candidate.py / csih.sh) ---- */
static const char *const SOURCES[] = {
    "tui.c", "render.c", "term.c", "chat.c", "clock.c", "tools.c", "cols.cx", "home.cx",
    "file.c", "shell.c", "edit.c", "gate.c", "json.cx", "session.c", "agent.c", "plugin.c",
    "net.c", "reload_state.c", "reload_session_decode.c", "reload_session_encode.c",
    "reload_io.c", "reload_load.c", "reload_consume.c", "journal_checkpoint.c",
    "reload_owner.c", "csih_message.c", "csih_message_io.c", "context_index.c"};
static const char *const AGENT[] = {
    "agent.c", "agent_cli.c", "cols.cx", "home.cx", "file.c", "edit.c", "shell.c",
    "json.cx", "session.c", "net.c", "plugin.c"};
static const char *const INCLUDES[] = {"csih_cols.h", "csih_home.h", "json.h"};
#define NSOURCES (sizeof SOURCES / sizeof *SOURCES)
#define NAGENT (sizeof AGENT / sizeof *AGENT)
#define NINCLUDES (sizeof INCLUDES / sizeof *INCLUDES)

typedef struct {
    const char *name, *binary;
    const char *const *src;
    size_t nsrc;
    const char *marker;
} Gate;
static const Gate GATES[] = {
    {"tui-selftest", "candidate", SOURCES, NSOURCES, "selftest ok"},
    {"agent-selftest", "agent-selftest", AGENT, NAGENT, "agent: all cases pass"}};
#define NGATES (sizeof GATES / sizeof *GATES)

/* ---- errors: fail() longjmps to main, which prints {"error": ...} and exits 1 ---- */
static jmp_buf rc_jb;
static char rc_err[1024];
static void fail(const char *msg) {
    snprintf(rc_err, sizeof rc_err, "%s", msg);
    longjmp(rc_jb, 1);
}
static void failp(const char *msg, const char *path) {
    snprintf(rc_err, sizeof rc_err, "%s: %s", msg, path);
    longjmp(rc_jb, 1);
}
#define REQUIRE(c, m) do { if (!(c)) fail(m); } while (0)

/* ---- growable byte buffer + ensure_ascii JSON writer (no failure path: safe in error print) ---- */
typedef struct { char *p; size_t n, cap; } Buf;
static void bn(Buf *b, const void *s, size_t l) {
    if (b->n + l + 1 > b->cap) {
        size_t c = b->cap ? b->cap : 256;
        char *t;
        while (c < b->n + l + 1) c *= 2;
        t = realloc(b->p, c);
        if (!t) { fputs("out of memory\n", stderr); exit(1); }
        b->p = t; b->cap = c;
    }
    memcpy(b->p + b->n, s, l);
    b->n += l;
    b->p[b->n] = 0;
}
static void bs(Buf *b, const char *s) { bn(b, s, strlen(s)); }
static void bjs(Buf *b, const char *s) {
    const unsigned char *p = (const unsigned char *)s;
    char tmp[16];
    bs(b, "\"");
    while (*p) {
        unsigned cp = *p;
        int len = 1, k;
        if (cp >= 0x80) {
            if ((cp & 0xE0) == 0xC0) { len = 2; cp &= 0x1F; }
            else if ((cp & 0xF0) == 0xE0) { len = 3; cp &= 0x0F; }
            else if ((cp & 0xF8) == 0xF0) { len = 4; cp &= 0x07; }
            else { bs(b, "\\ufffd"); p++; continue; }
            for (k = 1; k < len; k++) {
                if (p[k] == 0 || (p[k] & 0xC0) != 0x80) { len = -1; break; }
                cp = (cp << 6) | (p[k] & 0x3F);
            }
            if (len < 0) { bs(b, "\\ufffd"); p++; continue; }
        }
        p += len;
        if (cp == '"') bs(b, "\\\"");
        else if (cp == '\\') bs(b, "\\\\");
        else if (cp == '\n') bs(b, "\\n");
        else if (cp == '\r') bs(b, "\\r");
        else if (cp == '\t') bs(b, "\\t");
        else if (cp == '\b') bs(b, "\\b");
        else if (cp == '\f') bs(b, "\\f");
        else if (cp < 0x20) { snprintf(tmp, sizeof tmp, "\\u%04x", cp); bs(b, tmp); }
        else if (cp < 0x80) { char c = (char)cp; bn(b, &c, 1); }
        else if (cp < 0x10000) { snprintf(tmp, sizeof tmp, "\\u%04x", cp); bs(b, tmp); }
        else {
            unsigned hi = 0xD800 + ((cp - 0x10000) >> 10), lo = 0xDC00 + ((cp - 0x10000) & 0x3FF);
            snprintf(tmp, sizeof tmp, "\\u%04x\\u%04x", hi, lo);
            bs(b, tmp);
        }
    }
    bs(b, "\"");
}

/* ---- small string helpers ---- */
static char *xdup(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = malloc(n);
    if (!p) fail("out of memory");
    memcpy(p, s, n);
    return p;
}
static char *pjoin(const char *a, const char *b) {
    size_t n = strlen(a) + strlen(b) + 2;
    char *p = malloc(n);
    if (!p) fail("out of memory");
    snprintf(p, n, "%s/%s", a, b);
    return p;
}
static char *absolute(const char *p) {
    char cwd[PATHCAP];
    if (p[0] == '/') return xdup(p);
    if (!getcwd(cwd, sizeof cwd)) fail("getcwd failed");
    return pjoin(cwd, p);
}
static char *real(const char *p) {
    char *r = realpath(p, 0);
    if (!r) failp("cannot resolve", p);
    return r;
}
static char *dirname_of(const char *p) {
    char *s = xdup(p);
    char *q = strrchr(s, '/');
    if (q == s) s[1] = 0;
    else if (q) *q = 0;
    else strcpy(s, ".");
    return s;
}
static const char *suffix_of(const char *name) {
    const char *d = strrchr(name, '.');
    return (d && d != name) ? d : "";
}
static int streq_s(const char *a, const char *b) { return a && b && !strcmp(a, b); }
static int is_under(const char *a, const char *b) {
    size_t bl = strlen(b);
    return !strcmp(a, b) || (!strncmp(a, b, bl) && a[bl] == '/');
}

/* ---- trusted(path, directory, private) ---- */
static void trusted(const char *path, int directory, int private_) {
    struct stat s;
    if (lstat(path, &s) != 0) failp("cannot stat", path);
    if (s.st_uid != getuid() || S_ISLNK(s.st_mode)) failp("untrusted owner/symlink", path);
    if (directory ? !S_ISDIR(s.st_mode) : !S_ISREG(s.st_mode)) failp("wrong file kind", path);
    if (private_ && MODEBITS(s.st_mode) != (directory ? 0700 : 0600)) failp("wrong private permissions", path);
}

static char *slurp(const char *path, size_t *len) {
    char *b = cx_read(path, len);
    if (!b) failp("cannot read", path);
    return b;
}
static void file_sha(const char *path, char hex[65]) {
    size_t n = 0;
    char *b = slurp(path, &n);
    cx_sha256_hex(b, n, hex);
    free(b);
}
static void write_file(const char *path, const char *data, size_t n, int mode) {
    if (cx_write(path, data, n) != 0) failp("cannot write", path);
    if (chmod(path, (mode_t)mode) != 0) failp("cannot chmod", path);
}

/* ---- manifest rows ---- */
typedef struct { char *path; char sha[65]; } Row;
typedef struct { Row *v; size_t n, cap; } Rows;

static void rows_push(Rows *r, const char *path, const char *sha) {
    if (r->n == r->cap) {
        Row *t;
        r->cap = r->cap ? r->cap * 2 : 16;
        t = realloc(r->v, r->cap * sizeof *r->v);
        if (!t) fail("out of memory");
        r->v = t;
    }
    r->v[r->n].path = xdup(path);
    memcpy(r->v[r->n].sha, sha, 65);
    r->n++;
}
static int rows_eq(const Rows *a, const Rows *b) {
    size_t i;
    if (a->n != b->n) return 0;
    for (i = 0; i < a->n; i++)
        if (strcmp(a->v[i].path, b->v[i].path) || memcmp(a->v[i].sha, b->v[i].sha, 65)) return 0;
    return 1;
}
static int rows_has(const Rows *r, const char *path) {
    size_t i;
    for (i = 0; i < r->n; i++)
        if (!strcmp(r->v[i].path, path)) return 1;
    return 0;
}
static int cmp_row(const void *a, const void *b) {
    return strcmp(((const Row *)a)->path, ((const Row *)b)->path);
}
static void canon_rows(Buf *b, const Rows *r) {
    size_t i;
    bs(b, "[");
    for (i = 0; i < r->n; i++) {
        if (i) bs(b, ",");
        bs(b, "{\"path\":");
        bjs(b, r->v[i].path);
        bs(b, ",\"sha256\":");
        bjs(b, r->v[i].sha);
        bs(b, "}");
    }
    bs(b, "]");
}

/* ---- directory listing, sorted by name (Python sorted(iterdir, key=name)) ---- */
typedef struct { char **v; size_t n; } Names;
static int cmp_str(const void *a, const void *b) { return strcmp(*(char *const *)a, *(char *const *)b); }
static Names list_dir(const char *dir) {
    Names out = {0, 0};
    size_t cap = 0;
    struct dirent *e;
    DIR *d = opendir(dir);
    if (!d) failp("cannot read directory", dir);
    while ((e = readdir(d))) {
        char **t;
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        if (out.n == cap) {
            cap = cap ? cap * 2 : 16;
            t = realloc(out.v, cap * sizeof *out.v);
            if (!t) fail("out of memory");
            out.v = t;
        }
        out.v[out.n++] = xdup(e->d_name);
    }
    closedir(d);
    if (out.n) qsort(out.v, out.n, sizeof *out.v, cmp_str);
    return out;
}

static int is_source_suffix(const char *name) {
    const char *s = suffix_of(name);
    return !strcmp(s, ".c") || !strcmp(s, ".h") || !strcmp(s, ".inc") || !strcmp(s, ".cx");
}

static void visit(const char *folder, const char *rel, int strict, Rows *rows) {
    Names names = list_dir(folder);
    size_t i;
    for (i = 0; i < names.n; i++) {
        const char *name = names.v[i];
        char *full = pjoin(folder, name);
        char *relp = rel[0] ? pjoin(rel, name) : xdup(name);
        struct stat s;
        if (lstat(full, &s) != 0) failp("cannot stat", full);
        REQUIRE(!S_ISLNK(s.st_mode), "source symlink");
        REQUIRE(s.st_uid == getuid(), "source owner mismatch");
        if (S_ISDIR(s.st_mode)) {
            if (strict) REQUIRE(MODEBITS(s.st_mode) == 0700, "frozen directory permissions mismatch");
            visit(full, relp, strict, rows);
        } else if (S_ISREG(s.st_mode)) {
            if (is_source_suffix(name)) {
                char hex[65];
                if (strict) REQUIRE(MODEBITS(s.st_mode) == 0600, "frozen input permissions mismatch");
                file_sha(full, hex);
                rows_push(rows, relp, hex);
            } else if (strict) {
                failp("unknown frozen input", full);
            }
        } else {
            failp("unsupported source file kind", full);
        }
    }
}

/* Every quoted #include in a row must resolve, inside the app, to another row. */
static void check_includes(const char *app, const char *appr, const char *relpath, const Rows *rows) {
    char *file = pjoin(app, relpath);
    size_t n = 0;
    char *text = slurp(file, &n);
    char *dir = dirname_of(file);
    size_t al = strlen(appr), i = 0;
    while (i < n) {
        size_t e = i, len;
        char *line, *p, *d, *q, *quoted, *target, *rel;
        while (e < n && text[e] != '\n') e++;
        len = e - i;
        line = malloc(len + 1);
        if (!line) fail("out of memory");
        memcpy(line, text + i, len);
        line[len] = 0;
        p = line;
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == '#') {
            p++;
            while (*p && isspace((unsigned char)*p)) p++;
            if (!strncmp(p, "include", 7) && isspace((unsigned char)p[7])) {
                p += 7;
                while (*p && isspace((unsigned char)*p)) p++;
                d = p;
                REQUIRE(*d != 0, "unsupported nonliteral include");
                if (*d != '<') {
                    REQUIRE(*d == '"', "unsupported nonliteral include");
                    q = d + 1;
                    while (*q && *q != '"' && *q != '\\') q++;
                    REQUIRE(q > d + 1 && *q == '"', "unsupported nonliteral include");
                    quoted = malloc((size_t)(q - (d + 1)) + 1);
                    if (!quoted) fail("out of memory");
                    memcpy(quoted, d + 1, (size_t)(q - (d + 1)));
                    quoted[q - (d + 1)] = 0;
                    target = pjoin(dir, quoted);
                    {
                        char *tr = realpath(target, 0);
                        REQUIRE(tr != 0, "include outside supported .c/.h/.inc inputs");
                        REQUIRE(!strncmp(tr, appr, al) && tr[al] == '/', "include escapes frozen app");
                        rel = tr + al + 1;
                        REQUIRE(rows_has(rows, rel), "include outside supported .c/.h/.inc inputs");
                    }
                }
            }
        }
        free(line);
        i = e + 1;
    }
    free(text);
}

static Rows manifest(const char *app, int strict) {
    Rows rows = {0, 0, 0};
    size_t i;
    char *appr;
    trusted(app, 1, strict);
    visit(app, "", strict, &rows);
    if (rows.n) qsort(rows.v, rows.n, sizeof *rows.v, cmp_row);
    REQUIRE(rows.n > 0, "empty source manifest");
    for (i = 0; i < NSOURCES; i++) REQUIRE(rows_has(&rows, SOURCES[i]), "missing required source");
    for (i = 0; i < NAGENT; i++) REQUIRE(rows_has(&rows, AGENT[i]), "missing required source");
    appr = real(app);
    for (i = 0; i < rows.n; i++) check_includes(app, appr, rows.v[i].path, &rows);
    return rows;
}

/* ---- uuid-like hex (32 chars) from /dev/urandom ---- */
static void uuid_hex(char out[33]) {
    unsigned char r[16];
    size_t i;
    FILE *f = fopen("/dev/urandom", "rb");
    if (!f || fread(r, 1, 16, f) != 16) fail("random source unavailable");
    fclose(f);
    for (i = 0; i < 16; i++) snprintf(out + 2 * i, 3, "%02x", r[i]);
}

/* write receipt: temp file in the same directory, O_EXCL, fsync, refuse existing, rename */
static void atomic_write(const char *path, const char *data, size_t n) {
    char hex[33], tmpname[64];
    char *dir = dirname_of(path);
    char *tmp;
    struct stat s;
    int fd, dfd;
    size_t off = 0;
    uuid_hex(hex);
    snprintf(tmpname, sizeof tmpname, ".receipt-%s", hex);
    tmp = pjoin(dir, tmpname);
    fd = open(tmp, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0600);
    if (fd < 0) failp("cannot create receipt temp", tmp);
    while (off < n) {
        ssize_t w = write(fd, data + off, n - off);
        if (w <= 0) { close(fd); unlink(tmp); fail("short write"); }
        off += (size_t)w;
    }
    if (fsync(fd) != 0) { close(fd); unlink(tmp); fail("fsync failed"); }
    close(fd);
    if (lstat(path, &s) == 0) {
        unlink(tmp);
        if (S_ISLNK(s.st_mode)) fail("receipt symlink");
        fail("refusing existing receipt");
    }
    if (rename(tmp, path) != 0) { unlink(tmp); failp("cannot rename receipt", path); }
    dfd = open(dir, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
    if (dfd < 0) failp("cannot open directory", dir);
    fsync(dfd);
    close(dfd);
    unlink(tmp); /* already renamed: ENOENT is expected */
}

/* ---- argv vectors (NULL-terminated) ---- */
typedef struct { char **v; size_t n; } Argv;
static void apush(Argv *a, char *s) {
    char **t = realloc(a->v, (a->n + 2) * sizeof *a->v);
    if (!t) fail("out of memory");
    a->v = t;
    a->v[a->n++] = s;
    a->v[a->n] = 0;
}
static Argv expected_command(const char *dir, const char *binary, const char *const *src, size_t nsrc, int build) {
    Argv a = {0, 0};
    size_t i;
    char *app = pjoin(dir, "app");
    if (build) {
        apush(&a, xdup("/bin/sh"));
        apush(&a, pjoin(dir, "compiler.com"));
        for (i = 0; i < NINCLUDES; i++) {
            apush(&a, xdup("-include"));
            apush(&a, pjoin(app, INCLUDES[i]));
        }
        apush(&a, xdup("-o"));
        apush(&a, pjoin(dir, binary));
        for (i = 0; i < nsrc; i++) apush(&a, xdup(src[i]));
    } else {
        apush(&a, pjoin(dir, binary));
        apush(&a, xdup("selftest"));
    }
    return a;
}

/* ---- commands: own process group, stdout/stderr captured on pipes, timeout kills the group ---- */
typedef struct {
    Argv argv;
    char *cwd;
    int rc;
    Buf out, err;
    char sout[65], serr[65];
} Rec;
/* File scope, not locals: unisacc 0.0.38 placed local Rec arrays where memset() faulted. */
static Rec build_r[2], run_r[2]; /* 2 == NGATES, checked in build() */

static int run_command(const Argv *av, const char *cwd, char *const envp[], Buf *out, Buf *err) {
    int po[2], pe[2], st = 0, rc, fo = 1, fe = 1;
    pid_t pid;
    time_t deadline;
    char chunk[4096];
    if (pipe(po) != 0 || pipe(pe) != 0) fail("pipe failed");
    pid = fork();
    if (pid < 0) fail("fork failed");
    if (pid == 0) {
        setpgid(0, 0);
        dup2(po[1], 1);
        dup2(pe[1], 2);
        close(po[0]); close(po[1]); close(pe[0]); close(pe[1]);
        if (chdir(cwd) != 0) _exit(127);
        execve(av->v[0], av->v, envp);
        _exit(127);
    }
    close(po[1]);
    close(pe[1]);
    deadline = time(0) + TIMEOUT_SECS;
    while (fo || fe) {
        struct pollfd pf[2];
        int r, k;
        pf[0].fd = fo ? po[0] : -1; pf[0].events = POLLIN; pf[0].revents = 0;
        pf[1].fd = fe ? pe[0] : -1; pf[1].events = POLLIN; pf[1].revents = 0;
        r = poll(pf, 2, 50);
        if (r < 0 && errno != EINTR) { kill(-pid, SIGKILL); waitpid(pid, &st, 0); fail("poll failed"); }
        for (k = 0; k < 2; k++) {
            int *open_flag = k ? &fe : &fo;
            Buf *dst = k ? err : out;
            int fd = k ? pe[0] : po[0];
            if (!*open_flag || !pf[k].revents) continue;
            {
                ssize_t got = read(fd, chunk, sizeof chunk);
                if (got > 0) bn(dst, chunk, (size_t)got);
                else if (got == 0 || errno != EINTR) { close(fd); *open_flag = 0; }
            }
        }
        if (time(0) >= deadline) { kill(-pid, SIGKILL); kill(pid, SIGKILL); waitpid(pid, &st, 0); fail("command timeout; process group killed"); }
    }
    for (;;) {
        pid_t w = waitpid(pid, &st, WNOHANG);
        if (w == pid) break;
        if (w < 0 && errno != EINTR) fail("waitpid failed");
        if (time(0) >= deadline) { kill(-pid, SIGKILL); kill(pid, SIGKILL); waitpid(pid, &st, 0); fail("command timeout; process group killed"); }
        usleep(20000);
    }
    rc = (st & 0x7f) ? -(st & 0x7f) : (st >> 8) & 0xff;
    return rc;
}

/* Fills *r in place (no struct return: unisacc 0.0.38 miscopies the large Rec return value). */
static void record_command(Rec *r, const char *dir, const char *name, const char *kind, const Argv *av, const char *cwd, char *const envp[]) {
    char *path;
    const char *suffixes[2] = {"stdout", "stderr"};
    Buf *bufs[2];
    int k;
    memset(r, 0, sizeof *r);
    r->argv = *av;
    r->cwd = xdup(cwd);
    r->rc = run_command(av, cwd, envp, &r->out, &r->err);
    bufs[0] = &r->out;
    bufs[1] = &r->err;
    for (k = 0; k < 2; k++) {
        char fname[256];
        snprintf(fname, sizeof fname, "%s-%s.%s", name, kind, suffixes[k]);
        path = pjoin(dir, fname);
        write_file(path, bufs[k]->p ? bufs[k]->p : "", bufs[k]->n, 0600);
    }
    cx_sha256_hex(r->out.p ? r->out.p : "", r->out.n, r->sout);
    cx_sha256_hex(r->err.p ? r->err.p : "", r->err.n, r->serr);
}

static void canon_record(Buf *b, const Rec *r) {
    size_t i;
    char tmp[32];
    bs(b, "{\"argv\":[");
    for (i = 0; i < r->argv.n; i++) {
        if (i) bs(b, ",");
        bjs(b, r->argv.v[i]);
    }
    bs(b, "],\"cwd\":");
    bjs(b, r->cwd);
    snprintf(tmp, sizeof tmp, ",\"rc\":%d", r->rc);
    bs(b, tmp);
    bs(b, ",\"stderr_sha256\":");
    bjs(b, r->serr);
    bs(b, ",\"stdout_sha256\":");
    bjs(b, r->sout);
    bs(b, "}");
}

/* Python: out.rstrip().splitlines()[-1:] == [marker] */
static int last_line_is(const Buf *b, const char *marker) {
    size_t e = b->n, s;
    if (!b->p) return 0;
    while (e > 0 && isspace((unsigned char)b->p[e - 1])) e--;
    if (e == 0) return 0;
    s = e;
    while (s > 0 && b->p[s - 1] != '\n' && b->p[s - 1] != '\r') s--;
    return (e - s) == strlen(marker) && !memcmp(b->p + s, marker, e - s);
}
static int has_fail(const Buf *b) {
    size_t i;
    if (!b->p) return 0;
    for (i = 0; i + 4 <= b->n; i++)
        if (!memcmp(b->p + i, "FAIL", 4)) return 1;
    return 0;
}

/* ---- result of build/verify ---- */
typedef struct { char *candidate_dir, *hash, *binary, *receipt; } Result;

static char **make_env(const char *home, const char *tmpdir) {
    static char kv[11][512];
    static char *envp[12];
    snprintf(kv[0], 512, "PATH=/bin:/usr/bin");
    snprintf(kv[1], 512, "HOME=%s", home);
    snprintf(kv[2], 512, "TMPDIR=%s", tmpdir);
    snprintf(kv[3], 512, "LC_ALL=C");
    snprintf(kv[4], 512, "CSIH_ROLE=");
    snprintf(kv[5], 512, "CSIH_PEER=");
    snprintf(kv[6], 512, "CSIH_ENDPOINT=http://127.0.0.1:1/v1/chat/completions");
    snprintf(kv[7], 512, "CSIH_MODEL=candidate-gate");
    snprintf(kv[8], 512, "DEEPSEEK_API_KEY=LOCAL_GATE_ONLY");
    snprintf(kv[9], 512, "NO_PROXY=127.0.0.1,localhost");
    snprintf(kv[10], 512, "no_proxy=127.0.0.1,localhost");
    {
        int i;
        for (i = 0; i < 11; i++) envp[i] = kv[i];
        envp[11] = 0;
    }
    return envp;
}

static Result verify(const char *dirarg);

static Result build(const char *source_arg, const char *compiler_arg, const char *root_arg) {
    char *source = absolute(source_arg), *compiler = absolute(compiler_arg), *root = absolute(root_arg);
    char compiler_hash[65], digest[65], name[80], ah[NGATES][65];
    char *directory, *app, *frozen, *home, *tmp, *keyfile, **envp;
    Rows original, strict;
    Buf canon = {0, 0, 0}, rec = {0, 0, 0};
    size_t g, i;
    char hex[33];

    trusted(source, 1, 0);
    trusted(compiler, 0, 0);
    trusted(root, 1, 1);
    source = real(source);
    compiler = real(compiler);
    root = real(root);
    REQUIRE(!is_under(root, source), "private root cannot be inside source tree");

    REQUIRE(NGATES == 2, "gate table size must match static record arrays");
    original = manifest(source, 0);
    {
        size_t n = 0;
        char *cb = slurp(compiler, &n);
        cx_sha256_hex(cb, n, compiler_hash);
        free(cb);
    }

    uuid_hex(hex);
    snprintf(name, sizeof name, "candidate-%s", hex);
    directory = pjoin(root, name);
    if (mkdir(directory, 0700) != 0) failp("cannot create candidate directory", directory);
    app = pjoin(directory, "app");
    if (mkdir(app, 0700) != 0) failp("cannot create app directory", app);

    for (i = 0; i < original.n; i++) {
        char *target = pjoin(app, original.v[i].path), *parent = xdup(app), *src, *data_path;
        const char *p = original.v[i].path, *slash;
        size_t n = 0;
        char *data;
        while ((slash = strchr(p, '/')) != 0) {
            char *part = malloc((size_t)(slash - p) + 1), *next;
            memcpy(part, p, (size_t)(slash - p));
            part[slash - p] = 0;
            next = pjoin(parent, part);
            if (mkdir(next, 0700) != 0) {
                REQUIRE(errno == EEXIST, "cannot create candidate subdirectory");
                trusted(next, 1, 1);
            }
            parent = next;
            p = slash + 1;
        }
        src = pjoin(source, original.v[i].path);
        data_path = src;
        data = slurp(data_path, &n);
        write_file(target, data, n, 0600);
        free(data);
    }

    frozen = pjoin(directory, "compiler.com");
    {
        size_t n = 0;
        char *cb = slurp(compiler, &n);
        write_file(frozen, cb, n, 0600);
        free(cb);
    }
    {
        char fh[65], ch[65];
        Rows again;
        again = manifest(source, 0);
        strict = manifest(app, 1);
        file_sha(frozen, fh);
        file_sha(compiler, ch);
        REQUIRE(rows_eq(&again, &original) && rows_eq(&strict, &original) && !strcmp(fh, compiler_hash) && !strcmp(ch, compiler_hash),
                "inputs changed during freeze");
    }
    canon_rows(&canon, &original);
    cx_sha256_hex(canon.p, canon.n, digest);

    home = pjoin(directory, "private-home");
    if (mkdir(home, 0700) != 0) failp("cannot create home", home);
    tmp = pjoin(directory, "tmp");
    if (mkdir(tmp, 0700) != 0) failp("cannot create tmp", tmp);
    keyfile = pjoin(home, "env.jsonl");
    write_file(keyfile, "{\"deepseek\":{\"api_key\":\"LOCAL_GATE_ONLY\"}}\n", 39, 0600);
    envp = make_env(home, tmp);

    for (g = 0; g < NGATES; g++) {
        Argv bav = expected_command(directory, GATES[g].binary, GATES[g].src, GATES[g].nsrc, 1);
        Argv rav;
        char *artifact, fh[65];
        size_t n = 0;
        char *cb;
        record_command(&build_r[g], directory, GATES[g].name, "build", &bav, app, envp);
        REQUIRE(build_r[g].rc == 0, "candidate build failed");
        artifact = pjoin(directory, GATES[g].binary);
        trusted(artifact, 0, 0);
        chmod(artifact, 0700);
        file_sha(artifact, ah[g]);
        rav = expected_command(directory, GATES[g].binary, GATES[g].src, GATES[g].nsrc, 0);
        record_command(&run_r[g], directory, GATES[g].name, "run", &rav, app, envp);
        REQUIRE(run_r[g].rc == 0 && run_r[g].err.n == 0 && !has_fail(&run_r[g].out) &&
                    last_line_is(&run_r[g].out, GATES[g].marker),
                "candidate gate failed");
        strict = manifest(app, 1);
        cb = slurp(frozen, &n);
        cx_sha256_hex(cb, n, fh);
        free(cb);
        {
            char ch[65];
            file_sha(artifact, ch);
            REQUIRE(rows_eq(&strict, &original) && !strcmp(fh, compiler_hash) && !strcmp(ch, ah[g]),
                    "frozen input/artifact changed during gate");
        }
    }

    /* canonical receipt: keys sorted, separators "," and ":" (same bytes as the Python canonical()) */
    bs(&rec, "{\"compiler_sha256\":");
    bjs(&rec, compiler_hash);
    bs(&rec, ",\"gates\":[");
    for (g = 0; g < NGATES; g++) {
        if (g) bs(&rec, ",");
        bs(&rec, "{\"binary\":");
        bjs(&rec, GATES[g].binary);
        bs(&rec, ",\"binary_sha256\":");
        bjs(&rec, ah[g]);
        bs(&rec, ",\"build\":");
        canon_record(&rec, &build_r[g]);
        bs(&rec, ",\"name\":");
        bjs(&rec, GATES[g].name);
        bs(&rec, ",\"run\":");
        canon_record(&rec, &run_r[g]);
        bs(&rec, "}");
    }
    bs(&rec, "],\"required_gates\":[");
    for (g = 0; g < NGATES; g++) {
        if (g) bs(&rec, ",");
        bjs(&rec, GATES[g].name);
    }
    bs(&rec, "],\"schema\":");
    bjs(&rec, SCHEMA);
    bs(&rec, ",\"source_manifest\":");
    canon_rows(&rec, &original);
    bs(&rec, ",\"source_sha256\":");
    bjs(&rec, digest);
    bs(&rec, "}\n");

    atomic_write(pjoin(directory, "receipt.json"), rec.p, rec.n);
    return verify(directory);
}

static int allowed_entry(const char *n) {
    static const char *fixed[] = {"app", "compiler.com", "candidate", "agent-selftest", "receipt.json", "private-home", "tmp"};
    size_t i, g;
    char buf[256];
    for (i = 0; i < sizeof fixed / sizeof *fixed; i++)
        if (!strcmp(n, fixed[i])) return 1;
    for (g = 0; g < NGATES; g++) {
        static const char *kinds[2] = {"build", "run"}, *sfx[2] = {"stdout", "stderr"};
        size_t k, s;
        for (k = 0; k < 2; k++)
            for (s = 0; s < 2; s++) {
                snprintf(buf, sizeof buf, "%s-%s.%s", GATES[g].name, kinds[k], sfx[s]);
                if (!strcmp(n, buf)) return 1;
            }
    }
    return 0;
}

static void check_unique(const jvalue *v) {
    size_t i, j;
    if (!v) return;
    if (v->kind == J_OBJ) {
        for (i = 0; i < v->nkeys; i++)
            for (j = 0; j < i; j++)
                REQUIRE(strcmp(v->keys[i], v->keys[j]) != 0, "duplicate receipt key");
        for (i = 0; i < v->nkeys; i++) check_unique(v->vals[i]);
    } else if (v->kind == J_ARR) {
        for (i = 0; i < v->len; i++) check_unique(v->items[i]);
    }
}

/* object has exactly these keys (keys are unique by check_unique) */
static int exact_keys(const jvalue *v, const char *const *names, size_t n) {
    size_t i;
    if (!v || v->kind != J_OBJ || v->nkeys != n) return 0;
    for (i = 0; i < n; i++)
        if (!jget((jvalue *)v, names[i])) return 0;
    return 1;
}

static Result verify(const char *dirarg) {
    char *d = absolute(dirarg), *directory, *parent, *receipt_path, *app, *compiler;
    Names ents;
    size_t n = 0, i, g, k;
    char *text;
    char perr[256];
    jvalue *root;
    Rows rows;
    jvalue *srm, *rg, *gs;
    char ch[65], digest_hex[65];
    Buf cb = {0, 0, 0};
    Result res;
    static const char *top[] = {"schema", "source_manifest", "source_sha256", "compiler_sha256", "required_gates", "gates"};
    static const char *item_keys[] = {"name", "binary", "binary_sha256", "build", "run"};
    static const char *rec_keys[] = {"argv", "cwd", "rc", "stdout_sha256", "stderr_sha256"};
    static const char *row_keys[] = {"path", "sha256"};
    static const char *kinds[2] = {"build", "run"}, *sfx[2] = {"stdout", "stderr"};

    trusted(d, 1, 1);
    directory = real(d);
    parent = dirname_of(directory);
    trusted(parent, 1, 1);
    receipt_path = pjoin(directory, "receipt.json");
    trusted(receipt_path, 0, 1);

    ents = list_dir(directory);
    for (i = 0; i < ents.n; i++) REQUIRE(allowed_entry(ents.v[i]), "unknown candidate entry");
    for (i = 0; i < ents.n; i++) {
        struct stat s;
        char *full = pjoin(directory, ents.v[i]);
        if (lstat(full, &s) != 0) failp("cannot stat", full);
        REQUIRE(!S_ISLNK(s.st_mode), "candidate entry symlink");
    }

    text = slurp(receipt_path, &n);
    root = json_parse(text, n, perr, sizeof perr);
    REQUIRE(root != 0, "receipt is not valid JSON");
    check_unique(root);
    REQUIRE(exact_keys(root, top, sizeof top / sizeof *top) && streq_s(jstr(jget(root, "schema")), SCHEMA),
            "receipt schema mismatch");

    app = pjoin(directory, "app");
    rows = manifest(app, 1);
    srm = jget(root, "source_manifest");
    REQUIRE(srm && srm->kind == J_ARR && srm->len == rows.n, "source manifest/hash mismatch");
    for (i = 0; i < rows.n; i++) {
        jvalue *o = srm->items[i];
        REQUIRE(exact_keys(o, row_keys, 2) && streq_s(jstr(jget(o, "path")), rows.v[i].path) &&
                    streq_s(jstr(jget(o, "sha256")), rows.v[i].sha),
                "source manifest/hash mismatch");
    }
    canon_rows(&cb, &rows);
    cx_sha256_hex(cb.p, cb.n, digest_hex);
    REQUIRE(streq_s(jstr(jget(root, "source_sha256")), digest_hex), "source manifest/hash mismatch");

    compiler = pjoin(directory, "compiler.com");
    trusted(compiler, 0, 1);
    file_sha(compiler, ch);
    REQUIRE(streq_s(jstr(jget(root, "compiler_sha256")), ch), "compiler hash mismatch");

    rg = jget(root, "required_gates");
    REQUIRE(rg && rg->kind == J_ARR && rg->len == NGATES, "required gate set mismatch");
    for (g = 0; g < NGATES; g++)
        REQUIRE(streq_s(jstr(rg->items[g]), GATES[g].name), "required gate set mismatch");
    gs = jget(root, "gates");
    REQUIRE(gs && gs->kind == J_ARR && gs->len == NGATES, "required gate set mismatch");

    for (g = 0; g < NGATES; g++) {
        jvalue *item = gs->items[g];
        char *artifact = pjoin(directory, GATES[g].binary);
        char fh[65];
        REQUIRE(exact_keys(item, item_keys, 5) && streq_s(jstr(jget(item, "name")), GATES[g].name) &&
                    streq_s(jstr(jget(item, "binary")), GATES[g].binary),
                "gate identity mismatch");
        trusted(artifact, 0, 0);
        {
            struct stat s;
            if (lstat(artifact, &s) != 0) failp("cannot stat", artifact);
            REQUIRE(MODEBITS(s.st_mode) == 0700, "artifact permissions not 0700");
        }
        file_sha(artifact, fh);
        REQUIRE(streq_s(jstr(jget(item, "binary_sha256")), fh), "artifact hash mismatch");

        for (k = 0; k < 2; k++) {
            jvalue *rec = jget(item, kinds[k]);
            Argv exp = expected_command(directory, GATES[g].binary, GATES[g].src, GATES[g].nsrc, k == 0);
            jvalue *av, *rcv;
            size_t a;
            Buf data[2] = {{0, 0, 0}, {0, 0, 0}};
            REQUIRE(exact_keys(rec, rec_keys, 5), "unknown command record");
            av = jget(rec, "argv");
            REQUIRE(av && av->kind == J_ARR && av->len == exp.n, "command/rc mismatch");
            for (a = 0; a < exp.n; a++)
                REQUIRE(streq_s(jstr(av->items[a]), exp.v[a]), "command/rc mismatch");
            REQUIRE(streq_s(jstr(jget(rec, "cwd")), app), "command/rc mismatch");
            rcv = jget(rec, "rc");
            REQUIRE(rcv && rcv->kind == J_NUM && rcv->n == 0, "command/rc mismatch");
            for (a = 0; a < 2; a++) {
                char fname[256], *path;
                size_t dn = 0;
                char *db;
                snprintf(fname, sizeof fname, "%s-%s.%s", GATES[g].name, kinds[k], sfx[a]);
                path = pjoin(directory, fname);
                trusted(path, 0, 1);
                db = slurp(path, &dn);
                data[a].p = db;
                data[a].n = dn;
                {
                    char hh[65];
                    cx_sha256_hex(db, dn, hh);
                    REQUIRE(streq_s(jstr(jget(rec, a ? "stderr_sha256" : "stdout_sha256")), hh), "gate output hash mismatch");
                }
            }
            if (k == 1)
                REQUIRE(data[1].n == 0 && !has_fail(&data[0]) && last_line_is(&data[0], GATES[g].marker),
                        "gate success mismatch");
        }
    }

    memset(&res, 0, sizeof res);
    res.candidate_dir = directory;
    res.hash = xdup(jstr(jget(root, "source_sha256")));
    res.binary = pjoin(directory, "candidate");
    res.receipt = receipt_path;
    free(text);
    return res;
}

int main(int argc, char **argv) {
    Result res;
    Buf o = {0, 0, 0};
    if (setjmp(rc_jb)) {
        Buf e = {0, 0, 0};
        bs(&e, "{\"error\": ");
        bjs(&e, rc_err);
        bs(&e, "}\n");
        fputs(e.p, stdout);
        return 1;
    }
    if (argc == 5 && !strcmp(argv[1], "build")) res = build(argv[2], argv[3], argv[4]);
    else if (argc == 3 && !strcmp(argv[1], "verify")) res = verify(argv[2]);
    else fail("usage: build SOURCE_APP COMPILER PRIVATE_ROOT | verify CANDIDATE_DIR");
    bs(&o, "{\"binary\": ");
    bjs(&o, res.binary);
    bs(&o, ", \"candidate_dir\": ");
    bjs(&o, res.candidate_dir);
    bs(&o, ", \"hash\": ");
    bjs(&o, res.hash);
    bs(&o, ", \"receipt\": ");
    bjs(&o, res.receipt);
    bs(&o, "}\n");
    fputs(o.p, stdout);
    return 0;
}
