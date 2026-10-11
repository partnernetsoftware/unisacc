/* seed/unisa_zdata.c -- C99 reference for unisa.padded.ZData (K6-4b).
 *
 *   unisa-zdata OPS
 *       OPS is a text file of operations; each op is one line.  The interpreter mirrors
 *       unisa/padded.py ZData segment semantics (zero segments are lengths, data segments are
 *       bytes; extend merges big all-zero runs into a zero segment; a slice write that makes a
 *       zero segment non-zero splits it).  Observable output per op, in order:
 *         extend HEX             bytes via hex (all-zero and >= BIG => zero segment)
 *         zeros K                extend(bytes(K))
 *         extzero K              extend_zero(K)
 *         set S HEX              self[S:S+len]=bytes  (length must match, else refused)
 *         read S E               print the bytes of self[S:E] as hex
 *         count S E              print count(0, S, E)
 *         len                    print len(self)
 *         dump                   print every segment: SEG start Z len  |  SEG start D len hex
 *         copy M BASE            copy_into a zeroed mem of size M at BASE; print mem as hex
 *       Negative ops (both sides must refuse, nothing printed for them):
 *         stepread S E STEP      slice with step != 1 is refused (python: assert step == 1)
 *         countx S E X           count(X, S, E) with X != 0 is refused
 *         setlen S HEX           write whose length differs from the slice is refused
 *
 * Point-sample equivalence over the scripted operation sequences only.  Not a product closure
 * member; keeps unisa/padded.py.  Refusal stops the interpreter at that op (exit 1).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BIG 4096

typedef struct { size_t start; int zero; size_t len; unsigned char *d; } Seg;
static Seg *segs; static size_t nseg, capseg; static size_t n;

static void die(const char *why, const char *what) {
    fprintf(stderr, "unisa-zdata: %s %s\n", why, what ? what : "");
    exit(1);
}

static void seg_reserve(size_t k) {
    if (k > capseg) { size_t nc = capseg ? capseg * 2 : 16; while (nc < k) nc *= 2;
        segs = realloc(segs, nc * sizeof *segs); if (!segs) die("out of memory", NULL); capseg = nc; }
}
static void seg_insert(size_t at, Seg s) {
    seg_reserve(nseg + 1);
    memmove(&segs[at + 1], &segs[at], (nseg - at) * sizeof *segs);
    segs[at] = s; nseg++;
}
static void seg_remove(size_t at) { memmove(&segs[at], &segs[at + 1], (nseg - at - 1) * sizeof *segs); nseg--; }

static int hexval(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
static unsigned char *from_hex(const char *h, size_t *outn) {
    size_t hn = strlen(h), i;
    unsigned char *b;
    if (hn % 2) die("odd-length hex", h);
    b = malloc(hn / 2 + 1); if (!b) die("out of memory", NULL);
    for (i = 0; i < hn / 2; i++) {
        int a = hexval(h[2*i]), c = hexval(h[2*i+1]);
        if (a < 0 || c < 0) { free(b); die("non-hex digit", h); }
        b[i] = (unsigned char)(a * 16 + c);
    }
    *outn = hn / 2; return b;
}
static int all_zero(const unsigned char *p, size_t k) { size_t i; for (i = 0; i < k; i++) if (p[i]) return 0; return 1; }

static void zd_extend(const unsigned char *raw, size_t k) {
    if (!k) return;
    if (k >= BIG && all_zero(raw, k)) {
        if (nseg && segs[nseg-1].zero) segs[nseg-1].len += k;
        else { Seg s = {n, 1, k, NULL}; seg_insert(nseg, s); }
    } else if (nseg && !segs[nseg-1].zero) {
        Seg *l = &segs[nseg-1];
        l->d = realloc(l->d, l->len + k); if (!l->d) die("out of memory", NULL);
        memcpy(l->d + l->len, raw, k); l->len += k;
    } else {
        Seg s = {n, 0, k, malloc(k)}; if (!s.d) die("out of memory", NULL);
        memcpy(s.d, raw, k); seg_insert(nseg, s);
    }
    n += k;
}
static void zd_extend_zero(size_t k) {
    if (!k) return;
    if (nseg && segs[nseg-1].zero) segs[nseg-1].len += k;
    else { Seg s = {n, 1, k, NULL}; seg_insert(nseg, s); }
    n += k;
}

/* index of the segment containing or preceding s (python: max(bisect_right(starts,s)-1, 0)) */
static size_t first_index(size_t s) {
    size_t i = 0, j;
    for (j = 0; j < nseg; j++) { if (segs[j].start <= s) i = j; else break; }
    return i;
}

