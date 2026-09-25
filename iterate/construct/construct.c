/* construct.c -- the weight constructor, in the C subset unisacc compiles. [J10]
 *
 * Reads one gold table, weights/gold/<stage>.tsv, and prints the net that
 * unisa/construct.py build_net builds from it, in the canonical text form
 * that tools/netdump.py prints from the Python side.  Standalone; it is not
 * part of the compiler.
 *
 *   construct [-d] weights/gold/prec.tsv
 *   construct -u out.uns2 weights/gold/prec.tsv weights/gold/reloc.tsv
 *   construct -t weights/gold/tyinfo.tsv
 *   construct -Q      (self-test of the quotient-key sets at word boundaries)
 *   construct -G      (self-test of the ws_ word-set layer, incl. lexicographic order)
 *
 * -u writes the UNS2 blob (unisa/uns2.py dump) of the stages given.  Every
 * mode checks the deployment invariants over the full domain (exit 3).
 *
 * -d adds the intermediate dumps (quotient groups, decision list with ranks,
 * chosen units) so a divergence can be located at its first step.
 *
 * Scope: at most 3 fields, MAXH heads, MAXOK raw keys and MAXQ quotient keys; a set
 * of quotient keys is QW = ceil(MAXQ / QB) longs of QB (62) bits, see "quotient-key sets" below.  Checked stages: prec, reloc (one head) and tyinfo
 * (three heads: per-head candidates, pick, T4 cross-head sharing).  -t
 * prints which multi-head branches a stage took; README.md lists the ones
 * tyinfo does not reach.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define MAXF 3
#define MAXV 128   /* raw values per field: storage only, never a bit index */
#define MAXH 16   /* heads: storage only (no head bitset); checked by the reader, exit 4 */
#define MAXC 96   /* classes per head; a class set is CW words of the ws_ layer */
#define CW 2       /* ceil(MAXC / 62) */
#define MAXOK 4352
#define MAXQ 3200  /* quotient keys; a key set is QW longs (qset below) */
#define QB 62      /* key bits per word: bits 0..61, never bit 62 or the sign bit 63 */
#define QW ((MAXQ + QB - 1) / QB)   /* ceil(MAXQ / QB) */
#define MAXG 124  /* value groups per field: a field's group set is GW words (ws_) */
#define GW 2       /* ceil(MAXG / 62) */
#define MAXR 96   /* decision-list rules; <= MAXU (rep_factored cap = nr units), checked in main */
#define MAXU 200
#define MAXCU MAXR  /* candidate row capacity: a candidate has <= nr <= MAXR units (README) */
#define MAXCAND 288  /* candidate slots, a global count over all heads and T4 rounds; abi needs
                        270 under its construction order (README) -- not a general bound */
#define BUFSZ 262144
#define MAXP 512

char buf[BUFSZ + 1];
char *parts[MAXP];
int np;
char *gpath;
char *sname;
int nf;
char *fname[MAXF];
int nv[MAXF];
char *val[MAXF][MAXV];
int nh;
char *hname[MAXH];
int ncl[MAXH];
char *cls[MAXH][MAXC];
int nok;
int ostride[MAXF];
int olab[MAXOK][MAXH];
int oseen[MAXOK];
int dbg;
int tflag;                   /* -t: print the branch trace instead of the net */
int tbreak;                  /* test entry: 1 bias, 2 act (see build) */

void die(char *msg) {
    printf("construct: %s: %s\n", gpath, msg);
    exit(1);
}

void dieln(int ln, char *msg) {
    printf("construct: %s: line %d: %s\n", gpath, ln, msg);
    exit(1);
}

int streq(char *a, char *b) { return strcmp(a, b) == 0; }

int prefix(char *s, char *p) {
    int i = 0;
    while (p[i]) { if (s[i] != p[i]) return 0; i = i + 1; }
    return 1;
}

/* the header column for head h is "=> " + name */
int headcol(char *s, char *hn) {
    if (s[0] != '=' || s[1] != '>' || s[2] != ' ') return 0;
    return streq(s + 3, hn);
}

void split(char *line) {
    int i = 0;
    np = 1;
    parts[0] = line;
    while (line[i]) {
        if (line[i] == '\t') {
            if (np >= MAXP) die("too many columns");
            line[i] = 0;
            parts[np] = line + i + 1;
            np = np + 1;
        }
        i = i + 1;
    }
}

/* The input contract of unisa/tsvgold.py load_stage, check for check. */
void load(char *path) {
    FILE *f;
    int n, p, q, e, ln, header, i, j, k, v, c;
    char *line;
    gpath = path;
    f = fopen(path, "rb");
    if (!f) die("cannot open");
    n = (int)fread(buf, 1, BUFSZ, f);
    fclose(f);
    if (n >= BUFSZ) die("file too large");
    buf[n] = 0;
    p = 0; ln = 0; header = 0; nf = 0; nh = 0; sname = 0;
    while (p < n) {
        line = buf + p;
        q = p;
        while (q < n && buf[q] != '\n' && buf[q] != '\r') q = q + 1;
        e = q;
        if (q < n) {
            if (buf[q] == '\r' && q + 1 < n && buf[q + 1] == '\n') q = q + 1;
            q = q + 1;
        }
        buf[e] = 0;
        p = q;
        ln = ln + 1;
        if (prefix(line, "# stage ")) {
            sname = line + 8;
            i = 0;
            while (sname[i] && sname[i] != ':') i = i + 1;
            sname[i] = 0;
            continue;
        }
        split(line);
        if (streq(parts[0], "#field")) {
            if (header) dieln(ln, "schema after the header");
            if (np < 3) dieln(ln, "a field with no values");
            if (nf >= MAXF) dieln(ln, "more fields than this constructor holds");
            if (np - 2 > MAXV) dieln(ln, "more values than this constructor holds");
            fname[nf] = parts[1];
            nv[nf] = np - 2;
            for (i = 2; i < np; i = i + 1) val[nf][i - 2] = parts[i];
            nf = nf + 1;
            continue;
        }
        if (streq(parts[0], "#head")) {
            if (header) dieln(ln, "schema after the header");
            if (np < 4) dieln(ln, "a head with no classes");
            if (nh >= MAXH) {   /* before hname/ncl/cls[nh] are written: exit 4 */
                printf("construct: %s: line %d: capacity: head %s is head %d, more than %d\n", gpath, ln, parts[1], nh + 1, MAXH);
                exit(4);
            }
            /* a class set (gcls, bcls, rankset) is CW words of the ws_ layer:
               class c is element c and its rank crank[c] is element crank[c],
               both < ncl.  ncl <= MAXC = 96 keeps both inside CW = 2 words
               (the second word holds 34).  Checked here, before cls[] is
               stored and before any class set is written: exit 4. */
            if (np - 3 > MAXC) {
                printf("construct: %s: capacity: head %s has %d classes, more than %d\n", gpath, parts[1], np - 3, MAXC);
                exit(4);
            }
            hname[nh] = parts[1];
            ncl[nh] = np - 3;
            for (i = 3; i < np; i = i + 1) cls[nh][i - 3] = parts[i];
            nh = nh + 1;
            continue;
        }
        if (!header) {
            if (np != nf + nh) dieln(ln, "header is not the schema's");
            for (i = 0; i < nf; i = i + 1)
                if (!streq(parts[i], fname[i])) dieln(ln, "header is not the schema's");
            for (i = 0; i < nh; i = i + 1)
                if (!headcol(parts[nf + i], hname[i])) dieln(ln, "header is not the schema's");
            header = 1;
            nok = 1;
            for (i = nf - 1; i >= 0; i = i - 1) {
                ostride[i] = nok;
                nok = nok * nv[i];
                if (nok > MAXOK) {   /* before oseen/olab are indexed: exit 4 */
                    printf("construct: %s: capacity: %d raw keys so far, more than %d\n", gpath, nok, MAXOK);
                    exit(4);
                }
            }
            for (k = 0; k < nok; k = k + 1) oseen[k] = 0;
            continue;
        }
        if (np != nf + nh) dieln(ln, "wrong number of columns");
        k = 0;
        for (i = 0; i < nf; i = i + 1) {
            v = -1;
            for (j = 0; j < nv[i]; j = j + 1) if (streq(parts[i], val[i][j])) { v = j; break; }
            if (v < 0) dieln(ln, "not a value of its field");
            k = k + v * ostride[i];
        }
        for (i = 0; i < nh; i = i + 1) {
            c = -1;
            for (j = 0; j < ncl[i]; j = j + 1) if (streq(parts[nf + i], cls[i][j])) { c = j; break; }
            if (c < 0) dieln(ln, "not a class of its head");
            olab[k][i] = c;
        }
        if (oseen[k]) dieln(ln, "key repeats");
        oseen[k] = 1;
    }
    if (sname == 0 || nf == 0 || nh == 0 || !header) die("not a gold table with a schema");
    for (i = 0; i < nf; i = i + 1)
        for (j = 0; j < i; j = j + 1) if (streq(fname[i], fname[j])) die("field names repeat");
    for (i = 0; i < nh; i = i + 1)
        for (j = 0; j < i; j = j + 1) if (streq(hname[i], hname[j])) die("head names repeat");
    for (i = 0; i < nf; i = i + 1)
        for (j = 0; j < nv[i]; j = j + 1)
            for (k = 0; k < j; k = k + 1) if (streq(val[i][j], val[i][k])) die("values repeat");
    for (i = 0; i < nh; i = i + 1)
        for (j = 0; j < ncl[i]; j = j + 1)
            for (k = 0; k < j; k = k + 1) if (streq(cls[i][j], cls[i][k])) die("classes repeat");
    for (k = 0; k < nok; k = k + 1) if (!oseen[k]) die("keys do not cover the fields' product");
}

/* ------------------------------------------------------------ the domain -- */
int grp[MAXF][MAXV];      /* value -> quotient group */
int ng[MAXF];
int gfirst[MAXF][MAXV];   /* group -> its first (smallest) value */
int nq;
int qk[MAXQ][MAXF];       /* quotient key -> group per field, product order */
int qlab[MAXH][MAXQ];      /* head, quotient key -> class */
int ch;                   /* the head being constructed */
long qmask[MAXF * MAXV][QW]; /* field i, group g at row i * MAXV + g -> keys having it */
long ALL[QW];
int nqw;                  /* words in use: (nq + QB - 1) / QB */
/* field-group sets: a cube is long c[MAXF * GW]; field i's group set is the
   GW words at c + i * GW, over the domain of ng[i] groups (gnw[i] active
   words, tail gtl[i]); FULLW holds the "don't care" set of every field */
long FULLW[MAXF * GW];
int gnw[MAXF];
long gtl[MAXF];
int crank[MAXH][MAXC];     /* head, class -> its position in name order */

int odigit(int k, int i) { return (k / ostride[i]) % nv[i]; }

int sameslice(int i, int u, int v) {
    int k, h, k2;
    for (k = 0; k < nok; k = k + 1) {
        if (odigit(k, i) != v) continue;
        k2 = k + (u - v) * ostride[i];
        for (h = 0; h < nh; h = h + 1) if (olab[k][h] != olab[k2][h]) return 0;
    }
    return 1;
}

long bit(int j) { return ((long)1) << j; }

/* THE checked addition for every weight / logit accumulation (rank weights
   in ranks(), a merged cube's W2 in rep_from_dl, headfail's z, the final
   verifier's z; README "checked accumulation" lists them).  Preconditions:
   long is 64-bit (main checks sizeof(long) == 8) and a, b >= 0 (checked
   here: every summand is a rank weight bit(lv) >= 1, a W2 entry >= 0, or a
   sum of those).  The overflow test a > LONG_MAX - b is made BEFORE a + b
   is computed; LONG_MAX - b cannot overflow for b >= 0.  Exit 6 on either
   failure (5 is the qset self-test's). */
