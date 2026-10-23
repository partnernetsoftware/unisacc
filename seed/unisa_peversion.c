/* seed/unisa_peversion.c -- C99 reference for unisa.peversion (K6-3).
 *
 *   unisa-peversion IN OUT NAME VERSION
 *       read an unsigned PE32+ head from IN, write the head with a new .rsrc/RT_VERSION to OUT.
 *       Mirrors unisa/peversion.py add_version_info() and _version_info().  Any refusal (the
 *       Python ValueError set, or a short/truncated read) exits 1 with the reason on stderr and
 *       writes nothing to OUT.  NAME must be printable ASCII (out of the sampled domain otherwise).
 *
 * Point-sample equivalence only: the cases in tests/k6/peversion.tsv (tests/k6frozen.cx).  Not a product
 * closure member; the product keeps unisa/peversion.py.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define U32MAX 0xFFFFFFFFull
#define MAXIN (16u << 20)

static unsigned char *src; static size_t srclen;
static unsigned char *out; static size_t outcap;

static void die(const char *why) { fprintf(stderr, "unisa-peversion: %s\n", why); exit(1); }

static uint32_t rd16(const unsigned char *p, size_t off, size_t lim) {
    if (off + 2 > lim) die("short read");
    return (uint32_t)p[off] | ((uint32_t)p[off+1] << 8);
}
static uint32_t rd32(const unsigned char *p, size_t off, size_t lim) {
    if (off + 4 > lim) die("short read");
    return (uint32_t)p[off] | ((uint32_t)p[off+1] << 8) | ((uint32_t)p[off+2] << 16) | ((uint32_t)p[off+3] << 24);
}
static void wr16(unsigned char *p, size_t off, uint32_t v) { p[off] = v & 255; p[off+1] = (v >> 8) & 255; }
static void wr32(unsigned char *p, size_t off, uint32_t v) {
    p[off] = v & 255; p[off+1] = (v >> 8) & 255; p[off+2] = (v >> 16) & 255; p[off+3] = (v >> 24) & 255;
}
static uint64_t u32(uint64_t n) { if (n > U32MAX) die("PE uint32 overflow"); return n; }
static uint64_t align_up(uint64_t n, uint64_t a) { return u32((n + a - 1) / a * a); }

/* growable byte buffer for the version tree */
typedef struct { unsigned char *b; size_t n, cap; } Buf;
static void bput(Buf *x, const void *p, size_t k) {
    if (x->n + k > x->cap) {
        size_t nc = x->cap ? x->cap * 2 : 256;
        while (nc < x->n + k) nc *= 2;
        x->b = realloc(x->b, nc); if (!x->b) die("out of memory"); x->cap = nc;
    }
    memcpy(x->b + x->n, p, k); x->n += k;
}
static void bpad4(Buf *x) { unsigned char z[4] = {0,0,0,0}; size_t k = (4 - x->n % 4) % 4; if (k) bput(x, z, k); }

/* UTF-16LE of ASCII text plus the terminating NUL unit */
static size_t utf16z(const char *s, unsigned char **dst) {
    size_t n = strlen(s), i;
    *dst = malloc(2 * (n + 1)); if (!*dst) die("out of memory");
    for (i = 0; i < n; i++) { (*dst)[2*i] = (unsigned char)s[i]; (*dst)[2*i+1] = 0; }
    (*dst)[2*n] = 0; (*dst)[2*n+1] = 0;
    return 2 * (n + 1);
}

