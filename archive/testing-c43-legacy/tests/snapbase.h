/* snapbase: frozen-snapshot baseline shared by lib-latebind-W (tests/liblatebindWcheck.cx) and latebind-p6.
   File = '#' provenance lines, then the magic "WBASE1\n\0", then one segment per caller-named artifact: LE32 length
   and bytes, in order.  The reader parses the whole file first (nothing is written on a bad file), demands the
   caller's provenance strings and an exact segment count, and writes each segment to dir/<name>.  The checker compares
   each segment byte for byte with dir/<name>.  Host: native Linux x86_64 only; elsewhere UNVERIFIED (caller exits 77). */
#include <cx.h>
typedef struct { const unsigned char *p; size_t n; } snapbase_seg;
static int snapbase_host(const char *label){
    char *platform = 0; size_t n = 0; int rc = cx_run_timeout("uname -sm", 5, &platform, &n);
    int host = !rc && platform && strstr(platform, "Linux") && strstr(platform, "x86_64"); free(platform);
    if (!host) printf("UNVERIFIED_HOST %s baseline is native Linux x86_64\n", label);
    return host;
}
/* returns the file buffer (caller frees) with seg[0..count-1] set, or NULL on any malformation */
static unsigned char *snapbase_load(const char *path, const char *const *need, int nneed, int count, snapbase_seg *seg, int empty_ok){
    size_t n = 0, at = 0, z; int i, k;
    unsigned char *b = (unsigned char *)cx_read(path, &n);
    if (!b) return 0;
    while (at < n && b[at] == '#') { while (at < n && b[at] != '\n') at++; if (at == n) { free(b); return 0; } at++; }
    if (at > 4096 || n - at < 8 || memcmp(b + at, "WBASE1\n\0", 8)) { free(b); return 0; }
    for (k = 0; k < nneed; k++) if (!strstr((char *)b, need[k])) { free(b); return 0; }
    at += 8;
    for (i = 0; i < count; i++) {
        if (n - at < 4) { free(b); return 0; }
        z = (size_t)b[at] | ((size_t)b[at + 1] << 8) | ((size_t)b[at + 2] << 16) | ((size_t)b[at + 3] << 24); at += 4;
        if ((!z && !empty_ok) || z > n - at) { free(b); return 0; }
        seg[i].p = b + at; seg[i].n = z; at += z;
    }
    if (at != n) { free(b); return 0; }
    return b;
}
/* empty_ok: 0 rejects zero-length segments (lib-latebind-W); 1 accepts them (P6 has empty fragments) */
/* writes every segment to dir/<names[i]>; 0 on success, 1 when the baseline is malformed (nothing written) */
static int snapbase_read(const char *path, const char *const *need, int nneed, const char *const *names, int count, const char *dir, int empty_ok){
    snapbase_seg seg[64]; char out[4096]; int i; unsigned char *b;
    if (count > 64) return 1;
    b = snapbase_load(path, need, nneed, count, seg, empty_ok);
    if (!b) return 1;
    for (i = 0; i < count; i++) {
        snprintf(out, sizeof out, "%s/%s", dir, names[i]);
        if (cx_write(out, seg[i].p, seg[i].n)) { free(b); return 1; }
    }
    free(b); return 0;
}
/* 0: every segment equals dir/<names[i]>; 1: first mismatch (or missing artifact) in *first_bad; 2: baseline malformed */
static int snapbase_check(const char *path, const char *const *need, int nneed, const char *const *names, int count, const char *dir, const char **first_bad, int empty_ok){
    snapbase_seg seg[64]; char p[4096]; int i; unsigned char *b;
    size_t m = 0; char *x; int ok;
    if (count > 64) return 2;
    b = snapbase_load(path, need, nneed, count, seg, empty_ok);
    if (!b) return 2;
    for (i = 0; i < count; i++) {
        snprintf(p, sizeof p, "%s/%s", dir, names[i]);
        x = cx_read(p, &m); ok = x && m == seg[i].n && !memcmp(x, seg[i].p, m); free(x);
        if (!ok) { if (first_bad) *first_bad = names[i]; free(b); return 1; }
    }
    free(b); return 0;
}
