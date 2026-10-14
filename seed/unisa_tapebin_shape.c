/* seed/unisa_tapebin_shape.c -- C99 reference for unisa.tapebin_shape (K6-11b), point-sample functions.
 *
 *   unisa-tapebin-shape render         the opcode table: "opcode\tname\tshape" rows for SHAPE (71 entries)
 *   unisa-tapebin-shape records        the record kind table (render_records)
 *   unisa-tapebin-shape c              the C header text (render_c)
 *   unisa-tapebin-shape check KIND FILE
 *       KIND in render records c; compares FILE byte-for-byte with the rendered text.  Prints OK
 *       when equal; refuses ("stale") otherwise.  This is the comparator behind --check, applied to a
 *       file the caller names (the repository snapshots are compared by the test script, read-only).
 *
 * Mirrors unisa/tapebin_shape.py render, render_records, render_c, and the comparison in main().
 * The operation order is tape.SHAPE's insertion order: the non-fp literal table, then fp.OPS3
 * (14 "rrr"), then fp.OPS2 (10 "rr").  Writing (--write) is not implemented here, by design.
 * Refusals print the reason on stderr and nothing on stdout.  Point-sample equivalence over the
 * listed renderings only; not a product closure member; keeps unisa/tapebin_shape.py and tape.py.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void die(const char *why, const char *what) {
    fprintf(stderr, "unisa-tapebin-shape: %s %s\n", why, what ? what : "");
    exit(1);
}

typedef struct { const char *op, *shape; } Row;
static const Row SHAPE_ROWS[] = {
    {"imm", "ri"}, {"mov", "rr"}, {"add64", "rrr"}, {"sub64", "rrr"}, {"mul64", "rrr"},
    {"xor64", "rrr"}, {"and64", "rrr"}, {"or64", "rrr"}, {"shl64", "rrr"}, {"shr64", "rrr"},
    {"lshr64", "rrr"}, {".div", "rrr"}, {".mod", "rrr"}, {".udiv", "rrr"}, {".umod", "rrr"},
    {"slt64", "rrr"}, {"sle64", "rrr"}, {"ult64", "rrr"}, {"ule64", "rrr"}, {"eq", "rrr"}, {"ne", "rrr"},
    {"load64", "rri"}, {"store64", "rir"}, {".ld", "rrii"}, {".st", "riri"}, {".lea", "rs"},
    {".zero", "rii"}, {"jump", "L"}, {"jumpz", "rL"}, {"call", "L"}, {"callr", "r"}, {"callm", "ri"},
    {".hostcall", "rr"}, {".librarycall", "rr"}, {".libraryaddr", "rsiiii"}, {".libraryfn", "ri"}, {".hostaddr", "ri"},
    {"ret", ""}, {".frame", "i"}, {".arg", "ir"}, {".print", "r"}, {".write", "rr"}, {".exit", "r"},
    {".sys", "srrr"}, {".sys6", "srrrrrr"}, {".argc", "r"}, {".argv", "rr"}, {"nop", ""},
    /* fp.OPS3: floating point, three registers */
    {"fadd64", "rrr"}, {"fsub64", "rrr"}, {"fmul64", "rrr"}, {"fdiv64", "rrr"}, {"flt64", "rrr"},
    {"fle64", "rrr"}, {"feq64", "rrr"}, {"fadd32", "rrr"}, {"fsub32", "rrr"}, {"fmul32", "rrr"},
    {"fdiv32", "rrr"}, {"flt32", "rrr"}, {"fle32", "rrr"}, {"feq32", "rrr"},
    /* fp.OPS2: floating point, two registers */
    {"cvtid", "rr"}, {"cvtud", "rr"}, {"cvtis", "rr"}, {"cvtus", "rr"}, {"cvtdi", "rr"},
    {"cvtdu", "rr"}, {"cvtsd", "rr"}, {"cvtds", "rr"}, {"fsqrt64", "rr"}, {"fsqrt32", "rr"},
};
#define NROWS ((int)(sizeof SHAPE_ROWS / sizeof SHAPE_ROWS[0]))

/* LINK_RECORDS in insertion order, UNIT_RECORD: the records table of tape.py */
static const struct { const char *name; int v; } LINKS[] = { {"global", 4}, {"extern", 5}, {"gdef", 7} };
#define UNIT_RECORD 6

