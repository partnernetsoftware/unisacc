/* seed/unisa_tape.c -- C99 reference for unisa.tape (K6-10), point-sample functions.
 *
 *   unisa-tape build SCRIPT text|canon
 *       SCRIPT lines, fields separated by blanks (blank lines skipped):
 *         L NAME                  Tape.label(NAME)
 *         E OP ARG...             Tape.emit(OP, ARG...)   ARG: i = int(x,0), r/s/L = the token
 *         S NAME HEX ALIGN        Tape.string(NAME, bytes.fromhex(HEX), align=ALIGN)
 *         B NAME N                Tape.string(NAME, N, align=8)   zero global by size
 *       then prints to_text() (text) or to_canonical_text() (canon).
 *   unisa-tape parse FILE text|canon
 *       parse(FILE), then prints to_text() or to_canonical_text().
 *
 * Mirrors unisa/tape.py: SHAPE (the non-floating-point ops), _fmt, _quote, _unescape, _strip_comment,
 * _regroup, parse(), Tape.label / emit / string, to_text / to_canonical_text.  Every refusal (python:
 * assert, ValueError, KeyError, IndexError) prints its reason on stderr and nothing on stdout.
 * Not covered, and refused rather than accepted: the fp.py ops (OPS3/OPS2 in SHAPE); integers outside
 * int64; non-ASCII bytes; NUL bytes in the input; underscores in integer literals; the 1 MiB .str cap.
 * Point-sample equivalence over the listed samples only; not a product closure member; keeps
 * unisa/tape.py.  No floating point here.
 */
#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATA_BASE 0x100
#define MAXTOK 64

static void die(const char *why, const char *what) {
    fprintf(stderr, "unisa-tape: %s %s\n", why, what ? what : "");
    exit(1);
}

/* ---- growable byte string: the buffered stdout and the Tape data area ---- */
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

/* ---- SHAPE of tape.py (non-fp).  r=register i=integer L=label s=symbol/name ---- */
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

static const char *shape_of(const char *op) {
    int i;
    for (i = 0; i < NSH; i++) if (!strcmp(SH[i].op, op)) return SH[i].shape;
    return NULL;
}
static int is_reg(const char *t) { return strlen(t) == 2 && t[0] == 'r' && t[1] >= '0' && t[1] <= '7'; }

