/* seed/unisa_tapebin.c -- C99 reference for unisa.tapebin (K6-11), point-sample functions.
 *
 *   unisa-tapebin encode SCRIPT ORIGIN SHA|-
 *       SCRIPT lines, fields separated by blanks (blank lines skipped):
 *         U                       records.append(("unit", 2))
 *         G KIND NAME             records.append((KIND, NAME)), KIND in global extern gdef
 *         L NAME                  Tape.label(NAME)
 *         E OP ARG...             Tape.emit(OP, ARG...)   ARG: i = int(x,0), r/s/L = the token
 *         S NAME HEX ALIGN        Tape.string(NAME, bytes.fromhex(HEX), align=ALIGN)
 *         B NAME N                Tape.string(NAME, N, align=8)
 *       prints encode(tape, origin_target=ORIGIN, source_sha=SHA) as one lowercase hex line.
 *   unisa-tapebin decode HEXFILE
 *       prints "ORIGIN n", then "SHA hex" when section 4 is present, then to_canonical_text().
 *   unisa-tapebin target NAME              target_id(NAME), a decimal line
 *   unisa-tapebin checkorigin ORIGIN NAME FORCE(0|1)    check_origin(...): prints OK
 *
 * Mirrors unisa/tapebin.py: canonical ULEB128/SLEB128 (_u/_s and _Reader), name and constant pools,
 * _collect, _encode_records, encode (sections, header, SHA-256 digest), decode (header, directory,
 * section layout and padding, pools, records, re-encode comparison), target_id, check_origin.
 * The operation table is the non-floating-point part of tape.SHAPE in the same order.
 * Refusals print the reason on stderr and nothing on stdout.
 *
 * Not covered (refused or not claimed; never accepted with different output):
 *  - the fp ops (opcodes past the non-fp table);
 *  - decode does not repeat the python text round trip (parse(to_canonical_text()) == records); the C
 *    side instead refuses every pool name outside [A-Za-z0-9_.$]+, which python may accept;
 *  - an L/s operand stored as an integer is refused here (python refuses it through the text check);
 *  - the 1 GiB size cap is compared, but no sample reaches it.
 * Point-sample equivalence over the listed samples only; not a product closure member; keeps
 * unisa/tapebin.py and unisa/tape.py.  No floating point here.
 */
#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXTOK 64

static void die(const char *why, const char *what) {
    fprintf(stderr, "unisa-tapebin: %s %s\n", why, what ? what : "");
    exit(1);
}

/* ---- growable byte string ---- */
typedef struct { unsigned char *p; size_t n, cap; } Str;
static void reserve(Str *s, size_t extra) {
    if (s->n + extra + 1 > s->cap) {
        size_t nc = s->cap ? s->cap * 2 : 256;
        while (nc < s->n + extra + 1) nc *= 2;
        s->p = realloc(s->p, nc); if (!s->p) die("out of memory", NULL); s->cap = nc;
    }
}
static void sadd(Str *s, const void *p, size_t k) { reserve(s, k); if (k) memcpy(s->p + s->n, p, k); s->n += k; }
static void sputs(Str *s, const char *c) { sadd(s, c, strlen(c)); }
static void sputc(Str *s, unsigned char c) { sadd(s, &c, 1); }
static void sputf(Str *s, const char *fmt, ...) {
    va_list ap; int k;
    va_start(ap, fmt); k = vsnprintf(NULL, 0, fmt, ap); va_end(ap);
    if (k < 0) die("format", NULL);
    reserve(s, (size_t)k);
    va_start(ap, fmt); vsnprintf((char *)s->p + s->n, (size_t)k + 1, fmt, ap); va_end(ap);
    s->n += (size_t)k;
}
static char *dupn(const char *p, size_t n) {
    char *r = malloc(n + 1); if (!r) die("out of memory", NULL);
    memcpy(r, p, n); r[n] = 0; return r;
}
static void flush_out(Str *o) { if (o->n) fwrite(o->p, 1, o->n, stdout); }

/* ---- SHA-256 (FIPS 180-4); K and H0 are the fractional parts of the cube and square roots of the
 *      first primes, checked against python's hashlib when tests/k6/tapebin.tsv was captured ---- */
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