char *sumwhat;               /* the site, for the diagnostic */
long ladd(long a, long b) {
    if (a < 0 || b < 0) {
        printf("construct: %s: overflow guard: negative summand %ld + %ld in %s\n", gpath, a, b, sumwhat);
        exit(6);
    }
    if (a > LONG_MAX - b) {
        printf("construct: %s: overflow: %s: %ld + %ld exceeds LONG_MAX\n", gpath, sumwhat, a, b);
        exit(6);
    }
    return a + b;
}

int popc(long m) {
    int c = 0;
    while (m) { m = m & (m - 1); c = c + 1; }
    return c;
}

/* ------------------------------------------------------- word sets (ws_) -- */
/* The one set layer every multi-word set goes through (quotient keys, and
   the field-group and class sets built on it).  A set over a domain of n
   elements 0..n-1 is long s[sw] (sw = the storage words); element j is bit
   j % WB of word j / WB, WB = 62.  Each helper takes the domain explicitly:
     nw   = ws_nw(n) = ceil(n / WB) active words (0 for the empty domain),
     sw   = storage words, sw >= nw,
     tail = ws_tail(n), the mask of the last active word (0 when n = 0).
   Invariants: every active word is in [0, 2^62) and has no bit at or
   above n; every word from nw to sw - 1 is 0.  Every writing helper
   re-establishes both (it zeroes words nw..sw-1 and masks before anything
   counts or shifts), so a leftover from an earlier, larger domain (another
   field, another stage) can never reach equality, order or a count.  The
   readers look at the active words only.  n = 0: nw = 0, tail = 0, every
   set is empty, every loop runs zero times, ws_cmp says equal. */
#define WB 62
#define WM ((((long)1) << WB) - 1)   /* the 62 low bits: one full word */
int ws_nw(int n) { if (n <= 0) return 0; return (n + WB - 1) / WB; }
long ws_tail(int n) {
    int r;
    if (n <= 0) return 0;
    r = n - (ws_nw(n) - 1) * WB;
    if (r == WB) return WM;
    return (((long)1) << r) - 1;
}
/* the mask of active word w */
long ws_wm(int w, int nw, long tail) { return w == nw - 1 ? tail : WM; }
void ws_zero(long *d, int nw, int sw) { int w; for (w = 0; w < sw; w = w + 1) d[w] = 0; }
void ws_full(long *d, int nw, int sw, long tail) {
    int w;
    for (w = 0; w < nw; w = w + 1) d[w] = ws_wm(w, nw, tail);
    for (w = nw; w < sw; w = w + 1) d[w] = 0;
}
void ws_copy(long *d, long *a, int nw, int sw) {
    int w;
    for (w = 0; w < nw; w = w + 1) d[w] = a[w];
    for (w = nw; w < sw; w = w + 1) d[w] = 0;
}
/* element j < n: the caller's domain check */
void ws_set(long *d, int j) { d[j / WB] = d[j / WB] | (((long)1) << (j % WB)); }
int ws_test(long *a, int j) { return (int)((a[j / WB] >> (j % WB)) & 1); }
void ws_and(long *d, long *a, long *b, int nw, int sw) {
    int w;
    for (w = 0; w < nw; w = w + 1) d[w] = a[w] & b[w];
    for (w = nw; w < sw; w = w + 1) d[w] = 0;
}
void ws_or(long *d, long *a, long *b, int nw, int sw) {
    int w;
    for (w = 0; w < nw; w = w + 1) d[w] = a[w] | b[w];
    for (w = nw; w < sw; w = w + 1) d[w] = 0;
}
/* a & ~b: the ~ makes a word negative when a is (a raw all-ones operand);
   each word is masked to its active bits in the same expression, before
   anything counts or shifts it */
void ws_andnot(long *d, long *a, long *b, int nw, int sw, long tail) {
    int w;
    for (w = 0; w < nw; w = w + 1) d[w] = (a[w] & ~b[w]) & ws_wm(w, nw, tail);
    for (w = nw; w < sw; w = w + 1) d[w] = 0;
}
/* cut every active word to the domain (qtail) */
void ws_mask(long *d, int nw, int sw, long tail) {
    int w;
    for (w = 0; w < nw; w = w + 1) d[w] = d[w] & ws_wm(w, nw, tail);
    for (w = nw; w < sw; w = w + 1) d[w] = 0;
}
int ws_empty(long *a, int nw) { int w; for (w = 0; w < nw; w = w + 1) if (a[w]) return 0; return 1; }
int ws_eq(long *a, long *b, int nw) { int w; for (w = 0; w < nw; w = w + 1) if (a[w] != b[w]) return 0; return 1; }
/* a is a subset of b */
int ws_sub(long *a, long *b, int nw) { int w; for (w = 0; w < nw; w = w + 1) if (a[w] & ~b[w]) return 0; return 1; }
int ws_meets(long *a, long *b, int nw) { int w; for (w = 0; w < nw; w = w + 1) if (a[w] & b[w]) return 1; return 0; }
int ws_meets3(long *a, long *b, long *c, int nw) {
    int w;
    for (w = 0; w < nw; w = w + 1) if (a[w] & b[w] & c[w]) return 1;
    return 0;
}
int ws_popc(long *a, int nw) { int w, n = 0; for (w = 0; w < nw; w = w + 1) n = n + popc(a[w]); return n; }
int ws_popcand(long *a, long *b, int nw) { int w, n = 0; for (w = 0; w < nw; w = w + 1) n = n + popc(a[w] & b[w]); return n; }
/* the smallest member >= j, or -1: ascending */
int ws_next(long *a, int j, int n) {
    while (j < n) {
        if ((a[j / WB] >> (j % WB)) == 0) { j = (j / WB + 1) * WB; continue; }
        if (ws_test(a, j)) return j;
        j = j + 1;
    }
    return -1;
}
/* does a have a member above bit p of word w */
int ws_above(long *a, int w, int p, int nw) {
    int x;
    if ((a[w] >> p) >> 1) return 1;
    for (x = w + 1; x < nw; x = x + 1) if (a[x]) return 1;
    return 0;
}
/* Python's order of the sorted member tuples (lexicographic): scan from the
   LOW word up; at the lowest differing element p the set holding p is
   smaller exactly when the other set has a member above p (else the other
   is a proper prefix).  Not the order of the words as one big integer:
   {0,62} < {1} here. */
int ws_cmp(long *a, long *b, int nw) {
    int w, p;
    long d;
    for (w = 0; w < nw; w = w + 1) {
        d = a[w] ^ b[w];
        if (d == 0) continue;
        p = 0;
        while (!((d >> p) & 1)) p = p + 1;
        if ((a[w] >> p) & 1) return ws_above(b, w, p, nw) ? -1 : 1;
        return ws_above(a, w, p, nw) ? 1 : -1;
    }
    return 0;
}

/* ------------------------------------------------- quotient-key sets ------ */
/* The q* functions are thin wrappers over ws_ on the quotient-key domain
   (n = nq, nw = nqw, tail = the last word of ALL).  They keep their old
   semantics exactly: storage is QW words but only words 0..nqw-1 are read
   or written (sw = nqw), as before.
   A set of quotient keys is long s[QW]; key j is bit j % QB of word j / QB.
   QB is 62, so every stored word is in [0, 2^62): no shift reaches bit 63
   (1 << b with b <= 61), popc never subtracts from a negative value or
   from LONG_MIN, and every right shift is of a non-negative word.  The one
   place a word goes negative is a ~b inside qandnot; a & ~b with a >= 0 is
   >= 0 again, and qtail cuts it to ALL before anything counts or shifts it.
   Only words 0..nqw-1 are read or written.  Every result is a subset of an
   operand that is already inside the domain, or is cut by qtail, so no bit
   at or above nq is ever set: complements are taken only as ALL & ~x. */
long qtl;                  /* ws_tail(nq): ALL's last active word */
void qzero(long *d) { ws_zero(d, nqw, nqw); }
void qcopy(long *d, long *a) { ws_copy(d, a, nqw, nqw); }
void qset(long *d, int j) { ws_set(d, j); }
int qtest(long *a, int j) { return ws_test(a, j); }
void qand(long *d, long *a, long *b) { ws_and(d, a, b, nqw, nqw); }
void qor(long *d, long *a, long *b) { ws_or(d, a, b, nqw, nqw); }
void qtail(long *d) { ws_mask(d, nqw, nqw, qtl); }
void qandnot(long *d, long *a, long *b) { ws_andnot(d, a, b, nqw, nqw, qtl); }
int qempty(long *a) { return ws_empty(a, nqw); }
int qeq(long *a, long *b) { return ws_eq(a, b, nqw); }
int qmeets(long *a, long *b) { return ws_meets(a, b, nqw); }
int qmeets3(long *a, long *b, long *c) { return ws_meets3(a, b, c, nqw); }
int qpopc(long *a) { return ws_popc(a, nqw); }
int qpopcand(long *a, long *b) { return ws_popcand(a, b, nqw); }
/* the smallest member >= j, or -1: ascending, the order of range(D.n) */
int qnext(long *a, int j) { return ws_next(a, j, nq); }
void qsetall(void) {
    nqw = ws_nw(nq);
    qtl = ws_tail(nq);
    ws_full(ALL, nqw, QW, qtl);
}

/* -Q: the set operations at the word boundaries (keys QB-1, QB, 2QB-1, 2QB,
   the first key of the last word and the one before it, and the last key)
   for domain sizes around those boundaries, at the start and at the end of
   the capacity (MAXQ); every word must stay in [0, 2^62); exit 5 on the
   first failure */
long sa[QW], sb[QW], sc[QW];
void qfail(int n, char *what) { printf("construct: qset self-test: nq %d: %s\n", n, what); exit(5); }
void qselftest(void) {
    int sizes[14], keys[8], t, i, j, n, cnt, prev, w;
    sizes[0] = 1; sizes[1] = QB - 1; sizes[2] = QB; sizes[3] = QB + 1; sizes[4] = 2 * QB - 1;
    sizes[5] = 2 * QB; sizes[6] = 2 * QB + 1; sizes[7] = 528;
    sizes[8] = (QW - 1) * QB - 1; sizes[9] = (QW - 1) * QB; sizes[10] = (QW - 1) * QB + 1;
    sizes[11] = MAXQ - 1; sizes[12] = MAXQ;
    sizes[13] = QW * QB;
    for (t = 0; t < 14; t = t + 1) {
        nq = sizes[t];
        if (nq > MAXQ) break;   /* QW * QB > MAXQ: not a legal domain */
        qsetall();
        keys[0] = 0; keys[1] = QB - 1; keys[2] = QB; keys[3] = 2 * QB - 1; keys[4] = 2 * QB; keys[5] = nq - 1;
        keys[6] = (nqw - 1) * QB - 1; keys[7] = (nqw - 1) * QB;   /* the last word's boundary */
        if (qpopc(ALL) != nq) qfail(nq, "popcount of ALL");
        for (w = 0; w < QW; w = w + 1) {
            if (ALL[w] < 0 || (ALL[w] >> QB) != 0) qfail(nq, "ALL has bit 62 or 63 set");
            for (j = 0; j < QB; j = j + 1) {
                i = w * QB + j;
                if (((ALL[w] >> j) & 1) != (i < nq)) qfail(nq, "ALL has a bit outside the domain");
            }
        }
        qzero(sa);
        n = 0;
        for (i = 0; i < 8; i = i + 1) if (keys[i] >= 0 && keys[i] < nq && !qtest(sa, keys[i])) { qset(sa, keys[i]); n = n + 1; }
        for (i = 0; i < 8; i = i + 1) if (keys[i] >= 0 && keys[i] < nq && !qtest(sa, keys[i])) qfail(nq, "test after set");
        if (qpopc(sa) != n) qfail(nq, "popcount");
        for (w = 0; w < nqw; w = w + 1) if (sa[w] < 0 || (sa[w] >> QB) != 0) qfail(nq, "set has bit 62 or 63");
        /* iterate in order: strictly ascending, every member once */
        cnt = 0; prev = -1;
        for (j = qnext(sa, 0); j >= 0; j = qnext(sa, j + 1)) {
            if (j <= prev || !qtest(sa, j)) qfail(nq, "iteration order");
            prev = j; cnt = cnt + 1;
        }
        if (cnt != n) qfail(nq, "iteration count");
        /* complement = ALL & ~a: disjoint, covers, nothing past nq */
        qandnot(sb, ALL, sa);
        if (qpopc(sb) != nq - n) qfail(nq, "complement size");
        if (qmeets(sa, sb)) qfail(nq, "complement meets the set");
        qor(sc, sa, sb);
        if (!qeq(sc, ALL)) qfail(nq, "set | complement != ALL");
        qand(sc, sa, sb);
        if (!qempty(sc)) qfail(nq, "set & complement not empty");
        /* a raw ~ past the tail must be cut: fill every word, then andnot */
        for (w = 0; w < QW; w = w + 1) sc[w] = -1;
        qandnot(sb, sc, sa);
        if (qpopc(sb) != nq - n) qfail(nq, "tail bits survive andnot");
        for (w = 0; w < nqw; w = w + 1) if (sb[w] < 0 || (sb[w] >> QB) != 0) qfail(nq, "andnot left bit 62 or 63");
        if (qeq(sa, sb)) qfail(nq, "equality");
        qcopy(sc, sa);
        if (!qeq(sa, sc)) qfail(nq, "equality of a copy");
        printf("qset self-test nq %d (%d words): members", nq, nqw);
        for (j = qnext(sa, 0); j >= 0; j = qnext(sa, j + 1)) printf(" %d", j);
        printf(", complement %d, ok\n", qpopc(sb));
    }
}

