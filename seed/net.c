/* C99 counterpart of exec/c/net.py: flat table -> threshold network.
   The action arities are the runtime's declared wire interface. */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../exec/c/core.h"

typedef struct { int64_t *v; size_t n; } Numbers;
typedef struct { char **v; size_t n, cap; } Lines;

static void fail(const char *why) { fprintf(stderr, "seed-net: %s\n", why); exit(1); }
static void *alloc(size_t n) { void *p = malloc(n ? n : 1); if (!p) fail("out of memory"); return p; }
static void *grow(void *p, size_t n) { void *q = realloc(p, n ? n : 1); if (!q) fail("out of memory"); return q; }

static char *read_line(FILE *f) {
    size_t n = 0, cap = 256;
    char *s = alloc(cap);
    int c;
    while ((c = fgetc(f)) != EOF && c != '\n') {
        if (n + 1 >= cap) {
            if (cap > SIZE_MAX / 2) fail("line too long");
            cap *= 2; s = grow(s, cap);
        }
        s[n++] = (char)c;
    }
    if (c == EOF && n == 0) { free(s); return NULL; }
    if (n && s[n - 1] == '\r') n--;
    s[n] = 0;
    return s;
}

static void push_line(Lines *a, char *s) {
    if (a->n == a->cap) {
        if (a->cap > SIZE_MAX / 2 / sizeof(*a->v)) fail("too many lines");
        a->cap = a->cap ? a->cap * 2 : 128;
        a->v = grow(a->v, a->cap * sizeof(*a->v));
    }
    a->v[a->n++] = s;
}

static Numbers numbers(const char *s, char tag) {
    Numbers a = {0, 0}; size_t cap = 0;
    if (*s++ != tag || (*s && *s != ' ' && *s != '\t')) fail("record tag");
    while (*s) {
        char *end; long long x;
        while (*s == ' ' || *s == '\t') s++;
        if (!*s) break;
        errno = 0; x = strtoll(s, &end, 10);
        if (errno || end == s || (*end && *end != ' ' && *end != '\t')) fail("integer record");
        if (a.n == cap) { cap = cap ? cap * 2 : 16; a.v = grow(a.v, cap * sizeof(*a.v)); }
        a.v[a.n++] = x; s = end;
    }
    return a;
}

static int index_int(int64_t x) {
    if (x < 0 || x > 100000000) fail("record count outside bound");
    return (int)x;
}

static int64_t answer(const Numbers *r, const int *where, int lo, int key, int part) {
    int j = where[key - lo];
    return j < 0 ? r->v[part == 1 ? 2 : 3] : r->v[(size_t)j + (size_t)part];
}

static void emit_row(FILE *out, const Numbers *r, int top, int ns, int nq, size_t *total) {
    int mode, n, lo, hi, rs = -1, best = 1;
    int *count = NULL, *where; unsigned char *ret = NULL;
    int64_t lastn, lastq, bn, bq;
    size_t j, units = 0;
    if (r->n < 4) fail("short row");
    mode = index_int(r->v[0]); n = index_int(r->v[1]);
    if (mode > 2 || r->n != (size_t)4 + (size_t)3 * n) fail("row shape");
    lo = mode == 1 ? -1 : 0; hi = mode == 1 ? top : 256;
    where = alloc((size_t)(hi - lo + 1) * sizeof(*where));
    for (int x = lo; x <= hi; x++) where[x - lo] = -1;
    if (r->v[2] < -1 || r->v[2] >= ns || r->v[3] < 0 || r->v[3] >= nq) fail("default answer");
    if (mode == 1 && (r->v[2] != -1 || r->v[3] != 0)) fail("stack default");
    for (j = 4; j < r->n; j += 3) {
        if (r->v[j] < lo || r->v[j] > hi || r->v[j+1] < -1 || r->v[j+1] >= ns ||
            r->v[j+2] < 0 || r->v[j+2] >= nq) fail("row entry");
        if (where[r->v[j] - lo] >= 0) fail("duplicate row key");
        where[r->v[j] - lo] = (int)j;
    }
    if (mode == 1) {
        count = alloc((size_t)nq * sizeof(*count)); memset(count, 0, (size_t)nq * sizeof(*count));
        for (j = 4; j < r->n; j += 3)
            if (r->v[j] >= 0 && r->v[j+1] == r->v[j]) count[r->v[j+2]]++;
        for (j = 0; j < (size_t)nq; j++) if (count[j] > best) { best = count[j]; rs = (int)j; }
        if (rs >= 0) {
            ret = alloc((size_t)top + 1); memset(ret, 0, (size_t)top + 1);
            for (j = 4; j < r->n; j += 3)
                if (r->v[j] >= 0 && r->v[j+1] == r->v[j] && r->v[j+2] == rs) ret[r->v[j]] = 1;
        }
    }
    bn = lastn = answer(r, where, lo, lo, 1); bq = lastq = answer(r, where, lo, lo, 2);
    /* Count before emitting, matching net.py's H header. */
    for (int x = lo + 1; x <= hi; x++) {
        int64_t nx, sq;
        if (ret && ret[x]) continue;
        nx = answer(r, where, lo, x, 1); sq = answer(r, where, lo, x, 2);
        if (nx != lastn || sq != lastq) units++;
        lastn = nx; lastq = sq;
    }
    fprintf(out, "H %d %d %d %zu %lld %lld", rs >= 0 ? 3 : mode, lo, hi, units,
            (long long)bn, (long long)bq);
    if (rs >= 0) {
        int m = 0;
        for (int x = 0; x <= top; x++) if (ret[x]) m++;
        fprintf(out, " %d %d", rs, m);
        for (int x = 0; x <= top; x++) if (ret[x]) fprintf(out, " %d", x);
    }
    lastn = bn; lastq = bq;
    fputc(' ', out);
    size_t emitted = 0;
    for (int x = lo + 1; x <= hi; x++) {
        int64_t nx, sq;
        if (ret && ret[x]) continue;
        nx = answer(r, where, lo, x, 1); sq = answer(r, where, lo, x, 2);
        if (nx != lastn || sq != lastq) {
            if (emitted++) fputc(' ', out);
            fprintf(out, "%d %lld %lld", x, (long long)(nx - lastn), (long long)(sq - lastq));
        }
        lastn = nx; lastq = sq;
    }
    fputc('\n', out); *total += units;
    free(count); free(ret); free(where);
}

