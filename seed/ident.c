/* seed/ident.c -- the product source identity, in C99 + POSIX (0.0.32 B5).
 *
 * Prints what `exec/c/provenance.py identity` prints: SHA-256 over the
 * models.closure() file set (exec unisa src kernel include weights seed,
 * filtered the same way, plus iterate/kernel/typekw.tsv), followed by
 * the .S files in exec/c/asm.  Each file contributes its POSIX path, a NUL, and the raw
 * 32-byte SHA-256 of its contents.  Paths sort by their parts, byte-wise
 * (Python's sorted(key=parts)), which is plain byte order with '/' lowest.
 *
 * Usage: ident [ROOT]   (default: the current directory)
 */
#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

typedef struct { uint32_t s[8]; uint64_t n; unsigned char b[64]; size_t k; } Sha;
static const uint32_t K[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
#define ROR(x, r) (((x) >> (r)) | ((x) << (32 - (r))))
static void sha_block(Sha *h, const unsigned char *p) {
    uint32_t w[64], a, b, c, d, e, f, g, x, t1, t2; int i;
    for (i = 0; i < 16; i++)
        w[i] = (uint32_t)p[4*i] << 24 | (uint32_t)p[4*i+1] << 16 | (uint32_t)p[4*i+2] << 8 | p[4*i+3];
    for (i = 16; i < 64; i++)
        w[i] = w[i-16] + (ROR(w[i-15], 7) ^ ROR(w[i-15], 18) ^ (w[i-15] >> 3)) + w[i-7]
             + (ROR(w[i-2], 17) ^ ROR(w[i-2], 19) ^ (w[i-2] >> 10));
    a = h->s[0]; b = h->s[1]; c = h->s[2]; d = h->s[3]; e = h->s[4]; f = h->s[5]; g = h->s[6]; x = h->s[7];
    for (i = 0; i < 64; i++) {
        t1 = x + (ROR(e, 6) ^ ROR(e, 11) ^ ROR(e, 25)) + ((e & f) ^ (~e & g)) + K[i] + w[i];
        t2 = (ROR(a, 2) ^ ROR(a, 13) ^ ROR(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
        x = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
    }
    h->s[0] += a; h->s[1] += b; h->s[2] += c; h->s[3] += d; h->s[4] += e; h->s[5] += f; h->s[6] += g; h->s[7] += x;
}
static void sha_init(Sha *h) {
    static const uint32_t iv[8] = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    memcpy(h->s, iv, sizeof iv); h->n = 0; h->k = 0;
}
static void sha_put(Sha *h, const void *data, size_t n) {
    const unsigned char *p = data;
    h->n += n;
    while (n) {
        size_t m = 64 - h->k; if (m > n) m = n;
        memcpy(h->b + h->k, p, m); h->k += m; p += m; n -= m;
        if (h->k == 64) { sha_block(h, h->b); h->k = 0; }
    }
}
static void sha_end(Sha *h, unsigned char out[32]) {
    uint64_t bits = h->n * 8; unsigned char pad = 0x80, z = 0, len[8]; int i;
    sha_put(h, &pad, 1);
    while (h->k != 56) sha_put(h, &z, 1);
    for (i = 0; i < 8; i++) len[i] = (unsigned char)(bits >> (56 - 8 * i));
    sha_put(h, len, 8);
    for (i = 0; i < 32; i++) out[i] = (unsigned char)(h->s[i / 4] >> (24 - 8 * (i % 4)));
}

static void die(const char *why, const char *what) { fprintf(stderr, "ident: %s: %s\n", why, what); exit(1); }
static char **paths; static size_t npaths, cap;
static void add(const char *p) {
    if (npaths == cap) { cap = cap ? cap * 2 : 1024; paths = realloc(paths, cap * sizeof *paths); if (!paths) die("out of memory", p); }
    paths[npaths] = malloc(strlen(p) + 1); if (!paths[npaths]) die("out of memory", p);
    strcpy(paths[npaths++], p);
}
static int part_cmp(const void *x, const void *y) {
    const unsigned char *a = *(unsigned char *const *)x, *b = *(unsigned char *const *)y;
    for (;; a++, b++) {
        int ca = *a == '/' ? 1 : *a ? *a + 1 : 0, cb = *b == '/' ? 1 : *b ? *b + 1 : 0;
        if (ca != cb) return ca - cb;
        if (!ca) return 0;
    }
}
static const char *suffix(const char *p) {
    const char *base = strrchr(p, '/'), *dot; base = base ? base + 1 : p;
    dot = strrchr(base, '.'); return dot && dot != base ? dot : "";
}
static int keep(const char *rel) {
    static const char *const exts[] = {".py", ".c", ".h", ".inc", ".tsv", ".json", ".sh"};
    const char *sfx = suffix(rel); size_t i; int ok = 0;
    for (i = 0; i < sizeof exts / sizeof *exts; i++) if (!strcmp(sfx, exts[i])) ok = 1;
    if (!ok) return 0;
    if (!strncmp(rel, "seed/", 5)) {
        const char *base = strrchr(rel, '/') + 1;
        return !strcmp(base, "tbl.c") || !strcmp(base, "net.c") || !strcmp(base, "json.h")
            || !strcmp(base, "gen.c") || !strcmp(base, "facts.h") || !strcmp(base, "ident.c");
    }
    if (!strncmp(rel, "exec/build/", 11))
        return !strchr(rel + 11, '/') && !strcmp(sfx, ".py");
    return 1;
}
/* rglob('*') without following directory symlinks; is_file() follows file symlinks */
static void walk(const char *rel) {
    DIR *d = opendir(rel); struct dirent *e;
    if (!d) die("cannot open directory", rel);
    while ((e = readdir(d))) {
        char *p; struct stat ls, st;
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        p = malloc(strlen(rel) + strlen(e->d_name) + 2); if (!p) die("out of memory", rel);
        sprintf(p, "%s/%s", rel, e->d_name);
        if (lstat(p, &ls)) die("cannot stat", p);
        if (S_ISDIR(ls.st_mode)) walk(p);
        else if (!stat(p, &st) && S_ISREG(st.st_mode) && keep(p)) add(p);
        free(p);
    }
    closedir(d);
}
static void file_entry(Sha *h, const char *rel) {
    /* read(2), not stdio: the product's <stdio.h> refuses ferror() in a unit
       that never calls fgetc (plans/v0.0.32.md D3) */
    Sha f; unsigned char buf[1 << 16], dg[32]; long n; int fd = open(rel, O_RDONLY);
    if (fd < 0) die("cannot open", rel);
    sha_init(&f);
    while ((n = read(fd, buf, sizeof buf)) > 0) sha_put(&f, buf, (size_t)n);
    if (n < 0) die("read failed", rel);
    close(fd);
    sha_end(&f, dg);
    sha_put(h, rel, strlen(rel) + 1);
    sha_put(h, dg, 32);
}
int main(int argc, char **argv) {
    static const char *const dirs[] = {"exec", "unisa", "src", "kernel", "include", "weights", "seed"};
    Sha h; unsigned char out[32]; size_t i, first; int j;
    if (argc > 2) die("usage", "ident [ROOT]");
    if (argc == 2 && chdir(argv[1])) die("cannot enter", argv[1]);
    sha_init(&h);
    for (j = 0; j < 7; j++) {
        first = npaths; walk(dirs[j]);
        qsort(paths + first, npaths - first, sizeof *paths, part_cmp);
    }
    for (i = 0; i < npaths; i++) file_entry(&h, paths[i]);
    file_entry(&h, "iterate/kernel/typekw.tsv");
    first = npaths;
    {   DIR *d = opendir("exec/c/asm"); struct dirent *e;
        if (!d) die("cannot open directory", "exec/c/asm");
        while ((e = readdir(d))) {
            size_t n = strlen(e->d_name); char p[1024];
            if (n > 2 && !strcmp(e->d_name + n - 2, ".S") && n < sizeof p - 12) {
                sprintf(p, "exec/c/asm/%s", e->d_name); add(p);
            }
        }
        closedir(d);
    }
    qsort(paths + first, npaths - first, sizeof *paths, part_cmp);
    for (i = first; i < npaths; i++) file_entry(&h, paths[i]);
    sha_end(&h, out);
    for (i = 0; i < 32; i++) printf("%02x", out[i]);
    printf("\n");
    return 0;
}