static int hexval(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
/* python int(s, 0) when base0 (sign, 0x, no nonzero leading zero) or int(s) when !base0 */
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

/* ---- Tape state ---- */
typedef struct { int isint; long long v; char *s; } Arg;
typedef struct { const char *op; int n; Arg a[7]; } Insn;
enum { R_LABEL, R_INSN, R_STR, R_BSS, R_UNIT, R_GLOBAL, R_EXTERN, R_GDEF };
typedef struct { int kind; char *name; Str blob; long long num; Insn *in; } Rec;
typedef struct { char *name; long long addr; } Sym;
typedef struct { char *name; long long pc; } Lab;

static Insn **code; static size_t ncode, capcode;
static Rec *recs; static size_t nrec, caprec;
static Sym *syms; static size_t nsym, capsym;
static Lab *labels; static size_t nlab, caplab;
static Str data;

static Rec *push_rec(int kind) {
    Rec *r;
    if (nrec == caprec) { caprec = caprec ? caprec * 2 : 64; recs = realloc(recs, caprec * sizeof *recs); if (!recs) die("out of memory", NULL); }
    r = &recs[nrec++]; memset(r, 0, sizeof *r); r->kind = kind; return r;
}
static void push_code(Insn *in) {
    if (ncode == capcode) { capcode = capcode ? capcode * 2 : 64; code = realloc(code, capcode * sizeof *code); if (!code) die("out of memory", NULL); }
    code[ncode++] = in;
}
static int find_sym(const char *name) {
    size_t i;
    for (i = 0; i < nsym; i++) if (!strcmp(syms[i].name, name)) return (int)i;
    return -1;
}
static void add_sym(const char *name, long long addr) {
    if (nsym == capsym) { capsym = capsym ? capsym * 2 : 64; syms = realloc(syms, capsym * sizeof *syms); if (!syms) die("out of memory", NULL); }
    syms[nsym].name = dupn(name, strlen(name)); syms[nsym].addr = addr; nsym++;
}
static void set_label(const char *name, long long pc) {
    size_t i;
    for (i = 0; i < nlab; i++) if (!strcmp(labels[i].name, name)) { labels[i].pc = pc; return; }
    if (nlab == caplab) { caplab = caplab ? caplab * 2 : 64; labels = realloc(labels, caplab * sizeof *labels); if (!labels) die("out of memory", NULL); }
    labels[nlab].name = dupn(name, strlen(name)); labels[nlab].pc = pc; nlab++;
}

/* Tape.label */
static void t_label(const char *name) {
    set_label(name, (long long)ncode);
    push_rec(R_LABEL)->name = dupn(name, strlen(name));
}

/* Tape.emit: arity (python assert) and integer operands (python int()) are checked; registers are not */
static void t_emit(const char *op, int ntok, char **tok, int check_regs) {
    const char *sh = shape_of(op); int i; Insn *in; Rec *r;
    if (!sh) die("unknown tape op (python: assert/KeyError)", op);
    if ((int)strlen(sh) != ntok) die("operand count (python: assert)", op);
    in = calloc(1, sizeof *in); if (!in) die("out of memory", NULL);
    in->op = dupn(op, strlen(op)); in->n = ntok;
    for (i = 0; i < ntok; i++) {
        if (sh[i] == 'i') {
            if (!int_lit(tok[i], 1, &in->a[i].v)) die("not an integer (python: ValueError)", tok[i]);
            in->a[i].isint = 1;
        } else {
            if (sh[i] == 'r' && check_regs && !is_reg(tok[i])) die("not a register (python: assert)", tok[i]);
            in->a[i].isint = 0; in->a[i].s = dupn(tok[i], strlen(tok[i]));
        }
    }
    push_code(in);
    r = push_rec(R_INSN); r->in = in;
}

/* Tape.string bytes path: returns 1 when a new symbol was added (python: early return on a known name) */
static int add_bytes(const char *name, const unsigned char *raw, size_t n, size_t align) {
    size_t i;
    if (find_sym(name) >= 0) return 0;
    if (align > 1) while (data.n % align) { unsigned char z = 0; sadd(&data, &z, 1); }
    add_sym(name, DATA_BASE + (long long)data.n);
    sadd(&data, raw, n);
    (void)i;
    return 1;
}
/* Tape.string zero-by-size path (raw > 16): pads, then reserves n zero bytes */
static int add_zero_big(const char *name, size_t n, size_t align) {
    if (find_sym(name) >= 0) return 0;
    if (align > 1) while (data.n % align) { unsigned char z = 0; sadd(&data, &z, 1); }
    add_sym(name, DATA_BASE + (long long)data.n);
    { size_t k; for (k = 0; k < n; k++) { unsigned char z = 0; sadd(&data, &z, 1); } }
    return 1;
}
static int all_zero(const unsigned char *p, size_t n) {
    size_t i; for (i = 0; i < n; i++) if (p[i]) return 0;
    return 1;
}

/* ---- output: to_text / to_canonical_text / _fmt / _quote ---- */
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
    const char *op = in->op; const Arg *a = in->a; size_t start = o->n; int i;
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
            if (a[i].isint) sputf(o, "%lld", a[i].v); else sputs(o, a[i].s);
        }
        while (o->n > start && isspace(o->p[o->n - 1])) o->n--;   /* python rstrip() */
    }
}

