/* Bounded format/IO locator, not signature authentication. No magic scan.
 * packagefooter_locate(FILE *, PackageFooter *) returns 1 on success, 0 on
 * malformed/unsupported input or IO error. Output is zeroed on failure;
 * stream position is unspecified. Offsets fit the host fseek/ftell long.
 * packagefooter_locate_bytes(const unsigned char *, long, PackageFooter *)
 * uses the same algorithm over an immutable span; it never opens a file.
 * Caller owns FILE/span and validates/parses the returned package separately.
 * Output pointer must be non-null; the span must contain size valid bytes. */
#ifndef UNISA_PACKAGEFOOTER_H
#define UNISA_PACKAGEFOOTER_H
#include <stdio.h>
typedef struct {
    long file_size, footer_offset, package_offset, package_length;
    long certificate_offset, certificate_length;
    int padding;
} PackageFooter;
typedef struct {
    FILE *file;
    const unsigned char *data;
    long size;
} PfReader;
static int pf_read(PfReader *r, long size, long off, unsigned char *b, long n) {
    long i;
    if (off < 0 || n < 0 || off > size || n > size - off) return 0;
    if (r->file) {
        if (fseek(r->file, off, SEEK_SET)) return 0;
        return (long)fread(b, 1, n, r->file) == n;
    }
    if (!r->data) return 0;
    i = 0;
    while (i < n) { b[i] = r->data[off + i]; i = i + 1; }
    return 1;
}
/* Accumulate against an existing file bound, never overflow signed long. */
static int pf_number(unsigned char *b, int n, long limit, long *out) {
    long v = 0; int i = n - 1;
    while (i >= 0) {
        if (v > limit / 256) return 0;
        v = v * 256;
        if (b[i] > limit - v) return 0;
        v = v + b[i]; i = i - 1;
    }
    *out = v; return 1;
}
static int pf_zero(PfReader *r, long size, long off, long n) {
    unsigned char b[8]; long i;
    if (n > 7 || !pf_read(r, size, off, b, n)) return 0;
    i = 0; while (i < n) { if (b[i]) return 0; i = i + 1; }
    return 1;
}
static int pf_locate(PfReader *r, PackageFooter *out) {
    unsigned char b[64]; unsigned char foot[16];
    long size, pe, opt, optsize, dirs, count, sec, sects, minoff, headers;
    long cert = 0, certlen = 0, end, pos, len, padded, raw, rawlen;
    long footer = 0, packlen = 0; int pad, found = 0;
    out->file_size = 0; out->footer_offset = 0;
    out->package_offset = 0; out->package_length = 0;
    out->certificate_offset = 0; out->certificate_length = 0; out->padding = 0;
    size = r->size; if (size < 16) return 0;
    if (!r->file && !r->data) return 0;
    minoff = 0;
    if (!pf_read(r, size, 0, b, 2)) return 0;
    if (b[0] == 77 && b[1] == 90) {
        if (!pf_read(r, size, 0, b, 64)) return 0;
        if (!pf_number(b + 60, 4, size, &pe) || pe < 64) return 0;
        if (!pf_read(r, size, pe, b, 24)) return 0;
        if (b[0] != 80 || b[1] != 69 || b[2] || b[3]) return 0;
        if (!pf_number(b + 6, 2, size, &sects) || !sects) return 0;
        if (!pf_number(b + 20, 2, size - pe - 24, &optsize)) return 0;
        opt = pe + 24;
        if (optsize < 2 || !pf_read(r, size, opt, b, 2)) return 0;
        dirs = 0;
        if (b[0] == 11 && b[1] == 1) dirs = 96;
        if (b[0] == 11 && b[1] == 2) dirs = 112;
        if (!dirs || optsize < dirs) return 0;
        if (!pf_read(r, size, opt + dirs - 4, b, 4)) return 0;
        if (!pf_number(b, 4, (optsize - dirs) / 8, &count) || count < 5) return 0;
        if (!pf_read(r, size, opt + dirs + 32, b, 8)) return 0;
        if (!pf_number(b, 4, size, &cert) || !pf_number(b + 4, 4, size, &certlen)) return 0;
        if ((!cert && certlen) || (cert && !certlen)) return 0;
        if (cert) { if (cert % 8 || certlen < 8 || certlen != size - cert) return 0; }
        sec = opt + optsize;
        if (sects > (size - sec) / 40) return 0;
        minoff = sec + sects * 40;
        if (!pf_read(r, size, opt + 60, b, 4)) return 0;
        if (!pf_number(b, 4, size, &raw) || raw < minoff) return 0;
        minoff = raw; headers = raw;
        while (sects > 0) {
            if (!pf_read(r, size, sec, b, 40)) return 0;
            if (!pf_number(b + 16, 4, size, &rawlen) || !pf_number(b + 20, 4, size, &raw)) return 0;
            if (rawlen) {
                if (raw < headers || rawlen > size - raw) return 0;
                /* Sections are allowed in arbitrary order. */
                if (raw + rawlen > minoff) minoff = raw + rawlen;
            }
            sec = sec + 40; sects = sects - 1;
        }
        if (cert && cert < minoff) return 0;
    }
    if (cert) {
        pos = cert;
        while (pos < size) {
            if (!pf_read(r, size, pos, b, 8)) return 0;
            if (!pf_number(b, 4, size - pos, &len) || len < 8) return 0;
            if (b[4] != 0 || b[5] != 2 || b[6] != 2 || b[7] != 0) return 0;
            padded = (8 - len % 8) % 8;
            if (padded > size - pos - len || !pf_zero(r, size, pos + len, padded)) return 0;
            pos = pos + len + padded;
        }
        if (pos != size) return 0;
        end = cert;
    } else end = size;
    pad = 0;
    while (pad <= (cert ? 7 : 0)) {
        pos = end - pad - 16;
        if (pos >= minoff && pf_zero(r, size, end - pad, pad) && pf_read(r, size, pos, foot, 16)) {
            if (foot[0] == 85 && foot[1] == 78 && foot[2] == 73 && foot[3] == 80 &&
                foot[4] == 75 && foot[5] == 71 && foot[6] == 49 && foot[7] == 10) {
                if (!pf_number(foot + 8, 8, pos - minoff, &len) || !len) return 0;
                found = found + 1; footer = pos; packlen = len;
            }
        }
        pad = pad + 1;
    }
    if (found != 1) return 0;
    out->file_size = size; out->footer_offset = footer;
    out->package_offset = footer - packlen; out->package_length = packlen;
    out->certificate_offset = cert; out->certificate_length = certlen;
    out->padding = end - footer - 16; return 1;
}
static int packagefooter_locate(FILE *f, PackageFooter *out) {
    PfReader r;
    r.file = f; r.data = 0; r.size = -1;
    if (f) { if (!fseek(f, 0, SEEK_END)) r.size = ftell(f); }
    return pf_locate(&r, out);
}
static int packagefooter_locate_bytes(const unsigned char *data, long size, PackageFooter *out) {
    PfReader r;
    r.file = 0; r.data = data; r.size = size;
    return pf_locate(&r, out);
}
#endif
