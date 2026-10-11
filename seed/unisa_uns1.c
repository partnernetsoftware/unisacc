/* seed/unisa_uns1.c -- C99 reference for unisa.control.uns1 (K6-7), point-sample functions.
 *
 *   unisa-uns1 dumps DTYPE FLAGS ACC SPEC     SPEC lines: "name rows cols v1 ... vN" (N = rows*cols);
 *                                             print the blob as lowercase hex on one line
 *                                             DTYPE: 0 = i8, 2 = f32 (other dtypes refused)
 *   unisa-uns1 loads HEX                      print the header and every tensor:
 *                                               HEADER dtype flags nt nparams acc
 *                                               T name rows cols scale
 *                                               W value           (one per weight, "%.17g")
 *   unisa-uns1 pad4 N                         print "PAD4 N LEN" (len(_pad4(bytes(N))))
 *
 * Mirrors unisa/control/uns1.py: header 16 B (MAGIC, '<BBHIf'); tensor name[16] + '<HHf' + payload
 * padded to 4; _quant_i8 (maxabs/127, round-half-even via rint, clamp +-127, two's complement byte);
 * f32 payload as little-endian floats; loads asserts MAGIC and refuses short reads.
 * Refusals (both sides) by name, nothing printed for the refused command.  Point-sample equivalence
 * over the listed fixtures only; not a product closure member; keeps uns1.py.  Builds with
 * -ffp-contract=off.
 */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void die(const char *why, const char *what) {
    fprintf(stderr, "unisa-uns1: %s %s\n", why, what ? what : "");
    exit(1);
}

typedef struct { char name[17]; uint32_t rows, cols; double *w; size_t n; } Tensor;

static unsigned char *obuf; static size_t olen, ocap;
static void put(const void *p, size_t k) {
    if (olen + k > ocap) { size_t nc = ocap ? ocap * 2 : 4096; while (nc < olen + k) nc *= 2;
        obuf = realloc(obuf, nc); if (!obuf) die("out of memory", NULL); ocap = nc; }
    memcpy(obuf + olen, p, k); olen += k;
}
static void put_u8(uint8_t v) { put(&v, 1); }
static void put_u16(uint16_t v) { unsigned char b[2] = { (unsigned char)(v & 255), (unsigned char)(v >> 8) }; put(b, 2); }
static void put_u32(uint32_t v) { unsigned char b[4] = { (unsigned char)(v & 255), (unsigned char)((v >> 8) & 255),
    (unsigned char)((v >> 16) & 255), (unsigned char)(v >> 24) }; put(b, 4); }
static void put_f32(double x) { float f = (float)x; uint32_t u; memcpy(&u, &f, 4); put_u32(u); }
static void pad4_zero(size_t raw_len) { size_t k = (4 - raw_len % 4) % 4; unsigned char z[4] = {0,0,0,0}; if (k) put(z, k); }