static void canonical(Str *o) {
    size_t i;
    for (i = 0; i < nrec; i++) {
        Rec *r = &recs[i];
        switch (r->kind) {
        case R_UNIT: sputf(o, ".unit %lld\n", r->num); break;
        case R_GLOBAL: sputf(o, ".global %s\n", r->name); break;
        case R_EXTERN: sputf(o, ".extern %s\n", r->name); break;
        case R_GDEF: sputf(o, ".gdef %s\n", r->name); break;
        case R_LABEL: sputf(o, "%s:\n", r->name); break;
        case R_STR: sputf(o, ".str %s ", r->name); quote(o, r->blob.p, r->blob.n); sputc(o, '\n'); break;
        case R_BSS: sputf(o, ".bss %s %lld\n", r->name, r->num); break;
        case R_INSN: sputs(o, "  "); fmt_insn(o, r->in); sputc(o, '\n'); break;
        }
    }
}

static int has_link_or_unit(void) {
    size_t i;
    for (i = 0; i < nrec; i++) if (recs[i].kind >= R_UNIT) return 1;
    return 0;
}

static void to_text(Str *o) {
    size_t *ord, i, j, k; long long nd = (long long)data.n; size_t before = o->n;
    if (has_link_or_unit()) { canonical(o); return; }
    ord = malloc((nsym ? nsym : 1) * sizeof *ord); if (!ord) die("out of memory", NULL);
    for (i = 0; i < nsym; i++) ord[i] = i;
    for (i = 1; i < nsym; i++) {            /* stable insertion sort by address (python sorted) */
        size_t x = ord[i]; j = i;
        while (j > 0 && syms[ord[j - 1]].addr > syms[x].addr) { ord[j] = ord[j - 1]; j--; }
        ord[j] = x;
    }
    for (i = 0; i < nsym; i++) {
        long long end = syms[ord[i]].addr - DATA_BASE, nxt = nd, zeros = 0;
        for (k = 0; k < nsym; k++) { long long e2 = syms[k].addr - DATA_BASE; if (e2 > end && e2 < nxt) nxt = e2; }
        for (k = (size_t)end; k < (size_t)nxt; k++) if (data.p[k] == 0) zeros++;
        if (nxt - end > 16 && zeros == nxt - end) sputf(o, ".bss %s %lld\n", syms[ord[i]].name, nxt - end);
        else { sputf(o, ".str %s ", syms[ord[i]].name); quote(o, data.p + end, (size_t)(nxt - end)); sputc(o, '\n'); }
    }
    free(ord);
    for (k = 0; k <= ncode; k++) {          /* labels at each pc, names sorted; then the instruction */
        const char *prev = NULL;
        for (;;) {
            const char *nx = NULL; size_t m;
            for (m = 0; m < nlab; m++) {
                if ((size_t)labels[m].pc != k) continue;
                if (prev && strcmp(labels[m].name, prev) <= 0) continue;
                if (!nx || strcmp(labels[m].name, nx) < 0) nx = labels[m].name;
            }
            if (!nx) break;
            sputf(o, "%s:\n", nx);
            prev = nx;
        }
        if (k < ncode) { sputs(o, "  "); fmt_insn(o, code[k]); sputc(o, '\n'); }
    }
    if (o->n == before) sputc(o, '\n');     /* python "\n".join([]) + "\n" */
}

/* ---- parsing (tape.py _unescape, _strip_comment, _regroup, parse) ---- */
static int unesc(const unsigned char *s, size_t len, Str *out) {
    size_t i = 0;
    while (i < len) {
        unsigned char c = s[i];
        if (c == '\\' && i + 1 < len) {
            unsigned char n = s[i + 1];
            if (n == 'x') {
                if (i + 3 >= len || hexval(s[i + 2]) < 0 || hexval(s[i + 3]) < 0) return 0;
                { unsigned char b = (unsigned char)(hexval(s[i + 2]) * 16 + hexval(s[i + 3])); sputc(out, b); }
                i += 4; continue;
            }
            switch (n) {
            case 'n': sputc(out, '\n'); break;
            case 't': sputc(out, '\t'); break;
            case 'r': sputc(out, '\r'); break;
            case '0': sputc(out, 0); break;
            default: sputc(out, n); break;     /* \\ \" \' and any other: the character itself */
            }
            i += 2; continue;
        }
        if (c == '\\') return 0;               /* trailing string escape */
        if (c == '"') return 0;                /* unescaped quote */
        sputc(out, c);
        i++;
    }
    return 1;
}

