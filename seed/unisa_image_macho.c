/* seed/unisa_image_macho.c -- C99 reference for unisa.image.macho (K6-14), static write() point samples.
 *
 *   unisa-image-macho write SPEC
 *       SPEC lines (fields separated by blanks; blank lines skipped):
 *         arch ARCH                ARCH in x86_64 arm64
 *         text f HH N | text x HEX | text x -     the text image (fill / hex / empty)
 *         data f HH N | data x HEX | data x -     the data image
 *         entry N                  signed decimal; LC_MAIN entryoff = HDRS + N
 *         full N                   bss-extended size of __data (optional; default len(data))
 *       prints write(arch, text, data, entry, full) as one lowercase hex line.
 *
 * Mirrors unisa/image/macho.py write(): 64-bit Mach-O header; load commands __PAGEZERO, __TEXT
 * (__text), __DATA (__data, __bss), __LINKEDIT, LC_LOAD_DYLINKER, LC_LOAD_DYLIB, LC_MAIN,
 * LC_BUILD_VERSION, LC_DYLD_INFO_ONLY (bind of four dl symbols), LC_SYMTAB, LC_DYSYMTAB,
 * LC_CODE_SIGNATURE; SLACK; the image; the ad-hoc SuperBlob (CodeDirectory with SHA-256 over each 4 KB
 * page of out[0:sigoff], identifier "unisa").  Page sizes: PAGE 0x4000, PAGE4 0x1000.
 * Unknown arch, a bss size below zero (full < len(data)), and an entry whose address is not a valid
 * unsigned 64-bit value are refused (python: KeyError / struct.error).
 *
 * Point-sample equivalence over the listed specs only.  Writing the bytes is not a claim that the
 * image loads, runs, or is signed for a real kernel; no product closure; keeps unisa/image/macho.py
 * and unisa/bits.py.  Refusals print the reason on stderr and nothing on stdout.
 */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void die(const char *why, const char *what);

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

static void die(const char *why, const char *what) {
    fprintf(stderr, "unisa-image-macho: %s %s\n", why, what ? what : "");
    exit(1);
}

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
static void be(Buf *b, uint64_t v, int n) { unsigned char e[8]; int k; for (k = 0; k < n; k++) e[k] = (unsigned char)(v >> (8 * (n - 1 - k))); badd(b, e, (size_t)n); }
static void bstr16(Buf *b, const char *s) { unsigned char e[16]; size_t k = strlen(s); memset(e, 0, 16); memcpy(e, s, k > 16 ? 16 : k); badd(b, e, 16); }

#define VMADDR 0x100000000ull
#define PAGE 0x4000ull
#define PAGE4 0x1000ull
#define DLPREFIX 32ull
#define SEG 72ull
#define SECT 80ull
#define NCMDS 12u
#define SLACK 156ull
#define STRTAB 8ull
#define CS_MAGIC_EMBEDDED 0xFADE0CC0u
#define CS_MAGIC_CODEDIRECTORY 0xFADE0C02u
#define CS_ADHOC 0x00000002u
#define CS_EXECSEG_MAIN_BINARY 0x1ull
static const char IDENT[] = "unisa";           /* IDENT = b"unisa\0": 6 bytes incl. the NUL */
#define IDENT_LEN 6ull
static const char DYLD[] = "/usr/lib/dyld";
static const char LIBSYS[] = "/usr/lib/libSystem.B.dylib";
static const char *DLIMPORTS[4] = {"dlopen", "dlsym", "dlclose", "dlerror"};

static uint64_t dylinker_len(void) { return 12 + 20; }
static uint64_t dylib_len(void) { size_t k = strlen(LIBSYS) + 1; size_t p = (8 - k % 8) % 8; return 24 + k + p; }
static uint64_t cmdsz(void) {
    return SEG + (SEG + SECT) + (SEG + 2 * SECT) + SEG + dylinker_len() + dylib_len()
           + 24 + 24 + 48 + 24 + 80 + 16;
}
static uint64_t hdrs_of(void) { return 32 + cmdsz() + SLACK; }
static uint64_t cd_len(void) { return 88 + IDENT_LEN; }
static uint64_t sig_len(uint64_t code_limit) {
    uint64_t slots = (code_limit + PAGE4 - 1) / PAGE4;
    return 12 + 8 + cd_len() + 32 * slots;
}