static int hexval(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static void dumps_cmd(int dtype, unsigned flags, double acc, const char *spec) {
    FILE *f = fopen(spec, "r"); char line[1 << 16]; Tensor *ts = NULL; size_t nt = 0, cap = 0, i;
    uint32_t nparams = 0;
    if (dtype != 0 && dtype != 2) die("dtype not in the sampled set (i8, f32)", NULL);
    if (!f) die("cannot open spec", spec);
    while (fgets(line, sizeof line, f)) {
        char *tok[1 << 14]; int nk = 0, k; char *p = line; char *end;
        size_t L = strlen(line); if (L && line[L-1] == '\n') line[--L] = 0;
        if (!L || line[0] == '#') continue;
        while (*p) {
            while (*p == ' ' || *p == '\t') p++;
            if (!*p) break;
            if (nk >= (1 << 14)) die("too many tokens", NULL);
            tok[nk++] = p;
            while (*p && *p != ' ' && *p != '\t') p++;
            if (*p) *p++ = 0;
        }
        if (nk < 3) die("spec line needs name rows cols", NULL);
        if (nt == cap) { cap = cap ? cap * 2 : 8; ts = realloc(ts, cap * sizeof *ts); if (!ts) die("out of memory", NULL); }
        memset(&ts[nt], 0, sizeof ts[nt]);
        strncpy(ts[nt].name, tok[0], 16); ts[nt].name[16] = 0;
        ts[nt].rows = (uint32_t)strtoul(tok[1], &end, 10); if (*end) die("bad rows", tok[1]);
        ts[nt].cols = (uint32_t)strtoul(tok[2], &end, 10); if (*end) die("bad cols", tok[2]);
        ts[nt].n = (size_t)ts[nt].rows * ts[nt].cols;
        if ((size_t)nk != 3 + ts[nt].n) die("spec value count != rows*cols", tok[0]);
        ts[nt].w = malloc((ts[nt].n ? ts[nt].n : 1) * sizeof(double)); if (!ts[nt].w) die("out of memory", NULL);
        for (k = 0; k < (int)ts[nt].n; k++) {
            ts[nt].w[k] = strtod(tok[3 + k], &end); if (*end) die("bad weight", tok[3 + k]);
        }
        nparams += (uint32_t)ts[nt].n;
        nt++;
    }
    fclose(f);
    /* header: MAGIC, dtype, flags, nTensors, nParams, acc */
    put("UNS1", 4); put_u8((uint8_t)dtype); put_u8((uint8_t)flags); put_u16((uint16_t)nt);
    put_u32(nparams); put_f32(acc);
    for (i = 0; i < nt; i++) {
        Tensor *t = &ts[i]; size_t raw, j; double scale = 1.0; unsigned char nm[16];
        memset(nm, 0, sizeof nm); memcpy(nm, t->name, strlen(t->name) > 16 ? 16 : strlen(t->name));
        if (dtype == 2) {
            scale = 1.0;
            put(nm, 16); put_u16((uint16_t)t->rows); put_u16((uint16_t)t->cols); put_f32(scale);
            for (j = 0; j < t->n; j++) put_f32(t->w[j]);
            raw = 4 * t->n;
        } else {   /* i8: _quant_i8 */
            double m = 0.0, s, inv; size_t q;
            for (j = 0; j < t->n; j++) { double a = fabs(t->w[j]); if (a > m) m = a; }
            s = (m > 0) ? (m / 127.0) : 1.0;
            inv = 1.0 / s;
            put(nm, 16); put_u16((uint16_t)t->rows); put_u16((uint16_t)t->cols); put_f32(s);
            for (q = 0; q < t->n; q++) {
                double r = rint(t->w[q] * inv);
                long v = (long)r;
                if (v < -127) v = -127; else if (v > 127) v = 127;
                put_u8((uint8_t)(v & 0xFF));
            }
            raw = t->n;
        }
        pad4_zero(raw);
        free(t->w);
    }
    free(ts);
    {
        size_t k;
        for (k = 0; k < olen; k++) printf("%02x", obuf[k]);
        printf("\n");
    }
    olen = 0;
}

static char *sbuf; static size_t slen, scap;
static void outf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
#include <stdarg.h>
static void outf(const char *fmt, ...) {
    va_list ap; int k;
    va_start(ap, fmt); k = vsnprintf(NULL, 0, fmt, ap); va_end(ap);
    if (k < 0) die("format", NULL);
    if (slen + (size_t)k + 1 > scap) { size_t nc = scap ? scap * 2 : 4096; while (nc < slen + (size_t)k + 1) nc *= 2;
        sbuf = realloc(sbuf, nc); if (!sbuf) die("out of memory", NULL); scap = nc; }
    va_start(ap, fmt); vsnprintf(sbuf + slen, scap - slen, fmt, ap); va_end(ap);
    slen += (size_t)k;
}

static unsigned char *from_hex(const char *h, size_t *n) {
    size_t hn = strlen(h), k; unsigned char *b;
    if (hn % 2) die("odd-length hex", NULL);
    b = malloc(hn / 2 + 1); if (!b) die("out of memory", NULL);
    for (k = 0; k < hn / 2; k++) {
        int a = hexval(h[2*k]), c = hexval(h[2*k+1]);
        if (a < 0 || c < 0) { free(b); die("non-hex digit", NULL); }
        b[k] = (unsigned char)(a * 16 + c);
    }
    *n = hn / 2; return b;
}

static uint32_t rd_u16(const unsigned char *p, size_t off, size_t L) { if (off + 2 > L) die("short read", NULL); return p[off] | (p[off+1] << 8); }
static uint32_t rd_u32(const unsigned char *p, size_t off, size_t L) { if (off + 4 > L) die("short read", NULL);
    return (uint32_t)p[off] | ((uint32_t)p[off+1] << 8) | ((uint32_t)p[off+2] << 16) | ((uint32_t)p[off+3] << 24); }
static float as_f32(uint32_t u) { float f; memcpy(&f, &u, 4); return f; }

static void loads_cmd(const char *hex) {
    size_t L, off, i; unsigned char *b = from_hex(hex, &L);
    uint8_t dtype, flags; uint16_t nt; uint32_t nparams, acc_u; float acc;
    if (L < 16) die("short header", NULL);
    if (memcmp(b, "UNS1", 4) != 0) die("not UNS1", NULL);
    dtype = b[4]; flags = b[5]; nt = (uint16_t)rd_u16(b, 6, L); nparams = rd_u32(b, 8, L);
    acc_u = rd_u32(b, 12, L); acc = as_f32(acc_u);
    outf("HEADER %d %d %d %u %.17g\n", dtype, flags, nt, nparams, (double)acc);
    off = 16;
    for (i = 0; i < nt; i++) {
        char name[17]; uint32_t rows, cols, u; float scale; size_t n, raw, j;
        if (off + 24 > L) die("short tensor header", NULL);
        memcpy(name, b + off, 16); name[16] = 0;
        { size_t q = 0; while (q < 16 && name[q]) q++; name[q] = 0; }
        rows = rd_u16(b, off + 16, L); cols = rd_u16(b, off + 18, L); u = rd_u32(b, off + 20, L); scale = as_f32(u);
        off += 24;
        n = (size_t)rows * cols;
        outf("T %s %u %u %.17g\n", name, rows, cols, (double)scale);
        if (dtype == 2) {
            if (off + 4 * n > L) die("short f32 payload", NULL);
            for (j = 0; j < n; j++) outf("W %.17g\n", (double)as_f32(rd_u32(b, off + 4 * j, L)));
            raw = 4 * n;
        } else if (dtype == 0) {
            if (off + n > L) die("short i8 payload", NULL);
            for (j = 0; j < n; j++) outf("W %.17g\n", (double)(int8_t)b[off + j] * (double)scale);
            raw = n;
        } else die("dtype not in the sampled set (i8, f32)", NULL);
        off += raw + ((4 - raw % 4) % 4);
    }
    free(b);
    fputs(sbuf, stdout);
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: unisa-uns1 dumps DTYPE FLAGS ACC SPEC | loads HEX | pad4 N\n"); return 2; }
    if (!strcmp(argv[1], "dumps") && argc == 6) {
        dumps_cmd((int)strtol(argv[2], NULL, 10), (unsigned)strtoul(argv[3], NULL, 10), strtod(argv[4], NULL), argv[5]);
        return 0;
    }
    if (!strcmp(argv[1], "loads") && argc == 3) { loads_cmd(argv[2]); return 0; }
    if (!strcmp(argv[1], "pad4") && argc == 3) {
        unsigned long n = strtoul(argv[2], NULL, 10);
        printf("PAD4 %lu %lu\n", n, n + (4 - n % 4) % 4);
        return 0;
    }
    fprintf(stderr, "usage: unisa-uns1 dumps DTYPE FLAGS ACC SPEC | loads HEX | pad4 N\n");
    return 2;
}
