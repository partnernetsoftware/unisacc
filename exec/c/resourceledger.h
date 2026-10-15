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
#if !(defined(__UNISA__) && defined(_WIN32))   /* UA on Windows: sys/stat.h pulls sys/_win.h, which breaks compiler.c (include order); capture refuses by name there */
#include <sys/stat.h>
#endif
#include "../../include/unisa_sha256.h"

typedef struct { unsigned char *key; int kn; int present; unsigned char *value; int vn; char blob[64]; } RLRow;
typedef struct { RLRow *rows; int n; int has_id; char stage[128]; char input_hex[65]; } RLLedger;

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
    FILE *f = fopen(path, "rb"); unsigned char *b; long sz;
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END) || (sz = ftell(f)) < 0 || fseek(f, 0, SEEK_SET)) { fclose(f); return 0; }
    b = malloc((size_t)sz + 1); if (!b) { fclose(f); return 0; }
    if (sz && fread(b, 1, (size_t)sz, f) != (size_t)sz) { fclose(f); free(b); return 0; }
    fclose(f); *n = (size_t)sz; return b;
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
static int rl_load(const char *dir, RLLedger *L, char *err, size_t errn) {
    char path[4096]; unsigned char *b; size_t n = 0, at = 0; int cap = 0;
    L->rows = 0; L->n = 0; L->has_id = 0; L->stage[0] = 0; L->input_hex[0] = 0;
    snprintf(path, sizeof path, "%s/resources.tsv", dir);
    b = rl_readfile(path, &n);
    if (!b) { snprintf(err, errn, "INTERFACE_ERROR resource-ledger-absent %s", path); return 1; }
    while (at < n) {
        size_t e = at; char f[5][4096]; int nf = 0, k; char *line;
        size_t L2;
        while (e < n && b[e] != '\n') e++;
        if (e == n) { free(b); snprintf(err, errn, "INTERFACE_ERROR resource-row-truncated"); return 1; }
        L2 = e - at; line = malloc(L2 + 1); if (!line) { free(b); return 1; }
        memcpy(line, b + at, L2); line[L2] = 0; at = e + 1;
        if (!strncmp(line, "#SC1\t", 5)) {
            /* identity header: the first line only, at most once */
            int ok = e == L2 && !L->has_id && rl_parse_identity(line, L);
            free(line);
            if (!ok) { free(b); snprintf(err, errn, "INTERFACE_ERROR resource-identity-malformed"); return 1; }
            continue;
        }
        { char *p = line; for (k = 0; k < 5; k++) { char *t = strchr(p, '\t'); if (k < 4 && !t) { nf = -1; break; } if (k == 4 && strchr(p, '\t')) { nf = -2; break; } snprintf(f[k], sizeof f[k], "%.*s", (int)(t ? (size_t)(t - p) : strlen(p)), p); p = t ? t + 1 : p + strlen(p); } nf = nf < 0 ? nf : 5; }
        if (nf != 5) { free(line); free(b); snprintf(err, errn, "INTERFACE_ERROR resource-row-fields"); return 1; }
        {   /* key */
            size_t kl = strlen(f[0]); unsigned char *kb; size_t q;
            if (!kl || kl % 2) { free(line); free(b); snprintf(err, errn, "INTERFACE_ERROR resource-key-malformed"); return 1; }
            kb = malloc(kl / 2);
            for (q = 0; q < kl; q += 2) { int hi = rl_hexv((unsigned char)f[0][q]), lo = rl_hexv((unsigned char)f[0][q+1]); if (hi < 0 || lo < 0) { free(kb); free(line); free(b); snprintf(err, errn, "INTERFACE_ERROR resource-key-malformed"); return 1; } kb[q/2] = (unsigned char)(hi * 16 + lo); }
            /* duplicate key: named refusal, never first-entry-wins */
            { int j; for (j = 0; j < L->n; j++) if (L->rows[j].kn == (int)(kl / 2) && !memcmp(L->rows[j].key, kb, kl / 2)) { free(kb); free(line); free(b); snprintf(err, errn, "INTERFACE_ERROR duplicate-resource-key %s", f[0]); return 1; } }
            if (L->n == cap) { cap = cap ? cap * 2 : 16; L->rows = realloc(L->rows, (size_t)cap * sizeof *L->rows); if (!L->rows) { free(kb); free(line); free(b); return 1; } }
            L->rows[L->n].key = kb; L->rows[L->n].kn = (int)(kl / 2); L->rows[L->n].present = 0; L->rows[L->n].value = 0; L->rows[L->n].vn = 0;
            L->n++;
            {   RLRow *r = &L->rows[L->n - 1]; int len = 0;
                r->blob[0] = 0;
                if (!strcmp(f[1], "0")) r->present = 0; else if (!strcmp(f[1], "1")) r->present = 1; else { free(line); free(b); snprintf(err, errn, "INTERFACE_ERROR resource-presence-invalid"); return 1; }
                if (!rl_length(f[3], &len)) { free(line); free(b); snprintf(err, errn, "INTERFACE_ERROR resource-length-invalid"); return 1; }
                if (!r->present) {
                    if (strcmp(f[2], "-") || len != 0 || strcmp(f[4], "-")) { free(line); free(b); snprintf(err, errn, "INTERFACE_ERROR resource-absent-malformed"); return 1; }
                } else {
                    char bp[4096], hex[65]; size_t bl = 0; unsigned char *v;
                    if (!strcmp(f[2], "-") || strchr(f[2], '/') || !strcmp(f[2], "..") || strlen(f[4]) != 64) { free(line); free(b); snprintf(err, errn, "INTERFACE_ERROR resource-blob-malformed"); return 1; }
                    snprintf(bp, sizeof bp, "%s/%s", dir, f[2]);
                    v = rl_readfile(bp, &bl);
                    if (!v || (int)bl != len) { free(v); free(line); free(b); snprintf(err, errn, "INTERFACE_ERROR resource-bytes-absent %s", f[0]); return 1; }
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
    int j;
    for (j = 0; j < L->n; j++) if (L->rows[j].kn == kn && !memcmp(L->rows[j].key, key, (size_t)kn)) return &L->rows[j];
    return 0;
}

typedef struct { const unsigned char *key; int kn; const unsigned char *data; int len; } RLBlob;
#define RL_SNAP_MAX 1024
/* stage labels: "<stage>:<route>", stage in the registered table, route "[warn/][multi/]<target>/<arch>/<format>/O<digit>" */
static inline int rl_token_is(const char *p, size_t n, const char *tok) { return strlen(tok) == n && !memcmp(p, tok, n); }
static inline int rl_stage_label_ok(const char *label) {
    static const char *stages[] = {"e3", "lower"}, *targets[] = {"lnx", "osx", "win"}, *arches[] = {"x86_64", "arm64"},
                       *formats[] = {"tape", "image", "run", "object"};
    const char *colon = label ? strchr(label, ':') : 0, *r, *q; size_t i, n; int ok;
    if (!colon) return 0;
    ok = 0; for (i = 0; i < sizeof stages / sizeof stages[0]; i++) if (rl_token_is(label, (size_t)(colon - label), stages[i])) ok = 1;
    if (!ok) return 0;
    r = colon + 1;
    if (!strncmp(r, "warn/", 5)) r += 5;
    if (!strncmp(r, "multi/", 6)) r += 6;
    q = strchr(r, '/'); if (!q) return 0; n = (size_t)(q - r);
    ok = 0; for (i = 0; i < 3; i++) if (rl_token_is(r, n, targets[i])) ok = 1;
    if (!ok) return 0;
    r = q + 1; q = strchr(r, '/'); if (!q) return 0; n = (size_t)(q - r);
    ok = 0; for (i = 0; i < 2; i++) if (rl_token_is(r, n, arches[i])) ok = 1;
    if (!ok) return 0;
    r = q + 1; q = strchr(r, '/'); if (!q) return 0; n = (size_t)(q - r);
    ok = 0; for (i = 0; i < 4; i++) if (rl_token_is(r, n, formats[i])) ok = 1;
    if (!ok) return 0;
    r = q + 1;
    return r[0] == 'O' && r[1] >= '0' && r[1] <= '9' && r[2] == 0;
}
#if defined(_WIN32) || defined(__UNISA__)   /* debt: the Windows no-replace commit (MoveFileEx without REPLACE_EXISTING) is not implemented yet; UA's libc has no link(), so a UA-built product refuses snapshots by name too */
static inline int rl_snapshot_write(const char *dir, const char *stage_label, const unsigned char *input, int inputn,
                                    const RLBlob *b, int nb, char *err, size_t errn) {
    (void)dir; (void)stage_label; (void)input; (void)inputn; (void)b; (void)nb;
    snprintf(err, errn, "INTERFACE_ERROR resource-snapshot-unsupported-platform"); return 1;
}
static inline int rl_snapshot_load(const char *dir, const char *upstream_stage_label, RLLedger *L, int observe, char *err, size_t errn) {
    (void)dir; (void)upstream_stage_label; (void)L; (void)observe;
    snprintf(err, errn, "INTERFACE_ERROR resource-snapshot-unsupported-platform"); return 1;
}
#else   /* POSIX: mkdir/link no-replace commit with a COMMIT marker written last */
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
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
#define RL_SNAP_MAX 999   /* blob numbers are resource-NNN.bytes: the writer and the reader share this range */
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
    snprintf(tmp, sizeof tmp, "%s.tmp%ld", dir, (long)getpid());
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
    if (mkdir(dir, 0700)) { rl_snap_unlink_all(tmp, names, nf); rmdir(tmp);
        snprintf(err, errn, errno == EEXIST ? "INTERFACE_ERROR snapshot-target-exists %s" : "INTERFACE_ERROR resource-snapshot-write-failed target %s", dir); return 1; }
    for (nfin = 0; nfin < nf && ok; nfin++) {
        char from[4096], to[4096]; snprintf(from, sizeof from, "%s/%s", tmp, names[nfin]); snprintf(to, sizeof to, "%s/%s", dir, names[nfin]);
        if (link(from, to)) ok = 0;
    }
    if (!ok) {
        rl_snap_unlink_all(dir, names, nfin - 1); rmdir(dir); rl_snap_unlink_all(tmp, names, nf); rmdir(tmp);
        snprintf(err, errn, "INTERFACE_ERROR resource-snapshot-write-failed link"); return 1;
    }
    rl_snap_unlink_all(tmp, names, nf); rmdir(tmp);
    return 0;
}
/* loader (standalone only; uses rl_load): COMMIT lists exactly the snapshot's files with their sha256; the ledger's references and
   the listed blobs correspond one to one; the header of resources.tsv matches the upstream input.bin (rl_check_identity).
   Callable address keys (the producer's \0library/callablemake and \0library/callablecall, 21 bytes each) are process-local:
   they load only when observe is set (observation or same-ledger replay).  Internal consistency only: no external authentication. */
static inline int rl_snapshot_load(const char *dir, const char *upstream_stage_label, RLLedger *L, int observe, char *err, size_t errn) {
    static char listed[RL_SNAP_MAX + 3][64]; static char lhash[RL_SNAP_MAX + 3][65]; static int refs[RL_SNAP_MAX + 3];
    char path[4096], line[512], *buf = NULL; FILE *f; int nl = 0, i, r, hasin = 0, hastab = 0;
    static const char callmake[] = "library/callablemake", callcall[] = "library/callablecall";
    if (!dir || !upstream_stage_label || !L) { snprintf(err, errn, "INTERFACE_ERROR snapshot-bad-arguments"); return 1; }
    snprintf(path, sizeof path, "%s/COMMIT", dir); f = fopen(path, "rb");
    if (!f) { snprintf(err, errn, "INTERFACE_ERROR snapshot-incomplete no COMMIT %s", dir); return 1; }
#define RL_SNAP_LFAIL(msg) do { fclose(f); free(buf); snprintf(err, errn, "INTERFACE_ERROR %s", msg); return 1; } while (0)
    while (fgets(line, sizeof line, f)) {
        size_t ln = strlen(line); int hx;
        if (ln == 0 || line[ln-1] != '\n') RL_SNAP_LFAIL("snapshot-commit-malformed");
        line[--ln] = 0;
        if (ln < 67 || line[64] != ' ' || line[65] != ' ') RL_SNAP_LFAIL("snapshot-commit-malformed");
        for (hx = 0; hx < 64; hx++) if (!((line[hx] >= '0' && line[hx] <= '9') || (line[hx] >= 'a' && line[hx] <= 'f'))) RL_SNAP_LFAIL("snapshot-commit-malformed");
        if (!rl_snap_name_ok(line + 66)) RL_SNAP_LFAIL("snapshot-commit-name");
        for (i = 0; i < nl; i++) if (!strcmp(listed[i], line + 66)) RL_SNAP_LFAIL("snapshot-commit-duplicate");
        if (nl >= RL_SNAP_MAX + 3) RL_SNAP_LFAIL("snapshot-commit-too-many");
        memcpy(lhash[nl], line, 64); lhash[nl][64] = 0; snprintf(listed[nl], sizeof listed[0], "%s", line + 66); refs[nl] = 0; nl++;
    }
    fclose(f);
    for (i = 0; i < nl; i++) { if (!strcmp(listed[i], "input.bin")) hasin = 1; if (!strcmp(listed[i], "resources.tsv")) hastab = 1; }
    if (!hasin || !hastab) { snprintf(err, errn, "INTERFACE_ERROR snapshot-commit-incomplete"); return 1; }
    /* every listed file exists and matches its sha256 */
    for (i = 0; i < nl; i++) {
        FILE *g; long cap; size_t n; char got[65];
        snprintf(path, sizeof path, "%s/%s", dir, listed[i]); g = fopen(path, "rb");
        if (!g) { snprintf(err, errn, "INTERFACE_ERROR snapshot-listed-file-absent %s", listed[i]); return 1; }
        if (fseek(g, 0, SEEK_END) || (cap = ftell(g)) < 0 || fseek(g, 0, SEEK_SET)) { fclose(g); snprintf(err, errn, "INTERFACE_ERROR snapshot-listed-file-size %s", listed[i]); return 1; }
        buf = malloc(cap ? (size_t)cap : 1);
        if (!buf) { fclose(g); snprintf(err, errn, "INTERFACE_ERROR snapshot-out-of-memory"); return 1; }
        n = cap ? fread(buf, 1, (size_t)cap, g) : 0; fclose(g);
        if (n != (size_t)cap) { free(buf); buf = NULL; snprintf(err, errn, "INTERFACE_ERROR snapshot-listed-file-read %s", listed[i]); return 1; }
        unisa_sha256_hex(buf, n, got); free(buf); buf = NULL;
        if (strcmp(got, lhash[i])) { snprintf(err, errn, "INTERFACE_ERROR snapshot-file-sha-mismatch %s", listed[i]); return 1; }
    }
    /* no file in the directory is unlisted */
    { DIR *d = opendir(dir); struct dirent *e;
      if (!d) { snprintf(err, errn, "INTERFACE_ERROR snapshot-dir-open"); return 1; }
      while ((e = readdir(d))) {
          int found = 0;
          if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..") || !strcmp(e->d_name, "COMMIT")) continue;
          for (i = 0; i < nl; i++) if (!strcmp(listed[i], e->d_name)) found = 1;
          if (!found) { snprintf(err, errn, "INTERFACE_ERROR snapshot-extra-file %s", e->d_name); closedir(d); return 1; }
      }
      closedir(d); }
    /* the ledger is parsed once (rl_load) and its references are matched to the COMMIT list one by one */
    snprintf(path, sizeof path, "%s/input.bin", dir); f = fopen(path, "rb");
    if (!f) { snprintf(err, errn, "INTERFACE_ERROR snapshot-input-absent"); return 1; }
    if (fseek(f, 0, SEEK_END) || (r = (int)ftell(f)) < 0 || fseek(f, 0, SEEK_SET)) { fclose(f); snprintf(err, errn, "INTERFACE_ERROR snapshot-input-size"); return 1; }
    { size_t n = (size_t)r; unsigned char *ib = malloc(n ? n : 1); size_t rn;
      if (!ib) { fclose(f); snprintf(err, errn, "INTERFACE_ERROR snapshot-out-of-memory"); return 1; }
      rn = n ? fread(ib, 1, n, f) : 0; fclose(f);
      if (rn != n) { free(ib); snprintf(err, errn, "INTERFACE_ERROR snapshot-input-read"); return 1; }
      if (rl_load(dir, L, err, errn)) { free(ib); return 1; }
      if (rl_check_identity(L, upstream_stage_label, ib, (int)n, err, errn)) { free(ib); return 1; }
      free(ib); }
    for (i = 0; i < L->n; i++) {
        int j, found = -1;
        if (!L->rows[i].present) continue;
        for (j = 0; j < nl; j++) if (!strcmp(listed[j], L->rows[i].blob)) found = j;
        if (!rl_snap_blob_name_ok(L->rows[i].blob)) { snprintf(err, errn, "INTERFACE_ERROR snapshot-ledger-bad-blob-name %s", L->rows[i].blob); return 1; }
        if (found < 0) { snprintf(err, errn, "INTERFACE_ERROR snapshot-ledger-unlisted-blob %s", L->rows[i].blob); return 1; }
        if (++refs[found] > 1) { snprintf(err, errn, "INTERFACE_ERROR snapshot-ledger-duplicate-reference %s", L->rows[i].blob); return 1; }
        if (L->rows[i].kn == 21 && L->rows[i].key[0] == 0 && (!memcmp(L->rows[i].key + 1, callmake, 20) || !memcmp(L->rows[i].key + 1, callcall, 20))) {
            if (!observe) { snprintf(err, errn, "INTERFACE_ERROR snapshot-callable-address-not-portable (needs --observe-callables)"); return 1; }
        }
    }
    for (i = 0; i < nl; i++)
        if (!strcmp(listed[i], "input.bin") || !strcmp(listed[i], "resources.tsv")) continue;
        else if (refs[i] != 1) { snprintf(err, errn, "INTERFACE_ERROR snapshot-commit-unreferenced %s", listed[i]); return 1; }
    return 0;
}
#endif   /* _WIN32 */
#endif