/* _version_block(key, value, text, children) -> a fresh Buf holding the block */
static Buf version_block(const char *key, const unsigned char *value, size_t vlen, int text, Buf *children, size_t nchild) {
    Buf b = {0,0,0}; unsigned char hdr[6] = {0,0,0,0,0,0};
    unsigned char *kb; size_t klen = utf16z(key, &kb), value_len, i;
    bput(&b, hdr, 6); bput(&b, kb, klen); free(kb);
    bpad4(&b);
    if (vlen) bput(&b, value, vlen);
    for (i = 0; i < nchild; i++) { bpad4(&b); bput(&b, children[i].b, children[i].n); }
    value_len = text ? vlen / 2 : vlen;
    if (b.n > 65535 || value_len > 65535) die("VERSIONINFO uint16 overflow");
    wr16(b.b, 0, (uint32_t)b.n); wr16(b.b, 2, (uint32_t)value_len); wr16(b.b, 4, (uint32_t)(text ? 1 : 0));
    return b;
}

static int version_ok_part(const char *s, size_t n) {
    size_t i; if (n < 1 || n > 5) return 0;
    for (i = 0; i < n; i++) if (s[i] < '0' || s[i] > '9') return 0;
    return 1;
}

/* _version_info(product_name, version): returns a Buf with the VS_VERSION_INFO block */
static Buf version_info(const char *name, const char *version) {
    size_t i, nlen = strlen(name), vlen = strlen(version);
    uint32_t parts[4] = {0,0,0,0}; int ncomp = 0; size_t start = 0;
    uint32_t ms, ls; unsigned char fixed[52]; Buf strs[4], table, sfi, trans, vfi, root;
    unsigned char *vutf; size_t vutf_n; unsigned char *nutf; size_t nutf_n;
    const char *keys[4] = {"ProductName", "ProductVersion", "FileDescription", "FileVersion"};
    const char *vals[4];
    unsigned char tr[4] = {0x09,0x04,0xB0,0x04};
    if (nlen < 1 || nlen > 1024) die("product_name must be bounded, nonempty text without controls");
    for (i = 0; i < nlen; i++) if ((unsigned char)name[i] < 32 || (unsigned char)name[i] > 126) die("product_name must be printable ASCII");
    /* version: 3 or 4 dot-separated decimal parts, 1..5 digits each, each <= 65535 */
    for (i = 0; i <= vlen; i++) {
        if (i == vlen || version[i] == '.') {
            size_t plen = i - start;
            if (!version_ok_part(version + start, plen)) die("version must have three or four decimal uint16 components");
            if (ncomp >= 4) die("version must have three or four decimal uint16 components");
            {
                uint32_t v = 0; size_t k;
                for (k = 0; k < plen; k++) v = v * 10 + (uint32_t)(version[start + k] - '0');
                if (v > 65535) die("version component exceeds uint16");
                parts[ncomp++] = v;
            }
            start = i + 1;
        }
    }
    if (ncomp < 3) die("version must have three or four decimal uint16 components");
    ms = (parts[0] << 16) | parts[1]; ls = (parts[2] << 16) | parts[3];
    wr32(fixed, 0, 0xFEEF04BD); wr32(fixed, 4, 0x10000); wr32(fixed, 8, ms); wr32(fixed, 12, ls);
    wr32(fixed, 16, ms); wr32(fixed, 20, ls); wr32(fixed, 24, 0x3F); wr32(fixed, 28, 0);
    wr32(fixed, 32, 0x40004); wr32(fixed, 36, 1); wr32(fixed, 40, 0); wr32(fixed, 44, 0); wr32(fixed, 48, 0);
    vals[0] = name; vals[1] = version; vals[2] = name; vals[3] = version;
    for (i = 0; i < 4; i++) {
        size_t kl; unsigned char *kk; Buf tmp;
        vutf_n = utf16z(vals[i], &vutf);
        tmp = version_block(keys[i], vutf, vutf_n, 1, NULL, 0);
        strs[i] = tmp; free(vutf);
        (void)kl; (void)kk;
    }
    table = version_block("040904B0", NULL, 0, 1, strs, 4);
    sfi = version_block("StringFileInfo", NULL, 0, 1, &table, 1);
    trans = version_block("Translation", tr, 4, 0, NULL, 0);
    vfi = version_block("VarFileInfo", NULL, 0, 1, &trans, 1);
    {
        Buf kids[2]; kids[0] = sfi; kids[1] = vfi;
        root = version_block("VS_VERSION_INFO", fixed, 52, 0, kids, 2);
    }
    (void)nutf; (void)nutf_n;
    for (i = 0; i < 4; i++) free(strs[i].b);
    free(table.b); free(sfi.b); free(trans.b); free(vfi.b);
    return root;
}

