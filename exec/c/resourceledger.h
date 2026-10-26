/* resourceledger.h: the explicit resource ledger (S step 1).  Shared by the product (exec/c/run.c, strict mode) and the
   replay runner (tests/middleflowrunner.c): one parser, one lookup, one SHA-256.  No cx.h dependency.
   Rows (LF, TAB separated):  keyhex  presence(0|1)  blob  length  sha256
     presence 1 = PRESENT: blob file in the ledger directory, exactly `length` bytes, sha256 must match (0 bytes is EMPTY, still present)
     presence 0 = ABSENT:  blob "-", length 0, sha "-" (the ledger explicitly says the resource does not exist)
   A key missing from the ledger is MISSING (the caller fails by name), never ABSENT and never a disk fallback.
   Identity (SC1): the first line may be a header  #SC1<TAB>stage=ID<TAB>input_sha256=HEX  binding the ledger to one stage
   label and to the input bytes of the run that wrote it.  Strict runs require it and check it (rl_check_identity); a
   mismatch or a missing header is a named INTERFACE_ERROR, never a silent reuse.
   Errors are returned as text; the caller prints it and fails (rc 2, empty stdout). */
#ifndef UNISA_RESOURCELEDGER_H
#define UNISA_RESOURCELEDGER_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#endif
#if !(defined(__UNISA__) && defined(_WIN32))   /* UA on Windows: sys/stat.h pulls sys/_win.h, which breaks compiler.c (include order); capture refuses by name there */
#include <sys/stat.h>
#endif
#include "../../include/unisa_sha256.h"

typedef struct { unsigned char *key; int kn; int present; unsigned char *value; int vn; char blob[64]; } RLRow;
typedef struct { RLRow *rows; int n; int has_id; int trace, cursor; char stage[128]; char input_hex[65]; } RLLedger;
typedef struct { const char *name; const unsigned char *bytes; size_t n; } RLFileView;
/* Clear only zeroed or loader-produced ledgers. Rows returned by rl_find are
 * borrowed until this call; failure outputs are always empty and clearable. */
static void rl_clear(RLLedger *L) {
    int i; if (!L) return;
    for (i = 0; i < L->n; i++) { free(L->rows[i].key); free(L->rows[i].value); }
    free(L->rows); memset(L, 0, sizeof *L);
}

static int rl_hexv(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}
/* strict decimal length: digits only, no sign, no leading zero unless 0, bounded to INT32_MAX */
static int rl_length(const char *s, int *out) {
    long long v = 0; const char *p = s;
    if (!*p) return 0;
    if (*p == '0' && p[1]) return 0;
    for (; *p; p++) { if (*p < '0' || *p > '9') return 0; v = v * 10 + (*p - '0'); if (v > 2147483647LL) return 0; }
    *out = (int)v; return 1;
}
static unsigned char *rl_readfile(const char *path, size_t *n) {
    unsigned char *b; int saved;
#ifndef _WIN32
    int fd; struct stat st; size_t size, at = 0;
    /* Refuse symlink blobs; regular-file and size checks remain. */
    errno = 0; fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return 0;
    if (fstat(fd, &st)) { saved = errno ? errno : EIO; close(fd); errno = saved; return 0; }
    if (!S_ISREG(st.st_mode)) { saved = S_ISDIR(st.st_mode) ? EISDIR : EINVAL; close(fd); errno = saved; return 0; }
    if (st.st_size < 0 || st.st_size > 2147483647L) { close(fd); errno = EFBIG; return 0; }
    size = (size_t)st.st_size; b = malloc(size + 1);
    if (!b) { close(fd); errno = ENOMEM; return 0; }
    while (at < size) {
        ssize_t z = read(fd, b + at, size - at);
        if (z < 0 && errno == EINTR) continue;
        if (z <= 0) { saved = z < 0 && errno ? errno : EIO; close(fd); free(b); errno = saved; return 0; }
        at += (size_t)z;
    }
    /* A file that grew after fstat must not be accepted as a valid prefix. */
    for (;;) { unsigned char extra; ssize_t z = read(fd, &extra, 1);
        if (z < 0 && errno == EINTR) continue;
        if (z != 0) { saved = z < 0 && errno ? errno : EIO; close(fd); free(b); errno = saved; return 0; }
        break;
    }
    if (close(fd)) { saved = errno ? errno : EIO; free(b); errno = saved; return 0; }
    *n = size; return b;
#else
    FILE *f; long sz;
    errno = 0; f = fopen(path, "rb"); if (!f) return 0;
    if (fseek(f, 0, SEEK_END) || (sz = ftell(f)) < 0 || fseek(f, 0, SEEK_SET)) {
        saved = errno ? errno : EIO; fclose(f); errno = saved; return 0;
    }
    if (sz > 2147483647L) { fclose(f); errno = EFBIG; return 0; }
    b = malloc((size_t)sz + 1); if (!b) { fclose(f); errno = ENOMEM; return 0; }
    if (sz && fread(b, 1, (size_t)sz, f) != (size_t)sz) {
        saved = errno ? errno : EIO; fclose(f); free(b); errno = saved; return 0;
    }
    if (fclose(f)) { saved = errno ? errno : EIO; free(b); errno = saved; return 0; }
    *n = (size_t)sz; return b;
#endif
}
/* Snapshot views are verified, invocation-owned bytes. Copying them gives each
 * returned ledger its own storage; neither scratch pointers nor file addresses escape. */