/* ---- operation table: the non-fp part of tape.SHAPE, same order (opcode = index) ---- */
typedef struct { const char *op, *shape; } OpShape;
static const OpShape SH[] = {
    {"imm", "ri"}, {"mov", "rr"}, {"add64", "rrr"}, {"sub64", "rrr"}, {"mul64", "rrr"},
    {"xor64", "rrr"}, {"and64", "rrr"}, {"or64", "rrr"}, {"shl64", "rrr"}, {"shr64", "rrr"},
    {"lshr64", "rrr"}, {".div", "rrr"}, {".mod", "rrr"}, {".udiv", "rrr"}, {".umod", "rrr"},
    {"slt64", "rrr"}, {"sle64", "rrr"}, {"ult64", "rrr"}, {"ule64", "rrr"}, {"eq", "rrr"}, {"ne", "rrr"},
    {"load64", "rri"}, {"store64", "rir"}, {".ld", "rrii"}, {".st", "riri"}, {".lea", "rs"},
    {".zero", "rii"}, {"jump", "L"}, {"jumpz", "rL"}, {"call", "L"}, {"callr", "r"}, {"callm", "ri"},
    {".hostcall", "rr"}, {".librarycall", "rr"}, {".libraryaddr", "rsiiii"}, {".libraryfn", "ri"}, {".hostaddr", "ri"},
    {"ret", ""}, {".frame", "i"}, {".arg", "ir"}, {".print", "r"}, {".write", "rr"}, {".exit", "r"},
    {".sys", "srrr"}, {".sys6", "srrrrrr"}, {".argc", "r"}, {".argv", "rr"}, {"nop", ""},
};
#define NSH ((int)(sizeof SH / sizeof SH[0]))
static int op_index(const char *op) {
    int i;
    for (i = 0; i < NSH; i++) if (!strcmp(SH[i].op, op)) return i;
    return -1;
}
static int is_reg(const char *t) { return strlen(t) == 2 && t[0] == 'r' && t[1] >= '0' && t[1] <= '7'; }

static const char *TARGETS[] = {"lnx/x86_64", "lnx/arm64", "osx/x86_64", "osx/arm64", "win/x86_64", "win/arm64"};
#define NTGT ((int)(sizeof TARGETS / sizeof TARGETS[0]))