static uint64_t dlbind_len(void) { uint64_t bl = 0; int i; for (i = 0; i < 4; i++) bl += 5 + 1 + strlen(DLIMPORTS[i]) + 1 + 1; return bl + 1; }

/* DLBIND: four eager binds (dylib ordinal 1, pointer type, __DATA segment 2), then a NUL */
static void dlbind(Buf *b) {
    int i;
    for (i = 0; i < 4; i++) {
        unsigned char hdr[5] = {0x11, 0x51, 0x72, (unsigned char)(8 * i), 0x40};
        badd(b, hdr, 5);
        badd(b, "_", 1); badd(b, DLIMPORTS[i], strlen(DLIMPORTS[i])); bzero(b, 1);
        { unsigned char nop = 0x90; badd(b, &nop, 1); }
    }
    bzero(b, 1);
}

static void dylinker(Buf *b) {
    le(b, 0xE, 4); le(b, dylinker_len(), 4); le(b, 12, 4);
    badd(b, DYLD, strlen(DYLD)); bzero(b, 20 - strlen(DYLD));
}
static void dylib(Buf *b) {
    size_t k = strlen(LIBSYS) + 1; size_t p = (8 - k % 8) % 8;
    le(b, 0xC, 4); le(b, dylib_len(), 4); le(b, 24, 4); le(b, 0, 4); le(b, 0x10000, 4); le(b, 0x10000, 4);
    badd(b, LIBSYS, strlen(LIBSYS)); bzero(b, 1 + p);
}

static void seg(Buf *b, const char *name, uint64_t vm, uint64_t vmsize, uint64_t fo, uint64_t fs, uint32_t maxp, uint32_t initp, uint32_t nsects) {
    le(b, 0x19, 4); le(b, SEG + SECT * nsects, 4); bstr16(b, name);
    le(b, vm, 8); le(b, vmsize, 8); le(b, fo, 8); le(b, fs, 8);
    le(b, maxp, 4); le(b, initp, 4); le(b, nsects, 4); le(b, 0, 4);
}
static void sect(Buf *b, const char *sect_name, const char *seg_name, uint64_t addr, uint64_t size, uint32_t off, uint32_t flags) {
    bstr16(b, sect_name); bstr16(b, seg_name);
    le(b, addr, 8); le(b, size, 8); le(b, off, 4); le(b, 2, 4); le(b, 0, 4); le(b, 0, 4);
    le(b, flags, 4); le(b, 0, 4); le(b, 0, 4); le(b, 0, 4);
}

/* ad-hoc SuperBlob over image[0:code_limit], SHA-256 per 4 KB page; exec_limit = textsz */
static void signature(Buf *b, const unsigned char *image, uint64_t code_limit, uint64_t exec_limit) {
    uint64_t slots = (code_limit + PAGE4 - 1) / PAGE4, i;
    Buf cd = {0};
    be(&cd, CS_MAGIC_CODEDIRECTORY, 4); be(&cd, cd_len() + 32 * slots, 4); be(&cd, 0x20400, 4);
    be(&cd, CS_ADHOC, 4); be(&cd, cd_len(), 4); be(&cd, 88, 4); be(&cd, 0, 4);
    be(&cd, slots, 4); be(&cd, code_limit, 4);
    be(&cd, 32, 1); be(&cd, 2, 1); be(&cd, 0, 1); be(&cd, 12, 1); be(&cd, 0, 4);
    be(&cd, 0, 4); be(&cd, 0, 4); be(&cd, 0, 4); be(&cd, 0, 8); be(&cd, 0, 8); be(&cd, exec_limit, 8); be(&cd, CS_EXECSEG_MAIN_BINARY, 8);
    if (cd.n != 88) die("CodeDirectory fixed part", NULL);
    badd(&cd, IDENT, IDENT_LEN);
    for (i = 0; i < slots; i++) {
        uint64_t lo = i * PAGE4, hi = (i + 1) * PAGE4;
        unsigned char d[32];
        if (hi > code_limit) hi = code_limit;
        sha256(image + lo, (size_t)(hi - lo), d);
        badd(&cd, d, 32);
    }
    be(b, CS_MAGIC_EMBEDDED, 4); be(b, 12 + 8 + cd.n, 4); be(b, 1, 4);
    be(b, 0, 4); be(b, 20, 4);
    badd(b, cd.p, cd.n);
}

