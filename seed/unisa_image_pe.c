/* seed/unisa_image_pe.c -- C99 reference for unisa.image.pe (K6-15), static write() point samples.
 *
 *   unisa-image-pe write SPEC
 *       SPEC lines (fields separated by blanks; blank lines skipped):
 *         arch ARCH                ARCH in x86_64 arm64
 *         text f HH N | text x HEX | text x -     the .text image
 *         data f HH N | data x HEX | data x -     the .data image
 *         entry N                  signed; AddressOfEntryPoint = TEXT_RVA + N (must fit 32 bits)
 *         full N                   signed; mapped size of .data (python max(1, full); default len(data))
 *         bss N                    signed; extra zero bytes added to the .data virtual size (default 0)
 *         reloc N                  non-negative data-relative RVA carried as a DIR64 relocation (repeatable)
 *       prints write(arch, text, data, entry, relocs, bss, full, stub=b"", dynamic=False) as one hex line.
 *
 * Mirrors unisa/image/pe.py with stub empty and dynamic False: DOS header (e_lfanew 64), PE signature,
 * COFF header (4 sections, 240-byte PE32+ optional header, characteristics 0x22), import section
 * (KERNEL32.dll, 22 IMPORTS, load-config directory with the stack-cookie pointer), .text/.rdata/.data/
 * .reloc sections at 0x1000 RVA alignment and 0x200 file alignment, and the .reloc section decoded from
 * the cookie pointer plus the given relocations.  Refused (python: KeyError / struct.error): unknown
 * arch; an entry whose address does not fit an unsigned 32-bit value; a data size outside 32 bits.
 * Not covered: stub (APE shell), dynamic=True (forwarding imports), negative relocs (refused here as a
 * spec rule on both sides).  Point-sample equivalence only; not a product closure; keeps pe.py and bits.py.
 * Writing these bytes is not a claim that the image loads on Windows or is an APE.
 */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void die(const char *why, const char *what);

/* ---- SHA-256 block is not needed by pe.py; kept out of this file ---- */