static unsigned char *rl_file_bytes(const char *dir, const char *name,
                                    const RLFileView *views, int count, size_t *n) {
    char path[4096]; int i, z; unsigned char *b;
    if (views) {
        for (i = 0; i < count; i++) if (!strcmp(name, views[i].name)) {
            b = malloc(views[i].n + 1);
            if (!b) { errno = ENOMEM; return 0; }
            if (views[i].n) memcpy(b, views[i].bytes, views[i].n);
            *n = views[i].n; return b;
        }
        errno = ENOENT; return 0;
    }
    z = snprintf(path, sizeof path, "%s/%s", dir, name);
    if (z < 0 || (size_t)z >= sizeof path) { errno = ENAMETOOLONG; return 0; }
    return rl_readfile(path, n);
}
/* the header line  #SC1<TAB>stage=ID<TAB>input_sha256=HEX  (ID: no TAB, 1..127 bytes; HEX: 64 lowercase digits) */
static int rl_parse_identity(const char *line, RLLedger *L) {
    const char *s = line + 5, *t = strchr(s, '\t'), *v; size_t sn, j;
    if (!t || strncmp(s, "stage=", 6)) return 0;
    sn = (size_t)(t - (s + 6));
    if (sn == 0 || sn >= sizeof L->stage) return 0;
    memcpy(L->stage, s + 6, sn); L->stage[sn] = 0;
    v = t + 1;
    if (strncmp(v, "input_sha256=", 13)) return 0;
    v += 13;
    if (strlen(v) != 64) return 0;
    for (j = 0; j < 64; j++) if (!((v[j] >= '0' && v[j] <= '9') || (v[j] >= 'a' && v[j] <= 'f'))) return 0;
    memcpy(L->input_hex, v, 64); L->input_hex[64] = 0;
    L->has_id = 1;
    return 1;
}
/* 0: loaded; otherwise err holds the named reason (INTERFACE_ERROR <reason> ...) */
static int rl_load_parse(const char *dir, const RLFileView *views, int count, RLLedger *L, char *err, size_t errn) {
    unsigned char *b; size_t n = 0, at = 0; int cap = 0;
    L->rows = 0; L->n = 0; L->has_id = 0; L->trace = 0; L->cursor = 0; L->stage[0] = 0; L->input_hex[0] = 0;
    b = rl_file_bytes(dir, "resources.tsv", views, count, &n);
    if (!b) { snprintf(err, errn, errno == ENOENT ? "INTERFACE_ERROR resource-ledger-absent %s" : errno == ENOMEM ? "INTERFACE_ERROR resource-ledger-allocation-failed %s" : "INTERFACE_ERROR resource-ledger-read-failed %s errno=%d", dir, errno); return 1; }
    while (at < n) {
        size_t e = at; char f[5][4096]; int nf = 0, k; char *line;
        size_t L2;
        while (e < n && b[e] != '\n') e++;
        if (e == n) { free(b); snprintf(err, errn, "INTERFACE_ERROR resource-row-truncated"); return 1; }
        L2 = e - at;
        /* Validate the complete byte row before C string parsing. Otherwise an
           embedded NUL hides a suffix in either the identity or a data row. */
        if (memchr(b + at, 0, L2)) { free(b); snprintf(err, errn, "INTERFACE_ERROR resource-row-nul"); return 1; }
        line = malloc(L2 + 1); if (!line) { free(b); return 1; }
        memcpy(line, b + at, L2); line[L2] = 0; at = e + 1;
        if (!strncmp(line, "#SC1\t", 5) || !strncmp(line, "#SC2\t", 5)) {
            /* identity header: the first line only, at most once */
            int ok = e == L2 && !L->has_id && rl_parse_identity(line, L);
            if (ok) L->trace = line[3] == '2';
            free(line);
            if (!ok) { free(b); snprintf(err, errn, "INTERFACE_ERROR resource-identity-malformed"); return 1; }
            continue;
        }
        { char *p = line; for (k = 0; k < 5; k++) { char *t = strchr(p, '\t'); if (k < 4 && !t) { nf = -1; break; } if (k == 4 && strchr(p, '\t')) { nf = -2; break; } size_t width = t ? (size_t)(t - p) : strlen(p); if (width >= sizeof f[k]) { nf = -3; break; } memcpy(f[k], p, width); f[k][width] = 0; p = t ? t + 1 : p + strlen(p); } nf = nf < 0 ? nf : 5; }
        if (nf != 5) { free(line); free(b); snprintf(err, errn, nf == -3 ? "INTERFACE_ERROR resource-row-field-too-long" : "INTERFACE_ERROR resource-row-fields"); return 1; }
        {   /* key */
            size_t kl = strlen(f[0]); unsigned char *kb; size_t q;
            if (!kl || kl % 2) { free(line); free(b); snprintf(err, errn, "INTERFACE_ERROR resource-key-malformed"); return 1; }
            kb = malloc(kl / 2); if (!kb) { free(line); free(b); return 1; }
            for (q = 0; q < kl; q += 2) { int hi = rl_hexv((unsigned char)f[0][q]), lo = rl_hexv((unsigned char)f[0][q+1]); if (hi < 0 || lo < 0) { free(kb); free(line); free(b); snprintf(err, errn, "INTERFACE_ERROR resource-key-malformed"); return 1; } kb[q/2] = (unsigned char)(hi * 16 + lo); }
            /* SC1 duplicate keys are refused; SC2 preserves the ordered queries. */
            { int j; if (!L->trace) for (j = 0; j < L->n; j++) if (L->rows[j].kn == (int)(kl / 2) && !memcmp(L->rows[j].key, kb, kl / 2)) { free(kb); free(line); free(b); snprintf(err, errn, "INTERFACE_ERROR duplicate-resource-key %s", f[0]); return 1; } }
            if (L->n == cap) { RLRow *grown; cap = cap ? cap * 2 : 16; grown = realloc(L->rows, (size_t)cap * sizeof *L->rows); if (!grown) { free(kb); free(line); free(b); return 1; } L->rows = grown; }
            L->rows[L->n].key = kb; L->rows[L->n].kn = (int)(kl / 2); L->rows[L->n].present = 0; L->rows[L->n].value = 0; L->rows[L->n].vn = 0;
            L->n++;
            {   RLRow *r = &L->rows[L->n - 1]; int len = 0;
                r->blob[0] = 0;
                if (!strcmp(f[1], "0")) r->present = 0; else if (!strcmp(f[1], "1")) r->present = 1; else { free(line); free(b); snprintf(err, errn, "INTERFACE_ERROR resource-presence-invalid"); return 1; }
                if (!rl_length(f[3], &len)) { free(line); free(b); snprintf(err, errn, "INTERFACE_ERROR resource-length-invalid"); return 1; }
                if (!r->present) {
                    if (strcmp(f[2], "-") || len != 0 || strcmp(f[4], "-")) { free(line); free(b); snprintf(err, errn, "INTERFACE_ERROR resource-absent-malformed"); return 1; }
                } else {
                    char hex[65]; size_t bl = 0; unsigned char *v;
                    if (!strcmp(f[2], "-") || strchr(f[2], '/') || !strcmp(f[2], "..") || strlen(f[2]) >= sizeof r->blob || strlen(f[4]) != 64) { free(line); free(b); snprintf(err, errn, "INTERFACE_ERROR resource-blob-malformed"); return 1; }
                    v = rl_file_bytes(dir, f[2], views, count, &bl);
                    if (!v) { int saved = errno; free(line); free(b); snprintf(err, errn, saved == ENOENT ? "INTERFACE_ERROR resource-bytes-absent %s" : saved == ENOMEM ? "INTERFACE_ERROR resource-ledger-allocation-failed %s" : "INTERFACE_ERROR resource-bytes-read-failed %s errno=%d", f[0], saved); return 1; }
                    if (bl != (size_t)len) { free(v); free(line); free(b); snprintf(err, errn, "INTERFACE_ERROR resource-bytes-length-mismatch %s", f[0]); return 1; }
                    unisa_sha256_hex(v, bl, hex);
                    if (strcmp(hex, f[4])) { free(v); free(line); free(b); snprintf(err, errn, "INTERFACE_ERROR resource-hash-mismatch %s", f[0]); return 1; }
                    r->value = v; r->vn = (int)bl; snprintf(r->blob, sizeof r->blob, "%s", f[2]);
                }
            }
        }
        free(line);
    }
    free(b);
    return 0;
}
/* Output may be fresh/uninitialized or previously cleared. A live ledger must
 * be released before reuse. Every failure produces an empty, clearable output;
 * rl_find borrows a row only until rl_clear. No compile-time ownership claim. */