/* -G: the ws_ layer itself, independent of any stage: domain sizes 0, 1,
   61, 62, 63, 96, 123, 124 in storage of GSW = 3 words (one more than the
   124-element domain needs, so the unused-word zeroing is observable);
   every operand starts as junk (all ones, negative words included) and
   every result must be inside [0, 2^62), inside the domain, and zero in the
   unused words.  ws_cmp is checked against an oracle that compares the
   ascending member lists lexicographically, on fixed cases and on pseudo-
   random pairs.  Exit 5 on the first failure. */
#define GSW 3
long ga[GSW], gb[GSW], gc[GSW], gd[GSW];
int gla[GSW * WB], glb[GSW * WB];
int gn, gnwt;
void gfail(char *what) { printf("construct: wset self-test: n %d: %s\n", gn, what); exit(5); }
/* every word in [0, 2^62), no bit at or above n, unused words 0 */
void gwf(long *a, char *what) {
    int w, j;
    for (w = 0; w < GSW; w = w + 1) {
        if (a[w] < 0 || (a[w] >> WB) != 0) gfail(what);
        if (w >= gnwt && a[w] != 0) gfail(what);
        for (j = 0; j < WB; j = j + 1) if (w * WB + j >= gn && ((a[w] >> j) & 1)) gfail(what);
    }
}
void gjunk(long *a) { int w; for (w = 0; w < GSW; w = w + 1) a[w] = -1; }
/* the oracle: member lists, compared as Python compares tuples */
int glist(long *a, int *l) { int n = 0, j; for (j = 0; j < gn; j = j + 1) if ((a[j / WB] >> (j % WB)) & 1) { l[n] = j; n = n + 1; } return n; }
int gorac(long *a, long *b) {
    int na = glist(a, gla), nb = glist(b, glb), i;
    for (i = 0; i < na && i < nb; i = i + 1) if (gla[i] != glb[i]) return gla[i] < glb[i] ? -1 : 1;
    if (na != nb) return na < nb ? -1 : 1;
    return 0;
}
int gncmp;
void gcmp1(long *a, long *b, int want, char *what) {
    int r = ws_cmp(a, b, gnwt), o = gorac(a, b), r2 = ws_cmp(b, a, gnwt);
    if (r != o || r2 != -o) gfail(what);
    if (want != 2 && r != want) gfail(what);
    gncmp = gncmp + 1;
}
/* a set from a -1-terminated member list, members >= n dropped */
void gmk(long *a, int m0, int m1, int m2) {
    ws_zero(a, gnwt, GSW);
    if (m0 >= 0 && m0 < gn) ws_set(a, m0);
    if (m1 >= 0 && m1 < gn) ws_set(a, m1);
    if (m2 >= 0 && m2 < gn) ws_set(a, m2);
}
long grs;
int grnd(void) { grs = (grs * 1103515245 + 12345) % 2147483648; return (int)(grs / 65536); }
void wselftest(void) {
    int sizes[8], keys[8], t, i, j, k, n, cnt, prev;
    long tl;
    sizes[0] = 0; sizes[1] = 1; sizes[2] = 61; sizes[3] = 62; sizes[4] = 63;
    sizes[5] = 96; sizes[6] = 123; sizes[7] = 124;
    grs = 1;
    for (t = 0; t < 8; t = t + 1) {
        gn = sizes[t]; gnwt = ws_nw(gn); tl = ws_tail(gn); gncmp = 0;
        if (gnwt != (gn + WB - 1) / WB || gnwt > GSW) gfail("ws_nw");
        /* the tail: exactly the bits of the last active word below n */
        for (j = 0; j < WB; j = j + 1)
            if (((tl >> j) & 1) != (gn > 0 && (gnwt - 1) * WB + j < gn)) gfail("ws_tail bit");
        if (tl < 0 || (tl >> WB) != 0) gfail("ws_tail has bit 62 or 63");
        /* empty and full */
        gjunk(ga); ws_zero(ga, gnwt, GSW); gwf(ga, "ws_zero");
        if (!ws_empty(ga, gnwt) || ws_popc(ga, gnwt) != 0 || ws_next(ga, 0, gn) != -1) gfail("empty set");
        gjunk(gb); ws_full(gb, gnwt, GSW, tl); gwf(gb, "ws_full");
        if (ws_popc(gb, gnwt) != gn) gfail("popcount of the full set");
        if (gn > 0 && (ws_empty(gb, gnwt) || ws_cmp(ga, gb, gnwt) != -1)) gfail("empty < full");
        gjunk(gc); ws_andnot(gc, gb, gb, gnwt, GSW, tl); gwf(gc, "full andnot full");
        if (!ws_empty(gc, gnwt)) gfail("full andnot full is not empty");
        if (!ws_sub(ga, gb, gnwt) || (gn > 0 && ws_sub(gb, ga, gnwt))) gfail("subset of empty / full");
        /* boundary members */
        keys[0] = 0; keys[1] = WB - 1; keys[2] = WB; keys[3] = gn - 1; keys[4] = gn - 2;
        keys[5] = (gnwt - 1) * WB - 1; keys[6] = (gnwt - 1) * WB; keys[7] = 2 * WB - 1;
        gjunk(ga); ws_zero(ga, gnwt, GSW); n = 0;
        for (i = 0; i < 8; i = i + 1) if (keys[i] >= 0 && keys[i] < gn && !ws_test(ga, keys[i])) { ws_set(ga, keys[i]); n = n + 1; }
        gwf(ga, "ws_set");
        for (i = 0; i < 8; i = i + 1) if (keys[i] >= 0 && keys[i] < gn && !ws_test(ga, keys[i])) gfail("test after set");
        if (ws_popc(ga, gnwt) != n) gfail("popcount of the boundary members");
        cnt = 0; prev = -1;
        for (j = ws_next(ga, 0, gn); j >= 0; j = ws_next(ga, j + 1, gn)) {
            if (j <= prev || !ws_test(ga, j)) gfail("iteration order");
            prev = j; cnt = cnt + 1;
        }
        if (cnt != n) gfail("iteration count");
        /* complement, union, meet, copy, equality */
        gjunk(gc); ws_andnot(gc, gb, ga, gnwt, GSW, tl); gwf(gc, "complement");
        if (ws_popc(gc, gnwt) != gn - n || ws_meets(ga, gc, gnwt)) gfail("complement");
        gjunk(gd); ws_or(gd, ga, gc, gnwt, GSW); gwf(gd, "ws_or");
        if (!ws_eq(gd, gb, gnwt)) gfail("set | complement != full");
        gjunk(gd); ws_and(gd, ga, gc, gnwt, GSW); gwf(gd, "ws_and");
        if (!ws_empty(gd, gnwt) || ws_popcand(ga, gc, gnwt) != 0 || ws_meets3(ga, gb, gc, gnwt)) gfail("set & complement");
        gjunk(gd); ws_copy(gd, ga, gnwt, GSW); gwf(gd, "ws_copy");
        if (!ws_eq(gd, ga, gnwt) || ws_cmp(gd, ga, gnwt) != 0) gfail("a copy is not equal");
        if (n > 0 && ws_eq(ga, gc, gnwt)) gfail("equality");
        /* andnot against a RAW all-ones operand (every word -1, negative):
           masked before popc sees it */
        gjunk(gd); gjunk(gc); ws_andnot(gc, gd, ga, gnwt, GSW, tl); gwf(gc, "all-ones andnot");
        if (ws_popc(gc, gnwt) != gn - n) gfail("tail bits survive all-ones andnot");
        gjunk(gd); gjunk(gc); ws_andnot(gc, ga, gd, gnwt, GSW, tl); gwf(gc, "andnot all-ones");
        if (!ws_empty(gc, gnwt)) gfail("x andnot all-ones is not empty");
        gjunk(gc); ws_mask(gc, gnwt, GSW, tl); gwf(gc, "ws_mask");
        if (!ws_eq(gc, gb, gnwt)) gfail("all-ones masked != full");
        /* lexicographic order: fixed cases (-1 = no member), want or 2 = oracle only */
        gmk(ga, -1, -1, -1); gmk(gb, -1, -1, -1); gcmp1(ga, gb, 0, "empty vs empty");
        if (gn > 0) { gmk(gb, 0, -1, -1); gcmp1(ga, gb, -1, "empty vs {0}"); }
        gmk(ga, 0, gn - 1, -1); gmk(gb, 0, gn - 1, -1); gcmp1(ga, gb, 0, "equal sets");
        if (gn > 2) { gmk(ga, 1, -1, -1); gmk(gb, 1, gn - 1, -1); gcmp1(ga, gb, -1, "a proper prefix"); }
        if (gn > 63) {
            gmk(ga, 0, 62, -1); gmk(gb, 1, -1, -1); gcmp1(ga, gb, -1, "{0,62} vs {1}");
            gmk(ga, 5, 61, -1); gmk(gb, 5, 62, -1); gcmp1(ga, gb, -1, "first difference at the word boundary");
            gmk(ga, 5, 62, -1); gmk(gb, 5, 63, -1); gcmp1(ga, gb, -1, "first difference in word 1");
            gmk(ga, 5, 62, -1); gmk(gb, 5, 62, 63); gcmp1(ga, gb, -1, "a prefix ending in word 1");
            gmk(ga, 61, -1, -1); gmk(gb, 62, -1, -1); gcmp1(ga, gb, -1, "{61} vs {62}");
        }
        if (gn > 70) { gmk(ga, 3, 70, -1); gmk(gb, 70, -1, -1); gcmp1(ga, gb, -1, "{3,70} vs {70}"); }
        /* pseudo-random pairs; sparse ones so equal prefixes and prefixes occur */
        for (k = 0; k < 400 && gn > 0; k = k + 1) {
            ws_zero(ga, gnwt, GSW); ws_zero(gb, gnwt, GSW);
            for (i = 0; i < 1 + k % 5; i = i + 1) { j = grnd() % gn; ws_set(ga, j); if (grnd() % 2) ws_set(gb, j); }
            if (grnd() % 3 == 0) { j = grnd() % gn; ws_set(gb, j); }
            gcmp1(ga, gb, 2, "random pair");
        }
        printf("wset self-test n %d (%d words, tail %d bits): %d boundary members, %d comparisons vs oracle, ok\n", gn, gnwt, popc(tl), n, gncmp);
    }
}