static void die(const char *why, const char *what) {
    fprintf(stderr, "unisa-image-pe: %s %s\n", why, what ? what : "");
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
static void pad_to(Buf *b, size_t n) { if (b->n < n) bzero(b, n - b->n); }
static size_t round_up(size_t v, size_t a) { return (v + a - 1) / a * a; }

#define IMAGEBASE 0x140000000ull
#define SECT_ALIGN 0x1000ull
#define FILE_ALIGN 0x200ull
#define TEXT_RVA 0x1000ull
#define HDR_FILE 0x400ull
#define LOADCFG 0x140ull
#define COOKIE_FIELD 0x58ull
static const char DLL[] = "KERNEL32.dll";
/* IMPORTS of unisa/image/pe.py, in order: the IAT slot of a name is its index */
static const char *IMPORTS[] = {
    "GetStdHandle", "WriteFile", "ReadFile", "CloseHandle",
    "CreateFileA", "ExitProcess", "GetCommandLineA", "VirtualAlloc",
    "VirtualProtect", "VirtualFree", "FlushInstructionCache",
    "SetFilePointer", "DeleteFileA", "MoveFileExA",
    "CreateProcessW", "WaitForSingleObject", "GetExitCodeProcess",
    "TerminateProcess", "SetStdHandle", "GetCurrentProcess",
    "GetCurrentProcessId", "GetModuleFileNameW",
};
#define NIMP ((int)(sizeof IMPORTS / sizeof IMPORTS[0]))

/* _idata(rva, cookie): the import section; returns its bytes and the IAT and load-config offsets */
static void idata(Buf *b, uint64_t rva, uint64_t cookie, uint64_t *iat_off_out, uint64_t *cfg_off_out) {
    uint64_t n = NIMP, int_off = 40, iat_off, nm_off, off, dll_rva, cfg_off, i;
    uint64_t hint[NIMP];
    Buf names = {0}, bb = {0};
    iat_off = int_off + (n + 1) * 8;
    nm_off = iat_off + (n + 1) * 8;
    off = nm_off;
    for (i = 0; i < n; i++) {
        size_t L = 2 + strlen(IMPORTS[i]) + 1, pad = L % 2;
        hint[i] = rva + off;
        le(&names, 0, 2); badd(&names, IMPORTS[i], strlen(IMPORTS[i])); bzero(&names, 1 + pad);
        off += L + pad;
    }
    dll_rva = rva + off;
    badd(&names, DLL, strlen(DLL)); bzero(&names, 1);
    off += strlen(DLL) + 1;
    off = (off + 7) / 8 * 8;
    cfg_off = off;
    off += LOADCFG;
    le(&bb, rva + int_off, 4); le(&bb, 0, 4); le(&bb, 0, 4); le(&bb, dll_rva, 4); le(&bb, rva + iat_off, 4);
    bzero(&bb, 20);
    for (i = 0; i < 2; i++) {
        uint64_t j;
        for (j = 0; j < n; j++) le(&bb, hint[j], 8);
        le(&bb, 0, 8);
    }
    if (bb.n != nm_off) die("idata layout", NULL);
    badd(&bb, names.p, names.n);
    pad_to(&bb, (size_t)cfg_off);
    {
        Buf cfg = {0}; uint64_t k;
        le(&cfg, LOADCFG, 4); bzero(&cfg, LOADCFG - 4);
        if (cookie) { unsigned char c8[8]; for (k = 0; k < 8; k++) c8[k] = (unsigned char)(cookie >> (8 * k)); memcpy(cfg.p + COOKIE_FIELD, c8, 8); }
        badd(&bb, cfg.p, cfg.n);
    }
    badd(b, bb.p, bb.n);
    *iat_off_out = iat_off; *cfg_off_out = cfg_off;
}

static int cmp_u64(const void *a, const void *b) { uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b; return (x > y) - (x < y); }
/* _reloc(rvas): one block per 4 KB page of sorted unique RVAs; DIR64 words, ABSOLUTE padding when odd */
static void reloc(Buf *b, uint64_t *rvas, size_t nr) {
    size_t i = 0, k;
    qsort(rvas, nr, sizeof *rvas, cmp_u64);
    {   /* unique */
        size_t w = 0;
        for (k = 0; k < nr; k++) if (k == 0 || rvas[k] != rvas[k - 1]) rvas[w++] = rvas[k];
        nr = w;
    }
    while (i < nr) {
        uint64_t page = rvas[i] / 0x1000 * 0x1000;
        uint16_t words[64]; size_t nw = 0, j;
        while (i < nr && rvas[i] - page < 0x1000) {
            if (nw >= 64) die("relocation page overflow", NULL);
            words[nw++] = (uint16_t)((10u << 12) | (unsigned)(rvas[i] - page));
            i++;
        }
        if (nw % 2) { if (nw >= 64) die("relocation page overflow", NULL); words[nw++] = 0; }
        le(b, page, 4); le(b, 8 + 2 * nw, 4);
        for (j = 0; j < nw; j++) le(b, words[j], 2);
    }
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
static uint64_t udec(const char *s, const char *what) {
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
        cnt = udec(tok[3], tok[3]);
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
    int entry_neg = 0, full_neg = 0, bss_neg = 0;
    uint64_t entry_mag = 0, full_mag = 0, bss = 0;
    uint64_t rvas[64]; size_t nrel = 0;
    Buf text = {0}, data = {0}, out = {0}, p = {0}, o = {0}, rdata = {0}, rl = {0};
    uint64_t rd_rva, dt_rva, rl_rva, rd_file, dt_file, rl_file, rd_r, iat_off, cfg_off, idata_len;
    uint64_t dvs, img_size, cookie_rva, entry_addr, fullc, dt_size_r, textlen, datalen, reloc_len;
    Buf reloc_b = {0}, idb = {0};
    if (argc != 3 || strcmp(argv[1], "write")) { fprintf(stderr, "usage: unisa-image-pe write SPEC\n"); return 2; }
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
        else if (!strcmp(tok[0], "full") && nt == 2) { sdec(tok[1], tok[1], &full_neg, &full_mag); have_full = 1; }
        else if (!strcmp(tok[0], "bss") && nt == 2) { sdec(tok[1], tok[1], &bss_neg, &bss); }
        else if (!strcmp(tok[0], "reloc") && nt == 2) {
            if (nrel >= 64) die("too many relocs", NULL);
            rvas[nrel++] = udec(tok[1], tok[1]);
        }
        else die("malformed spec line", tok[0]);
    }
    fclose(f);
    if (!have_arch || !have_text || !have_data || !have_entry) die("spec needs arch, text, data, entry", NULL);
    if (!have_full) { full_neg = 0; full_mag = data.n; }
    /* python: max(1, full) for the mapped size; the sign of full is not otherwise checked */
    fullc = full_neg ? 1 : (full_mag >= 1 ? full_mag : 1);
    /* entry: TEXT_RVA + entry must fit an unsigned 32-bit value (python: struct "I") */
    if (entry_neg) { if (entry_mag > TEXT_RVA) die("entry address is negative (python: struct.error)", NULL); entry_addr = TEXT_RVA - entry_mag; }
    else { if (entry_mag > 0xFFFFFFFFull - TEXT_RVA) die("entry address does not fit 32 bits (python: struct.error)", NULL); entry_addr = TEXT_RVA + entry_mag; }
    textlen = text.n;
    rd_rva = TEXT_RVA + round_up(textlen, SECT_ALIGN);
    /* the import section, to learn its length and offsets (rva 0 first, as _rvas/idata_dims do) */
    idata(&idb, 0, 0, &iat_off, &cfg_off); idata_len = idb.n;
    dt_rva = rd_rva + round_up(idata_len, SECT_ALIGN);
    cookie_rva = dt_rva + round_up(fullc, 8);
    /* the real import section at rd_rva, with the cookie pointer */
    { Buf real = {0}; uint64_t io, co; idata(&real, rd_rva, IMAGEBASE + cookie_rva, &io, &co); idb = real; iat_off = io; cfg_off = co; }
    rd_file = HDR_FILE + round_up(textlen, FILE_ALIGN);
    dt_file = rd_file + round_up(idb.n, FILE_ALIGN);
    {   /* dvs = round_up(max(1, full), 8) + 8 + bss, signed; python packs it as an unsigned 32-bit field */
        long long sv = (long long)(round_up(fullc, 8) + 8);
        long long sb = bss_neg ? -(long long)bss : (long long)bss;
        if (bss > (1ull << 62) || sv + sb < 0 || sv + sb > 0xFFFFFFFFll) die("data size does not fit 32 bits (python: struct.error)", NULL);
        dvs = (uint64_t)(sv + sb);
    }
    /* data is padded to 8 bytes (at least one) */
    { size_t want = round_up(data.n > 0 ? data.n : 1, 8); pad_to(&data, want); }
    rl_rva = dt_rva + round_up(dvs, SECT_ALIGN);
    rl_file = dt_file + round_up(data.n, FILE_ALIGN);
    { uint64_t rr[65]; size_t k, nr = 0; rr[nr++] = rd_rva + cfg_off + COOKIE_FIELD;
      for (k = 0; k < nrel; k++) rr[nr++] = dt_rva + rvas[k];
      reloc(&reloc_b, rr, nr); }
    reloc_len = reloc_b.n;
    img_size = rl_rva + round_up(reloc_len, SECT_ALIGN);
    if (img_size > 0xFFFFFFFFull) die("image size does not fit 32 bits", NULL);
    /* DOS header with e_lfanew = 64 (no stub), PE signature, COFF, optional header (PE32+) */
    { unsigned char dos[64]; memset(dos, 0, sizeof dos); dos[0] = 'M'; dos[1] = 'Z'; dos[0x3C] = 64; badd(&p, dos, 64); }
    badd(&p, "PE\0\0", 4);
    le(&p, x86 ? 0x8664u : 0xAA64u, 2); le(&p, 4, 2); le(&p, 0, 4); le(&p, 0, 4); le(&p, 0, 4); le(&p, 240, 2); le(&p, 0x22, 2);
    le(&o, 0x20B, 2); le(&o, 14, 1); le(&o, 0, 1); le(&o, round_up(textlen, FILE_ALIGN), 4); le(&o, 0, 4); le(&o, 0, 4);
    le(&o, entry_addr, 4); le(&o, TEXT_RVA, 4);
    le(&o, IMAGEBASE, 8); le(&o, SECT_ALIGN, 4); le(&o, FILE_ALIGN, 4);
    le(&o, 4, 2); le(&o, 0, 2); le(&o, 0, 2); le(&o, 0, 2); le(&o, 4, 2); le(&o, 0, 2);
    le(&o, 0, 4); le(&o, img_size, 4); le(&o, HDR_FILE, 4); le(&o, 0, 4); le(&o, 3, 2); le(&o, 0x8160, 2);
    le(&o, 0x100000, 8); le(&o, 0x1000, 8); le(&o, 0x100000, 8); le(&o, 0x1000, 8); le(&o, 0, 4); le(&o, 16, 4);
    {
        uint64_t dA[16], dS[16]; int i;
        for (i = 0; i < 16; i++) { dA[i] = 0; dS[i] = 0; }
        dA[1] = rd_rva; dS[1] = 40;
        dA[5] = rl_rva; dS[5] = reloc_len;
        dA[10] = rd_rva + cfg_off; dS[10] = LOADCFG;
        dA[12] = rd_rva + iat_off; dS[12] = ((uint64_t)NIMP + 1) * 8;
        for (i = 0; i < 16; i++) { le(&o, dA[i], 4); le(&o, dS[i], 4); }
    }
    if (o.n != 240) die("optional header size", NULL);
    badd(&p, o.p, o.n);
    {
        const char *names[4] = {".text", ".rdata", ".data", ".reloc"};
        uint64_t vs[4], rv[4], fs[4], fo[4], fl[4]; int i;
        vs[0] = textlen; rv[0] = TEXT_RVA; fs[0] = textlen; fo[0] = HDR_FILE; fl[0] = 0x60000020u;
        vs[1] = idb.n; rv[1] = rd_rva; fs[1] = idb.n; fo[1] = rd_file; fl[1] = 0x40000040u;
        vs[2] = dvs; rv[2] = dt_rva; fs[2] = data.n; fo[2] = dt_file; fl[2] = 0xC0000040u;
        vs[3] = reloc_len; rv[3] = rl_rva; fs[3] = reloc_len; fo[3] = rl_file; fl[3] = 0x42000040u;
        for (i = 0; i < 4; i++) {
            size_t k;
            for (k = 0; k < 8; k++) { unsigned char nb = k < strlen(names[i]) ? (unsigned char)names[i][k] : 0; badd(&p, &nb, 1); }
            le(&p, vs[i], 4); le(&p, rv[i], 4); le(&p, round_up(fs[i], FILE_ALIGN), 4); le(&p, fo[i], 4);
            le(&p, 0, 4); le(&p, 0, 4); le(&p, 0, 2); le(&p, 0, 2); le(&p, fl[i], 4);
        }
    }
    pad_to(&p, HDR_FILE);
    badd(&out, p.p, p.n);
    badd(&out, text.p ? text.p : (const unsigned char *)"", text.n); bzero(&out, round_up(textlen, FILE_ALIGN) - textlen);
    badd(&out, idb.p, idb.n); bzero(&out, round_up(idb.n, FILE_ALIGN) - idb.n);
    badd(&out, data.p ? data.p : (const unsigned char *)"", data.n); bzero(&out, round_up(data.n, FILE_ALIGN) - data.n);
    badd(&out, reloc_b.p, reloc_b.n); bzero(&out, round_up(reloc_len, FILE_ALIGN) - reloc_len);
    { size_t i; for (i = 0; i < out.n; i++) printf("%02x", out.p[i]); printf("\n"); }
    (void)rdata; (void)rl; (void)rd_r; (void)dt_size_r; (void)datalen; (void)rl_file; (void)cfg_off;
    return 0;
}