static void zd_get(size_t s, size_t e, unsigned char *out) {
    size_t i;
    memset(out, 0, e - s);
    for (i = first_index(s); i < nseg && segs[i].start < e; i++) {
        size_t a = segs[i].start, b = a + segs[i].len;
        size_t lo = s > a ? s : a, hi = e < b ? e : b;
        if (b > s && !segs[i].zero && lo < hi) memcpy(out + (lo - s), segs[i].d + (lo - a), hi - lo);
    }
}

static void zd_set(size_t s, size_t e, const unsigned char *raw) {
    size_t i, nsplit = 0, k;
    struct { size_t idx, a, len, lo, hi; } sp[64];
    for (i = first_index(s); i < nseg && segs[i].start < e; i++) {
        size_t a = segs[i].start, b = a + segs[i].len;
        size_t lo = s > a ? s : a, hi = e < b ? e : b;
        if (b <= s || lo >= hi) continue;
        if (!segs[i].zero) memcpy(segs[i].d + (lo - a), raw + (lo - s), hi - lo);
        else if (!all_zero(raw + (lo - s), hi - lo)) {
            if (nsplit >= 64) die("too many splits", NULL);
            sp[nsplit].idx = i; sp[nsplit].a = a; sp[nsplit].len = segs[i].len;
            sp[nsplit].lo = lo; sp[nsplit].hi = hi; nsplit++;
        }
    }
    for (k = nsplit; k > 0; k--) {
        size_t idx = sp[k-1].idx, a = sp[k-1].a, len = sp[k-1].len, lo = sp[k-1].lo, hi = sp[k-1].hi;
        unsigned char *part = malloc(hi - lo); size_t m = 0;
        Seg pieces[3]; size_t np = 0;
        if (!part) die("out of memory", NULL);
        memcpy(part, raw + (lo - s), hi - lo); (void)m;
        if (lo > a) { Seg z = {a, 1, lo - a, NULL}; pieces[np++] = z; }
        { Seg d = {lo, 0, hi - lo, part}; pieces[np++] = d; }
        if (a + len > hi) { Seg z = {hi, 1, a + len - hi, NULL}; pieces[np++] = z; }
        seg_remove(idx);
        { size_t q; for (q = 0; q < np; q++) seg_insert(idx + q, pieces[q]); }
    }
}

static size_t zd_count0(size_t s, size_t e) {
    size_t i, total = 0;
    for (i = first_index(s); i < nseg && segs[i].start < e; i++) {
        size_t a = segs[i].start, b = a + segs[i].len;
        size_t lo = s > a ? s : a, hi = e < b ? e : b;
        if (b <= s || lo >= hi) continue;
        if (segs[i].zero) total += hi - lo;
        else { size_t j; for (j = lo; j < hi; j++) if (segs[i].d[j - a] == 0) total++; }
    }
    return total;
}

static void print_hex(const unsigned char *p, size_t k) { size_t i; for (i = 0; i < k; i++) printf("%02x", p[i]); }