static int rl_load_views(const char *dir, const RLFileView *views, int count,
                         RLLedger *L, char *err, size_t errn) {
    RLLedger next = {0}; int rc;
    if (!L || !dir) {
        if (L) *L = next;
        if (err && errn) snprintf(err, errn, "INTERFACE_ERROR resource-ledger-bad-arguments");
        return 1;
    }
    if (err && errn) err[0] = 0;
    rc = rl_load_parse(dir, views, count, &next, err, errn);
    if (rc) {
        rl_clear(&next);
        if (err && errn && !err[0]) snprintf(err, errn, "INTERFACE_ERROR resource-ledger-allocation-failed");
    }
    *L = next; return rc;
}
static int rl_load(const char *dir, RLLedger *L, char *err, size_t errn) {
    return rl_load_views(dir, 0, 0, L, err, errn);
}
/* SC1 identity of a run: the ledger's stage and input must be this run's stage label and this run's input bytes */
static void rl_input_hex(const unsigned char *input, int inputn, char hex[65]) {
    static const unsigned char none[1] = {0};
    unisa_sha256_hex(input ? input : none, (size_t)(inputn < 0 ? 0 : inputn), hex);
}
static int rl_check_identity(const RLLedger *L, const char *stage, const unsigned char *input, int inputn, char *err, size_t errn) {
    char hex[65];
    if (!L->has_id) { snprintf(err, errn, "INTERFACE_ERROR resource-identity-absent"); return 1; }
    if (strcmp(L->stage, stage)) { snprintf(err, errn, "INTERFACE_ERROR resource-identity-mismatch stage ledger=%s run=%s", L->stage, stage); return 1; }
    rl_input_hex(input, inputn, hex);
    if (strcmp(L->input_hex, hex)) { snprintf(err, errn, "INTERFACE_ERROR resource-identity-mismatch input ledger=%s run=%s", L->input_hex, hex); return 1; }
    return 0;
}
/* writes the header line for a capture (stage label and the sha256 of the input bytes the capture runs on) */
static inline int rl_identity_line(char *b, size_t cap, const char *stage, const char *hex) {
    int n = snprintf(b, cap, "#SC1\tstage=%s\tinput_sha256=%s\n", stage, hex); return n > 0 && (size_t)n < cap;
}
static void rl_write_identity_hex(FILE *f, const char *stage, const char *hex) { char line[512]; if (rl_identity_line(line, sizeof line, stage, hex)) fputs(line, f); }
static void rl_write_identity(FILE *f, const char *stage, const unsigned char *input, int inputn) {
    char hex[65]; rl_input_hex(input, inputn, hex); rl_write_identity_hex(f, stage, hex);
}
/* exact key lookup: the row, or NULL when the ledger does not list the key (MISSING for the caller) */
static const RLRow *rl_find(const RLLedger *L, const unsigned char *key, int kn) {
    int j; if (L->trace) return 0; /* SC2 is ordered, never a first-entry map. */
    for (j = 0; j < L->n; j++) if (L->rows[j].kn == kn && !memcmp(L->rows[j].key, key, (size_t)kn)) return &L->rows[j];
    return 0;
}