/* `;` starts a comment, but not inside a string literal */
static size_t strip_comment_len(const char *raw) {
    size_t i = 0, n = strlen(raw); int q = 0;
    while (i < n) {
        char c = raw[i];
        if (q) {
            if (c == '\\') { i += 2; continue; }
            if (c == '"') q = 0;
        } else {
            if (c == '"') q = 1;
            else if (c == ';') return i;
        }
        i++;
    }
    return n;
}

/* split on blanks (python str.split()); tokens are written in place */
static int split_ws(char *s, char **tok, int max) {
    int n = 0;
    for (;;) {
        while (*s && isspace((unsigned char)*s)) s++;
        if (!*s) break;
        if (n >= max) die("too many tokens", NULL);
        tok[n++] = s;
        while (*s && !isspace((unsigned char)*s)) s++;
        if (*s) *s++ = 0;
    }
    return n;
}

static char *trim(char *s) {
    char *e;
    while (*s && isspace((unsigned char)*s)) s++;
    e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1])) *--e = 0;
    return s;
}

/* _regroup: a flattened [rb+K] comes back as "rb+K"; split it at the last + or - */
static int regroup_op(const char *op) {
    return !strcmp(op, "load64") || !strcmp(op, "store64") || !strcmp(op, ".ld") || !strcmp(op, ".st")
        || !strcmp(op, ".zero") || !strcmp(op, "callm");
}

static void parse_insn(char *s, int lineno) {
    char *body = dupn(s, strlen(s)), *tok[MAXTOK], *gr[2 * MAXTOK];
    int i, n, ng = 0; const char *op;
    for (i = 0; body[i]; i++) if (body[i] == '[' || body[i] == ']' || body[i] == ',') body[i] = ' ';
    n = split_ws(body, tok, MAXTOK);
    if (n == 0) { fprintf(stderr, "unisa-tape: line %d: empty instruction\n", lineno); exit(1); }
    op = tok[0];
    if (!shape_of(op)) { fprintf(stderr, "unisa-tape: line %d: unknown op %s\n", lineno, op); exit(1); }
    for (i = 1; i < n; i++) {
        const char *t = tok[i]; size_t L = strlen(t); int cut = -1, j;
        if (regroup_op(op) && t[0] == 'r') {
            for (j = 1; j < (int)L; j++) if (t[j] == '+' || t[j] == '-') cut = j;
        }
        if (cut > 0) {
            gr[ng++] = dupn(t, (size_t)cut);
            gr[ng++] = dupn(t + cut, L - (size_t)cut);
        } else gr[ng++] = (char *)t;
    }
    if ((int)strlen(shape_of(op)) != ng) { fprintf(stderr, "unisa-tape: line %d: wrong operand count for %s\n", lineno, op); exit(1); }
    for (i = 0; i < ng; i++) {           /* integer operands: the python reader names the line (underscores refused) */
        long long v;
        if (shape_of(op)[i] == 'i' && !int_lit(gr[i], 1, &v)) { fprintf(stderr, "unisa-tape: line %d: invalid integer literal\n", lineno); exit(1); }
    }
    t_emit(op, ng, gr, 1);               /* appends the code slot and its record */
}