static void load_file(const char *path, unsigned char **buf, size_t *n) {
    FILE *f = fopen(path, "rb"); size_t cap = 1 << 16, len = 0; unsigned char *b;
    if (!f) die("cannot open input");
    b = malloc(cap); if (!b) die("out of memory");
    for (;;) {
        size_t r = fread(b + len, 1, cap - len, f);
        len += r;
        if (len == cap) { if (cap >= MAXIN) die("input too large"); cap *= 2; b = realloc(b, cap); if (!b) die("out of memory"); continue; }
        if (r == 0) break;
    }
    fclose(f); *buf = b; *n = len;
}

static void ensure_out(size_t need) {
    if (need > outcap) {
        size_t nc = outcap ? outcap : 4096;
        while (nc < need) nc *= 2;
        out = realloc(out, nc); if (!out) die("out of memory"); outcap = nc;
    }
}

static void add_version(void) {
    const unsigned char *s = src; size_t L = srclen;
    uint32_t pe, count, opt_size, opt, sects, section_align, file_align, directories, headers, image_size;
    uint32_t initialized;
    uint64_t max_raw, max_virtual, i;
    uint64_t raw_lo[96], raw_hi[96], vr_lo[96], vr_hi[96]; size_t nraw = 0, nvr = 0;
    uint64_t first_raw;
    if (L < 64 || L > U32MAX || s[0] != 'M' || s[1] != 'Z') die("invalid DOS header or file size");
    pe = rd32(s, 60, L);
    if (pe < 64 || (uint64_t)pe + 24 > L || memcmp(s + pe, "PE\0\0", 4) != 0) die("invalid PE signature");
    count = rd16(s, pe + 6, L); opt_size = rd16(s, pe + 20, L);
    opt = pe + 24; sects = opt + opt_size;
    if (!(count >= 1 && count < 96) || opt_size < 152 || (uint64_t)sects + 40ull * count > L) die("invalid PE header bounds");
    if (rd16(s, opt, L) != 0x20B) die("requires PE32+");
    section_align = rd32(s, opt + 32, L); file_align = rd32(s, opt + 36, L);
    if (!(file_align >= 512 && file_align <= 65536 && (file_align & (file_align - 1)) == 0 &&
          section_align >= file_align && section_align <= 0x10000000u && (section_align & (section_align - 1)) == 0))
        die("unsupported section/file alignment");
    directories = rd32(s, opt + 108, L);
    if (!(directories >= 5 && directories <= (opt_size - 112) / 8)) die("invalid data directory count");
    for (i = 2; i <= 4; i += 2) {
        if (rd32(s, opt + 112 + 8 * i, L) != 0 || rd32(s, opt + 112 + 8 * i + 4, L) != 0)
            die("existing resource or certificate directory is forbidden");
    }
    headers = rd32(s, opt + 60, L); image_size = rd32(s, opt + 56, L);
    if (headers > L || headers % file_align || image_size % section_align) die("invalid header/image size alignment");
    max_raw = headers; max_virtual = align_up(headers, section_align);
    for (i = 0; i < count; i++) {
        size_t at = sects + 40 * i;
        char name[9]; uint32_t vsize, rva, rawsize, raw; uint64_t vend;
        memcpy(name, s + at, 8); name[8] = 0;
        if (strcmp(name, ".rsrc") == 0) die("existing resource section is forbidden");
        vsize = rd32(s, at + 8, L); rva = rd32(s, at + 12, L); rawsize = rd32(s, at + 16, L); raw = rd32(s, at + 20, L);
        if (rva % section_align || rva < align_up(headers, section_align)) die("invalid section RVA");
        vend = u32((uint64_t)rva + (vsize > rawsize ? vsize : rawsize));
        if (vend > max_virtual) max_virtual = vend;
        if (vend > rva) { vr_lo[nvr] = rva; vr_hi[nvr] = vend; nvr++; }
        if (rawsize) {
            uint64_t end = u32((uint64_t)raw + rawsize);
            if (raw < headers || raw % file_align || rawsize % file_align || end > L) die("invalid raw section range");
            raw_lo[nraw] = raw; raw_hi[nraw] = end; nraw++;
            if (end > max_raw) max_raw = end;
        }
    }
    /* sort ranges by start (insertion sort, n < 96), then check overlap */
    {
        size_t a, b;
        for (a = 1; a < nraw; a++) for (b = a; b > 0 && raw_lo[b-1] > raw_lo[b]; b--) {
            uint64_t t = raw_lo[b-1]; raw_lo[b-1] = raw_lo[b]; raw_lo[b] = t;
            t = raw_hi[b-1]; raw_hi[b-1] = raw_hi[b]; raw_hi[b] = t;
        }
        for (a = 1; a < nvr; a++) for (b = a; b > 0 && vr_lo[b-1] > vr_lo[b]; b--) {
            uint64_t t = vr_lo[b-1]; vr_lo[b-1] = vr_lo[b]; vr_lo[b] = t;
            t = vr_hi[b-1]; vr_hi[b-1] = vr_hi[b]; vr_hi[b] = t;
        }
        for (a = 1; a < nraw; a++) if (raw_lo[a] < raw_hi[a-1]) die("overlapping sections");
        for (a = 1; a < nvr; a++) if (vr_lo[a] < vr_hi[a-1]) die("overlapping sections");
    }
    if (image_size < align_up(max_virtual, section_align)) die("SizeOfImage truncates sections");
    if ((uint64_t)L != max_raw) die("overlay forbidden: pass un-packed PE head, never complete APE");
    {
        uint64_t new_header = (uint64_t)sects + 40ull * count;
        size_t k;
        first_raw = headers;
        for (k = 0; k < nraw; k++) if (raw_lo[k] < first_raw) first_raw = raw_lo[k];
        if (new_header + 40 > (headers < first_raw ? headers : first_raw)) die("insufficient zero section-header slack");
        for (k = 0; k < 40; k++) if (s[new_header + k] != 0) die("insufficient zero section-header slack");

        (void)0;
        /* the caller supplies name/version via globals set in main */
    }
    (void)initialized;
}