void domain(void) {
    int i, v, g, j, t, k, h;
    for (i = 0; i < nf; i = i + 1) {
        ng[i] = 0;
        for (v = 0; v < nv[i]; v = v + 1) {
            grp[i][v] = -1;
            for (g = 0; g < ng[i]; g = g + 1)
                if (sameslice(i, gfirst[i][g], v)) { grp[i][v] = g; break; }
            if (grp[i][v] < 0) { gfirst[i][ng[i]] = v; grp[i][v] = ng[i]; ng[i] = ng[i] + 1; }
        }
    }
    /* a field's group set is GW words of the ws_ layer: group g is bit
       g % 62 of word g / 62.  ng <= MAXG = GW * 62 keeps every group inside
       the storage.  Checked before any field set is written: exit 4. */
    for (i = 0; i < nf; i = i + 1)
        if (ng[i] > MAXG) {
            printf("construct: %s: capacity: field %s has %d value groups, more than %d\n", gpath, fname[i], ng[i], MAXG);
            exit(4);
        }
    nq = 1;
    for (i = 0; i < nf; i = i + 1) {
        nq = nq * ng[i];
        if (nq > MAXQ) {   /* before any bit(nq) or bit(g): exit 4 */
            printf("construct: %s: capacity: %d quotient keys so far, more than %d (a %d-word bitset)\n", gpath, nq, MAXQ, QW);
            exit(4);
        }
    }
    for (j = 0; j < nq; j = j + 1) {
        t = j;
        for (i = nf - 1; i >= 0; i = i - 1) { qk[j][i] = t % ng[i]; t = t / ng[i]; }
        k = 0;
        for (i = 0; i < nf; i = i + 1) k = k + gfirst[i][qk[j][i]] * ostride[i];
        for (h = 0; h < nh; h = h + 1) qlab[h][j] = olab[k][h];
    }
    qsetall();
    for (i = 0; i < nf; i = i + 1) {
        gnw[i] = ws_nw(ng[i]);
        gtl[i] = ws_tail(ng[i]);
        ws_full(FULLW + i * GW, gnw[i], GW, gtl[i]);
        for (g = 0; g < ng[i]; g = g + 1) qzero(qmask[i * MAXV + g]);
    }
    for (j = 0; j < nq; j = j + 1)
        for (i = 0; i < nf; i = i + 1) qset(qmask[i * MAXV + qk[j][i]], j);
    for (h = 0; h < nh; h = h + 1)
        for (i = 0; i < ncl[h]; i = i + 1) {
            crank[h][i] = 0;
            for (j = 0; j < ncl[h]; j = j + 1) if (strcmp(cls[h][j], cls[h][i]) < 0) crank[h][i] = crank[h][i] + 1;
        }
}

/* ----------------------------------------------------------------- cubes -- */
/* a cube is long c[MAXF * GW]: per field, a GW-word set of groups; the
   field's FULLW set = don't care */
int isfull(long *c, int i) { return ws_eq(c + i * GW, FULLW + i * GW, gnw[i]); }

long cmo[QW];
/* m = the quotient keys inside cube c */
void cubemask(long *c, long *m) {
    int i, g;
    qcopy(m, ALL);
    for (i = 0; i < nf; i = i + 1) {
        if (isfull(c, i)) continue;
        qzero(cmo);
        for (g = 0; g < ng[i]; g = g + 1) if (ws_test(c + i * GW, g)) qor(cmo, cmo, qmask[i * MAXV + g]);
        qand(m, m, cmo);
    }
}

int nlits(long *c) {
    int i, n = 0;
    for (i = 0; i < nf; i = i + 1) if (!isfull(c, i)) n = n + 1;
    return n;
}

int cmpcube(long *a, long *b) {
    int i, r;
    for (i = 0; i < nf; i = i + 1) { r = ws_cmp(a + i * GW, b + i * GW, gnw[i]); if (r) return r; }
    return 0;
}

int samecube(long *a, long *b) {
    int i;
    for (i = 0; i < nf; i = i + 1) if (!ws_eq(a + i * GW, b + i * GW, gnw[i])) return 0;
    return 1;
}

void cpcube(long *d, long *s) {
    int i;
    for (i = 0; i < nf; i = i + 1) ws_copy(d + i * GW, s + i * GW, gnw[i], GW);
}

void prcube(long *c) {
    int i, g, first;
    for (i = 0; i < nf; i = i + 1) {
        printf(" ");
        if (isfull(c, i)) { printf("*"); continue; }
        printf("{");
        first = 1;
        for (g = 0; g < ng[i]; g = g + 1)
            if (ws_test(c + i * GW, g)) { if (!first) printf(","); printf("%d", g); first = 0; }
        printf("}");
    }
}

/* ------------------------------------------- T1+T2: greedy decision list -- */
int ORD[6][3];
int nord;

void orders(void) {
    if (nf == 1) { nord = 1; ORD[0][0] = 0; return; }
    if (nf == 2) { nord = 2; ORD[0][0] = 0; ORD[0][1] = 1; ORD[1][0] = 1; ORD[1][1] = 0; return; }
    nord = 6;
    ORD[0][0] = 0; ORD[0][1] = 1; ORD[0][2] = 2;
    ORD[1][0] = 2; ORD[1][1] = 1; ORD[1][2] = 0;
    ORD[2][0] = 1; ORD[2][1] = 0; ORD[2][2] = 2;
    ORD[3][0] = 0; ORD[3][1] = 2; ORD[3][2] = 1;
    ORD[4][0] = 1; ORD[4][1] = 2; ORD[4][2] = 0;
    ORD[5][0] = 2; ORD[5][1] = 0; ORD[5][2] = 1;
}

long xcur[MAXF][QW], xoth[QW], xnm[QW];
/* out = the keys of the expanded cube */
void expand(int *seed, long *badmask, int *ord, long *cube, long *out) {
    int t, i, j, v;
    for (i = 0; i < nf; i = i + 1) { qcopy(xcur[i], qmask[i * MAXV + seed[i]]); ws_zero(cube + i * GW, gnw[i], GW); ws_set(cube + i * GW, seed[i]); }
    for (t = 0; t < nf; t = t + 1) {
        i = ord[t];
        qcopy(xoth, ALL);
        for (j = 0; j < nf; j = j + 1) if (j != i) qand(xoth, xoth, xcur[j]);
        if (!qmeets(xoth, badmask)) { ws_copy(cube + i * GW, FULLW + i * GW, gnw[i], GW); qcopy(xcur[i], ALL); continue; }
        for (v = 0; v < ng[i]; v = v + 1) {
            if (ws_test(cube + i * GW, v)) continue;
            qor(xnm, xcur[i], qmask[i * MAXV + v]);
            if (!qmeets3(xoth, xnm, badmask)) { ws_set(cube + i * GW, v); qcopy(xcur[i], xnm); }
        }
    }
    cubemask(cube, out);
}

int nr;
long rc[MAXR][MAXF * GW];
int rl[MAXR];
int lv[MAXR];

/* T4's pool: cubes other heads already pay for, sorted by cube_key.  It only
   breaks coverage ties (score = count, in pool, -literals) and, after REDUCE,
   replaces the supercube by the first pool cube between it and the prime. */
long pool[MAXU][MAXF * GW];
int npool;
int talign;                  /* trace: REDUCE aligned to a pool cube */
int ttie;                    /* trace: the pool flag changed the chosen rule */

int inpool(long *c) {
    int t;
    for (t = 0; t < npool; t = t + 1) if (samecube(pool[t], c)) return 1;
    return 0;
}

/* is candidate (cnt, pl, nl, cube, L) better than the best so far */
int betterthan(int cnt, int pl, int nl, long *cube, int L, int bcnt, int bpl, int bnl, long *bcube, int bL) {
    int r;
    if (cnt != bcnt) return cnt > bcnt;
    if (pl != bpl) return pl > bpl;
    if (nl != bnl) return nl < bnl;
    r = cmpcube(cube, bcube);
    if (r) return r < 0;
    return strcmp(cls[ch][L], cls[ch][bL]) < 0;
}

long labmask[MAXC][QW], rem[QW], bad[QW], cm[QW], bcm[QW], newly[QW];
void decision_list(void) {
    long cube[MAXF * GW], bcube[MAXF * GW], ncube[MAXF * GW], sup[MAXF * GW];
    int j, o, L, bL, cnt, bcnt, nl, bnl, have, i, g, pl, bpl, t, ok;
    int nL, ncnt, nnl;
    for (j = 0; j < ncl[ch]; j = j + 1) qzero(labmask[j]);
    for (j = 0; j < nq; j = j + 1) qset(labmask[qlab[ch][j]], j);
    qcopy(rem, ALL);
    nr = 0;
    bL = 0; bcnt = 0; bnl = 0; qzero(bcm); bpl = 0;
    nL = 0; ncnt = 0; nnl = 0;
    while (!qempty(rem)) {
        have = 0;
        for (j = qnext(rem, 0); j >= 0; j = qnext(rem, j + 1)) {
            L = qlab[ch][j];
            qandnot(bad, rem, labmask[L]);
            for (o = 0; o < nord; o = o + 1) {
                expand(qk[j], bad, ORD[o], cube, cm);
                cnt = qpopcand(cm, rem);
                nl = nlits(cube);
                pl = npool ? inpool(cube) : 0;
                if (!have || betterthan(cnt, pl, nl, cube, L, bcnt, bpl, bnl, bcube, bL)) {
                    bcnt = cnt; bpl = pl; bnl = nl; bL = L; qcopy(bcm, cm); cpcube(bcube, cube);
                }
                /* trace only: the same choice with the pool flag held at 0 */
                if (!have || betterthan(cnt, 0, nl, cube, L, ncnt, 0, nnl, ncube, nL)) {
                    ncnt = cnt; nnl = nl; nL = L; cpcube(ncube, cube);
                }
                have = 1;
            }
        }
        if (bL != nL || !samecube(bcube, ncube)) ttie = ttie + 1;
        qand(newly, bcm, rem);
        if (nr >= MAXR) {   /* before rc[nr] is written: exit 4 */
            printf("construct: %s: capacity: head %s needs more than %d decision-list rules\n", gpath, hname[ch], MAXR);
            exit(4);
        }
        /* REDUCE: the supercube of the keys this rule claims */
        for (i = 0; i < nf; i = i + 1) {
            ws_zero(sup + i * GW, gnw[i], GW);
            for (g = 0; g < ng[i]; g = g + 1) if (qmeets(qmask[i * MAXV + g], newly)) ws_set(sup + i * GW, g);
        }
        cpcube(rc[nr], sup);
        /* T4: align to the first pool cube with sup <= pc <= prime */
        for (t = 0; t < npool; t = t + 1) {
            ok = 1;
            for (i = 0; i < nf; i = i + 1)
                if (!ws_sub(sup + i * GW, pool[t] + i * GW, gnw[i]) || !ws_sub(pool[t] + i * GW, bcube + i * GW, gnw[i])) ok = 0;
            if (ok) {
                if (!samecube(pool[t], sup)) talign = talign + 1;
                cpcube(rc[nr], pool[t]);
                break;
            }
        }
        rl[nr] = bL;
        nr = nr + 1;
        qandnot(rem, rem, newly);
    }
}

/* ------------------------------------------------ T3: winner-vs-loser ranks */
char cons[MAXR][MAXR];
int fire[MAXQ][MAXR];
int nfire[MAXQ];