static int hexval(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
static int split_ws(char *s, char **tok, int max) {
    int n = 0;
    for (;;) {
        while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') s++;
        if (!*s) break;
        if (n >= max) die("too many fields", NULL);
        tok[n++] = s;
        while (*s && *s != ' ' && *s != '\t' && *s != '\r' && *s != '\n') s++;
        if (*s) *s++ = 0;
    }
    return n;
}
static uint64_t dec(const char *s, const char *what) {
    char *end = NULL; unsigned long long v;
    if (!*s || *s == '+' || *s == '-') die("needs a non-negative decimal", what);
    errno = 0; v = strtoull(s, &end, 10);
    if (*end || errno == ERANGE) die("needs a non-negative decimal that fits 64 bits", what);
    return (uint64_t)v;
}
static void sdec(const char *s, const char *what, int *neg, uint64_t *mag) {
    const char *p = s; char *end = NULL; unsigned long long v;
    *neg = 0;
    if (*p == '+' || *p == '-') { *neg = *p == '-'; p++; }
    if (!*p || *p < '0' || *p > '9') die("needs a signed decimal", what);
    errno = 0; v = strtoull(p, &end, 10);
    if (*end || errno == ERANGE) die("needs a signed decimal that fits 64 bits", what);
    *mag = (uint64_t)v;
}
static void image(Buf *b, int nt, char **tok) {
    if (nt == 4 && !strcmp(tok[1], "f")) {
        uint64_t cnt, k; int a, c; unsigned char v;
        if (strlen(tok[2]) != 2) die("fill byte needs two hex digits", tok[2]);
        a = hexval((unsigned char)tok[2][0]); c = hexval((unsigned char)tok[2][1]);
        if (a < 0 || c < 0) die("fill byte needs two hex digits", tok[2]);
        v = (unsigned char)(a * 16 + c);
        cnt = dec(tok[3], tok[3]);
        for (k = 0; k < cnt; k++) badd(b, &v, 1);
    } else if (nt == 3 && !strcmp(tok[1], "x")) {
        size_t L, k;
        if (!strcmp(tok[2], "-")) return;
        L = strlen(tok[2]);
        if (L % 2) die("odd-length hex", tok[2]);
        for (k = 0; k < L; k += 2) {
            int a = hexval((unsigned char)tok[2][k]), c = hexval((unsigned char)tok[2][k + 1]);
            unsigned char v;
            if (a < 0 || c < 0) die("non-hex digit", NULL);
            v = (unsigned char)(a * 16 + c);
            badd(b, &v, 1);
        }
    } else die("malformed image line", tok[0]);
}

int main(int argc, char **argv) {
    FILE *f; char line[1 << 16];
    int have_arch = 0, have_text = 0, have_data = 0, have_entry = 0, have_full = 0, x86 = 0;
    int entry_neg = 0; uint64_t entry_mag = 0, entry_addr = 0, full = 0;
    Buf text = {0}, data = {0}, out = {0}, m = {0};
    uint64_t hdrs, textsz, datavm, datasz, link, bindoff, sigoff, siglen, linksz, tend;
    if (argc != 3 || strcmp(argv[1], "write")) { fprintf(stderr, "usage: unisa-image-macho write SPEC\n"); return 2; }
    f = fopen(argv[2], "rb"); if (!f) die("cannot open", argv[2]);
    while (fgets(line, sizeof line, f)) {
        char *tok[8]; int nt = split_ws(line, tok, 8);
        if (nt == 0) continue;
        if (!strcmp(tok[0], "arch") && nt == 2) {
            if (!strcmp(tok[1], "x86_64")) x86 = 1;
            else if (!strcmp(tok[1], "arm64")) x86 = 0;
            else die("unknown arch (python: KeyError)", tok[1]);
            have_arch = 1;
        } else if (!strcmp(tok[0], "text")) { image(&text, nt, tok); have_text = 1; }
        else if (!strcmp(tok[0], "data")) { image(&data, nt, tok); have_data = 1; }
        else if (!strcmp(tok[0], "entry") && nt == 2) { sdec(tok[1], tok[1], &entry_neg, &entry_mag); have_entry = 1; }
        else if (!strcmp(tok[0], "full") && nt == 2) { full = dec(tok[1], tok[1]); have_full = 1; }
        else die("malformed spec line", tok[0]);
    }
    fclose(f);
    if (!have_arch || !have_text || !have_data || !have_entry) die("spec needs arch, text, data, entry", NULL);
    if (!have_full) full = data.n;
    if (full < data.n) die("bss size negative (python: struct.error)", NULL);
    hdrs = hdrs_of();
    /* entry: hdrs + entry must be a valid unsigned 64-bit value (python: LC_MAIN struct Q) */
    if (entry_neg) { if (entry_mag > hdrs) die("entry address is negative (python: struct.error)", NULL); entry_addr = hdrs - entry_mag; }
    else { if (entry_mag > UINT64_MAX - hdrs) die("entry does not fit 64 bits (python: struct.error)", NULL); entry_addr = hdrs + entry_mag; }
    tend = hdrs + text.n;
    textsz = (tend + PAGE - 1) / PAGE * PAGE;
    datavm = (full + DLPREFIX + PAGE - 1) / PAGE * PAGE;
    datasz = (data.n + DLPREFIX + PAGE - 1) / PAGE * PAGE;
    link = textsz + datasz;
    bindoff = link + STRTAB;
    { uint64_t bl = dlbind_len();
      sigoff = (bindoff + bl + 15) / 16 * 16;
      siglen = sig_len(sigoff);
      linksz = sigoff - link + siglen;
    }
    /* Mach-O header */
    le(&m, 0xFEEDFACFu, 4); le(&m, x86 ? 0x01000007u : 0x0100000Cu, 4); le(&m, x86 ? 3 : 0, 4);
    le(&m, 2, 4); le(&m, NCMDS, 4); le(&m, cmdsz(), 4); le(&m, 0x200085u, 4); le(&m, 0, 4);
    seg(&m, "__PAGEZERO", 0, VMADDR, 0, 0, 0, 0, 0);
    seg(&m, "__TEXT", VMADDR, textsz, 0, textsz, 5, 5, 1);
    sect(&m, "__text", "__TEXT", VMADDR + hdrs, text.n, (uint32_t)hdrs, 0x80000400u);
    seg(&m, "__DATA", VMADDR + textsz, datavm, textsz, datasz, 3, 3, 2);
    sect(&m, "__data", "__DATA", VMADDR + textsz, data.n + DLPREFIX, (uint32_t)textsz, 0);
    sect(&m, "__bss", "__DATA", VMADDR + textsz + DLPREFIX + data.n, full - data.n, 0, 1);
    seg(&m, "__LINKEDIT", VMADDR + textsz + datavm, (linksz + PAGE - 1) / PAGE * PAGE, link, linksz, 1, 1, 0);
    dylinker(&m);
    dylib(&m);
    le(&m, 0x80000028u, 4); le(&m, 24, 4); le(&m, entry_addr, 8); le(&m, 0, 8);   /* LC_MAIN */
    le(&m, 0x32, 4); le(&m, 24, 4); le(&m, 1, 4); le(&m, 13ull << 16, 4); le(&m, 13ull << 16, 4); le(&m, 0, 4);
    le(&m, 0x80000022u, 4); le(&m, 48, 4);                     /* LC_DYLD_INFO_ONLY */
    le(&m, 0, 4); le(&m, 0, 4);                                /* rebase off/size */
    le(&m, bindoff, 4); le(&m, dlbind_len(), 4);               /* bind off/size */
    { int i; for (i = 0; i < 6; i++) le(&m, 0, 4); }           /* weak, lazy, export: zero */
    le(&m, 2, 4); le(&m, 24, 4); le(&m, link, 4); le(&m, 0, 4); le(&m, link, 4); le(&m, STRTAB, 4);
    le(&m, 0xB, 4); le(&m, 80, 4); { int i; for (i = 0; i < 18; i++) le(&m, 0, 4); }
    le(&m, 0x1Du, 4); le(&m, 16, 4); le(&m, sigoff, 4); le(&m, siglen, 4);
    if (m.n != hdrs - SLACK) die("header size", NULL);
    bzero(&m, SLACK);
    badd(&out, m.p, m.n);
    badd(&out, text.p ? text.p : (const unsigned char *)"", text.n);
    bzero(&out, textsz - out.n);
    bzero(&out, DLPREFIX);
    badd(&out, data.p ? data.p : (const unsigned char *)"", data.n);
    bzero(&out, link - out.n);
    bzero(&out, STRTAB);
    { Buf db = {0}; dlbind(&db); badd(&out, db.p, db.n); }
    bzero(&out, sigoff - out.n);
    signature(&out, out.p, sigoff, textsz);
    { size_t i; for (i = 0; i < out.n; i++) printf("%02x", out.p[i]); printf("\n"); }
    return 0;
}
