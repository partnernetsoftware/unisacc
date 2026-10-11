/* seed/unisa_image_fat.c -- C99 reference for unisa.image.fat (K6-12), point-sample functions.
 *
 *   unisa-image-fat write SPEC
 *       SPEC lines (fields separated by blanks; blank lines skipped), in slice order:
 *         ARCH x HEX        slice image = bytes.fromhex(HEX); HEX "-" means the empty image
 *         ARCH f HH N       slice image = N bytes of value HH (HH: exactly two hex digits; N >= 0)
 *       prints write(slices) as one lowercase hex line.
 *
 * Mirrors unisa/image/fat.py write(): CPU table (cputype, cpusubtype, log2 align) for x86_64 and
 * arm64; header 8 bytes (FAT_MAGIC BE, nslices BE); 20-byte descriptors (cpu, sub, offset, size,
 * align, all BE); body: each slice padded from the running offset to round_up(off, 1 << align).
 * Unknown ARCH is refused (python: KeyError); a malformed line is refused.  An empty slice list is a
 * valid input and prints the 8-byte header.  Refusals print the reason on stderr and nothing on stdout.
 * Point-sample equivalence over the listed specs only; not a product closure member; keeps
 * unisa/image/fat.py and unisa/bits.py; does not cover elf/macho/pe/ape.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void die(const char *why, const char *what) {
    fprintf(stderr, "unisa-image-fat: %s %s\n", why, what ? what : "");
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
static void bbe32(Buf *b, uint32_t v) {
    unsigned char e[4] = { (unsigned char)(v >> 24), (unsigned char)(v >> 16), (unsigned char)(v >> 8), (unsigned char)v };
    badd(b, e, 4);
}
static void bzero(Buf *b, size_t k) {
    while (k--) { unsigned char z = 0; badd(b, &z, 1); }
}

/* CPU[arch] + (ALIGN[arch],): cputype, cpusubtype, log2 page alignment */
typedef struct { const char *arch; uint32_t cpu, sub, al; } CpuRow;
static const CpuRow CPU[] = {
    {"x86_64", 0x01000007u, 3, 12},
    {"arm64", 0x0100000Cu, 0, 14},
};
static const CpuRow *cpu_of(const char *arch) {
    size_t i;
    for (i = 0; i < sizeof CPU / sizeof CPU[0]; i++) if (!strcmp(CPU[i].arch, arch)) return &CPU[i];
    return NULL;
}

static int hexval(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

typedef struct { const CpuRow *row; Buf img; } Slice;

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

static Slice *read_spec(const char *path, size_t *nout) {
    FILE *f = fopen(path, "rb"); char line[1 << 16]; Slice *sl = NULL; size_t n = 0, cap = 0;
    if (!f) die("cannot open", path);
    while (fgets(line, sizeof line, f)) {
        char *tok[8]; int nt = split_ws(line, tok, 8);
        const CpuRow *row;
        Slice s; memset(&s, 0, sizeof s);
        if (nt == 0) continue;
        if (nt < 2) die("malformed slice line", line);
        row = cpu_of(tok[0]);
        if (!row) die("unknown arch (python: KeyError)", tok[0]);
        if (nt == 3 && !strcmp(tok[1], "x")) {
            size_t L = strcmp(tok[2], "-") ? strlen(tok[2]) : 0, k;
            if (L % 2) die("odd-length hex", tok[2]);
            if (strcmp(tok[2], "-")) {
                for (k = 0; k < L; k += 2) {
                    int a = hexval((unsigned char)tok[2][k]), b = hexval((unsigned char)tok[2][k + 1]);
                    unsigned char byte;
                    if (a < 0 || b < 0) die("non-hex digit", NULL);
                    byte = (unsigned char)(a * 16 + b);
                    badd(&s.img, &byte, 1);
                }
            }
        } else if (nt == 4 && !strcmp(tok[1], "f")) {
            long long cnt; char *end = NULL; int a, b; unsigned char byte;
            if (strlen(tok[2]) != 2) die("fill byte needs two hex digits", tok[2]);
            a = hexval((unsigned char)tok[2][0]); b = hexval((unsigned char)tok[2][1]);
            if (a < 0 || b < 0) die("fill byte needs two hex digits", tok[2]);
            cnt = strtoll(tok[3], &end, 10);
            if (*end || cnt < 0 || tok[3][0] == '+' || tok[3][0] == '-') die("fill count must be a non-negative decimal", tok[3]);
            byte = (unsigned char)(a * 16 + b);
            for (; cnt > 0; cnt--) badd(&s.img, &byte, 1);
        } else die("malformed slice line", line);
        s.row = row;
        if (n == cap) { cap = cap ? cap * 2 : 8; sl = realloc(sl, cap * sizeof *sl); if (!sl) die("out of memory", NULL); }
        sl[n++] = s;
    }
    fclose(f);
    *nout = n;
    return sl;
}

int main(int argc, char **argv) {
    Slice *sl; size_t n = 0, i, hdr, off;
    Buf descs = {0}, body = {0}, out = {0};
    if (argc != 3 || strcmp(argv[1], "write")) {
        fprintf(stderr, "usage: unisa-image-fat write SPEC\n");
        return 2;
    }
    sl = read_spec(argv[2], &n);
    hdr = 8 + 20 * n; off = hdr;
    for (i = 0; i < n; i++) {
        uint32_t al = sl[i].row->al; size_t unit = (size_t)1 << al;
        size_t at = (off + unit - 1) / unit * unit;
        bzero(&body, at - off);
        badd(&body, sl[i].img.p, sl[i].img.n);
        bbe32(&descs, sl[i].row->cpu); bbe32(&descs, sl[i].row->sub);
        bbe32(&descs, (uint32_t)at); bbe32(&descs, (uint32_t)sl[i].img.n); bbe32(&descs, al);
        off = at + sl[i].img.n;
    }
    bbe32(&out, 0xCAFEBABEu); bbe32(&out, (uint32_t)n);
    badd(&out, descs.p ? descs.p : (const unsigned char *)"", descs.n);
    badd(&out, body.p ? body.p : (const unsigned char *)"", body.n);
    for (i = 0; i < out.n; i++) printf("%02x", out.p[i]);
    printf("\n");
    return 0;
}