int main(int argc, char **argv) {
    FILE *f; char line[1 << 16];
    if (argc != 2) { fprintf(stderr, "usage: unisa-zdata OPS\n"); return 2; }
    f = fopen(argv[1], "r"); if (!f) die("cannot open", argv[1]);
    while (fgets(line, sizeof line, f)) {
        char op[32]; char a1[1 << 15]; char a2[1 << 15]; char a3[64];
        size_t s, e, k, m, base, nb;
        unsigned char *raw;
        size_t lines = strlen(line);
        if (lines && line[lines-1] == '\n') line[--lines] = 0;
        if (!lines || line[0] == '#') continue;
        a1[0] = a2[0] = a3[0] = 0;
        if (sscanf(line, "%31s", op) != 1) continue;
        if (!strcmp(op, "extend")) { sscanf(line, "%*s %32767s", a1); raw = from_hex(a1, &nb); zd_extend(raw, nb); free(raw); }
        else if (!strcmp(op, "zeros")) { sscanf(line, "%*s %zu", &k); raw = calloc(k ? k : 1, 1); if (!raw) die("out of memory", NULL); zd_extend(raw, k); free(raw); }
        else if (!strcmp(op, "extzero")) { sscanf(line, "%*s %zu", &k); zd_extend_zero(k); }
        else if (!strcmp(op, "set")) {
            sscanf(line, "%*s %zu %32767s", &s, a1); raw = from_hex(a1, &nb);
            if (s + nb > n) { free(raw); die("slice out of range", line); }
            zd_set(s, s + nb, raw); free(raw);
        }
        else if (!strcmp(op, "setlen")) {
            sscanf(line, "%*s %zu %zu %32767s", &s, &e, a1); raw = from_hex(a1, &nb);
            if (nb != e - s) { free(raw); die("write changes the length", line); }
            zd_set(s, e, raw); free(raw);
        }
        else if (!strcmp(op, "read")) {
            sscanf(line, "%*s %zu %zu", &s, &e);
            if (e > n || s > e) die("slice out of range", line);
            raw = malloc(e - s + 1); if (!raw) die("out of memory", NULL);
            zd_get(s, e, raw); print_hex(raw, e - s); printf("\n"); free(raw);
        }
        else if (!strcmp(op, "stepread")) {
            size_t step; sscanf(line, "%*s %zu %zu %zu", &s, &e, &step);
            if (step != 1) die("ZData: stepped slice", line);
            die("unexpected stepread", line);
        }
        else if (!strcmp(op, "count")) { sscanf(line, "%*s %zu %zu", &s, &e); if (e > n) e = n; printf("%zu\n", zd_count0(s, e)); }
        else if (!strcmp(op, "countx")) {
            long x; sscanf(line, "%*s %zu %zu %ld", &s, &e, &x);
            if (x != 0) die("ZData.count counts zeros only", line);
            die("unexpected countx", line);
        }
        else if (!strcmp(op, "len")) printf("%zu\n", n);
        else if (!strcmp(op, "dump")) {
            size_t i;
            for (i = 0; i < nseg; i++) {
                if (segs[i].zero) printf("SEG %zu Z %zu\n", segs[i].start, segs[i].len);
                else { printf("SEG %zu D %zu ", segs[i].start, segs[i].len); print_hex(segs[i].d, segs[i].len); printf("\n"); }
            }
        }
        else if (!strcmp(op, "copy")) {
            size_t i; sscanf(line, "%*s %zu %zu", &m, &base);
            unsigned char *mem = calloc(m ? m : 1, 1); if (!mem) die("out of memory", NULL);
            for (i = 0; i < nseg; i++) {
                if (segs[i].zero) continue;
                if (base + segs[i].start + segs[i].len > m) { free(mem); die("copy_into past mem", NULL); }
                memcpy(mem + base + segs[i].start, segs[i].d, segs[i].len);
            }
            print_hex(mem, m); printf("\n"); free(mem);
        }
        else die("unknown op", op);
        (void)a2; (void)a3;
    }
    fclose(f);
    return 0;
}