int main(int argc, char **argv) {
    FILE *in, *out; Lines lines = {0, 0, 0}; char *s;
    Numbers h; int ns, nq, nr, nstr, start, top; size_t j, total = 0;
    if (argc != 3) fail("usage: seed-net INPUT.tbl OUTPUT.net");
    in = fopen(argv[1], "rb"); if (!in) fail("cannot open input");
    while ((s = read_line(in)) != NULL) push_line(&lines, s);
    if (ferror(in) || fclose(in) || !lines.n) fail("cannot read input");
    h = numbers(lines.v[0], 'T'); if (h.n != 5) fail("table header");
    ns = index_int(h.v[0]); nq = index_int(h.v[1]); nr = index_int(h.v[2]);
    nstr = index_int(h.v[3]); start = index_int(h.v[4]);
    if (!ns || !nq || start >= ns || lines.n != 1u+(size_t)nstr+(size_t)nq+(size_t)ns) fail("table extent");
    top = ns - 1;
    for (j = 1u + (size_t)nstr; j < 1u + (size_t)nstr + (size_t)nq; j++) {
        Numbers q = numbers(lines.v[j], 'Q'); size_t p = 1;
        if (!q.n) fail("sequence shape");
        for (int k = 0; k < index_int(q.v[0]); k++) {
            int op;
            if (p >= q.n) fail("sequence truncation");
            op = index_int(q.v[p]); if (op >= NOP_ || p+1u+(size_t)ARITY[op] > q.n) fail("action shape");
            if (op == PUSH && q.v[p+1] > top) top = index_int(q.v[p+1]);
            p += 1u + (size_t)ARITY[op];
        }
        if (p != q.n) fail("sequence tail"); free(q.v);
    }
    for (j = 1u + (size_t)nstr + (size_t)nq; j < lines.n; j++) {
        Numbers r = numbers(lines.v[j], 'R');
        if (r.n < 4) fail("short row");
        if (r.v[0] == 1)
            for (size_t k = 4; k+2 < r.n; k += 3) if (r.v[k] > top) top = index_int(r.v[k]);
        free(r.v);
    }
    out = fopen(argv[2], "wb"); if (!out) fail("cannot open output");
    fprintf(out, "N %d %d %d %d %d %d\n", ns, nq, nr, nstr, start, top);
    for (j = 1; j < 1u+(size_t)nstr+(size_t)nq; j++) fprintf(out, "%s\n", lines.v[j]);
    for (; j < lines.n; j++) {
        Numbers r = numbers(lines.v[j], 'R'); emit_row(out, &r, top, ns, nq, &total); free(r.v);
    }
    if (fclose(out)) fail("cannot write output");
    for (j = 0; j < lines.n; j++) free(lines.v[j]);
    free(lines.v); free(h.v);
    fprintf(stderr, "threshold network: %d banks, %zu hidden units\n", ns, total);
    return 0;
}