typedef struct { const unsigned char *key; int kn; const unsigned char *data; int len; } RLBlob;
#ifdef _WIN32
#define RL_SNAP_MAX 1024
#else
#define RL_SNAP_MAX 999
#endif
/* stage labels: "<stage>:<route>", stage in the registered table.  The route is the driver's own shape (compiler.c:674-677,889;
   the only grammar authority): "<target>/<arch>/[warn/][multi/]<format>/O<digit>", target lnx|osx|win, arch x86_64|arm64,
   format tape|image|run|object, warn before multi.  A prefix before the target is not a driver shape and is refused. */
static inline int rl_token_is(const char *p, size_t n, const char *tok) { return strlen(tok) == n && !memcmp(p, tok, n); }
static inline int rl_stage_label_ok(const char *label) {
    /* "source" binds image front-end input; e3 binds the token input. */
    static const char *stages[] = {"e3", "lower", "source"}, *targets[] = {"lnx", "osx", "win"}, *arches[] = {"x86_64", "arm64"},
                       *formats[] = {"tape", "image", "run", "object"};
    const char *colon = label ? strchr(label, ':') : 0, *r, *q; size_t i, n; int ok;
    if (!colon) return 0;
    ok = 0; for (i = 0; i < sizeof stages / sizeof stages[0]; i++) if (rl_token_is(label, (size_t)(colon - label), stages[i])) ok = 1;
    if (!ok) return 0;
    r = colon + 1;
    q = strchr(r, '/'); if (!q) return 0; n = (size_t)(q - r);
    ok = 0; for (i = 0; i < 3; i++) if (rl_token_is(r, n, targets[i])) ok = 1;
    if (!ok) return 0;
    r = q + 1; q = strchr(r, '/'); if (!q) return 0; n = (size_t)(q - r);
    ok = 0; for (i = 0; i < 2; i++) if (rl_token_is(r, n, arches[i])) ok = 1;
    if (!ok) return 0;
    r = q + 1;
    if (!strncmp(r, "warn/", 5)) r += 5;
    if (!strncmp(r, "multi/", 6)) r += 6;
    q = strchr(r, '/'); if (!q) return 0; n = (size_t)(q - r);
    ok = 0; for (i = 0; i < 4; i++) if (rl_token_is(r, n, formats[i])) ok = 1;
    if (!ok) return 0;
    r = q + 1;
    return r[0] == 'O' && r[1] >= '0' && r[1] <= '9' && r[2] == 0;
}
#if defined(_WIN32)   /* Windows no-replace commit remains a separate platform contract. */
static inline int rl_snapshot_write(const char *dir, const char *stage_label, const unsigned char *input, int inputn,
                                    const RLBlob *b, int nb, char *err, size_t errn) {
    (void)dir; (void)stage_label; (void)input; (void)inputn; (void)b; (void)nb;
    snprintf(err, errn, "INTERFACE_ERROR resource-snapshot-unsupported-platform"); return 1;
}
static inline int rl_snapshot_load(const char *dir, const char *upstream_stage_label, RLLedger *L, int observe, char *err, size_t errn) {
    (void)dir; (void)upstream_stage_label; (void)observe; if (L) memset(L, 0, sizeof *L);
    snprintf(err, errn, "INTERFACE_ERROR resource-snapshot-unsupported-platform"); return 1;
}
#else   /* POSIX: mkdir/link no-replace commit with a COMMIT marker written last */
#include <dirent.h>
static inline int rl_snap_write_file(const char *path, const unsigned char *d, size_t n) {
    FILE *f = fopen(path, "wb"); size_t w = 0; int ok;
    if (!f) return 0;
    if (n) w = fwrite(d, 1, n, f);
    ok = (w == n);
    if (fflush(f) != 0) ok = 0;
    if (fclose(f) != 0) ok = 0;
    return ok;
}
static inline void rl_snap_unlink_all(const char *d, char (*names)[64], int n) {
    char p[4096]; int i;
    for (i = 0; i < n; i++) { snprintf(p, sizeof p, "%s/%s", d, names[i]); unlink(p); }
}
/* No-replace hard links for the carried POSIX runtime. This is an OS
 * mechanism adapter: resource semantics and COMMIT ordering remain above it.
 * Linux: syscall_64.tbl link=86, asm-generic unistd.h linkat=37.
 * Darwin: xnu bsd/kern/syscalls.master link=9 (BSD syscall class). */