int addc(int a, int b) {
    if (a < b && !cons[a][b]) { cons[a][b] = 1; return 1; }
    return 0;
}

long rkm[QW];
void ranks(void) {
    long z[MAXC];
    int zs[MAXC];
    int i, j, t, it, mx, b, grew, w, cstar, c, a, bb, ng2;
    int grpi[MAXR];
    for (j = 0; j < nq; j = j + 1) nfire[j] = 0;
    for (i = 0; i < nr; i = i + 1) {
        cubemask(rc[i], rkm);
        for (j = qnext(rkm, 0); j >= 0; j = qnext(rkm, j + 1))
            { fire[j][nfire[j]] = i; nfire[j] = nfire[j] + 1; }
    }
    for (i = 0; i < nr; i = i + 1) for (j = 0; j < nr; j = j + 1) cons[i][j] = 0;
    for (j = 0; j < nq; j = j + 1) {
        if (nfire[j] < 2) continue;
        w = fire[j][0];
        for (t = 1; t < nfire[j]; t = t + 1)
            if (rl[fire[j][t]] != rl[w]) addc(w, fire[j][t]);
    }
    for (it = 0; it < 500; it = it + 1) {
        for (i = nr - 1; i >= 0; i = i - 1) {
            mx = -1;
            for (b = 0; b < nr; b = b + 1) if (cons[i][b] && lv[b] > mx) mx = lv[b];
            lv[i] = mx + 1;
            if (lv[i] > 60) die("a rank does not fit a long");
        }
        grew = 0;
        sumwhat = "ranks z";
        for (j = 0; j < nq; j = j + 1) {
            if (nfire[j] < 2) continue;
            w = fire[j][0];
            cstar = rl[w];
            for (c = 0; c < ncl[ch]; c = c + 1) { z[c] = 0; zs[c] = 0; }
            for (t = 0; t < nfire[j]; t = t + 1) {
                c = rl[fire[j][t]];
                z[c] = ladd(z[c], bit(lv[fire[j][t]]));
                zs[c] = 1;
            }
            for (c = 0; c < ncl[ch]; c = c + 1) {
                if (!zs[c] || c == cstar || z[c] < z[cstar]) continue;
                ng2 = 0;
                for (t = 0; t < nfire[j]; t = t + 1)
                    if (rl[fire[j][t]] == c) { grpi[ng2] = fire[j][t]; ng2 = ng2 + 1; }
                for (a = 0; a < ng2; a = a + 1) {
                    grew = grew | addc(w, grpi[a]);
                    for (bb = a + 1; bb < ng2; bb = bb + 1) grew = grew | addc(grpi[a], grpi[bb]);
                }
            }
        }
        if (!grew) break;
    }
}

/* ------------------------------------------------------- representations -- */
/* a representation: units, each a cube and W2 weights per class (0 = none).
   Candidate ci, unit u lives at row ci * MAXCU + u (MAXCU = MAXR): flat 2-D, because unisacc
   (at 8d4011c) crashes on a 3-D array indexed by variables. */
int ncand;
int cn[MAXCAND];
long ccube[MAXCAND * MAXCU][MAXF * GW];
long cw[MAXCAND * MAXCU][MAXC];

int newunit(int ci, long *cube) {
    int u = cn[ci], c;
    /* every caller adds a unit only while cn[ci] < nr (README, candidate
       rows): rep_from_dl one per distinct rule cube, the k prefix k <= nr,
       the base and patch paths stop at cap = nr.  nr <= MAXR = MAXCU. */
    if (u >= nr || nr > MAXCU) die("internal: candidate unit beyond cap = nr rules");
    cpcube(ccube[ci * MAXCU + u], cube);
    for (c = 0; c < MAXC; c = c + 1) cw[ci * MAXCU + u][c] = 0;
    cn[ci] = u + 1;
    return u;
}

void rep_from_dl(int ci) {
    int r, u, found;
    cn[ci] = 0;
    sumwhat = "rep_from_dl merged cube weight";
    for (r = 0; r < nr; r = r + 1) {
        found = -1;
        for (u = 0; u < cn[ci]; u = u + 1) if (samecube(ccube[ci * MAXCU + u], rc[r])) { found = u; break; }
        if (found < 0) found = newunit(ci, rc[r]);
        cw[ci * MAXCU + found][rl[r]] = ladd(cw[ci * MAXCU + found][rl[r]], bit(lv[r]));
    }
}

int nparts;
int npart[4];
int psz[4][4];
int pf[16][MAXF];

void partitions(void) {
    int i, x, n;
    nparts = 0;
    if (nf < 2) return;
    if (nf == 2) {
        nparts = 1; npart[0] = 2;
        psz[0][0] = 1; pf[0 * 4 + 0][0] = 0; psz[0][1] = 1; pf[0 * 4 + 1][0] = 1;
        return;
    }
    for (i = 0; i < nf; i = i + 1) {
        npart[nparts] = 2;
        psz[nparts][0] = 1; pf[nparts * 4 + 0][0] = i;
        n = 0;
        for (x = 0; x < nf; x = x + 1) if (x != i) { pf[nparts * 4 + 1][n] = x; n = n + 1; }
        psz[nparts][1] = n;
        nparts = nparts + 1;
    }
    npart[nparts] = nf;
    for (i = 0; i < nf; i = i + 1) { psz[nparts][i] = 1; pf[nparts * 4 + i][0] = i; }
    nparts = nparts + 1;
}

long hfm[MAXU][QW];
int headfail(int ci, int *badj) {
    long z[MAXC], mx;
    int u, j, c, nb = 0, cntmx, arg;
    for (u = 0; u < cn[ci]; u = u + 1) cubemask(ccube[ci * MAXCU + u], hfm[u]);
    sumwhat = "headfail z";
    for (j = 0; j < nq; j = j + 1) {
        for (c = 0; c < ncl[ch]; c = c + 1) z[c] = 0;
        for (u = 0; u < cn[ci]; u = u + 1)
            if (qtest(hfm[u], j))
                for (c = 0; c < ncl[ch]; c = c + 1) z[c] = ladd(z[c], cw[ci * MAXCU + u][c]);
        mx = z[0]; arg = 0;
        for (c = 1; c < ncl[ch]; c = c + 1) if (z[c] > mx) { mx = z[c]; arg = c; }
        cntmx = 0;
        for (c = 0; c < ncl[ch]; c = c + 1) if (z[c] == mx) cntmx = cntmx + 1;
        if (cntmx != 1 || arg != qlab[ch][j]) { badj[nb] = j; nb = nb + 1; }
    }
    return nb;
}

/* class set s -> r, the same set over name ranks, so ws_cmp orders by
   sorted names; both over the head's ncl classes (CW storage words) */
void rankset(long *s, long *r, int *rk, int n) {
    int c;
    ws_zero(r, ws_nw(n), CW);
    for (c = 0; c < n; c = c + 1) if (ws_test(s, c)) ws_set(r, rk[c]);
}

/* -G, class half: rankset over 96 classes (CW words) under a reversed name
   order (class c has rank 95 - c: high indices -> low ranks) and under a
   scrambled one (rank (37c + 5) % 96); sets that cross the word boundary
   (classes and ranks 61, 62) must map member by member, and ws_cmp of two
   ranksets must equal the oracle on the rank lists */
int grk[MAXC];
long gs1[CW], gs2[CW];
void rselftest(void) {
    int pm, t, c, j, k, n, nc = 0, cases[8][3];
    gn = MAXC; gnwt = ws_nw(gn);
    cases[0][0] = 0; cases[0][1] = 62; cases[0][2] = -1;
    cases[1][0] = 1; cases[1][1] = -1; cases[1][2] = -1;
    cases[2][0] = 61; cases[2][1] = 62; cases[2][2] = 95;
    cases[3][0] = 3; cases[3][1] = 70; cases[3][2] = -1;
    cases[4][0] = 70; cases[4][1] = -1; cases[4][2] = -1;
    cases[5][0] = 33; cases[5][1] = 34; cases[5][2] = -1;
    cases[6][0] = 0; cases[6][1] = 95; cases[6][2] = -1;
    cases[7][0] = -1; cases[7][1] = -1; cases[7][2] = -1;
    for (pm = 0; pm < 2; pm = pm + 1) {
        for (c = 0; c < MAXC; c = c + 1) grk[c] = pm ? (37 * c + 5) % MAXC : MAXC - 1 - c;
        for (t = 0; t < 8; t = t + 1) {
            gmk(gs1, cases[t][0], cases[t][1], cases[t][2]);
            /* a class set's storage is CW words: junk in those, word CW (the
               test buffer's third) is outside it and stays 0 */
            gjunk(ga); ga[CW] = 0; rankset(gs1, ga, grk, MAXC); gwf(ga, "rankset");
            n = 0;
            for (j = 0; j < 3; j = j + 1) if (cases[t][j] >= 0) { if (!ws_test(ga, grk[cases[t][j]])) gfail("rankset member"); n = n + 1; }
            if (ws_popc(ga, gnwt) != n) gfail("rankset size");
            for (k = 0; k < 8; k = k + 1) {
                gmk(gs2, cases[k][0], cases[k][1], cases[k][2]);
                gjunk(gb); rankset(gs2, gb, grk, MAXC);
                gcmp1(ga, gb, 2, "rankset order");
                nc = nc + 1;
            }
        }
    }
    /* reversed order: {0} has rank 95, {95} rank 0; {61,62} -> {34,33} */
    for (c = 0; c < MAXC; c = c + 1) grk[c] = MAXC - 1 - c;
    gmk(gs1, 0, -1, -1); gmk(gs2, 95, -1, -1); rankset(gs1, ga, grk, MAXC); rankset(gs2, gb, grk, MAXC);
    if (ws_cmp(ga, gb, gnwt) != 1 || ws_cmp(gs1, gs2, gnwt) != -1) gfail("reversed ranks do not reverse the order");
    gmk(gs1, 61, 62, -1); rankset(gs1, ga, grk, MAXC);
    if (!ws_test(ga, 34) || !ws_test(ga, 33) || ws_popc(ga, gnwt) != 2) gfail("rankset across the word boundary");
    printf("wset self-test rankset n %d (%d words): 2 orders, %d cross-word pairs vs oracle, ok\n", MAXC, gnwt, nc);
}

int gcode[MAXQ];
long gcls[MAXQ][CW];
long bcls[MAXQ][CW];
long rsa[CW], rsb[CW];
/* The buckets of one part, flattened (was bmem[MAXQ][MAXQ]): every group
   value t lands in exactly one bucket bof[t], so the members of all buckets
   together are the ngv <= nq group values.  Bucket b (by identity, the
   number it got when first seen) owns bpool[boff[b] .. boff[b] + bn[b] - 1],
   filled in first-seen order; border only permutes bucket numbers, so the
   offsets never move with the sort. */
int bn[MAXQ];
int boff[MAXQ];
int bfill[MAXQ];
int bof[MAXQ];
int bpool[MAXQ];
int border[MAXQ];

int codedigit(int code, int pi, int pp, int fi) {
    int t = psz[pi][pp] - 1;
    while (t > fi) { code = code / ng[pf[pi * 4 + pp][t]]; t = t - 1; }
    return code % ng[pf[pi * 4 + pp][fi]];
}

