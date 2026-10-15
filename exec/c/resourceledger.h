/* resourceledger.h: the explicit resource ledger (S step 1).  Shared by the product (exec/c/run.c, strict mode) and the
   replay runner (tests/middleflowrunner.c): one parser, one lookup, one SHA-256.  No cx.h dependency.
   Rows (LF, TAB separated):  keyhex  presence(0|1)  blob  length  sha256
     presence 1 = PRESENT: blob file in the ledger directory, exactly `length` bytes, sha256 must match (0 bytes is EMPTY, still present)
     presence 0 = ABSENT:  blob "-", length 0, sha "-" (the ledger explicitly says the resource does not exist)
   A key missing from the ledger is MISSING (the caller fails by name), never ABSENT and never a disk fallback.
   Errors are returned as text; the caller prints it and fails (rc 2, empty stdout). */
#ifndef UNISA_RESOURCELEDGER_H
#define UNISA_RESOURCELEDGER_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/unisa_sha256.h"

typedef struct { unsigned char *key; int kn; int present; unsigned char *value; int vn; } RLRow;
typedef struct { RLRow *rows; int n; } RLLedger;

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
/* 0: loaded; otherwise err holds the named reason (INTERFACE_ERROR <reason> ...) */
static int rl_load(const char *dir, RLLedger *L, char *err, size_t errn) {
    char path[4096]; unsigned char *b; size_t n = 0, at = 0; int cap = 0;
    L->rows = 0; L->n = 0;
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
                    r->value = v; r->vn = (int)bl;
                }
            }
        }
        free(line);
    }
    free(b);
    return 0;
}
/* exact key lookup: the row, or NULL when the ledger does not list the key (MISSING for the caller) */
static const RLRow *rl_find(const RLLedger *L, const unsigned char *key, int kn) {
    int j;
    for (j = 0; j < L->n; j++) if (L->rows[j].kn == kn && !memcmp(L->rows[j].key, key, (size_t)kn)) return &L->rows[j];
    return 0;
}
#endif
