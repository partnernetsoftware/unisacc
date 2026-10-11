/* seed/unisa_ape.c -- C99 reference for unisa.ape (K6-16), build() point samples.
 *
 *   unisa-ape build MODE
 *       MODE in plain payload-p1 payload-p2 neg-name-only neg-bad-payload
 *       prints the bytes build() returns as one lowercase hex line (nothing is written to disk).
 *
 * Mirrors unisa/ape.py build() with product_name/product_version absent and compile_target a FIXTURE
 * (a stub that returns fixed bytes per target; see the fixture notes below).  Covered: the script text
 * (_script, _pad, _stub), the two-pass layout with the 8-byte PE alignment rule, the 16-byte slice
 * alignment, the gzip members of the four Unix slices (gzip.compress mtime=0: header 1f8b 08 00 00000000
 * 02 ff, raw deflate level 9 memLevel 8 via zlib, CRC32 and ISIZE trailer), SHA-256 keys of the compressed
 * slices, and the optional model payload with its UNIPKG1 trailer.
 *
 * Fixture compile_target (NOT a product emitter): for win/x86_64 it returns the DOS part exactly as
 * pe.write() splits a stub -- "MZ", stub[:62] padded to 62 bytes, e_lfanew = len(stub)+2 at 0x3C,
 * stub[62:], then a fixed "PE\0\0" and 16 zero bytes; for the four Unix targets it returns fixed bytes
 * of target-specific length.  The real PE image, version resources, and product compile_target are NOT
 * covered.  Refused (python: ValueError): product name without version, and a payload that is not a
 * model package.  Point-sample equivalence over the listed modes only; not a product closure; does not
 * claim loading, execution, or a multi-platform product build.  Build with -lz.
 */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

static void die(const char *why, const char *what);

typedef struct { unsigned char *p; size_t n, cap; } Buf;
static void badd(Buf *b, const void *p, size_t k) {
    if (b->n + k > b->cap) {
        size_t nc = b->cap ? b->cap * 2 : 4096;
        while (nc < b->n + k) nc *= 2;
        b->p = realloc(b->p, nc); if (!b->p) die("out of memory", NULL); b->cap = nc;
    }
    if (k) memcpy(b->p + b->n, p, k);
    b->n += k;
}
static void bzero(Buf *b, size_t k) { while (k--) { unsigned char z = 0; badd(b, &z, 1); } }
static void le(Buf *b, uint64_t v, int n) { unsigned char e[8]; int k; for (k = 0; k < n; k++) e[k] = (unsigned char)(v >> (8 * k)); badd(b, e, (size_t)n); }
static void bs(Buf *b, const char *s) { badd(b, s, strlen(s)); }
static void die(const char *why, const char *what) {
    fprintf(stderr, "unisa-ape: %s %s\n", why, what ? what : "");
    exit(1);
}
static size_t r16(size_t v) { return (v + 15) / 16 * 16; }

