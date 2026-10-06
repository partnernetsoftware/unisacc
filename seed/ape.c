/* seed/ape.c -- the APE container without Python (0.0.30 S1).
 *
 *   ape OUT WIN_X86_64 LNX_X86_64 LNX_ARM64 OSX_X86_64 OSX_ARM64
 *
 * Same layout as unisa/ape.py: the win/x86_64 PE at offset 0 with the shell
 * script in its DOS stub, then the four Unix slices gzipped, 16-aligned.  The
 * gzip members use stored deflate blocks (no compressor in C99 here); `gzip -dc`
 * reads them all the same.  The PE comes from `unisacc -b win/x86_64` with its
 * headers at 64..1024; this moves the PE header past the script and shifts the
 * section file offsets, as unisa/image/pe.py does when it is given a stub.
 * C99, no library beyond stdio/stdlib/string. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { unsigned char *p; long n, cap; } Buf;
static void put(Buf *b, const void *s, long n) {
    if (b->n + n > b->cap) { b->cap = (b->n + n) * 2 + 64; b->p = realloc(b->p, b->cap); if (!b->p) { perror("ape"); exit(1); } }
    memcpy(b->p + b->n, s, n); b->n += n;
}
static void putc1(Buf *b, int c) { unsigned char u = (unsigned char)c; put(b, &u, 1); }
static void put32(Buf *b, unsigned long v) { int i; for (i = 0; i < 4; i++) putc1(b, (int)(v >> 8 * i) & 255); }
static Buf slurp(const char *f) {
    Buf b = {0, 0, 0}; unsigned char tmp[65536]; size_t k; FILE *fp = fopen(f, "rb");
    if (!fp) { perror(f); exit(1); }
    while ((k = fread(tmp, 1, sizeof tmp, fp)) > 0) put(&b, tmp, (long)k);
    fclose(fp); return b;
}
static unsigned long crc32(const unsigned char *p, long n) {
    unsigned long c = 0xFFFFFFFFUL; long i; int k;
    for (i = 0; i < n; i++) { c ^= p[i]; for (k = 0; k < 8; k++) c = (c >> 1) ^ (0xEDB88320UL & (0UL - (c & 1))); }
    return c ^ 0xFFFFFFFFUL;
}
static Buf gz(Buf in) {          /* mtime 0, stored blocks */
    static const unsigned char hd[10] = {0x1f, 0x8b, 8, 0, 0, 0, 0, 0, 0, 255};
    Buf o = {0, 0, 0}; long at = 0;
    put(&o, hd, 10);
    do {
        long n = in.n - at > 65535 ? 65535 : in.n - at;
        putc1(&o, at + n == in.n); putc1(&o, (int)(n & 255)); putc1(&o, (int)(n >> 8));
        putc1(&o, (int)(~n & 255)); putc1(&o, (int)((~n >> 8) & 255));
        put(&o, in.p + at, n); at += n;
    } while (at < in.n);
    put32(&o, crc32(in.p, in.n)); put32(&o, (unsigned long)in.n);
    return o;
}
/* sha256, for the cache key (first 16 hex digits, as ape.py) */
static const unsigned long K[64] = {
0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,
0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,
0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,
0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
#define R32(x, n) ((((x) >> (n)) | ((x) << (32 - (n)))) & 0xFFFFFFFFUL)
static void sha256hex(const unsigned char *p, long n, char *out16) {
    unsigned long h[8] = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    long total = ((n + 9 + 63) / 64) * 64, blk; int i;
    for (blk = 0; blk < total; blk += 64) {
        unsigned long w[64], a, b, c, d, e, f, g, hh, t1, t2;
        for (i = 0; i < 64; i++) {
            long j = blk + i; unsigned long v;
            if (j < n) v = p[j]; else if (j == n) v = 0x80;
            else if (j >= total - 8) v = (unsigned long)((((unsigned long long)n * 8) >> (8 * (total - 1 - j))) & 255); else v = 0;
            if (i % 4 == 0) w[i / 4] = 0;
            w[i / 4] |= v << (8 * (3 - i % 4));
        }
        for (i = 16; i < 64; i++) {
            unsigned long s0 = R32(w[i-15], 7) ^ R32(w[i-15], 18) ^ (w[i-15] >> 3);
            unsigned long s1 = R32(w[i-2], 17) ^ R32(w[i-2], 19) ^ (w[i-2] >> 10);
            w[i] = (w[i-16] + s0 + w[i-7] + s1) & 0xFFFFFFFFUL;
        }
        a = h[0]; b = h[1]; c = h[2]; d = h[3]; e = h[4]; f = h[5]; g = h[6]; hh = h[7];
        for (i = 0; i < 64; i++) {
            t1 = (hh + (R32(e, 6) ^ R32(e, 11) ^ R32(e, 25)) + ((e & f) ^ (~e & g & 0xFFFFFFFFUL)) + K[i] + w[i]) & 0xFFFFFFFFUL;
            t2 = ((R32(a, 2) ^ R32(a, 13) ^ R32(a, 22)) + ((a & b) ^ (a & c) ^ (b & c))) & 0xFFFFFFFFUL;
            hh = g; g = f; f = e; e = (d + t1) & 0xFFFFFFFFUL; d = c; c = b; b = a; a = (t1 + t2) & 0xFFFFFFFFUL;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
        for (i = 0; i < 8; i++) h[i] &= 0xFFFFFFFFUL;
    }
    sprintf(out16, "%08lx%08lx", h[0], h[1]);
}

static const char *NAME[4] = {"Linux|x86_64", "Linux|aarch64", "Darwin|x86_64", "Darwin|arm64"};
static void script(Buf *s, long off[4], long len[4], char key[4][17]) {
    char line[256]; int i;
    s->n = 0;
    put(s, "u=$(uname -s)$(uname -m)\ncase \"$u\" in\n", 38);
    for (i = 0; i < 4; i++) {
        const char *bar = strchr(NAME[i], '|'); char o[16], a[16];
        memcpy(o, NAME[i], (size_t)(bar - NAME[i])); o[bar - NAME[i]] = 0; strcpy(a, bar + 1);
        if (!strcmp(a, "aarch64")) sprintf(line, "  %saarch64|%sarm64) o=%ld n=%ld k=%s;;\n", o, o, off[i], len[i], key[i]);
        else sprintf(line, "  %s%s) o=%ld n=%ld k=%s;;\n", o, a, off[i], len[i], key[i]);
        put(s, line, (long)strlen(line));
    }
    {
        static const char *rest =
            "  *) echo \"unisacc: no slice for $u\" >&2; exit 1;;\n"
            "esac\n"
            "d=\"${XDG_CACHE_HOME:-$HOME/.cache}/unisacc\"\n"
            "[ -n \"$HOME\" ] && mkdir -p \"$d\" 2>/dev/null || d=\"${TMPDIR:-/tmp}\"\n"
            "t=\"$d/unisacc-$k\"\n"
            "if [ ! -x \"$t\" ]; then\n"
            "  tail -c +$o \"$0\" | head -c $n | gzip -dc > \"$t.$$\" && chmod +x \"$t.$$\" && mv -f \"$t.$$\" \"$t\" || { rm -f \"$t.$$\"; exit 1; }\n"
            "fi\n"
            "exec \"$t\" \"$@\"\n";
        put(s, rest, (long)strlen(rest));
    }
}
static unsigned long rd32(const unsigned char *p) { return p[0] | p[1] << 8 | (unsigned long)p[2] << 16 | (unsigned long)p[3] << 24; }
static void wr32(unsigned char *p, unsigned long v) { int i; for (i = 0; i < 4; i++) p[i] = (unsigned char)(v >> 8 * i); }

/* the PE with `stub` (= "qFpD='\n...'\n" + script) in its DOS header */
static Buf pe_with_stub(Buf pe, Buf stub) {
    Buf o = {0, 0, 0}; unsigned long peoff = rd32(pe.p + 0x3C), nsect, opt, hdr, i, delta;
    unsigned char dos[64];
    if (pe.n < 1024 || pe.p[0] != 'M' || pe.p[1] != 'Z' || peoff != 64 || memcmp(pe.p + 64, "PE\0\0", 4)) { fprintf(stderr, "ape: win slice is not a plain unisacc PE\n"); exit(1); }
    nsect = pe.p[64 + 6] | pe.p[64 + 7] << 8; opt = pe.p[64 + 20] | pe.p[64 + 21] << 8;
    hdr = 4 + 20 + opt + 40 * nsect;
    if (rd32(pe.p + 64 + 24 + 60) != 1024) { fprintf(stderr, "ape: unexpected SizeOfHeaders\n"); exit(1); }
    {   unsigned long want = 488 + (unsigned long)stub.n;
        unsigned long hf = (want + 511) / 512 * 512; if (hf < 1024) hf = 1024; delta = hf - 1024; }
    memset(dos, 0, 64); dos[0] = 'M'; dos[1] = 'Z';
    memcpy(dos + 2, stub.p, 62); wr32(dos + 0x3C, (unsigned long)stub.n + 2);
    put(&o, dos, 64); put(&o, stub.p + 62, stub.n - 62);
    {   long ph = o.n; put(&o, pe.p + 64, (long)hdr);
        wr32(o.p + ph + 24 + 60, 1024 + delta);
        for (i = 0; i < nsect; i++) {
            unsigned char *s = o.p + ph + 24 + opt + 40 * i;
            if (rd32(s + 20)) wr32(s + 20, rd32(s + 20) + delta);
        } }
    while (o.n < (long)(1024 + delta)) putc1(&o, 0);
    put(&o, pe.p + 1024, pe.n - 1024);
    return o;
}
static void mkstub(Buf *st, Buf *sc, long want) {
    static const char dots[] = "...................................................";  /* 51 */
    st->n = 0; put(st, "qFpD='\n", 7); put(st, dots, 51); put(st, "....'\n", 6);
    put(st, sc->p, sc->n);
    if (want) {   /* pad the script back to a fixed length with a comment */
        long room = want - sc->n; if (room < 2) { fprintf(stderr, "ape: script grew\n"); exit(1); }
        putc1(st, '#'); while (room-- > 2) putc1(st, '.'); putc1(st, '\n');
    }
}
int main(int argc, char **argv) {
    Buf win, img[4], sc = {0, 0, 0}, st = {0, 0, 0}, head, out = {0, 0, 0};
    long off[4], len[4], want, base, at; char key[4][17]; int i; FILE *fp;
    if (argc != 7) { fprintf(stderr, "usage: ape OUT WIN_X86_64 LNX_X86_64 LNX_ARM64 OSX_X86_64 OSX_ARM64\n"); return 2; }
    win = slurp(argv[2]);
    for (i = 0; i < 4; i++) { Buf raw = slurp(argv[3 + i]); img[i] = gz(raw); free(raw.p); len[i] = img[i].n; off[i] = 1L << 30; strcpy(key[i], "0000000000000000"); }
    script(&sc, off, len, key);
    want = sc.n + 40;
    { long n0 = 2 + 64; want += (8 - (n0 + want) % 8) % 8; }   /* PE signature 8-aligned */
    mkstub(&st, &sc, want);
    head = pe_with_stub(win, st);
    base = (head.n + 15) / 16 * 16; at = base;
    for (i = 0; i < 4; i++) { off[i] = at + 1; sha256hex(img[i].p, img[i].n, key[i]); at += (img[i].n + 15) / 16 * 16; }
    script(&sc, off, len, key); mkstub(&st, &sc, want);
    free(head.p); head = pe_with_stub(win, st);
    if ((head.n + 15) / 16 * 16 != base) { fprintf(stderr, "ape: header length moved\n"); return 1; }
    if (rd32(head.p + 0x3C) % 8) { fprintf(stderr, "ape: PE offset not 8-aligned\n"); return 1; }
    put(&out, head.p, head.n); while (out.n < base) putc1(&out, 0);
    for (i = 0; i < 4; i++) { put(&out, img[i].p, img[i].n); while (out.n % 16) putc1(&out, 0); }
    fp = fopen(argv[1], "wb"); if (!fp || fwrite(out.p, 1, (size_t)out.n, fp) != (size_t)out.n || fclose(fp)) { perror(argv[1]); return 1; }
    printf("%s  %ld B\n", argv[1], out.n);
    return 0;
}
