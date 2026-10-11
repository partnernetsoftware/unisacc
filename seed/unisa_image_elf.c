/* seed/unisa_image_elf.c -- C99 reference for unisa.image.elf (K6-13), static write() point samples.
 *
 *   unisa-image-elf write SPEC
 *       SPEC lines (fields separated by blanks; blank lines skipped):
 *         arch ARCH                ARCH in x86_64 arm64
 *         text f HH N | text x HEX | text x -     the text image (fill / hex / empty)
 *         data f HH N | data x HEX | data x -     the data image
 *         entry N                  entry offset within text (signed decimal; e_entry = VADDR + HDRS + N)
 *         full N                   p_memsz of the rw segment (optional; default len(data))
 *       prints write(arch, text, data, entry, full) (static path) as one lowercase hex line.
 *
 * Mirrors unisa/image/elf.py write() with dynamic=False: ELF64 header (e_ident, type exec, machine,
 * entry = VADDR + HDRS + entry, phoff 64, phentsize 56, phnum 2); two PT_LOAD headers, r-x over the
 * headers and text, rw- over data at p_offset = round_up(tend, PAGE) with p_vaddr = VADDR + p_offset.
 * VADDR 0x400000, PAGE 0x1000.  Unknown arch and negative entry are refused (python: KeyError /
 * struct.error); a signed entry is accepted whenever VADDR + HDRS + entry fits an unsigned 64-bit value,
 * as in python.  The dynamic=True path is not implemented here (not claimed).  Refusals print the
 * reason on stderr and nothing on stdout.  Point-sample equivalence over the listed specs only; not a
 * product closure member; keeps unisa/image/elf.py and unisa/bits.py.
 */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VADDR 0x400000ull
#define PAGE 0x1000ull
#define EHDR 64u
#define PHDR 56u
#define NPH 2u

static void die(const char *why, const char *what) {
    fprintf(stderr, "unisa-image-elf: %s %s\n", why, what ? what : "");
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
    errno = 0;
    v = strtoull(s, &end, 10);
    if (*end || errno == ERANGE) die("needs a non-negative decimal that fits 64 bits", what);
    return (uint64_t)v;
}
/* signed decimal as python int(): optional sign, digits; the magnitude must fit 64 bits */
static void sdec(const char *s, const char *what, int *neg, uint64_t *mag) {
    const char *p = s; char *end = NULL; unsigned long long v;
    *neg = 0;
    if (*p == '+' || *p == '-') { *neg = *p == '-'; p++; }
    if (!*p || *p < '0' || *p > '9') die("needs a signed decimal", what);
    errno = 0;
    v = strtoull(p, &end, 10);
    if (*end || errno == ERANGE) die("needs a signed decimal that fits 64 bits", what);
    *mag = (uint64_t)v;
}

/* a segment image from a spec: f HH N (fill), x HEX (bytes), x - (empty) */
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
    int have_arch = 0, have_text = 0, have_data = 0, have_entry = 0, have_full = 0;
    const char *arch = NULL; int x86 = 0;
    Buf text = {0}, data = {0}, out = {0}, hdr = {0};
    int entry_neg = 0; uint64_t entry_mag = 0, entry_addr = 0, full = 0; size_t tend, doff, hdrs;
    if (argc != 3 || strcmp(argv[1], "write")) { fprintf(stderr, "usage: unisa-image-elf write SPEC\n"); return 2; }
    f = fopen(argv[2], "rb"); if (!f) die("cannot open", argv[2]);
    while (fgets(line, sizeof line, f)) {
        char *tok[8]; int nt = split_ws(line, tok, 8);
        if (nt == 0) continue;
        if (!strcmp(tok[0], "arch") && nt == 2) {
            if (!strcmp(tok[1], "x86_64")) x86 = 1;
            else if (!strcmp(tok[1], "arm64")) x86 = 0;
            else die("unknown arch (python: KeyError)", tok[1]);
            arch = tok[1]; have_arch = 1;
        } else if (!strcmp(tok[0], "text")) { image(&text, nt, tok); have_text = 1; }
        else if (!strcmp(tok[0], "data")) { image(&data, nt, tok); have_data = 1; }
        else if (!strcmp(tok[0], "entry") && nt == 2) { sdec(tok[1], tok[1], &entry_neg, &entry_mag); have_entry = 1; }
        else if (!strcmp(tok[0], "full") && nt == 2) { full = dec(tok[1], tok[1]); have_full = 1; }
        else die("malformed spec line", tok[0]);
    }
    fclose(f);
    if (!have_arch || !have_text || !have_data || !have_entry) die("spec needs arch, text, data, entry", NULL);
    if (!have_full) full = data.n;
    hdrs = EHDR + PHDR * NPH;
    /* e_entry = VADDR + hdrs + entry: refused unless the sum is a valid unsigned 64-bit value (python: struct Q) */
    {
        uint64_t base = VADDR + hdrs;
        if (entry_neg) {
            if (entry_mag > base) die("entry address is negative (python: struct.error)", NULL);
            entry_addr = base - entry_mag;
        } else {
            if (entry_mag > UINT64_MAX - base) die("entry address does not fit 64 bits (python: struct.error)", NULL);
            entry_addr = base + entry_mag;
        }
    }
    tend = hdrs + text.n;
    doff = (size_t)((tend + PAGE - 1) / PAGE * PAGE);
    /* e_ident: 7f ELF, class 2, data 1, version 1, SYSV, pad to 16 */
    { unsigned char id[16] = {0x7f, 'E', 'L', 'F', 2, 1, 1, 0}; badd(&hdr, id, 16); }
    le(&hdr, 2, 2);                                  /* ET_EXEC */
    le(&hdr, x86 ? 0x3E : 0xB7, 2);                  /* EM_X86_64 / EM_AARCH64 */
    le(&hdr, 1, 4);                                  /* EV_CURRENT */
    le(&hdr, entry_addr, 8);                         /* e_entry */
    le(&hdr, EHDR, 8);                               /* e_phoff */
    le(&hdr, 0, 8);                                  /* e_shoff */
    le(&hdr, 0, 4);                                  /* e_flags */
    le(&hdr, EHDR, 2); le(&hdr, PHDR, 2); le(&hdr, NPH, 2);
    le(&hdr, 0, 2); le(&hdr, 0, 2); le(&hdr, 0, 2);  /* shentsize, shnum, shstrndx */
    /* PT_LOAD r-x over headers and text */
    le(&hdr, 1, 4); le(&hdr, 5, 4); le(&hdr, 0, 8); le(&hdr, VADDR, 8); le(&hdr, VADDR, 8);
    le(&hdr, tend, 8); le(&hdr, tend, 8); le(&hdr, PAGE, 8);
    /* PT_LOAD rw- over data */
    le(&hdr, 1, 4); le(&hdr, 6, 4); le(&hdr, doff, 8); le(&hdr, VADDR + doff, 8); le(&hdr, VADDR + doff, 8);
    le(&hdr, data.n, 8); le(&hdr, full, 8); le(&hdr, PAGE, 8);
    if (hdr.n != hdrs) die("header size", NULL);
    badd(&out, hdr.p, hdr.n);
    badd(&out, text.p ? text.p : (const unsigned char *)"", text.n);
    bzero(&out, doff - tend);
    badd(&out, data.p ? data.p : (const unsigned char *)"", data.n);
    { size_t i; for (i = 0; i < out.n; i++) printf("%02x", out.p[i]); printf("\n"); }
    (void)arch;
    return 0;
}