typedef struct { unsigned char *p; size_t n, cap; } Str;
static void sadd(Str *s, const char *p, size_t k) {
    if (s->n + k + 1 > s->cap) {
        size_t nc = s->cap ? s->cap * 2 : 1024;
        while (nc < s->n + k + 1) nc *= 2;
        s->p = realloc(s->p, nc); if (!s->p) die("out of memory", NULL); s->cap = nc;
    }
    memcpy(s->p + s->n, p, k); s->n += k;
}
static void sputs(Str *s, const char *c) { sadd(s, c, strlen(c)); }
static void sputf(Str *s, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
#include <stdarg.h>
static void sputf(Str *s, const char *fmt, ...) {
    char buf[512]; va_list ap; int k;
    va_start(ap, fmt); k = vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap);
    if (k < 0 || (size_t)k >= sizeof buf) die("format", NULL);
    sadd(s, buf, (size_t)k);
}

static void render(Str *o) {
    int i;
    sputs(o, "opcode\tname\tshape\n");
    for (i = 0; i < NROWS; i++) sputf(o, "%d\t%s\t%s\n", i, SHAPE_ROWS[i].op, SHAPE_ROWS[i].shape);
}

static void render_records(Str *o) {
    size_t i;
    sputs(o, "kind\tname\tshape\n0\tlabel\tname\n1\t.str\tname,const\n2\t.bss\tname,size\n3\tinsn\topcode,operands\n");
    for (i = 0; i < sizeof LINKS / sizeof LINKS[0]; i++) if (LINKS[i].v < UNIT_RECORD) sputf(o, "%d\t.%s\tname\n", LINKS[i].v, LINKS[i].name);
    sputf(o, "%d\t.unit\tversion\n", UNIT_RECORD);
    for (i = 0; i < sizeof LINKS / sizeof LINKS[0]; i++) if (LINKS[i].v > UNIT_RECORD) sputf(o, "%d\t.%s\tname\n", LINKS[i].v, LINKS[i].name);
}

static void render_c(Str *o) {
    int i; size_t k;
    sputs(o, "/* Generated from unisa/tape.py SHAPE; do not edit. */\n");
    sputf(o, "#define TB_OPCOUNT %d\n", NROWS);
    sputf(o, "#define TB_RECORD_UNIT %d\n", UNIT_RECORD);
    for (k = 0; k < sizeof LINKS / sizeof LINKS[0]; k++) {
        char up[16]; size_t j;
        for (j = 0; LINKS[k].name[j] && j < sizeof up - 1; j++) up[j] = (char)(LINKS[k].name[j] >= 'a' && LINKS[k].name[j] <= 'z' ? LINKS[k].name[j] - 32 : LINKS[k].name[j]);
        up[j] = 0;
        sputf(o, "#define TB_RECORD_%s %d\n", up, LINKS[k].v);
    }
    sputs(o, "char *tb_opnames[TB_OPCOUNT] = {");
    for (i = 0; i < NROWS; i++) sputf(o, "%s\"%s\"", i ? ", " : "", SHAPE_ROWS[i].op);
    sputs(o, "};\n");
    sputs(o, "char *tb_shapes[TB_OPCOUNT] = {");
    for (i = 0; i < NROWS; i++) sputf(o, "%s\"%s\"", i ? ", " : "", SHAPE_ROWS[i].shape);
    sputs(o, "};\n");
}

static char *read_file(const char *path, size_t *n) {
    FILE *f = fopen(path, "rb"); char *b; size_t cap = 1 << 16, len = 0, k;
    if (!f) die("cannot open", path);
    b = malloc(cap);
    if (!b) die("out of memory", NULL);
    while ((k = fread(b + len, 1, cap - len, f)) > 0) { len += k; if (len == cap) { cap *= 2; b = realloc(b, cap); if (!b) die("out of memory", NULL); } }
    fclose(f);
    *n = len;
    return b;
}

int main(int argc, char **argv) {
    Str out = {0};
    if (argc == 2 && !strcmp(argv[1], "render")) render(&out);
    else if (argc == 2 && !strcmp(argv[1], "records")) render_records(&out);
    else if (argc == 2 && !strcmp(argv[1], "c")) render_c(&out);
    else if (argc == 4 && !strcmp(argv[1], "check")) {
        Str want = {0}; size_t n = 0; char *got;
        if (!strcmp(argv[2], "render")) render(&want);
        else if (!strcmp(argv[2], "records")) render_records(&want);
        else if (!strcmp(argv[2], "c")) render_c(&want);
        else die("unknown kind", argv[2]);
        got = read_file(argv[3], &n);
        if (n != want.n || memcmp(got, want.p, n)) die("stale", argv[3]);
        fputs("OK\n", stdout);
        return 0;
    } else {
        fprintf(stderr, "usage: unisa-tapebin-shape render | records | c | check KIND FILE\n");
        return 2;
    }
    if (out.n) fwrite(out.p, 1, out.n, stdout);
    return 0;
}