static const uint32_t SK[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};
static uint32_t rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }
static void sha256(const unsigned char *msg, size_t len, unsigned char out[32]) {
    uint32_t h[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    size_t total = ((len + 9 + 63) / 64) * 64, off, i;
    unsigned char *m = calloc(total, 1); uint64_t bits = (uint64_t)len * 8;
    if (!m) die("out of memory", NULL);
    memcpy(m, msg, len); m[len] = 0x80;
    for (i = 0; i < 8; i++) m[total - 1 - i] = (unsigned char)(bits >> (8 * i));
    for (off = 0; off < total; off += 64) {
        uint32_t w[64], a, b, c, d, e, f, g, hh; int t;
        for (t = 0; t < 16; t++) w[t] = ((uint32_t)m[off + 4*t] << 24) | ((uint32_t)m[off + 4*t + 1] << 16) | ((uint32_t)m[off + 4*t + 2] << 8) | m[off + 4*t + 3];
        for (t = 16; t < 64; t++) {
            uint32_t s0 = rotr(w[t-15], 7) ^ rotr(w[t-15], 18) ^ (w[t-15] >> 3);
            uint32_t s1 = rotr(w[t-2], 17) ^ rotr(w[t-2], 19) ^ (w[t-2] >> 10);
            w[t] = w[t-16] + s0 + w[t-7] + s1;
        }
        a = h[0]; b = h[1]; c = h[2]; d = h[3]; e = h[4]; f = h[5]; g = h[6]; hh = h[7];
        for (t = 0; t < 64; t++) {
            uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            uint32_t ch = (e & f) ^ (~e & g);
            uint32_t t1 = hh + S1 + ch + SK[t] + w[t];
            uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            uint32_t mj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t t2 = S0 + mj;
            hh = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    }
    for (i = 0; i < 8; i++) {
        out[4*i] = (unsigned char)(h[i] >> 24); out[4*i+1] = (unsigned char)(h[i] >> 16);
        out[4*i+2] = (unsigned char)(h[i] >> 8); out[4*i+3] = (unsigned char)h[i];
    }
    free(m);
}


/* gzip.compress(data, mtime=0): python's header for level 9 (xfl 2, OS 255), raw deflate, trailer */
static void gzip_member(Buf *o, const unsigned char *in, size_t n) {
    z_stream zs; unsigned char *tmp; size_t cap = n + n / 2 + 1024; int rc; uint32_t crc;
    memset(&zs, 0, sizeof zs);
    if (deflateInit2(&zs, 9, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY) != Z_OK) die("deflateInit2", NULL);
    tmp = malloc(cap); if (!tmp) die("out of memory", NULL);
    zs.next_in = (unsigned char *)in; zs.avail_in = (uInt)n;
    zs.next_out = tmp; zs.avail_out = (uInt)cap;
    rc = deflate(&zs, Z_FINISH);
    if (rc != Z_STREAM_END) die("deflate did not finish", NULL);
    { static const unsigned char gh[4] = {0x1f, 0x8b, 0x08, 0x00}; badd(o, gh, 4); }
    bzero(o, 4); bs(o, "\x02\xff");
    badd(o, tmp, zs.total_out);
    deflateEnd(&zs);
    free(tmp);
    crc = (uint32_t)crc32(0L, in, (uInt)n);
    le(o, crc, 4); le(o, (uint64_t)n, 4);
}

/* fixture compile_target for the four Unix slices: fixed bytes, target-specific length */
static void unix_image(Buf *o, const char *target) {
    size_t n = 0, i; int salt = 0;
    if (!strcmp(target, "lnx/x86_64")) { n = 300; salt = 1; }
    else if (!strcmp(target, "lnx/arm64")) { n = 517; salt = 2; }
    else if (!strcmp(target, "osx/x86_64")) { n = 1000; salt = 3; }
    else if (!strcmp(target, "osx/arm64")) { n = 1500; salt = 4; }
    else die("unknown target (python: KeyError)", target);
    for (i = 0; i < n; i++) { unsigned char c = (unsigned char)((i * 31 + salt * 7) & 255); badd(o, &c, 1); }
}

/* fixture win/x86_64 DOS head: exactly pe.write's stub split, with a fixed PE body */
static void win_head(Buf *o, const unsigned char *stub, size_t sn) {
    unsigned char dos[64]; size_t first = sn < 62 ? sn : 62, rest = sn > 62 ? sn - 62 : 0;
    memset(dos, 0, sizeof dos); dos[0] = 'M'; dos[1] = 'Z';
    if (sn) memcpy(dos + 2, stub, first);
    dos[0x3C] = (unsigned char)((sn + 2) & 255); dos[0x3D] = (unsigned char)(((sn + 2) >> 8) & 255);
    dos[0x3E] = (unsigned char)(((sn + 2) >> 16) & 255); dos[0x3F] = (unsigned char)(((sn + 2) >> 24) & 255);
    badd(o, dos, 64);
    if (rest) badd(o, stub + 62, rest);
    { static const unsigned char pe[4] = {'P', 'E', 0, 0}; badd(o, pe, 4); }
    bzero(o, 16);
}

/* _script(table, embedded): table rows are (name, off, len, key16) with name "OS|ARCH" */
typedef struct { const char *name; unsigned long long off, len; char key[17]; } Row;
static void script(Buf *o, const Row *rows, int nrows, int embedded) {
    int i;
    bs(o, "u=$(uname -s)$(uname -m)\n");
    bs(o, "case \"$u\" in\n");
    for (i = 0; i < nrows; i++) {
        const char *bar = strchr(rows[i].name, '|');
        size_t ol = (size_t)(bar - rows[i].name);
        char os[16], arch[16], pat[64], line[256];
        memcpy(os, rows[i].name, ol); os[ol] = 0;
        strcpy(arch, bar + 1);
        if (!strcmp(arch, "aarch64")) snprintf(pat, sizeof pat, "%saarch64|%sarm64", os, os);
        else snprintf(pat, sizeof pat, "%s%s", os, arch);
        snprintf(line, sizeof line, "%s%s) o=%llu n=%llu k=%s;;\n", i ? "  " : "  ", pat, rows[i].off, rows[i].len, rows[i].key);
        bs(o, line);
        if (i) { /* the python join puts the two spaces before the second and later cases */ }
    }
    bs(o, "  *) echo \"unisacc: no slice for $u\" >&2; exit 1;;\n");
    bs(o, "esac\n");
    bs(o, "d=\"${XDG_CACHE_HOME:-$HOME/.cache}/unisacc\"\n");
    bs(o, "[ -n \"$HOME\" ] && mkdir -p \"$d\" 2>/dev/null || d=\"${TMPDIR:-/tmp}\"\n");
    bs(o, "t=\"$d/unisacc-$k\"\n");
    bs(o, "if [ ! -x \"$t\" ]; then\n");
    bs(o, "  tail -c +$o \"$0\" | head -c $n | gzip -dc > \"$t.$$\" && chmod +x \"$t.$$\" && mv -f \"$t.$$\" \"$t\" || { rm -f \"$t.$$\"; exit 1; }\n");
    bs(o, "fi\n");
    if (embedded) bs(o, "UNISA_CONTAINER=\"$0\"; export UNISA_CONTAINER\n");
    bs(o, "exec \"$t\" \"$@\"\n");
}

static void stub_of(Buf *o, const Buf *s) {   /* _stub(script) */
    bs(o, "qFpD='\n");
    { size_t k; for (k = 0; k < 51; k++) bs(o, "."); }
    bs(o, "....'\n");
    badd(o, s->p ? s->p : (const unsigned char *)"", s->n);
}
/* _pad(script, want): comment padding up to want bytes; python asserts room >= 2 */
static void pad_to_want(Buf *o, const Buf *s, size_t want) {
    size_t room;
    if (s->n + 2 > want) die("script too long for the padding (python: assert)", NULL);
    room = want - s->n;
    badd(o, s->p ? s->p : (const unsigned char *)"", s->n);
    bs(o, "#");
    { size_t k; for (k = 0; k < room - 2; k++) bs(o, "."); }
    bs(o, "\n");
}

static const char *SLICE_NAME[4] = {"Linux|x86_64", "Linux|aarch64", "Darwin|x86_64", "Darwin|arm64"};
static const char *SLICE_TGT[4] = {"lnx/x86_64", "lnx/arm64", "osx/x86_64", "osx/arm64"};

/* build(compile_target, out, payload): returns the blob; `payload` is the model package or empty */
static void build(Buf *blob, const Buf *payload) {
    Buf imgs[4] = {{0}}, scr = {0}, stub_b = {0}, tmp = {0}, head = {0}, head2 = {0}; Row guess[4], table[4];
    size_t i, want, base, off, room;
    int embedded = payload->n > 0;
    if (embedded && !(payload->n >= 4 && (!memcmp(payload->p, "P 1 ", 4) || !memcmp(payload->p, "P 2 ", 4) || !memcmp(payload->p, "P 3 ", 4))))
        die("embedded payload must be a model package (python: ValueError)", NULL);
    for (i = 0; i < 4; i++) {
        Buf raw = {0};
        unix_image(&raw, SLICE_TGT[i]);
        gzip_member(&imgs[i], raw.p, raw.n);
        free(raw.p);
    }
    /* pass 1: plausible numbers, padded to a fixed length */
    for (i = 0; i < 4; i++) { guess[i].name = SLICE_NAME[i]; guess[i].off = 1ull << 30; guess[i].len = imgs[i].n; memcpy(guess[i].key, "0000000000000000", 17); }
    { Buf s = {0}; script(&s, guess, 4, embedded); scr = s; }
    want = scr.n + 40;
    { size_t x; Buf e = {0}; stub_of(&e, &(Buf){0}); x = 2 + e.n + want; want += (8 - x % 8) % 8; free(e.p); }
    { Buf sp = {0}, sc = {0}; script(&sc, guess, 4, embedded); pad_to_want(&sp, &sc, want); stub_of(&stub_b, &sp); }
    win_head(&head, stub_b.p, stub_b.n);
    base = r16(head.n);
    off = base;
    for (i = 0; i < 4; i++) {
        unsigned char dig[32]; char hx[65]; size_t k;
        sha256(imgs[i].p, imgs[i].n, dig);
        for (k = 0; k < 8; k++) snprintf(hx + 2 * k, 3, "%02x", dig[k]);
        table[i].name = SLICE_NAME[i]; table[i].off = off + 1; table[i].len = imgs[i].n;
        memcpy(table[i].key, hx, 16); table[i].key[16] = 0;
        off += r16(imgs[i].n);
    }
    { Buf sc = {0}, sp = {0}, st = {0}; script(&sc, table, 4, embedded); pad_to_want(&sp, &sc, want); stub_of(&st, &sp); tmp = st; }
    win_head(&head2, tmp.p, tmp.n);
    if (head2.n != head.n) die("head length changed between passes", NULL);
    {   size_t peoff = (size_t)tmp.n + 2;
        if (peoff % 8 != 0) die("PE signature is not 8-aligned", NULL);
        if (memcmp(head2.p + peoff, "PE\0\0", 4)) die("PE signature not at e_lfanew", NULL);
    }
    badd(blob, head2.p, head2.n);
    bzero(blob, base - head2.n);
    room = 0;
    for (i = 0; i < 4; i++) {
        badd(blob, imgs[i].p, imgs[i].n);
        bzero(blob, (16 - imgs[i].n % 16) % 16);
    }
    if (embedded) {
        badd(blob, payload->p, payload->n);
        bs(blob, "UNIPKG1\n");
        le(blob, payload->n, 8);
    }
    (void)room;
}

int main(int argc, char **argv) {
    Buf payload = {0}, blob = {0}; int refuse = 0;
    size_t k;
    if (argc != 3 || strcmp(argv[1], "build")) { fprintf(stderr, "usage: unisa-ape build MODE\n"); return 2; }
    if (!strcmp(argv[2], "plain")) { }
    else if (!strcmp(argv[2], "payload-p1")) bs(&payload, "P 1 demo\n");
    else if (!strcmp(argv[2], "payload-p2")) bs(&payload, "P 2 xyz\n");
    else if (!strcmp(argv[2], "neg-name-only")) refuse = 1;
    else if (!strcmp(argv[2], "neg-bad-payload")) bs(&payload, "hello");
    else { fprintf(stderr, "unknown mode\n"); return 2; }
    if (refuse) die("product name and version must be specified together (python: ValueError)", NULL);
    build(&blob, &payload);
    for (k = 0; k < blob.n; k++) printf("%02x", blob.p[k]);
    printf("\n");
    return 0;
}
