#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "resourceledger.h"
/* snapfrom LEDGER_DIR SNAP_DIR UPSTREAM_LABEL INPUT_FILE SKIP_INDEX(-1 keeps all PRESENT)
   Builds the RI overlay from the sealed ledger's PRESENT rows through the shared writer (rl_snapshot_write). */
int main(int argc, char **argv) {
    RLLedger L; char err[512]; int i, nb = 0, skip, k = 0;
    static RLBlob b[RL_SNAP_MAX]; unsigned char *in; size_t n = 0; FILE *f; long sz;
    if (argc != 6) { printf("usage: snapfrom LEDGER_DIR SNAP_DIR UPSTREAM_LABEL INPUT_FILE SKIP_INDEX\n"); return 2; }
    skip = atoi(argv[5]);
    if (rl_load(argv[1], &L, err, sizeof err)) { printf("load: %s\n", err); return 1; }
    f = fopen(argv[4], "rb"); if (!f) { printf("input: absent\n"); return 2; }
    fseek(f, 0, SEEK_END); sz = ftell(f); fseek(f, 0, SEEK_SET);
    if (sz < 0 || sz > (1 << 20)) { fclose(f); printf("input: too large or unreadable\n"); return 2; }
    in = malloc(sz ? (size_t)sz : 1); n = sz ? fread(in, 1, (size_t)sz, f) : 0; fclose(f);
    if (n != (size_t)sz) { printf("input: short read\n"); return 2; }
    for (i = 0; i < L.n; i++) {
        if (!L.rows[i].present) continue;
        if (k++ == skip) continue;                     /* the deliberately dropped PRESENT key */
        if (nb >= RL_SNAP_MAX) { printf("too many PRESENT rows\n"); return 1; }
        b[nb].key = L.rows[i].key; b[nb].kn = L.rows[i].kn; b[nb].data = L.rows[i].value; b[nb].len = L.rows[i].vn; nb++;
    }
    if (rl_snapshot_write(argv[2], argv[3], in, (int)n, b, nb, err, sizeof err)) { printf("write: %s\n", err); return 1; }
    printf("snapshot: %d PRESENT blobs (of %d rows)\n", nb, L.n); return 0;
}