static inline int rl_snap_link(const char *from, const char *to) {
#ifdef __UNISA__
#ifdef __APPLE__
    return (int)_unisa_ret(__syscall6(_UNISA_SC(9), (long)from, (long)to, 0, 0, 0));
#elif defined(__x86_64__)
    return (int)_unisa_ret(__syscall6(86, (long)from, (long)to, 0, 0, 0));
#elif defined(__aarch64__)
    return (int)_unisa_ret(__syscall6(37, -100, (long)from, -100, (long)to, 0));
#else
    errno = ENOSYS; return -1;
#endif
#else
    return link(from, to);
#endif
}
/* a listed name the reader accepts: input.bin, resources.tsv, or resource-NNN.bytes (three digits, 000..999) */
static inline int rl_snap_name_ok(const char *nm) {
    int i;
    if (!strcmp(nm, "input.bin") || !strcmp(nm, "resources.tsv")) return 1;
    if (strlen(nm) != 18 || strncmp(nm, "resource-", 9) || strcmp(nm + 12, ".bytes")) return 0;
    for (i = 9; i < 12; i++) if (nm[i] < '0' || nm[i] > '9') return 0;
    return 1;
}
static inline int rl_snap_blob_name_ok(const char *nm) {   /* resource-NNN.bytes only: a ledger row may reference nothing else */
    int i;
    if (strlen(nm) != 18 || strncmp(nm, "resource-", 9) || strcmp(nm + 12, ".bytes")) return 0;
    for (i = 9; i < 12; i++) if (nm[i] < '0' || nm[i] > '9') return 0;
    return 1;
}
static inline int rl_snapshot_write(const char *dir, const char *stage_label, const unsigned char *input, int inputn,
                                    const RLBlob *b, int nb, char *err, size_t errn) {
    static char names[RL_SNAP_MAX + 3][64]; static char commit[1 << 17]; static char table[1 << 18];
    static unsigned char skip[RL_SNAP_MAX + 1];
    char tmp[4096], p[4096], q[4096], hex[65], line[512]; size_t cat = 0, tat = 0; int nf = 0, k, j, nfin = 0, ok = 1, ninc = 0;
    if (!dir || !stage_label || inputn < 0 || (inputn > 0 && !input) || nb < 0 || (nb > 0 && !b)) {
        snprintf(err, errn, "INTERFACE_ERROR snapshot-bad-arguments"); return 1; }
    if (!rl_stage_label_ok(stage_label)) { snprintf(err, errn, "INTERFACE_ERROR snapshot-stage-label %s", stage_label); return 1; }
    if (nb > RL_SNAP_MAX) { snprintf(err, errn, "INTERFACE_ERROR snapshot-too-many-blobs"); return 1; }
    for (k = 0; k < nb; k++) {
        skip[k] = 0;
        if (b[k].kn <= 0) { snprintf(err, errn, "INTERFACE_ERROR snapshot-empty-key %d", k); return 1; }
        if (b[k].kn > 512 || b[k].len < 0 || !b[k].key || (b[k].len > 0 && !b[k].data)) {
            snprintf(err, errn, "INTERFACE_ERROR snapshot-bad-blob %d", k); return 1; }
    }
    /* duplicate keys: the same key with the same bytes is written once; the same key with other bytes is refused */
    for (k = 0; k < nb; k++) for (j = 0; j < k; j++)
        if (b[j].kn == b[k].kn && !memcmp(b[j].key, b[k].key, (size_t)b[k].kn)) {
            if (b[j].len == b[k].len && (b[k].len == 0 || !memcmp(b[j].data, b[k].data, (size_t)b[k].len))) { skip[k] = 1; break; }
            snprintf(err, errn, "INTERFACE_ERROR snapshot-key-conflict"); return 1; }
    { int z = snprintf(tmp, sizeof tmp, "%s.tmp%ld", dir, (long)getpid());
      size_t leaf = nb ? 18 : 13; /* longest generated child, including metadata */
      if (z < 0 || (size_t)z > sizeof tmp - leaf - 2) { snprintf(err, errn, "INTERFACE_ERROR snapshot-path-too-long"); return 1; }
    }
    if (mkdir(tmp, 0700)) { snprintf(err, errn, "INTERFACE_ERROR resource-snapshot-write-failed tmp-create %s", tmp); return 1; }
#define RL_SNAP_FAIL(msg) do { rl_snap_unlink_all(tmp, names, nf); rmdir(tmp); snprintf(err, errn, "INTERFACE_ERROR %s", msg); return 1; } while (0)
    snprintf(names[nf], sizeof names[0], "input.bin"); snprintf(p, sizeof p, "%s/%s", tmp, names[nf]);
    if (!rl_snap_write_file(p, input, (size_t)inputn)) { nf++; RL_SNAP_FAIL("resource-snapshot-write-failed input.bin"); }
    nf++;
    unisa_sha256_hex(input, (size_t)inputn, hex);
    snprintf(q, sizeof q, "%s  input.bin\n", hex); if (cat + strlen(q) + 1 > sizeof commit) RL_SNAP_FAIL("snapshot-capacity-exceeded");
    memcpy(commit + cat, q, strlen(q)); cat += strlen(q);
    if (!rl_identity_line(line, sizeof line, stage_label, hex)) RL_SNAP_FAIL("snapshot-capacity-exceeded");
    if (tat + strlen(line) + 1 > sizeof table) RL_SNAP_FAIL("snapshot-capacity-exceeded");
    memcpy(table, line, strlen(line)); tat = strlen(line);
    for (k = 0; k < nb; k++) {
        char kh[1025], bh[65], nm[64], row[1200]; int jj;
        if (skip[k]) continue;
        for (jj = 0; jj < b[k].kn; jj++) { kh[2*jj] = "0123456789abcdef"[b[k].key[jj] >> 4]; kh[2*jj+1] = "0123456789abcdef"[b[k].key[jj] & 15]; }
        kh[2*b[k].kn] = 0;
        snprintf(nm, sizeof nm, "resource-%03d.bytes", ninc);   /* the ordinal of the written blob, not the caller's index */
        snprintf(names[nf], sizeof names[0], "%s", nm); snprintf(p, sizeof p, "%s/%s", tmp, nm);
        if (!rl_snap_write_file(p, b[k].data, (size_t)b[k].len)) { nf++; RL_SNAP_FAIL("resource-snapshot-write-failed blob"); }
        nf++; ninc++;
        unisa_sha256_hex(b[k].data, (size_t)b[k].len, bh);
        snprintf(row, sizeof row, "%s\t1\t%s\t%d\t%s\n", kh, nm, b[k].len, bh);
        if (tat + strlen(row) + 1 > sizeof table) RL_SNAP_FAIL("snapshot-capacity-exceeded");
        memcpy(table + tat, row, strlen(row)); tat += strlen(row);
        snprintf(q, sizeof q, "%s  %s\n", bh, nm); if (cat + strlen(q) + 1 > sizeof commit) RL_SNAP_FAIL("snapshot-capacity-exceeded");
        memcpy(commit + cat, q, strlen(q)); cat += strlen(q);
    }
    snprintf(names[nf], sizeof names[0], "resources.tsv"); snprintf(p, sizeof p, "%s/%s", tmp, names[nf]);
    if (!rl_snap_write_file(p, (const unsigned char *)table, tat)) { nf++; RL_SNAP_FAIL("resource-snapshot-write-failed table"); }
    nf++;
    unisa_sha256_hex(table, tat, hex); snprintf(q, sizeof q, "%s  resources.tsv\n", hex);
    if (cat + strlen(q) + 1 > sizeof commit) RL_SNAP_FAIL("snapshot-capacity-exceeded");
    memcpy(commit + cat, q, strlen(q)); cat += strlen(q);
    snprintf(names[nf], sizeof names[0], "COMMIT"); snprintf(p, sizeof p, "%s/COMMIT", tmp);
    if (!rl_snap_write_file(p, (const unsigned char *)commit, cat)) { nf++; RL_SNAP_FAIL("resource-snapshot-write-failed COMMIT"); }
    nf++;
    if (mkdir(dir, 0700)) { int saved = errno; rl_snap_unlink_all(tmp, names, nf); rmdir(tmp);
        snprintf(err, errn, saved == EEXIST ? "INTERFACE_ERROR snapshot-target-exists %s" : "INTERFACE_ERROR resource-snapshot-write-failed target %s", dir); return 1; }
    for (nfin = 0; nfin < nf && ok; nfin++) {
        char from[4096], to[4096]; snprintf(from, sizeof from, "%s/%s", tmp, names[nfin]); snprintf(to, sizeof to, "%s/%s", dir, names[nfin]);
        if (rl_snap_link(from, to)) ok = 0;
    }
    if (!ok) {
        rl_snap_unlink_all(dir, names, nfin - 1); rmdir(dir); rl_snap_unlink_all(tmp, names, nf); rmdir(tmp);
        snprintf(err, errn, "INTERFACE_ERROR resource-snapshot-write-failed link"); return 1;
    }
    rl_snap_unlink_all(tmp, names, nf); rmdir(tmp);
    return 0;
}
/* loader (standalone only; parses retained verified views): COMMIT lists exactly the snapshot's files with their sha256; the ledger's references and
   the listed blobs correspond one to one; the header of resources.tsv matches the upstream input.bin (rl_check_identity).
   Callable address keys (the producer's \0library/callablemake and \0library/callablecall, 21 bytes each) are process-local:
   they load only when observe is set (observation or same-ledger replay).  Internal consistency only: no external authentication. */