static void parse_line(char *s, int lineno) {
    if (!strncmp(s, ".bss ", 5)) {
        char *tok[MAXTOK]; int n = split_ws(s + 5, tok, MAXTOK); long long nb; Rec *r;
        if (n != 2 || !int_lit(tok[1], 0, &nb) || nb < 0) die("malformed .bss", tok[0]);
        if (find_sym(tok[0]) < 0) {
            if (nb > 16) add_zero_big(tok[0], (size_t)nb, 8);
            else { unsigned char z[16] = {0}; add_bytes(tok[0], z, (size_t)nb, 8); }
        }
        r = push_rec(R_BSS); r->name = dupn(tok[0], strlen(tok[0])); r->num = nb;
        return;
    }
    {
        char *tok[MAXTOK]; int n; char *c = dupn(s, strlen(s));
        n = split_ws(c, tok, MAXTOK);
        if (n && !strcmp(tok[0], ".unit")) {
            Rec *r;
            if (n != 2 || strcmp(tok[1], "2") || nrec) die("malformed unit version record", NULL);
            r = push_rec(R_UNIT); r->num = 2;
            return;
        }
        if (n && (!strcmp(tok[0], ".global") || !strcmp(tok[0], ".extern") || !strcmp(tok[0], ".gdef"))) {
            const char *nm = n == 2 ? tok[1] : NULL; size_t k; Rec *r;
            if (n != 2 || !((nm[0] >= 'A' && nm[0] <= 'Z') || (nm[0] >= 'a' && nm[0] <= 'z') || nm[0] == '_')) die("malformed linkage record", NULL);
            for (k = 1; nm[k]; k++) {
                int ok = isalnum((unsigned char)nm[k]) || nm[k] == '_' || nm[k] == '.' || nm[k] == '$';
                if (!ok) die("malformed linkage record", NULL);
            }
            if (!strcmp(tok[0], ".gdef") && (nrec == 0 || recs[0].kind != R_UNIT)) die(".gdef needs .unit 2", NULL);
            r = push_rec(!strcmp(tok[0], ".global") ? R_GLOBAL : !strcmp(tok[0], ".extern") ? R_EXTERN : R_GDEF);
            r->name = dupn(nm, strlen(nm));
            return;
        }
    }
    if (!strncmp(s, ".str ", 5)) {
        char *rest = trim(s + 5), *sp, *lit; size_t L; Str blob = {0}; Rec *r;
        sp = rest; while (*sp && !isspace((unsigned char)*sp)) sp++;
        if (!*sp) die("malformed .str (python: unpack)", NULL);
        *sp++ = 0;
        lit = trim(sp); L = strlen(lit);
        if (L < 2 || lit[0] != '"' || lit[L - 1] != '"') die("malformed .str literal", NULL);
        if (!unesc((const unsigned char *)lit + 1, L - 2, &blob)) die("malformed string escape", NULL);
        add_bytes(rest, blob.p ? blob.p : (const unsigned char *)"", blob.n, 1);
        r = push_rec(R_STR); r->name = dupn(rest, strlen(rest));
        r->blob = blob;
        return;
    }
    {
        size_t L = strlen(s);
        if (L && s[L - 1] == ':' && !memchr(s, ' ', L - 1)) {
            char *nm = dupn(s, L - 1);
            t_label(nm);
            return;
        }
    }
    parse_insn(s, lineno);
}

static char *read_all(const char *path) {
    FILE *f = fopen(path, "rb"); size_t n = 0, cap = 4096; char *b; int c;
    if (!f) die("cannot open", path);
    b = malloc(cap);
    if (!b) die("out of memory", NULL);
    while ((c = fgetc(f)) != EOF) {
        if (c == 0) die("NUL byte in input (not covered)", path);
        if (n + 2 > cap) { cap *= 2; b = realloc(b, cap); if (!b) die("out of memory", NULL); }
        b[n++] = (char)c;
    }
    fclose(f);
    b[n] = 0;
    return b;
}