/* T5; returns 1 and fills candidate ci, or 0 for None */
int tfearlyb, tfearlyp;      /* trace: early rejections (cap reached) on the base / patch path */
long fcov[QW], fres[QW], ftmp[QW];
int rep_factored(int ci, int pi, int merge, int k) {
    long sets[MAXF * GW], pr[GW];
    int cnw, i, u, pp, j, code, fi, f, x, ngv, nbk, b, t, a, tmp, cap, it, nb, prod, cnt, s, e;
    int badj[MAXQ];
    /* Early rejection needs cap = nr > 0 and cap <= MAXCU: then no candidate
       here passes nr units, so newunit's row guard cannot fire first. */
    if (nr <= 0 || nr > MAXCU) {
        printf("construct: %s: capacity: rep_factored cap %d rules, outside 1..%d units\n", gpath, nr, MAXCU);
        exit(4);
    }
    cn[ci] = 0;
    cnw = ws_nw(ncl[ch]);
    qzero(fcov);
    if (k) {
        if (k > nr) return 0;
        for (i = 0; i < k; i = i + 1) {
            u = newunit(ci, rc[i]);
            cw[ci * MAXCU + u][rl[i]] = bit(k + 1 - i);
            cubemask(rc[i], ftmp);
            qor(fcov, fcov, ftmp);
        }
    }
    qandnot(fres, ALL, fcov);
    if (qempty(fres)) return 0;
    for (pp = 0; pp < npart[pi]; pp = pp + 1) {
        ngv = 0;
        for (j = qnext(fres, 0); j >= 0; j = qnext(fres, j + 1)) {
            code = 0;
            for (fi = 0; fi < psz[pi][pp]; fi = fi + 1) {
                f = pf[pi * 4 + pp][fi];
                code = code * ng[f] + qk[j][f];
            }
            x = -1;
            for (t = 0; t < ngv; t = t + 1) if (gcode[t] == code) { x = t; break; }
            if (x < 0) { x = ngv; gcode[x] = code; ws_zero(gcls[x], cnw, CW); ngv = ngv + 1; }
            ws_set(gcls[x], qlab[ch][j]);
        }
        if (ngv == 0) return 0;
        nbk = 0;
        for (t = 0; t < ngv; t = t + 1) {
            x = -1;
            for (b = 0; b < nbk; b = b + 1) if (ws_eq(bcls[b], gcls[t], cnw)) { x = b; break; }
            if (x < 0) { x = nbk; ws_copy(bcls[x], gcls[t], cnw, CW); bn[x] = 0; nbk = nbk + 1; }
            bof[t] = x;
            bn[x] = bn[x] + 1;
        }
        /* count (above), prefix sums, then fill in the original order */
        s = 0;
        for (b = 0; b < nbk; b = b + 1) { boff[b] = s; bfill[b] = 0; s = s + bn[b]; }
        if (s != ngv || ngv > nq) die("internal: bucket members are not the group values");
        for (t = 0; t < ngv; t = t + 1) {
            x = bof[t];
            bpool[boff[x] + bfill[x]] = gcode[t];
            bfill[x] = bfill[x] + 1;
        }
        for (b = 0; b < nbk; b = b + 1) if (bfill[b] != bn[b]) die("internal: a bucket was not filled to its count");
        /* buckets sorted by the sorted names of their classes (stable) */
        for (b = 0; b < nbk; b = b + 1) border[b] = b;
        for (a = 1; a < nbk; a = a + 1) {
            t = a;
            while (t > 0) {
                rankset(bcls[border[t - 1]], rsa, crank[ch], ncl[ch]);
                rankset(bcls[border[t]], rsb, crank[ch], ncl[ch]);
                if (ws_cmp(rsa, rsb, cnw) <= 0) break;
                tmp = border[t]; border[t] = border[t - 1]; border[t - 1] = tmp;
                t = t - 1;
            }
        }
        for (a = 0; a < nbk; a = a + 1) {
            b = border[a];
            /* gvs = sorted(bucket): codes are mixed-radix, first field most significant */
            for (s = 1; s < bn[b]; s = s + 1) {
                t = s;
                while (t > 0 && bpool[boff[b] + t - 1] > bpool[boff[b] + t]) {
                    tmp = bpool[boff[b] + t]; bpool[boff[b] + t] = bpool[boff[b] + t - 1]; bpool[boff[b] + t - 1] = tmp;
                    t = t - 1;
                }
            }
            prod = 0;
            if (merge && bn[b] > 1) {
                prod = 1;
                for (fi = 0; fi < psz[pi][pp]; fi = fi + 1) {
                    f = pf[pi * 4 + pp][fi];
                    ws_zero(pr, gnw[f], GW);
                    for (s = 0; s < bn[b]; s = s + 1) ws_set(pr, codedigit(bpool[boff[b] + s], pi, pp, fi));
                    prod = prod * ws_popc(pr, gnw[f]);
                }
                prod = (prod == bn[b]);
            }
            s = 0;
            while (s < bn[b]) {
                e = prod ? bn[b] : s + 1;
                for (i = 0; i < nf; i = i + 1) ws_copy(sets + i * GW, FULLW + i * GW, gnw[i], GW);
                for (fi = 0; fi < psz[pi][pp]; fi = fi + 1) {
                    f = pf[pi * 4 + pp][fi];
                    ws_zero(sets + f * GW, gnw[f], GW);
                    for (t = s; t < e; t = t + 1) ws_set(sets + f * GW, codedigit(bpool[boff[b] + t], pi, pp, fi));
                }
                /* Python appends units only and returns None once len(units)
                   >= cap: reaching cap here already decides None */
                if (cn[ci] >= nr) { tfearlyb = tfearlyb + 1; return 0; }
                u = newunit(ci, sets);
                for (i = 0; i < ncl[ch]; i = i + 1) if (ws_test(bcls[b], i)) cw[ci * MAXCU + u][i] = 1;
                s = e;
            }
        }
    }
    cap = nr;
    for (it = 0; it < 6; it = it + 1) {
        if (cn[ci] >= cap) return 0;
        nb = headfail(ci, badj);
        if (nb == 0) return 1;
        for (t = 0; t < nb; t = t + 1) {
            j = badj[t];
            for (i = 0; i < nf; i = i + 1) { ws_zero(sets + i * GW, gnw[i], GW); ws_set(sets + i * GW, qk[j][i]); }
            cnt = 0;
            for (u = 0; u < cn[ci]; u = u + 1) if (samecube(ccube[ci * MAXCU + u], sets)) cnt = 1;
            if (cnt) return 0;
            if (cn[ci] >= nr) { tfearlyp = tfearlyp + 1; return 0; }
            u = newunit(ci, sets);
            cw[ci * MAXCU + u][qlab[ch][j]] = bit(k + 2);
        }
    }
    return 0;
}

/* ------------------------------------------ per head: candidates, pick -- */
/* cands[hn] of build_net: head h's candidates are slots hc[h][0..nhc[h]-1];
   slot hc[h][0] is the decision-list representation.  dl_rules[hn] is kept
   per head (hnr, hrc, hrl, hlv) because T4 rebuilds it and the factored
   candidates read it. */
int hc[MAXH][MAXCAND];
int nhc[MAXH];
int hnr[MAXH];
long hrc[MAXH * MAXR][MAXF * GW];
int hrl[MAXH][MAXR];
int hlv[MAXH][MAXR];
int chosen[MAXH];
int tpickmoved;              /* trace: starts whose pick moved a head */
int tsel;                    /* trace: selections made */
int tfatt, tfnone, tfkept;   /* trace: T5 rep_factored calls, None results, kept */

void saverules(int h) {
    int r;
    hnr[h] = nr;
    for (r = 0; r < nr; r = r + 1) { cpcube(hrc[h * MAXR + r], rc[r]); hrl[h][r] = rl[r]; hlv[h][r] = lv[r]; }
}

void loadrules(int h) {
    int r;
    nr = hnr[h];
    for (r = 0; r < nr; r = r + 1) { cpcube(rc[r], hrc[h * MAXR + r]); rl[r] = hrl[h][r]; lv[r] = hlv[h][r]; }
}

long ucube[MAXU][MAXF * GW];      /* scratch for total_units, then the net's units */

/* total_units(sel): distinct cubes over every head's selected candidate */
int total_units(int *sel) {
    int h, u, v, ci, n = 0, dup;
    for (h = 0; h < nh; h = h + 1) {
        ci = hc[h][sel[h]];
        for (u = 0; u < cn[ci]; u = u + 1) {
            dup = 0;
            for (v = 0; v < n; v = v + 1) if (samecube(ucube[v], ccube[ci * MAXCU + u])) { dup = 1; break; }
            if (!dup) {
                if (n >= MAXU) {   /* pick's total_units: exit 4 */
                    printf("construct: %s: capacity: a selection has more than %d distinct units\n", gpath, MAXU);
                    exit(4);
                }
                cpcube(ucube[n], ccube[ci * MAXCU + u]);
                n = n + 1;
            }
        }
    }
    return n;
}

int candlen(int h, int i) { return cn[hc[h][i]]; }

/* build_net's pick: up to five passes over the heads, each head moving to the
   candidate with fewer total units (ties: the shorter candidate).  Returns
   whether it moved anything (trace only). */
int pick(int *sel) {
    int pass, h, bi, bv, i, v, moved, any = 0;
    int trial[MAXH];
    for (pass = 0; pass < 5; pass = pass + 1) {
        moved = 0;
        for (h = 0; h < nh; h = h + 1) {
            bi = sel[h]; bv = total_units(sel);
            for (i = 0; i < nhc[h]; i = i + 1) {
                if (i == sel[h]) continue;
                for (v = 0; v < nh; v = v + 1) trial[v] = sel[v];
                trial[h] = i;
                v = total_units(trial);
                if (v < bv || (v == bv && candlen(h, i) < candlen(h, bi))) { bi = i; bv = v; }
            }
            if (bi != sel[h]) { sel[h] = bi; moved = 1; any = 1; }
        }
        if (!moved) break;
    }
    return any;
}

int sumlen(int *sel) {
    int h, n = 0;
    for (h = 0; h < nh; h = h + 1) n = n + candlen(h, sel[h]);
    return n;
}

/* chosen = min(pick(start) for start in starts) by (total units, summed
   length), first minimum wins; starts: all dlist, then per head its
   shortest candidate (first shortest) with the others at 0 */
void selectall(void) {
    int s, h, i, sel[MAXH], best[MAXH], have = 0, bt = 0, bl = 0, t, l, m;
    tsel = tsel + 1;
    for (s = 0; s <= nh; s = s + 1) {
        for (h = 0; h < nh; h = h + 1) sel[h] = 0;
        if (s > 0) {
            h = s - 1; m = 0;
            for (i = 1; i < nhc[h]; i = i + 1) if (candlen(h, i) < candlen(h, m)) m = i;
            sel[h] = m;
        }
        if (pick(sel)) tpickmoved = tpickmoved + 1;
        t = total_units(sel); l = sumlen(sel);
        if (dbg && nh > 1) {
            printf("pick %d:", s);
            for (h = 0; h < nh; h = h + 1) printf(" %d", sel[h]);
            printf(" units %d len %d\n", t, l);
        }
        if (!have || t < bt || (t == bt && l < bl)) {
            have = 1; bt = t; bl = l;
            for (h = 0; h < nh; h = h + 1) best[h] = sel[h];
        }
    }
    for (h = 0; h < nh; h = h + 1) chosen[h] = best[h];
    if (dbg && nh > 1) {
        printf("chosen:");
        for (h = 0; h < nh; h = h + 1) printf(" %d", chosen[h]);
        printf(" units %d\n", bt);
    }
}

void prrules(int npl) {
    int j;
    if (nh > 1) printf("dl %s pool %d rules %d\n", hname[ch], npl, nr);
    for (j = 0; j < nr; j = j + 1) {
        printf("rule %d:", j);
        prcube(rc[j]);
        printf(" => %s rank %d", cls[ch][rl[j]], lv[j]);
        if (nh > 1 && npl && inpool(rc[j])) printf(" pool");
        printf("\n");
    }
}

void prcands(void) {
    int h, i;
    printf("select\n");
    for (h = 0; h < nh; h = h + 1) {
        printf("cands %s:", hname[h]);
        for (i = 0; i < nhc[h]; i = i + 1) printf(" %d", candlen(h, i));
        printf("\n");
    }
}