static inline int rl_snapshot_load_parse(const char *dir, const char *upstream_stage_label,
                                         RLLedger *L, int observe, char *err, size_t errn) {
    char listed[RL_SNAP_MAX + 3][64], hashes[RL_SNAP_MAX + 3][65];
    RLFileView files[RL_SNAP_MAX + 3]; int refs[RL_SNAP_MAX + 3];
    unsigned char *commit = 0; size_t cn = 0, at = 0; int nl = 0, i, rc = 1, input = -1, table = -1;
    static const char callmake[] = "library/callablemake", callcall[] = "library/callablecall";
    memset(files, 0, sizeof files); memset(refs, 0, sizeof refs);
    if (!dir || !upstream_stage_label || !L) { snprintf(err, errn, "INTERFACE_ERROR snapshot-bad-arguments"); return 1; }
    if (!rl_stage_label_ok(upstream_stage_label)) { snprintf(err, errn, "INTERFACE_ERROR snapshot-stage-label"); return 1; }
    commit = rl_file_bytes(dir, "COMMIT", 0, 0, &cn);
    if (!commit) {
        snprintf(err, errn, errno == ENOENT ? "INTERFACE_ERROR snapshot-incomplete no COMMIT" :
                 errno == ENOMEM ? "INTERFACE_ERROR snapshot-out-of-memory COMMIT" :
                 "INTERFACE_ERROR snapshot-commit-read-failed errno=%d", errno);
        return 1;
    }
#define RL_SNAP_LFAIL(msg) do { snprintf(err, errn, "INTERFACE_ERROR %s", msg); goto done; } while (0)
    while (at < cn) {
        size_t end = at, len, k, name_n;
        while (end < cn && commit[end] != '\n') end++;
        len = end - at;
        if (end == cn || len < 67 || len >= 512 || memchr(commit + at, 0, len) || commit[at+64] != ' ' || commit[at+65] != ' ')
            RL_SNAP_LFAIL("snapshot-commit-malformed");
        for (k = 0; k < 64; k++) if (!((commit[at+k] >= '0' && commit[at+k] <= '9') || (commit[at+k] >= 'a' && commit[at+k] <= 'f')))
            RL_SNAP_LFAIL("snapshot-commit-malformed");
        if (nl >= RL_SNAP_MAX + 3) RL_SNAP_LFAIL("snapshot-commit-too-many");
        name_n = len - 66;
        if (name_n >= sizeof listed[0]) RL_SNAP_LFAIL("snapshot-commit-name");
        memcpy(listed[nl], commit + at + 66, name_n); listed[nl][name_n] = 0;
        if (!rl_snap_name_ok(listed[nl])) RL_SNAP_LFAIL("snapshot-commit-name");
        for (i = 0; i < nl; i++) if (!strcmp(listed[i], listed[nl])) RL_SNAP_LFAIL("snapshot-commit-duplicate");
        memcpy(hashes[nl], commit + at, 64); hashes[nl][64] = 0;
        files[nl].name = listed[nl];
        if (!strcmp(listed[nl], "input.bin")) input = nl;
        if (!strcmp(listed[nl], "resources.tsv")) table = nl;
        nl++; at = end + 1;
    }
    if (input < 0 || table < 0) RL_SNAP_LFAIL("snapshot-commit-incomplete");
    /* Each file is read once, verified against the retained COMMIT, then kept
     * until parsing finishes. Parsing never reopens a table or resource file. */
    for (i = 0; i < nl; i++) {
        size_t n = 0; char hash[65];
        files[i].bytes = rl_file_bytes(dir, listed[i], 0, 0, &n); files[i].n = n;
        if (!files[i].bytes) {
            snprintf(err, errn, errno == ENOENT ? "INTERFACE_ERROR snapshot-listed-file-absent %s" :
                     errno == ENOMEM ? "INTERFACE_ERROR snapshot-out-of-memory %s" :
                     "INTERFACE_ERROR snapshot-listed-file-read-failed %s", listed[i]);
            goto done;
        }
        unisa_sha256_hex(files[i].bytes, n, hash);
        if (strcmp(hash, hashes[i])) RL_SNAP_LFAIL("snapshot-file-sha-mismatch");
    }
    { DIR *d = opendir(dir); struct dirent *e;
      if (!d) RL_SNAP_LFAIL("snapshot-dir-open");
      for (;;) {
          int found = 0; errno = 0; e = readdir(d);
          if (!e) { int saved = errno; if (saved) { closedir(d); snprintf(err, errn, "INTERFACE_ERROR snapshot-dir-read errno=%d", saved); goto done; } break; }
          if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..") || !strcmp(e->d_name, "COMMIT")) continue;
          for (i = 0; i < nl; i++) if (!strcmp(e->d_name, listed[i])) found = 1;
          if (!found) { closedir(d); RL_SNAP_LFAIL("snapshot-extra-file"); }
      }
      if (closedir(d)) RL_SNAP_LFAIL("snapshot-dir-close");
    }
    if (rl_load_views(dir, files, nl, L, err, errn)) goto done;
    if (L->trace) RL_SNAP_LFAIL("snapshot-trace-ledger-not-map");
    if (rl_check_identity(L, upstream_stage_label, files[input].bytes, (int)files[input].n, err, errn)) goto done;
    for (i = 0; i < L->n; i++) {
        int j, found = -1;
        if (!L->rows[i].present) continue;
        if (!rl_snap_blob_name_ok(L->rows[i].blob)) RL_SNAP_LFAIL("snapshot-ledger-bad-blob-name");
        for (j = 0; j < nl; j++) if (!strcmp(L->rows[i].blob, listed[j])) found = j;
        if (found < 0) RL_SNAP_LFAIL("snapshot-ledger-unlisted-blob");
        if (++refs[found] > 1) RL_SNAP_LFAIL("snapshot-ledger-duplicate-reference");
        if (L->rows[i].kn == 21 && L->rows[i].key[0] == 0 &&
            (!memcmp(L->rows[i].key + 1, callmake, 20) || !memcmp(L->rows[i].key + 1, callcall, 20))) {
            if (!observe) RL_SNAP_LFAIL("snapshot-callable-address-not-portable (needs --observe-callables)");
        }
    }
    for (i = 0; i < nl; i++) if (i != input && i != table && refs[i] != 1) RL_SNAP_LFAIL("snapshot-commit-unreferenced");
    rc = 0;
done:
    for (i = 0; i < nl; i++) free((void *)files[i].bytes);
    free(commit);
#undef RL_SNAP_LFAIL
    return rc;
}
static inline int rl_snapshot_load(const char *dir, const char *upstream_stage_label,
                                    RLLedger *L, int observe, char *err, size_t errn) {
    RLLedger next = {0}; int rc;
    if (!L) { snprintf(err, errn, "INTERFACE_ERROR snapshot-bad-arguments"); return 1; }
    rc = rl_snapshot_load_parse(dir, upstream_stage_label, &next, observe, err, errn);
    if (rc) rl_clear(&next);
    *L = next; return rc;
}
#endif   /* _WIN32 */
#endif