static int hexval(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
/* python int(s, 0) when base0 (sign, 0x, no nonzero leading zero) or int(s) when !base0; int64 range */
static int int_lit(const char *s, int base0, long long *out) {
    const char *p = s; int neg = 0; unsigned long long v = 0;
    if (*p == '+' || *p == '-') { neg = *p == '-'; p++; }
    if (base0 && p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        p += 2; if (!*p) return 0;
        for (; *p; p++) {
            int d = hexval((unsigned char)*p);
            if (d < 0 || v > (ULLONG_MAX >> 4)) return 0;
            v = v * 16 + (unsigned)d;
        }
    } else {
        if (!*p) return 0;
        if (base0 && p[0] == '0' && strspn(p, "0") != strlen(p)) return 0;
        for (; *p; p++) {
            if (!isdigit((unsigned char)*p) || v > (ULLONG_MAX - 9) / 10) return 0;
            v = v * 10 + (unsigned)(*p - '0');
        }
    }
    if (neg ? v > (1ull << 63) : v > (1ull << 63) - 1) return 0;
    *out = neg ? (long long)(0 - v) : (long long)v;
    return 1;
}

/* ---- records (Tape.records) ---- */
typedef struct { int isint, isbig; long long v; unsigned long long u; char *s; } Arg;
typedef struct { int op; int n; Arg a[7]; } Insn;
enum { R_LABEL, R_INSN, R_STR, R_BSS, R_UNIT, R_GLOBAL, R_EXTERN, R_GDEF };
typedef struct { int kind; char *name; Str blob; unsigned long long num; Insn *in; } Rec;

static Rec *recs; static size_t nrec, caprec;
static char **symnames; static size_t nsym, capsym;   /* Tape.syms keys, for the early return on a known name */

static Rec *push_rec(int kind) {
    Rec *r;
    if (nrec == caprec) { caprec = caprec ? caprec * 2 : 64; recs = realloc(recs, caprec * sizeof *recs); if (!recs) die("out of memory", NULL); }
    r = &recs[nrec++]; memset(r, 0, sizeof *r); r->kind = kind; return r;
}
static int has_sym(const char *name) {
    size_t i;
    for (i = 0; i < nsym; i++) if (!strcmp(symnames[i], name)) return 1;
    return 0;
}
static void add_sym(const char *name) {
    if (nsym == capsym) { capsym = capsym ? capsym * 2 : 64; symnames = realloc(symnames, capsym * sizeof *symnames); if (!symnames) die("out of memory", NULL); }
    symnames[nsym++] = dupn(name, strlen(name));
}

/* Tape.emit: arity (python assert) and integer operands (python int()) checked; registers are not */
static void t_emit(const char *op, int ntok, char **tok) {
    const char *sh; int i; Insn *in; Rec *r; int oi = op_index(op);
    if (oi < 0) die("unknown tape op (python: assert/KeyError)", op);
    sh = SH[oi].shape;
    if ((int)strlen(sh) != ntok) die("operand count (python: assert)", op);
    in = calloc(1, sizeof *in); if (!in) die("out of memory", NULL);
    in->op = oi; in->n = ntok;
    for (i = 0; i < ntok; i++) {
        if (sh[i] == 'i') {
            if (!int_lit(tok[i], 1, &in->a[i].v)) die("not an integer (python: ValueError)", tok[i]);
            in->a[i].isint = 1;
        } else {
            in->a[i].s = dupn(tok[i], strlen(tok[i]));
        }
    }
    r = push_rec(R_INSN); r->in = in;
}

/* Tape.string: a known name returns early; zero-by-size globals and all-zero blobs > 16 are .bss */
static void t_string(const char *name, const unsigned char *raw, size_t n, int zero_by_size) {
    size_t i; int allzero = 1; Rec *r;
    if (has_sym(name)) return;
    add_sym(name);
    if (zero_by_size && n > 16) {
        r = push_rec(R_BSS); r->name = dupn(name, strlen(name)); r->num = n;
        return;
    }
    for (i = 0; i < n; i++) if (raw[i]) allzero = 0;
    r = push_rec((n > 16 && allzero) ? R_BSS : R_STR);
    r->name = dupn(name, strlen(name)); r->num = n;
    if (n) sadd(&r->blob, raw, n);
}

/* ---- canonical text (Tape.to_canonical_text) ---- */
static void quote(Str *o, const unsigned char *b, size_t n) {
    size_t i;
    sputc(o, '"');
    for (i = 0; i < n; i++) {
        unsigned char c = b[i];
        if (c == '\n') sputs(o, "\\n");
        else if (c == '\t') sputs(o, "\\t");
        else if (c == '\r') sputs(o, "\\r");
        else if (c == 0) sputs(o, "\\0");
        else if (c == '\\') sputs(o, "\\\\");
        else if (c == '"') sputs(o, "\\\"");
        else if (c >= 32 && c < 127) sputc(o, c);
        else sputf(o, "\\x%02x", c);
    }
    sputc(o, '"');
}
static void fmt_insn(Str *o, const Insn *in) {
    const char *op = SH[in->op].op; const Arg *a = in->a; size_t start = o->n; int i;
    if (!strcmp(op, "callm")) sputf(o, "%-7s [%s%+lld]", op, a[0].s, a[1].v);
    else if (!strcmp(op, "load64") || !strcmp(op, ".ld")) {
        sputf(o, "%-7s %s, [%s%+lld]", op, a[0].s, a[1].s, a[2].v);
        if (!strcmp(op, ".ld")) sputf(o, ", %lld", a[3].v);
    } else if (!strcmp(op, "store64") || !strcmp(op, ".st")) {
        sputf(o, "%-7s [%s%+lld], %s", op, a[0].s, a[1].v, a[2].s);
        if (!strcmp(op, ".st")) sputf(o, ", %lld", a[3].v);
    } else if (!strcmp(op, ".zero")) sputf(o, "%-7s [%s%+lld], %lld", op, a[0].s, a[1].v, a[2].v);
    else {
        sputf(o, "%-7s ", op);
        for (i = 0; i < in->n; i++) {
            if (i) sputs(o, ", ");
            if (a[i].isint) { if (a[i].isbig) sputf(o, "%llu", a[i].u); else sputf(o, "%lld", a[i].v); }
            else sputs(o, a[i].s);
        }
        while (o->n > start && isspace(o->p[o->n - 1])) o->n--;   /* python rstrip() */
    }
}
static void canonical(Str *o) {
    size_t i;
    for (i = 0; i < nrec; i++) {
        Rec *r = &recs[i];
        switch (r->kind) {
        case R_UNIT: sputs(o, ".unit 2\n"); break;
        case R_GLOBAL: sputf(o, ".global %s\n", r->name); break;
        case R_EXTERN: sputf(o, ".extern %s\n", r->name); break;
        case R_GDEF: sputf(o, ".gdef %s\n", r->name); break;
        case R_LABEL: sputf(o, "%s:\n", r->name); break;
        case R_STR: sputf(o, ".str %s ", r->name); quote(o, r->blob.p, r->blob.n); sputc(o, '\n'); break;
        case R_BSS: sputf(o, ".bss %s %llu\n", r->name, r->num); break;
        case R_INSN: sputs(o, "  "); fmt_insn(o, r->in); sputc(o, '\n'); break;
        }
    }
}

/* ---- canonical integers (tapebin._u / _s) ---- */
static void put_u(Str *o, uint64_t v) {
    while (v >= 128) { sputc(o, (unsigned char)((v & 127) | 128)); v >>= 7; }
    sputc(o, (unsigned char)v);
}
static void put_s(Str *o, int64_t v) {
    for (;;) {
        unsigned char byte = (unsigned char)(v & 127);
        int done;
        v >>= 7;
        done = (v == 0 && byte < 64) || (v == -1 && byte >= 64);
        sputc(o, done ? byte : (unsigned char)(byte | 128));
        if (done) return;
    }
}

/* ---- reader (tapebin._Reader); the canonical check re-encodes the value and compares the bytes ---- */
typedef struct { const unsigned char *p; size_t n, pos; } Rd;
static const unsigned char *rd_take(Rd *r, uint64_t count) {
    const unsigned char *q;
    if (count > r->n - r->pos) die("truncated input", NULL);
    q = r->p + r->pos; r->pos += (size_t)count; return q;
}
static unsigned char rd_byte(Rd *r) { return *rd_take(r, 1); }
static uint64_t rd_uint(Rd *r) {
    size_t start = r->pos; uint64_t v = 0; int shift; Str chk = {0}; uint64_t out;
    for (shift = 0; shift < 70; shift += 7) {
        unsigned char b = rd_byte(r);
        if (shift < 64) v |= (uint64_t)(b & 127) << shift;
        if (b < 128) {
            if (shift == 63 && (b & 127) > 1) die("ULEB128 out of range", NULL);
            put_u(&chk, v);
            if (chk.n != r->pos - start || memcmp(chk.p, r->p + start, chk.n)) die("noncanonical ULEB128", NULL);
            out = v; free(chk.p); return out;
        }
    }
    die("ULEB128 overflow", NULL);
    return 0;
}
static int64_t rd_sint(Rd *r) {
    size_t start = r->pos; uint64_t v = 0; int shift; Str chk = {0}; int64_t iv;
    for (shift = 0; shift < 70; shift += 7) {
        unsigned char b = rd_byte(r);
        if (shift < 64) v |= (uint64_t)(b & 127) << shift;
        if (b < 128) {
            if ((b & 64) && shift + 7 < 64) v |= ~0ull << (shift + 7);   /* sign extension */
            iv = (int64_t)v;
            put_s(&chk, iv);
            if (chk.n != r->pos - start || memcmp(chk.p, r->p + start, chk.n)) die("noncanonical SLEB128", NULL);
            free(chk.p); return iv;
        }
    }
    die("SLEB128 overflow", NULL);
    return 0;
}

/* ---- pools ---- */
typedef struct { const char **v; size_t *len; size_t n, cap; } Pool;
static size_t pool_idx(Pool *p, const char *s, size_t n) {
    size_t i;
    for (i = 0; i < p->n; i++) if (p->len[i] == n && !memcmp(p->v[i], s, n)) return i;
    if (p->n == p->cap) {
        p->cap = p->cap ? p->cap * 2 : 32;
        p->v = realloc(p->v, p->cap * sizeof *p->v); p->len = realloc(p->len, p->cap * sizeof *p->len);
        if (!p->v || !p->len) die("out of memory", NULL);
    }
    p->v[p->n] = s; p->len[p->n] = n;
    return p->n++;
}
static int name_ok(const char *s, size_t n) {   /* tapebin._name_bytes: nonempty, no NUL, blank, CR, LF, colon */
    size_t i;
    if (!n) return 0;
    for (i = 0; i < n; i++) if (s[i] == 0 || strchr(" \t\r\n:", s[i])) return 0;
    return 1;
}
static int ident_ok(const char *s, size_t n) {   /* decode-side name domain of this reference (see header) */
    size_t i;
    if (!n) return 0;
    for (i = 0; i < n; i++) if (!(isalnum((unsigned char)s[i]) || s[i] == '_' || s[i] == '.' || s[i] == '$')) return 0;
    return 1;
}
static size_t name_add(Pool *names, const char *s) {
    size_t n = strlen(s);
    if (!name_ok(s, n)) die("invalid name", s);
    return pool_idx(names, s, n);
}

/* tapebin._collect: names in record order (label/str/bss/link, and L/s operands), consts in order */
static void collect(Pool *names, Pool *consts) {
    size_t i, k;
    for (i = 0; i < nrec; i++) {
        Rec *r = &recs[i];
        if (r->kind == R_UNIT) continue;
        if (r->kind == R_INSN) {
            const char *sh = SH[r->in->op].shape;
            for (k = 0; sh[k]; k++)
                if ((sh[k] == 'L' || sh[k] == 's') && r->in->a[k].s) name_add(names, r->in->a[k].s);
            continue;
        }
        name_add(names, r->name);
        if (r->kind == R_STR) pool_idx(consts, (const char *)r->blob.p, r->blob.n);
    }
}

static void put_arg_int(Str *o, const Arg *a) {
    /* tapebin: value < 2^63 -> tag 0 + SLEB; else tag 1 + ULEB */
    if (!a->isbig) { sputc(o, 0); put_s(o, a->v); return; }
    if (a->u < (1ull << 63)) { sputc(o, 0); put_s(o, (int64_t)a->u); return; }
    sputc(o, 1); put_u(o, a->u);
}

static void encode_records(Str *o, Pool *names, Pool *consts) {
    size_t i, k;
    put_u(o, nrec);
    for (i = 0; i < nrec; i++) {
        Rec *r = &recs[i];
        switch (r->kind) {
        case R_UNIT: sputc(o, 6); put_u(o, 2); break;
        case R_GLOBAL: sputc(o, 4); put_u(o, pool_idx(names, r->name, strlen(r->name))); break;
        case R_EXTERN: sputc(o, 5); put_u(o, pool_idx(names, r->name, strlen(r->name))); break;
        case R_GDEF: sputc(o, 7); put_u(o, pool_idx(names, r->name, strlen(r->name))); break;
        case R_LABEL: sputc(o, 0); put_u(o, pool_idx(names, r->name, strlen(r->name))); break;
        case R_STR:
            sputc(o, 1); put_u(o, pool_idx(names, r->name, strlen(r->name)));
            put_u(o, pool_idx(consts, (const char *)r->blob.p, r->blob.n)); break;
        case R_BSS: sputc(o, 2); put_u(o, pool_idx(names, r->name, strlen(r->name))); put_u(o, r->num); break;
        case R_INSN: {
            Insn *in = r->in; const char *sh = SH[in->op].shape; size_t nr = 0, j; int regs[7];
            sputc(o, 3); sputc(o, (unsigned char)in->op);
            for (k = 0; sh[k]; k++) if (sh[k] == 'r') {
                if (!is_reg(in->a[k].s)) die("not a register (python: ValueError)", in->a[k].s);
                regs[nr++] = in->a[k].s[1] - '0';
            }
            for (j = 0; j < nr; j += 2) sputc(o, (unsigned char)(regs[j] | ((j + 1 < nr ? regs[j + 1] : 15) << 4)));
            for (k = 0; sh[k]; k++) {
                Arg *a = &in->a[k];
                if (sh[k] == 'i') put_arg_int(o, a);
                else if (sh[k] == 'L' || sh[k] == 's') { sputc(o, 1); put_u(o, pool_idx(names, a->s, strlen(a->s))); }
            }
            break;
        }
        }
    }
}

/* encode(tape, origin, sha): sections 1 names, 2 consts, 3 records, 4 SOURCE_SHA; header; digest */
static void encode_tape(Str *o, int origin, const unsigned char *sha) {
    Pool names = {0}, consts = {0};
    Str sec[4]; int kind[4]; size_t count[4]; size_t nsec = 0, i, k, pos;
    Str dir = {0}, payload = {0}, body = {0}; unsigned char head[64], digest[32];
    int unit_first, unit_any = 0, unit_count = 0;
    if (origin < 0 || origin > 6) die("invalid origin target (python: ValueError)", NULL);
    for (i = 0; i < nrec; i++) {
        if (recs[i].kind == R_UNIT) { unit_count++; unit_any = 1; }
        if (recs[i].kind == R_GDEF) unit_any = 1;
    }
    unit_first = nrec && recs[0].kind == R_UNIT;
    if (unit_any && !unit_first) die("unit marker must be first", NULL);
    if (unit_count > 1) die("duplicate unit marker", NULL);
    collect(&names, &consts);
    memset(sec, 0, sizeof sec);
    { Str s = {0}; put_u(&s, names.n); for (i = 0; i < names.n; i++) { put_u(&s, names.len[i]); sadd(&s, names.v[i], names.len[i]); } sec[nsec] = s; kind[nsec] = 1; count[nsec] = names.n; nsec++; }
    { Str s = {0}; put_u(&s, consts.n); for (i = 0; i < consts.n; i++) { put_u(&s, consts.len[i]); sadd(&s, consts.v[i], consts.len[i]); } sec[nsec] = s; kind[nsec] = 2; count[nsec] = consts.n; nsec++; }
    { Str s = {0}; encode_records(&s, &names, &consts); sec[nsec] = s; kind[nsec] = 3; count[nsec] = nrec; nsec++; }
    if (sha) { Str s = {0}; sadd(&s, sha, 32); sec[nsec] = s; kind[nsec] = 4; count[nsec] = 1; nsec++; }
    pos = 64 + nsec * 24;
    for (i = 0; i < nsec; i++) {
        size_t pad = (8 - pos % 8) % 8; uint64_t off = pos + pad, len = sec[i].n; unsigned char e[24];
        for (k = 0; k < pad; k++) sputc(&payload, 0);
        memset(e, 0, sizeof e);
        e[0] = (unsigned char)kind[i]; e[2] = kind[i] <= 3 ? 1 : 0;
        for (k = 0; k < 8; k++) e[4 + k] = (unsigned char)(off >> (8 * k));
        for (k = 0; k < 8; k++) e[12 + k] = (unsigned char)(len >> (8 * k));
        for (k = 0; k < 4; k++) e[20 + k] = (unsigned char)((uint64_t)count[i] >> (8 * k));
        sadd(&dir, e, 24);
        sadd(&payload, sec[i].p, sec[i].n);
        pos = (size_t)(off + len);
    }
    sadd(&body, dir.p, dir.n); sadd(&body, payload.p, payload.n);
    if (body.n > (1u << 30)) die("file too large", NULL);
    sha256(body.p, body.n, digest);
    memset(head, 0, sizeof head);
    memcpy(head, "UTAPEBIN", 8);
    head[8] = 1; head[10] = unit_first ? 1 : 0; head[12] = 1;
    head[20] = (unsigned char)nsec; head[24] = 64;
    memcpy(head + 28, digest, 32);
    head[60] = (unsigned char)origin;
    sadd(o, head, 64);
    sadd(o, body.p, body.n);
}

/* ---- decode (tapebin.decode): validates, rebuilds the records, then re-encodes and compares ---- */
static Pool dnames, dconsts;   /* decoded pools (the bytes stay in the input buffer) */
static int dorigin, dhas_sha; static unsigned char dsha[32];

static void decode_blob(const unsigned char *blob, size_t len) {
    unsigned major, minor, opset, flags, target, n, hlen, origin, row;
    unsigned char digest[32], calc[32];
    size_t pos, i, k; int seen[6] = {0}, ord[5] = {0}; const unsigned char *sd[6]; size_t sl[6], sc[6];
    Rd r;
    if (len < 64 || len > (1u << 30)) die("invalid file length", NULL);
    if (memcmp(blob, "UTAPEBIN", 8)) die("bad magic", NULL);
    major = blob[8] | (blob[9] << 8); minor = blob[10] | (blob[11] << 8);
    opset = blob[12] | (blob[13] << 8); flags = blob[14] | (blob[15] << 8);
    target = blob[16] | (blob[17] << 8) | (blob[18] << 16) | ((unsigned)blob[19] << 24);
    n = blob[20] | (blob[21] << 8) | (blob[22] << 16) | ((unsigned)blob[23] << 24);
    hlen = blob[24] | (blob[25] << 8) | (blob[26] << 16) | ((unsigned)blob[27] << 24);
    memcpy(digest, blob + 28, 32);
    origin = blob[60] | (blob[61] << 8) | (blob[62] << 16) | ((unsigned)blob[63] << 24);
    if (major != 1 || opset != 1 || flags != 0 || target != 0 || hlen != 64 || (minor != 0 && minor != 1) || origin > 6)
        die("unsupported header", NULL);
    if (n < 3 || n > 5 || 64 + (size_t)n * 24 > len) die("invalid section count", NULL);
    sha256(blob + 64, len - 64, calc);
    if (memcmp(calc, digest, 32)) die("hash mismatch", NULL);
    pos = 64 + (size_t)n * 24;
    for (i = 0; i < n; i++) {
        const unsigned char *e = blob + 64 + i * 24;
        unsigned kind = e[0] | (e[1] << 8), sflags = e[2] | (e[3] << 8);
        uint64_t off = 0, ln = 0; size_t cnt, pad;
        for (k = 0; k < 8; k++) off |= (uint64_t)e[4 + k] << (8 * k);
        for (k = 0; k < 8; k++) ln |= (uint64_t)e[12 + k] << (8 * k);
        cnt = e[20] | (e[21] << 8) | (e[22] << 16) | ((size_t)e[23] << 24);
        if (kind < 1 || kind > 4 || seen[kind] || sflags != (kind <= 3 ? 1u : 0u)) die("unsupported section", NULL);
        pad = (8 - pos % 8) % 8;
        if (off != pos + pad || off > len || ln > len - off) die("noncanonical section layout", NULL);
        for (k = pos; k < off; k++) if (blob[k]) die("noncanonical section padding", NULL);
        seen[kind] = 1; ord[i] = (int)kind; sd[kind] = blob + off; sl[kind] = (size_t)ln; sc[kind] = cnt;
        pos = (size_t)(off + ln);
    }
    if (pos != len) die("noncanonical section order", NULL);
    if (!((n == 3 && ord[0] == 1 && ord[1] == 2 && ord[2] == 3) ||
          (n == 4 && ord[0] == 1 && ord[1] == 2 && ord[2] == 3 && ord[3] == 4)))
        die("noncanonical section order", NULL);
    /* names and constants pools */
    r.p = sd[1]; r.n = sl[1]; r.pos = 0;
    { uint64_t cn = rd_uint(&r), j; if (cn != sc[1] || cn > sl[1]) die("pool count mismatch", NULL);
      for (j = 0; j < cn; j++) {
          uint64_t l = rd_uint(&r); const unsigned char *s = rd_take(&r, l); size_t q;
          if (!name_ok((const char *)s, (size_t)l)) die("invalid names pool", NULL);
          for (q = 0; q < dnames.n; q++) if (dnames.len[q] == l && !memcmp(dnames.v[q], s, l)) die("invalid names pool", NULL);
          if (!ident_ok((const char *)s, (size_t)l)) die("name outside the reference's domain", NULL);
          pool_idx(&dnames, (const char *)s, (size_t)l);
      }
      if (r.pos != r.n) die("section has trailing bytes", NULL); }
    r.p = sd[2]; r.n = sl[2]; r.pos = 0;
    { uint64_t cn = rd_uint(&r), j; if (cn != sc[2] || cn > sl[2]) die("pool count mismatch", NULL);
      for (j = 0; j < cn; j++) {
          uint64_t l = rd_uint(&r); const unsigned char *s = rd_take(&r, l); size_t q;
          for (q = 0; q < dconsts.n; q++) if (dconsts.len[q] == l && !memcmp(dconsts.v[q], s, l)) die("duplicate constant", NULL);
          pool_idx(&dconsts, (const char *)s, (size_t)l);
      }
      if (r.pos != r.n) die("section has trailing bytes", NULL); }
    /* records */
    r.p = sd[3]; r.n = sl[3]; r.pos = 0;
    { uint64_t cn = rd_uint(&r);
      if (cn != sc[3] || cn > sl[3]) die("record count mismatch", NULL);
      for (row = 0; row < cn; row++) {
          unsigned kind = rd_byte(&r);
          if (kind == 6) {
              if (minor != 1 || row != 0 || rd_uint(&r) != 2) die("invalid unit version", NULL);
              push_rec(R_UNIT); continue;
          }
          if (kind == 4 || kind == 5 || kind == 7) {
              uint64_t idx; Rec *rr;
              if (kind == 7 && minor != 1) die(".gdef requires minor 1", NULL);
              idx = rd_uint(&r); if (idx >= dnames.n) die("name index out of range", NULL);
              rr = push_rec(kind == 4 ? R_GLOBAL : kind == 5 ? R_EXTERN : R_GDEF);
              rr->name = dupn(dnames.v[idx], dnames.len[idx]);
              continue;
          }
          if (kind <= 2) {
              uint64_t idx = rd_uint(&r); Rec *rr;
              if (idx >= dnames.n) die("name index out of range", NULL);
              if (kind == 0) { rr = push_rec(R_LABEL); rr->name = dupn(dnames.v[idx], dnames.len[idx]); }
              else if (kind == 1) {
                  uint64_t ci = rd_uint(&r);
                  if (ci >= dconsts.n) die("const index out of range", NULL);
                  rr = push_rec(R_STR); rr->name = dupn(dnames.v[idx], dnames.len[idx]);
                  sadd(&rr->blob, dconsts.v[ci], dconsts.len[ci]);
              } else { rr = push_rec(R_BSS); rr->name = dupn(dnames.v[idx], dnames.len[idx]); rr->num = rd_uint(&r); }
              continue;
          }
          if (kind != 3) die("unknown record kind", NULL);
          {
              unsigned opc = rd_byte(&r); const char *sh; size_t nr = 0, j; int regs[7]; const unsigned char *packed;
              Insn *in; Rec *rr;
              if (opc >= NSH) die("unknown opcode (fp ops not in this reference)", NULL);
              sh = SH[opc].shape;
              for (j = 0; sh[j]; j++) if (sh[j] == 'r') nr++;
              packed = rd_take(&r, (nr + 1) / 2);
              for (j = 0; j < nr; j++) {
                  int v = (packed[j / 2] >> (4 * (j % 2))) & 15;
                  if (v >= 8) die("invalid register", NULL);
                  regs[j] = v;
              }
              if ((nr & 1) && (packed[nr / 2] >> 4) != 15) die("invalid register padding", NULL);
              in = calloc(1, sizeof *in); if (!in) die("out of memory", NULL);
              in->op = (int)opc; in->n = (int)strlen(sh);
              { size_t ri = 0;
                for (j = 0; sh[j]; j++) {
                    Arg *a = &in->a[j];
                    if (sh[j] == 'r') { a->s = dupn("r0", 2); a->s[1] = (char)('0' + regs[ri]); ri++; continue; }
                    { unsigned tag = rd_byte(&r);
                      if (tag > 1) die("invalid operand tag", NULL);
                      if (sh[j] == 'i') {
                          if (tag == 0) { a->isint = 1; a->v = rd_sint(&r); }
                          else { uint64_t u = rd_uint(&r); a->isint = 1; a->isbig = 1; a->u = u; }
                      } else {
                          if (tag == 0) die("integer operand in a name position (python: text mismatch)", NULL);
                          { uint64_t idx = rd_uint(&r); if (idx >= dnames.n) die("name index out of range", NULL);
                            a->s = dupn(dnames.v[idx], dnames.len[idx]); }
                      }
                    }
                }
              }
              rr = push_rec(R_INSN); rr->in = in;
          }
      }
      if (r.pos != r.n) die("section has trailing bytes", NULL);
    }
    if (minor == 1 && (nrec == 0 || recs[0].kind != R_UNIT)) die("missing unit marker", NULL);
    for (k = 0; k < nrec; k++) if (recs[k].kind == R_GDEF && (nrec == 0 || recs[0].kind != R_UNIT)) die(".gdef needs unit marker", NULL);
    dorigin = (int)origin; dhas_sha = seen[4];
    if (seen[4]) {
        if (sl[4] != 32 || sc[4] != 1) die("invalid SOURCE_SHA", NULL);
        memcpy(dsha, sd[4], 32);
    }
    /* decode ends with encode(restored) == blob: re-encode the rebuilt records and compare */
    {
        Str again = {0};
        encode_tape(&again, dorigin, dhas_sha ? dsha : NULL);
        if (again.n != len || memcmp(again.p, blob, len)) die("noncanonical encoding", NULL);
    }
}

/* ---- script (encode command) ---- */
static int hex_bytes(const char *h, unsigned char *out, size_t cap, size_t *n) {
    size_t L = strlen(h), k;
    if (L % 2 || L / 2 > cap) return 0;
    for (k = 0; k < L; k += 2) {
        int a = hexval((unsigned char)h[k]), b = hexval((unsigned char)h[k + 1]);
        if (a < 0 || b < 0) return 0;
        out[k / 2] = (unsigned char)(a * 16 + b);
    }
    *n = L / 2;
    return 1;
}
static void build_script(const char *path) {
    FILE *f = fopen(path, "rb"); char line[1 << 16];
    if (!f) die("cannot open", path);
    while (fgets(line, sizeof line, f)) {
        char *tok[MAXTOK]; int n = 0; char *p = line;
        size_t L = strlen(line); if (L && line[L - 1] == '\n') line[--L] = 0;
        while (*p) {
            while (*p == ' ' || *p == '\t' || *p == '\r') p++;
            if (!*p) break;
            if (n >= MAXTOK) die("too many tokens", NULL);
            tok[n++] = p;
            while (*p && *p != ' ' && *p != '\t' && *p != '\r') p++;
            if (*p) *p++ = 0;
        }
        if (n == 0) continue;
        if (!strcmp(tok[0], "U") && n == 1) push_rec(R_UNIT);
        else if (!strcmp(tok[0], "G") && n == 3) {
            int kd = !strcmp(tok[1], "global") ? R_GLOBAL : !strcmp(tok[1], "extern") ? R_EXTERN :
                     !strcmp(tok[1], "gdef") ? R_GDEF : -1;
            Rec *r;
            if (kd < 0) die("unknown link kind", tok[1]);
            r = push_rec(kd); r->name = dupn(tok[2], strlen(tok[2]));
        } else if (!strcmp(tok[0], "L") && n == 2) {
            Rec *r = push_rec(R_LABEL); r->name = dupn(tok[1], strlen(tok[1]));
        } else if (!strcmp(tok[0], "E") && n >= 2) t_emit(tok[1], n - 2, tok + 2);
        else if (!strcmp(tok[0], "S") && n == 4) {
            unsigned char raw[4096]; size_t rn; long long al;
            if (!hex_bytes(tok[2], raw, sizeof raw, &rn) || !int_lit(tok[3], 0, &al) || al < 0) die("malformed S line", NULL);
            t_string(tok[1], raw, rn, 0);
        } else if (!strcmp(tok[0], "B") && n == 3) {
            long long nb;
            if (!int_lit(tok[2], 0, &nb) || nb < 0) die("malformed B line (python: ValueError)", tok[2]);
            if (nb <= 16) { unsigned char z[16]; memset(z, 0, sizeof z); t_string(tok[1], z, (size_t)nb, 1); }
            else { t_string(tok[1], NULL, (size_t)nb, 1); }
        } else die("unknown script form", tok[0]);
    }
    fclose(f);
}

static void reset_state(void) { nrec = 0; nsym = 0; memset(&dnames, 0, sizeof dnames); memset(&dconsts, 0, sizeof dconsts); }

int main(int argc, char **argv) {
    Str out = {0};
    if (argc == 3 && !strcmp(argv[1], "target")) {
        int i;
        for (i = 0; i < NTGT; i++) if (!strcmp(TARGETS[i], argv[2])) { sputf(&out, "%d\n", i + 1); flush_out(&out); return 0; }
        die("unknown target", argv[2]);
    }
    if (argc == 5 && !strcmp(argv[1], "checkorigin")) {
        long long origin; int i, tid = -1, force;
        if (!int_lit(argv[2], 0, &origin)) die("bad origin", argv[2]);
        for (i = 0; i < NTGT; i++) if (!strcmp(TARGETS[i], argv[3])) tid = i + 1;
        if (tid < 0) die("unknown target", argv[3]);
        force = strcmp(argv[4], "1") == 0;
        if (origin && origin != tid && !force) die("origin target differs; use --force-origin", NULL);
        sputs(&out, "OK\n"); flush_out(&out);
        return 0;
    }
    if (argc == 5 && !strcmp(argv[1], "encode")) {
        long long origin; unsigned char sha[32]; size_t shan = 0; const unsigned char *shap = NULL;
        if (!int_lit(argv[3], 0, &origin)) die("bad origin", argv[3]);
        if (strcmp(argv[4], "-")) {
            if (!hex_bytes(argv[4], sha, 32, &shan) || shan != 32) die("SOURCE_SHA must be 32 bytes", NULL);
            shap = sha;
        }
        reset_state();
        build_script(argv[2]);
        encode_tape(&out, (int)origin, shap);
        {   /* hex output */
            size_t k; Str hx = {0};
            for (k = 0; k < out.n; k++) sputf(&hx, "%02x", out.p[k]);
            sputc(&hx, '\n'); flush_out(&hx);
        }
        return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "decode")) {
        FILE *f = fopen(argv[2], "rb"); char *txt; size_t tl = 0, cap = 1 << 16, k; unsigned char *blob; size_t bn = 0;
        if (!f) die("cannot open", argv[2]);
        txt = malloc(cap);
        while ((k = fread(txt + tl, 1, cap - tl - 1, f)) > 0) { tl += k; if (tl + 1 >= cap) { cap *= 2; txt = realloc(txt, cap); } }
        fclose(f);
        while (tl && isspace((unsigned char)txt[tl - 1])) tl--;
        txt[tl] = 0;
        blob = malloc(tl / 2 + 1);
        if (!hex_bytes(txt, blob, tl / 2 + 1, &bn)) die("bad hex input", NULL);
        reset_state();
        decode_blob(blob, bn);
        sputf(&out, "ORIGIN %d\n", dorigin);
        if (dhas_sha) { size_t q; sputs(&out, "SHA "); for (q = 0; q < 32; q++) sputf(&out, "%02x", dsha[q]); sputc(&out, '\n'); }
        canonical(&out);
        flush_out(&out);
        return 0;
    }
    fprintf(stderr, "usage: unisa-tapebin encode SCRIPT ORIGIN SHA|- | decode HEXFILE | target NAME | checkorigin ORIGIN NAME FORCE\n");
    return 2;
}