/* T5 candidates of head ch from the rules in rc/rl (dl_rules[hn]); kept only
   when shorter than the head's dlist candidate */
void factored(void) {
    int pi, mg, k, lim;
    lim = nr < 9 ? nr : 9;
    for (pi = 0; pi < nparts; pi = pi + 1)
        for (mg = 0; mg < 2; mg = mg + 1)
            for (k = 0; k < lim; k = k + 1) {
                if (ncand >= MAXCAND || nhc[ch] >= MAXCAND) {   /* before slot ncand is written: exit 4 */
                    printf("construct: %s: capacity: head %s: candidate slot %d (head's %d), more than %d\n", gpath, hname[ch], ncand + 1, nhc[ch] + 1, MAXCAND);
                    exit(4);
                }
                tfatt = tfatt + 1;
                if (!rep_factored(ncand, pi, mg, k)) { tfnone = tfnone + 1; continue; }
                if (cn[ncand] < cn[hc[ch][0]]) {
                    tfkept = tfkept + 1;
                    hc[ch][nhc[ch]] = ncand; nhc[ch] = nhc[ch] + 1;
                    ncand = ncand + 1;
                }
            }
}

/* ------------------------------------------------------------- the net ---- */
int H;
int offs[MAXF];
int h0;
int b1[MAXU];
long W2[MAXH * MAXU][MAXC];  /* head h, unit j at row h * MAXU + j */
int tunits;                  /* trace: summed length of the chosen candidates */
int trounds;                 /* trace: T4 rounds run */
int tacc;                    /* trace: T4 decision lists accepted */
int trej;                    /* trace: T4 decision lists rejected (longer) */
int tchg;                    /* trace: accepted lists that differ from the old */

int w1(int i, int v, int j) {
    if (isfull(ucube[j], i)) return 0;
    return ws_test(ucube[j] + i * GW, grp[i][v]);
}

long mxlog;

int samerules(int h) {
    int r;
    if (nr != hnr[h]) return 0;
    for (r = 0; r < nr; r = r + 1)
        if (!samecube(rc[r], hrc[h * MAXR + r]) || rl[r] != hrl[h][r]) return 0;
    return 1;
}

long pall[MAXU][MAXF * GW];
long ptmp[MAXF * GW];
/* share's pool limit.  It is MAXU; only the TEST ENTRY -T pool (check.sh)
   lowers it to 1.  The pool is the chosen selection's distinct units, which
   total_units already bounded by MAXU, so no table reaches this check with
   poolcap = MAXU (README): the test entry is how the check is exercised. */
int poolcap = MAXU;

/* T4 (build_net, `share and len(heads) > 1`): up to three rounds; each
   rebuilds every head's decision list with the pool of cubes the chosen
   candidates pay for, minus the head's own dlist cubes; a list no longer
   than the old one replaces it (its factored candidates are appended);
   then re-select.  A round with no replacement ends it. */
void share(void) {
    int nall, round, h, u, v, ci, dup, improved, t, a;
    for (round = 0; round < 3; round = round + 1) {
        nall = 0;
        for (h = 0; h < nh; h = h + 1) {
            ci = hc[h][chosen[h]];
            for (u = 0; u < cn[ci]; u = u + 1) {
                dup = 0;
                for (v = 0; v < nall; v = v + 1) if (samecube(pall[v], ccube[ci * MAXCU + u])) { dup = 1; break; }
                if (!dup) {
                    if (nall >= poolcap) {   /* share's pool: exit 4 */
                        printf("construct: %s: capacity: share pool has more than %d units%s\n", gpath, poolcap, poolcap < MAXU ? " (test entry -T pool)" : "");
                        exit(4);
                    }
                    cpcube(pall[nall], ccube[ci * MAXCU + u]);
                    nall = nall + 1;
                }
            }
        }
        trounds = trounds + 1;
        improved = 0;
        for (h = 0; h < nh; h = h + 1) {
            ch = h;
            /* pool = the round's cubes that are not in this head's dlist
               candidate, sorted by cube_key */
            ci = hc[h][0];
            npool = 0;
            for (v = 0; v < nall; v = v + 1) {
                dup = 0;
                for (u = 0; u < cn[ci]; u = u + 1) if (samecube(pall[v], ccube[ci * MAXCU + u])) { dup = 1; break; }
                if (!dup) { cpcube(pool[npool], pall[v]); npool = npool + 1; }
            }
            for (a = 1; a < npool; a = a + 1) {
                t = a;
                while (t > 0 && cmpcube(pool[t - 1], pool[t]) > 0) {
                    cpcube(ptmp, pool[t]); cpcube(pool[t], pool[t - 1]); cpcube(pool[t - 1], ptmp);
                    t = t - 1;
                }
            }
            decision_list();
            ranks();
            if (dbg) prrules(npool);
            npool = 0;
            if (nr <= hnr[h]) {
                if (!samerules(h)) tchg = tchg + 1;
                tacc = tacc + 1;
                saverules(h);
                rep_from_dl(hc[h][0]);
                improved = 1;
                factored();
            } else trej = trej + 1;
        }
        if (!improved) break;
        if (dbg) prcands();
        selectall();
    }
}

/* Build the net for one gold table: the reader, T1-T5 per head, pick, T4,
   the net, then the verifier and the deployment invariants over the full
   original domain. */
void build(char *path) {
    int k, j, h, i, u, c, a, b, found, v, arg, cntmx, bad, t, ci;
    long z[MAXC], mx, mn, hv;
    load(path);
    domain();
    orders();
    partitions();
    ncand = 0; npool = 0; tunits = 0;
    talign = 0; ttie = 0; tpickmoved = 0; tfatt = 0; tfnone = 0; tfearlyb = 0; tfearlyp = 0; tfkept = 0; tsel = 0; trounds = 0; tacc = 0; trej = 0; tchg = 0;
    if (dbg) {
        for (i = 0; i < nf; i = i + 1) {
            printf("groups %d:", i);
            for (a = 0; a < ng[i]; a = a + 1) {
                printf(" [");
                b = 1;
                for (v = 0; v < nv[i]; v = v + 1)
                    if (grp[i][v] == a) { if (!b) printf(","); printf("%d", v); b = 0; }
                printf("]");
            }
            printf("\n");
        }
    }
    for (h = 0; h < nh; h = h + 1) {
        ch = h;
        decision_list();
        ranks();
        if (dbg) prrules(0);
        saverules(h);
        hc[h][0] = ncand; nhc[h] = 1; ncand = ncand + 1;
        rep_from_dl(hc[h][0]);
        factored();
    }
    if (dbg && nh > 1) prcands();
    selectall();
    if (nh > 1) share();
    /* the net: units in head order, each cube once; W2 per head */
    H = 0;
    for (h = 0; h < nh; h = h + 1) {
        ci = hc[h][chosen[h]];
        tunits = tunits + cn[ci];
        for (u = 0; u < cn[ci]; u = u + 1) {
            found = -1;
            for (j = 0; j < H; j = j + 1) if (samecube(ucube[j], ccube[ci * MAXCU + u])) { found = j; break; }
            if (found < 0) {
                if (H >= MAXU) die("more units than this constructor holds");
                found = H;
                cpcube(ucube[H], ccube[ci * MAXCU + u]);
                for (a = 0; a < nh; a = a + 1) for (c = 0; c < MAXC; c = c + 1) W2[a * MAXU + H][c] = 0;
                b1[H] = -(nlits(ucube[H]) - 1);
                H = H + 1;
            }
            for (c = 0; c < ncl[h]; c = c + 1) if (cw[ci * MAXCU + u][c]) W2[h * MAXU + found][c] = cw[ci * MAXCU + u][c];
        }
    }
    h0 = 0;
    for (i = 0; i < nf; i = i + 1) { offs[i] = h0; h0 = h0 + nv[i]; }
    if (dbg) {
        for (h = 0; h < nh; h = h + 1) printf("kind %s %s\n", hname[h], chosen[h] ? "factored" : "dlist");
        for (j = 0; j < H; j = j + 1) { printf("unit %d:", j); prcube(ucube[j]); printf("\n"); }
    }
    /* test entry (check.sh only): -T bias breaks b1 of unit 0; -T act breaks
       it the same way but skips invariant 2, so that invariant 1 is the one
       that must fire.  (With W1 one-hot per field, activation = b1 + fields
       hit <= 1 - t + t = 1: invariant 1 follows from invariant 2, and only
       a broken b1 can reach it.) */
    if (tbreak && H > 0) b1[0] = b1[0] + 1;
    /* deployment invariant 2, checked BEFORE the semantic enumeration so a
       bias fault is reported as one (exit 3), not as "not exact": uns2.load
       does not store b1, it derives b1 = 1 - (fields the unit's W1 touches);
       the net must agree.  W1 and b1 are ONE layer that every head shares
       (T4), so the invariant is per unit and covers all heads at once. */
    for (j = 0; j < H && tbreak != 2; j = j + 1) {
        t = 0;
        for (i = 0; i < nf; i = i + 1) {
            found = 0;
            for (v = 0; v < nv[i]; v = v + 1) if (w1(i, v, j)) found = 1;
            t = t + found;
        }
        if (b1[j] != 1 - t) {
            printf("construct: %s: deployment invariant broken: unit %d has b1 %d, 1 - constrained fields is %d\n", path, j, b1[j], 1 - t);
            exit(3);
        }
    }
    /* verify over the FULL original domain, every head; maxlogit as verify_int */
    mxlog = 0;
    sumwhat = "verifier z";
    bad = 0;
    for (k = 0; k < nok; k = k + 1) {
        for (h = 0; h < nh; h = h + 1) {
            for (c = 0; c < ncl[h]; c = c + 1) z[c] = 0;
            for (j = 0; j < H; j = j + 1) {
                hv = b1[j];
                for (i = 0; i < nf; i = i + 1) hv = hv + w1(i, odigit(k, i), j);
                /* deployment invariant 1: the deployed IntNet adds W2 once when
                   hv > 0, the verifier adds hv * W2; equal only for hv in {0,1} */
                if (hv > 1) {
                    printf("construct: %s: deployment invariant broken: unit %d activation %ld on key %d (must be 0 or 1)\n", path, j, hv, k);
                    exit(3);
                }
                /* hv * W2 is formed only here, AFTER the check above: hv is
                   0 or 1 at this point (hv > 1 exited; hv <= 0 adds nothing),
                   so the product is W2 itself and cannot overflow; the sum
                   goes through ladd */
                if (hv > 0) for (c = 0; c < ncl[h]; c = c + 1) z[c] = ladd(z[c], hv * W2[h * MAXU + j][c]);
            }
            mx = z[0]; mn = z[0]; arg = 0;
            for (c = 1; c < ncl[h]; c = c + 1) {
                if (z[c] > mx) { mx = z[c]; arg = c; }
                if (z[c] < mn) mn = z[c];
            }
            if ((mx < 0 ? -mx : mx) > mxlog) mxlog = mx < 0 ? -mx : mx;
            if (mn < -mxlog) mxlog = -mn;
            cntmx = 0;
            for (c = 0; c < ncl[h]; c = c + 1) if (z[c] == mx) cntmx = cntmx + 1;
            if (cntmx != 1 || arg != olab[k][h]) bad = bad + 1;
        }
    }
    if (bad) { printf("construct: %s: construction not exact (%d wrong)\n", path, bad); exit(1); }
}

/* -t: which branches of the multi-head code this stage took (not compared
   with Python; a debug print) */