static void parse_text(const char *text) {
    const char *p = text; int lineno = 0;
    while (*p) {
        const char *e = strchr(p, '\n'); size_t L = e ? (size_t)(e - p) : strlen(p);
        char *raw = dupn(p, L), *s;
        lineno++;
        p = e ? e + 1 : p + L;
        raw[strip_comment_len(raw)] = 0;
        s = trim(raw);
        if (!*s) continue;
        parse_line(s, lineno);
    }
}

/* ---- build: the script form of the Tape API (see the header) ---- */
static int hex_bytes(const char *h, Str *out) {
    size_t n = strlen(h), k;
    if (n % 2) return 0;
    for (k = 0; k < n; k += 2) {
        int a = hexval((unsigned char)h[k]), b = hexval((unsigned char)h[k + 1]);
        if (a < 0 || b < 0) return 0;
        sputc(out, (unsigned char)(a * 16 + b));
    }
    return 1;
}
static void build_text(const char *text) {
    const char *p = text; int lineno = 0;
    while (*p) {
        const char *e = strchr(p, '\n'); size_t L = e ? (size_t)(e - p) : strlen(p);
        char *line = dupn(p, L), *tok[MAXTOK]; int n;
        lineno++;
        p = e ? e + 1 : p + L;
        n = split_ws(line, tok, MAXTOK);
        if (n == 0) continue;
        if (!strcmp(tok[0], "L") && n == 2) t_label(tok[1]);
        else if (!strcmp(tok[0], "E") && n >= 2) t_emit(tok[1], n - 2, tok + 2, 0);
        else if (!strcmp(tok[0], "S") && n == 4) {
            Str raw = {0}; long long al; Rec *r;
            if (!hex_bytes(tok[2], &raw) || !int_lit(tok[3], 0, &al) || al < 0) die("malformed S line", NULL);
            if (find_sym(tok[1]) < 0) {
                add_bytes(tok[1], raw.p ? raw.p : (const unsigned char *)"", raw.n, (size_t)al);
                if (raw.n > 16 && all_zero(raw.p ? raw.p : (const unsigned char *)"", raw.n)) { r = push_rec(R_BSS); r->name = dupn(tok[1], strlen(tok[1])); r->num = (long long)raw.n; }
                else { r = push_rec(R_STR); r->name = dupn(tok[1], strlen(tok[1])); r->blob = raw; }
            }
        } else if (!strcmp(tok[0], "B") && n == 3) {
            long long nb; Rec *r;
            if (!int_lit(tok[2], 0, &nb) || nb < 0) die("malformed B line (python: ValueError)", tok[2]);
            if (find_sym(tok[1]) < 0) {
                if (nb > 16) { add_zero_big(tok[1], (size_t)nb, 8); r = push_rec(R_BSS); }
                else {
                    Str z = {0}; size_t k; for (k = 0; k < (size_t)nb; k++) sputc(&z, 0);
                    add_bytes(tok[1], z.p ? z.p : (const unsigned char *)"", z.n, 8);
                    r = push_rec(R_STR); r->blob = z;
                }
                r->name = dupn(tok[1], strlen(tok[1])); r->num = nb;
            }
        } else {
            fprintf(stderr, "unisa-tape: script line %d: unknown form\n", lineno); exit(1);
        }
    }
}

int main(int argc, char **argv) {
    Str out = {0}; int canon;
    if (argc != 4 || (strcmp(argv[3], "text") && strcmp(argv[3], "canon"))) {
        fprintf(stderr, "usage: unisa-tape build SCRIPT text|canon | parse FILE text|canon\n");
        return 2;
    }
    canon = !strcmp(argv[3], "canon");
    if (!strcmp(argv[1], "build")) build_text(read_all(argv[2]));
    else if (!strcmp(argv[1], "parse")) parse_text(read_all(argv[2]));
    else { fprintf(stderr, "usage: unisa-tape build SCRIPT text|canon | parse FILE text|canon\n"); return 2; }
    if (canon) canonical(&out); else to_text(&out);
    if (out.n) fwrite(out.p, 1, out.n, stdout);
    return 0;
}