static const char *g_name, *g_version;

int main(int argc, char **argv) {
    Buf info; uint64_t rva, raw, rawsize, new_image; size_t res_len;
    unsigned char resource[88];
    uint32_t pe, opt, count, image_size, initialized, sects_end, new_header;
    uint64_t lenres;
    FILE *f;
    if (argc != 5) { fprintf(stderr, "usage: unisa-peversion IN OUT NAME VERSION\n"); return 2; }
    load_file(argv[1], &src, &srclen);
    g_name = argv[3]; g_version = argv[4];
    /* validate the head first (refuses before any output is staged) */
    add_version();
    /* build the RT_VERSION payload; every refusal above or below exits before OUT is written */
    info = version_info(g_name, g_version);
    pe = rd32(src, 60, srclen); count = rd16(src, pe + 6, srclen);
    opt = pe + 24; image_size = rd32(src, opt + 56, srclen);
    initialized = rd32(src, opt + 8, srclen);
    sects_end = opt + rd16(src, pe + 20, srclen);
    (void)sects_end;
    {
        uint32_t section_align = rd32(src, opt + 32, srclen), file_align = rd32(src, opt + 36, srclen);
        uint32_t headers = rd32(src, opt + 60, srclen);
        uint64_t max_virtual = align_up(headers, section_align);
        uint32_t i;
        for (i = 0; i < count; i++) {
            size_t at = opt + rd16(src, pe + 20, srclen) + 40 * i;
            uint32_t vsize = rd32(src, at + 8, srclen), rva0 = rd32(src, at + 12, srclen), rawsz = rd32(src, at + 16, srclen);
            uint64_t vend = (uint64_t)rva0 + (vsize > rawsz ? vsize : rawsz);
            if (vend > max_virtual) max_virtual = vend;
        }
        rva = align_up(image_size > max_virtual ? image_size : max_virtual, section_align);
        raw = align_up(srclen, file_align);
        /* resource directory: three ID-only directories RT_VERSION / 1 / 0409 */
        memset(resource, 0, sizeof resource);
        {
            static const struct { size_t at; uint32_t id; uint32_t target; int dir; } ent[3] = {
                {0, 16, 24, 1}, {24, 1, 48, 1}, {48, 0x0409, 72, 0} };
            size_t k;
            for (k = 0; k < 3; k++) {
                wr16(resource, ent[k].at + 12, 0); wr16(resource, ent[k].at + 14, 1);
                wr32(resource, ent[k].at + 16, ent[k].id);
                wr32(resource, ent[k].at + 20, ent[k].target | (ent[k].dir ? 0x80000000u : 0));
            }
        }
        wr32(resource, 72, (uint32_t)(rva + 88)); wr32(resource, 76, (uint32_t)info.n);
        wr32(resource, 80, 1200); wr32(resource, 84, 0);
        res_len = 88 + info.n;
        rawsize = align_up(res_len, file_align);
        lenres = rva + res_len;
        new_image = align_up(lenres, section_align);
        (void)new_image;
        /* stage the output in memory; write to OUT only at the end */
        ensure_out(raw + rawsize);
        memset(out, 0, raw + rawsize);
        memcpy(out, src, srclen);
        {
            unsigned char *o = out;
            size_t p;
            /* resource bytes: header 88 then the version tree */
            unsigned char *rsrc = malloc(res_len); if (!rsrc) die("out of memory");
            memcpy(rsrc, resource, 88); memcpy(rsrc + 88, info.b, info.n);
            memcpy(o + raw, rsrc, res_len);
            free(rsrc);
            p = 0; (void)p;
            wr16(o, pe + 6, count + 1);
            wr32(o, opt + 8, (uint32_t)(initialized + rawsize));
            wr32(o, opt + 56, (uint32_t)new_image);
            wr32(o, opt + 64, 0);
            wr32(o, opt + 112 + 16, (uint32_t)rva);
            wr32(o, opt + 112 + 20, (uint32_t)res_len);
            new_header = opt + rd16(src, pe + 20, srclen) + 40 * count;
            /* section header: name .rsrc, vsize, rva, rawsize, raw, reloc 0, lineno 0, counts 0, flags */
            memcpy(o + new_header, ".rsrc\0\0\0", 8);
            wr32(o, new_header + 8, (uint32_t)res_len);
            wr32(o, new_header + 12, (uint32_t)rva);
            wr32(o, new_header + 16, (uint32_t)rawsize);
            wr32(o, new_header + 20, (uint32_t)raw);
            wr32(o, new_header + 24, 0); wr32(o, new_header + 28, 0);
            wr16(o, new_header + 32, 0); wr16(o, new_header + 34, 0);
            wr32(o, new_header + 36, 0x40000040);
        }
    }
    f = fopen(argv[2], "wb");
    if (!f) die("cannot open output");
    if (fwrite(out, 1, raw + rawsize, f) != raw + rawsize) { fclose(f); die("short write"); }
    fclose(f);
    free(info.b); free(src);
    return 0;
}