void trace(void) {
    int h, ci, r, u, v, x, hit;
    for (h = 0; h < nh; h = h + 1) {
        ci = hc[h][chosen[h]];
        printf("trace head %s: %d candidates, chose %d (%s), %d units\n", hname[h], nhc[h],
               chosen[h], chosen[h] ? "factored" : "dlist", cn[ci]);
        printf("trace head %s: candidate units", hname[h]);
        for (r = 0; r < nhc[h]; r = r + 1) printf(" %d", cn[hc[h][r]]);
        printf("\n");
        loadrules(h);
        x = 0;
        for (r = 0; r < nr; r = r + 1) {
            hit = 0;
            for (v = 0; v < nh; v = v + 1) {
                if (v == h) continue;
                for (u = 0; u < cn[hc[v][chosen[v]]]; u = u + 1)
                    if (samecube(rc[r], ccube[hc[v][chosen[v]] * MAXCU + u])) hit = 1;
            }
            x = x + hit;
        }
        printf("trace head %s: %d of %d final rules are cubes another head also uses\n", hname[h], x, nr);
    }
    printf("trace T5 partitions %d, rep_factored calls %d, None %d, not shorter than dlist %d, kept %d\n",
           nparts, tfatt, tfnone, tfatt - tfnone - tfkept, tfkept);
    printf("trace T5 early rejections (unit count reached cap = rules): base path %d, patch path %d\n", tfearlyb, tfearlyp);
    printf("trace selections %d, starts whose pick moved a head %d\n", tsel, tpickmoved);
    printf("trace T4 rounds %d, lists accepted %d (changed %d), rejected %d\n", trounds, tacc, tchg, trej);
    printf("trace T4 REDUCE aligned to a pool cube %d, pool flag changed a rule's choice %d\n", talign, ttie);
    printf("trace units: chosen candidates sum %d, net H %d, merged across heads %d\n", tunits, H, tunits - H);
}

void canon(void) {
    int i, j, c, v, h;
    /* the canonical form: intnet.to_dict's fields, in its order */
    printf("stage %s\n", sname);
    printf("H %d\n", H);
    printf("offs");
    for (i = 0; i < nf; i = i + 1) printf(" %d", offs[i]);
    printf("\nb1");
    for (j = 0; j < H; j = j + 1) printf(" %d", b1[j]);
    printf("\n");
    for (h = 0; h < nh; h = h + 1) {
        printf("head %s", hname[h]);
        for (c = 0; c < ncl[h]; c = c + 1) printf(" %s", cls[h][c]);
        printf("\n");
    }
    printf("feeds %d\n", h0);
    for (i = 0; i < nf; i = i + 1)
        for (v = 0; v < nv[i]; v = v + 1) {
            printf("f %d:", offs[i] + v);
            for (j = 0; j < H; j = j + 1) if (w1(i, v, j)) printf(" %d", j);
            printf("\n");
        }
    for (h = 0; h < nh; h = h + 1) {
        printf("w2 %s\n", hname[h]);
        for (j = 0; j < H; j = j + 1) {
            printf("r %d:", j);
            for (c = 0; c < ncl[h]; c = c + 1) if (W2[h * MAXU + j][c]) printf(" %d,%ld", c, W2[h * MAXU + j][c]);
            printf("\n");
        }
    }
    printf("maxlogit %ld\n", mxlog);
    printf("exact %d\n", nok);
}

/* ------------------------------------------------------------- UNS2 ------ */
/* unisa/uns2.py dump, for the stages given: header 16 B, then per stage in
   sorted name order: name[16] | h0 H u16 | nf nh u8 | pad u16 | flens u16[]
   | w1len u32 | W1 bits unit-major | per head: ncl u16 wb u8 pad u8 |
   len u32 | per unit: count 8 bits, then (class cb bits, weight wb bits).
   (The docstring of uns2.py says "presence H bits"; the code writes an
   8-bit count per unit, and the code is what is matched here.) */
#define MAXS 8
#define SECSZ 65536
char sec[MAXS][SECSZ];
int seclen[MAXS];
char secname[MAXS][17];
int secH[MAXS];
char tmpb[SECSZ];
int tlen;
int bacc;
int bcnt;
char *cur;
int clen;
int ccap;

void outb(int x) {
    if (clen >= ccap) die("UNS2 section too large");
    cur[clen] = (char)(x & 255);
    clen = clen + 1;
}
void out16(int x) { outb(x); outb(x >> 8); }
void out32(int x) { outb(x); outb(x >> 8); outb(x >> 16); outb(x >> 24); }

void tput(long val, int bits) {
    int i;
    for (i = bits - 1; i >= 0; i = i - 1) {
        bacc = (bacc << 1) | (int)((val >> i) & 1);
        bcnt = bcnt + 1;
        if (bcnt == 8) {
            if (tlen >= SECSZ) die("UNS2 section too large");
            tmpb[tlen] = (char)bacc; tlen = tlen + 1; bacc = 0; bcnt = 0;
        }
    }
}
void tstart(void) { tlen = 0; bacc = 0; bcnt = 0; }
void tflush(void) {
    int i;
    if (bcnt) { tmpb[tlen] = (char)(bacc << (8 - bcnt)); tlen = tlen + 1; bacc = 0; bcnt = 0; }
    out32(tlen);
    for (i = 0; i < tlen; i = i + 1) outb(tmpb[i]);
}

int bitlen(long x) { int n = 0; while (x) { x = x >> 1; n = n + 1; } return n; }

void section(int s) {
    int i, j, v, c, n, cb, wb, sl, h;
    long mw;
    cur = sec[s]; clen = 0; ccap = SECSZ;
    sl = (int)strlen(sname);
    for (i = 0; i < 16; i = i + 1) {
        if (i < sl) secname[s][i] = sname[i]; else secname[s][i] = 0;
        outb(secname[s][i]);
    }
    secname[s][16] = 0;
    secH[s] = H;
    out16(h0); out16(H); outb(nf); outb(nh); out16(0);
    for (i = 0; i < nf; i = i + 1) out16(nv[i]);
    tstart();
    for (j = 0; j < H; j = j + 1)
        for (i = 0; i < nf; i = i + 1)
            for (v = 0; v < nv[i]; v = v + 1) tput(w1(i, v, j), 1);
    tflush();
    for (h = 0; h < nh; h = h + 1) {
        mw = 0;
        for (j = 0; j < H; j = j + 1) for (c = 0; c < ncl[h]; c = c + 1) if (W2[h * MAXU + j][c] > mw) mw = W2[h * MAXU + j][c];
        if (mw == 0) mw = 1;
        wb = bitlen(mw); if (wb < 1) wb = 1;
        cb = bitlen(ncl[h] - 1); if (cb < 1) cb = 1;
        out16(ncl[h]); outb(wb); outb(0);
        tstart();
        for (j = 0; j < H; j = j + 1) {
            n = 0;
            for (c = 0; c < ncl[h]; c = c + 1) if (W2[h * MAXU + j][c]) n = n + 1;
            if (n >= 256) die("a unit has 256 or more W2 entries");
            tput(n, 8);
            for (c = 0; c < ncl[h]; c = c + 1) if (W2[h * MAXU + j][c]) { tput(c, cb); tput(W2[h * MAXU + j][c], wb); }
        }
        tflush();
    }
    seclen[s] = clen;
}

char hdr[16];

void writeuns2(char *opath, int ns) {
    FILE *f;
    int ord[MAXS], i, t, tmp, units = 0;
    for (i = 0; i < ns; i = i + 1) { ord[i] = i; units = units + secH[i]; }
    for (i = 1; i < ns; i = i + 1) {
        t = i;
        while (t > 0 && strcmp(secname[ord[t - 1]], secname[ord[t]]) > 0) {
            tmp = ord[t]; ord[t] = ord[t - 1]; ord[t - 1] = tmp; t = t - 1;
        }
    }
    for (i = 1; i < ns; i = i + 1)
        if (strcmp(secname[ord[i - 1]], secname[ord[i]]) == 0) { printf("construct: a stage is given twice\n"); exit(2); }
    cur = hdr; clen = 0; ccap = 16;
    outb('U'); outb('N'); outb('S'); outb('2');
    outb(1); outb(0); out16(ns); out32(units); out32(0);
    f = fopen(opath, "wb");
    if (!f) { printf("construct: cannot write %s\n", opath); exit(2); }
    fwrite(hdr, 1, 16, f);
    for (i = 0; i < ns; i = i + 1) fwrite(sec[ord[i]], 1, seclen[ord[i]], f);
    fclose(f);
}

/* -T summax | sumover | sumrun: the SAME ladd the accumulation sites call,
   on the three boundary cases.  summax: (LONG_MAX - 5) + 5 = LONG_MAX must
   succeed (exit 0).  sumover: (LONG_MAX - 5) + 6, one past it, must be
   rejected by ladd (exit 6) before the addition.  sumrun: a running sum of
   bit(60), the largest rank weight ranks() admits (8 rank-60 rules on one
   key): 7 terms fit (7 * 2^60 < LONG_MAX), the 8th must be rejected. */
void sumtest(int which) {
    long s;
    int i;
    gpath = "-T sum";
    sumwhat = "sum self-test";
    if (which == 0) {
        s = ladd(LONG_MAX - 5, 5);
        printf("sum self-test: max: (LONG_MAX - 5) + 5 = %ld = LONG_MAX %s\n", s, s == LONG_MAX ? "ok" : "WRONG");
        exit(s == LONG_MAX ? 0 : 1);
    }
    if (which == 1) {
        printf("sum self-test: one past: (LONG_MAX - 5) + 6\n");
        s = ladd(LONG_MAX - 5, 6);
        printf("sum self-test: one past was NOT rejected: %ld\n", s);
        exit(1);
    }
    s = 0;
    for (i = 1; i <= 8; i = i + 1) {
        s = ladd(s, bit(60));
        printf("sum self-test: run: %d x 2^60 = %ld\n", i, s);
    }
    printf("sum self-test: run: 8 x 2^60 was NOT rejected\n");
    exit(1);
}

int main(int argc, char **argv) {
    int ai, ns = 0;
    char *path = 0;
    char *upath = 0;
    dbg = 0;
    if (MAXR > MAXU) { printf("construct: MAXR %d > MAXU %d: rep_factored's cap = nr would pass MAXU\n", MAXR, MAXU); return 4; }
    if (sizeof(long) != 8) { printf("construct: long is not 64-bit; ladd's precondition fails\n"); return 6; }
    for (ai = 1; ai < argc; ai = ai + 1) {
        if (streq(argv[ai], "-d")) dbg = 1;
        else if (streq(argv[ai], "-t")) tflag = 1;
        else if (streq(argv[ai], "-Q")) { qselftest(); return 0; }
        else if (streq(argv[ai], "-G")) { wselftest(); rselftest(); return 0; }
        else if (streq(argv[ai], "-T") && ai + 1 < argc) {
            ai = ai + 1;
            if (streq(argv[ai], "bias")) tbreak = 1;
            else if (streq(argv[ai], "act")) tbreak = 2;
            else if (streq(argv[ai], "pool")) poolcap = 1;   /* test entry: share-pool check */
            else if (streq(argv[ai], "summax")) sumtest(0);
            else if (streq(argv[ai], "sumover")) sumtest(1);
            else if (streq(argv[ai], "sumrun")) sumtest(2);
            else { printf("construct: -T bias | act | pool | summax | sumover | sumrun\n"); return 2; }
        }
        else if (streq(argv[ai], "-u") && ai + 1 < argc && !upath) { ai = ai + 1; upath = argv[ai]; }
        else if (upath) {
            if (ns >= MAXS) { printf("construct: more than %d stages\n", MAXS); return 2; }
            build(argv[ai]);
            section(ns);
            ns = ns + 1;
        }
        else path = argv[ai];
    }
    if (upath) {
        if (ns == 0 || path || dbg) { printf("usage: construct -u out.uns2 <stage>.tsv...\n"); return 2; }
        writeuns2(upath, ns);
        return 0;
    }
    if (!path) { printf("usage: construct [-d] <stage>.tsv | construct -u out.uns2 <stage>.tsv...\n"); return 2; }
    build(path);
    if (tflag) trace(); else canon();
    return 0;
}
