/* genmodel.c -- the model region of kernel/unisa_model.inc, from declared
 * inputs only, in the C subset unisacc compiles. [J10 step 3, first slice]
 *
 *   genmodel -o OUT ORDER.tsv VOCAB.tsv TYPEKW.tsv BUILT.uns2 TSV...   (one TSV per stage)
 *
 * Reads ONLY the files named on its command line: the stage order
 * (iterate/kernel/order.tsv), the constructed weights (weights/built.uns2)
 * and the gold tables (weights/gold/<stage>.tsv).  It writes, to OUT:
 *   - #define S_<STAGE> i, in order.tsv order;
 *   - MODEL: each stage's units, decoded from UNS2 and re-laid in order.tsv
 *     order exactly as unisa/ckernel.py _blob does: per unit and field,
 *     MW little-endian u64 literal masks (a full mask is written 0, "don't
 *     care"), then u32 nEntries and 7-byte (unit u16, head u8, class u16,
 *     w u16) entries, heads outer, units, then the unit's row;
 *   - NSTAGE, the STAGE_* declarations, act[] and z[];
 *   - DENSE: for every stage, every key in field-major order (the last
 *     field varies fastest), every head: the index of the key's TSV label in
 *     that head's #head class list.  From the TSV labels, never from infer();
 *   - model_dims().
 * Stages are matched by NAME (UNS2 section name, the TSV's "# stage" line,
 * order.tsv), never by position; the three name sets must be equal and each
 * dimension (fields, values per field, heads, classes per head) must agree
 * between UNS2 and the TSV, or it exits.
 * Exit codes: 1 bad input (reader, UNS2 decode, mismatch), 2 usage, 4
 * capacity, 7 the output could not be opened, written or closed.
 *   - [second slice] the vocabularies and BF_/BH_/HD_, from the TSVs through
 *     the mapping and order of VOCAB.tsv (iterate/kernel/vocab.tsv);
 *   - [third slice] TYPEV / NTYPEV, from TYPEKW.tsv (iterate/kernel/typekw.tsv),
 *     at the typekw row of VOCAB.tsv, in the file's declared order.
 * Not written here: the provenance header, ENC_.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NST 18          /* stages: exactly, order.tsv and UNS2 and the TSVs */
#define MAXF 4          /* fields per stage: the kernel's STAGE_VN stride */
#define MAXV 256        /* values per field */
#define MAXH 16         /* heads per stage (heads_max may not exceed it) */
#define KERNEL_HEADS 16 /* the real kernel's STAGE_NCLS stride: (s << 4) + head */
#define MAXC 256        /* classes per head: DENSE holds a class in a byte */
#define MAXK 8192       /* keys per stage */
#define MAXU 4096       /* units per stage: the kernel's act[4096] */
#define MAXZ 512        /* classes per head: the kernel's z[512] */
#define MAXE 262144     /* W2 entries per stage */
#define MAXD 262144     /* DENSE bytes in all */
#define MAXM 2097152    /* MODEL bytes in all */
#define TSVSZ 262144
#define U2SZ 1048576
#define OUTSZ 8388608
#define MAXP 512
#define POOLSZ 262144   /* the string pool: every schema string of every TSV */
#define MAXSL 255       /* bytes in one schema string */

char *gpath;
void die(char *msg) { printf("genmodel: %s: %s\n", gpath, msg); exit(1); }
void dieln(int ln, char *msg) { printf("genmodel: %s: line %d: %s\n", gpath, ln, msg); exit(1); }
void cap(char *msg, int n, int lim) {
    printf("genmodel: %s: capacity: %s %d, more than %d\n", gpath, msg, n, lim);
    exit(4);
}
int streq(char *a, char *b) { return strcmp(a, b) == 0; }
int prefix(char *s, char *p) {
    int i = 0;
    while (p[i]) { if (s[i] != p[i]) return 0; i = i + 1; }
    return 1;
}

/* read a whole file into b (size lim); returns its length */
int slurp(char *path, char *b, int lim) {
    FILE *f;
    int n;
    gpath = path;
    f = fopen(path, "rb");
    if (!f) die("cannot open");
    n = (int)fread(b, 1, lim, f);
    fclose(f);
    if (n >= lim) die("file too large");
    if (n < 0) die("cannot read");
    b[n] = 0;
    return n;
}

/* -------------------------------------------------------------- lines -- */
char *parts[MAXP];
int np;
char *lp;       /* next line in the buffer being read */
char *lend;
int lno;
char *nextline(void) {
    char *s;
    if (lp >= lend) return 0;
    s = lp;
    while (lp < lend && *lp != '\n' && *lp != '\r') lp = lp + 1;
    if (lp < lend) {
        if (*lp == '\r' && lp + 1 < lend && lp[1] == '\n') { *lp = 0; lp = lp + 2; }
        else { *lp = 0; lp = lp + 1; }
    }
    lno = lno + 1;
    return s;
}
void split(char *line) {
    int i = 0;
    np = 1;
    parts[0] = line;
    while (line[i]) {
        if (line[i] == '\t') {
            if (np >= MAXP) dieln(lno, "too many columns");
            line[i] = 0;
            parts[np] = line + i + 1;
            np = np + 1;
        }
        i = i + 1;
    }
}

/* ---------------------------------------------------------- order.tsv -- */
char tbuf[TSVSZ + 1];
char oname[NST][17];
int nord;
int headsmax;
void load_order(char *path) {
    int n, i, hm;
    char *s;
    n = slurp(path, tbuf, TSVSZ);
    lp = tbuf; lend = tbuf + n; lno = 0; nord = 0; hm = 0;
    while ((s = nextline()) != 0) {
        if (s[0] == 0 || s[0] == '#') continue;
        split(s);
        if (np != 2) dieln(lno, "want two columns: stage NAME | heads_max N");
        if (streq(parts[0], "stage")) {
            if (nord >= NST) { printf("genmodel: %s: line %d: more than %d stages\n", path, lno, NST); exit(1); }
            if (parts[1][0] == 0 || strlen(parts[1]) > 16) dieln(lno, "stage name empty or longer than 16");
            for (i = 0; i < nord; i = i + 1)
                if (streq(oname[i], parts[1])) { printf("genmodel: %s: line %d: duplicate stage %s\n", path, lno, parts[1]); exit(1); }
            strcpy(oname[nord], parts[1]);
            nord = nord + 1;
        } else if (streq(parts[0], "heads_max")) {
            if (hm) dieln(lno, "heads_max given twice");
            hm = 1;
            headsmax = 0;
            i = 0;
            if (parts[1][0] == 0) dieln(lno, "heads_max is not a number");
            while (parts[1][i]) {
                if (parts[1][i] < '0' || parts[1][i] > '9' || i > 4) dieln(lno, "heads_max is not a number");
                headsmax = headsmax * 10 + (parts[1][i] - '0');
                i = i + 1;
            }
            if (headsmax < 1) dieln(lno, "heads_max < 1");
            /* The real kernel indexes STAGE_NCLS[(s << 4) + head] (kernel/unisa_core.c,
               inf): its per-stage stride is 16, fixed in the kernel's code.
               heads_max is that stride, not a free choice -- any other value
               lays the table out in a way the kernel does not read.  Refused
               here, as soon as the number is read: before the capacity check,
               before any layout, before the output is opened.  check.sh confirms
               the kernel source still says `<< 4`, so this cannot go stale. */
            if (headsmax != KERNEL_HEADS) {
                printf("genmodel: %s: heads_max %d, the kernel's STAGE_NCLS stride is %d\n", path, headsmax, KERNEL_HEADS);
                exit(8);
            }
            if (headsmax > MAXH) cap("heads_max", headsmax, MAXH);
        } else dieln(lno, "unknown line kind");
    }
    if (!hm) die("no heads_max line");
    if (nord != NST) { printf("genmodel: %s: %d stages, want exactly %d\n", path, nord, NST); exit(1); }
}

/* ----------------------------------------------------------- the TSVs -- */
char tname[NST][17];     /* per TSV (argument order): its "# stage" name */
int tnf[NST];
int tnv[NST][MAXF];
int tnh[NST];
int tncl[NST][MAXH];
int tnk[NST];            /* keys */
int tdoff[NST];          /* its answers in dpool: key-major, head-minor */
char dpool[MAXD];
int dplen;
int ntsv;
/* Every schema string (field and head names, values, classes) is copied into
   pool[] as it is read: tbuf is reused by the next file, so a pointer into it
   would be overwritten.  Kept until the output is written.  Per table t:
   field i's name tfn[t*MAXF+i], value j tval[(t*MAXF+i)*MAXV+j]; head i's
   name thn[t*MAXH+i], class j tcls[(t*MAXH+i)*MAXC+j]. */
char pool[POOLSZ];
int pn;
char *pstr(char *s) {
    int n = (int)strlen(s), i;
    char *r;
    if (n > MAXSL) cap("string length", n, MAXSL);          /* checked before copying */
    if (pn + n + 1 > POOLSZ) cap("string pool bytes", pn + n + 1, POOLSZ);
    r = pool + pn;
    for (i = 0; i <= n; i = i + 1) r[i] = s[i];
    pn = pn + n + 1;
    return r;
}
char *tfn[NST * MAXF];
char *tval[NST * MAXF * MAXV];
char *thn[NST * MAXH];
char *tcls[NST * MAXH * MAXC];
int seen[MAXK];

void load_tsv(char *path) {
    int n, t, header, nf, nh, i, j, k, v, c, nk, stride[MAXF];
    char *s;
    char *sn;
    if (ntsv >= NST) { printf("genmodel: %s: more than %d gold tables\n", path, NST); exit(1); }
    t = ntsv;
    n = slurp(path, tbuf, TSVSZ);
    lp = tbuf; lend = tbuf + n; lno = 0;
    header = 0; nf = 0; nh = 0; sn = 0; nk = 0;
    while ((s = nextline()) != 0) {
        if (prefix(s, "# stage ")) {
            if (sn) dieln(lno, "a second stage line");
            sn = s + 8;
            i = 0;
            while (sn[i] && sn[i] != ':') i = i + 1;
            sn[i] = 0;
            if (sn[0] == 0 || i > 16) dieln(lno, "stage name empty or longer than 16");
            continue;
        }
        split(s);
        if (streq(parts[0], "#field")) {
            if (header) dieln(lno, "schema after the header");
            if (np < 3) dieln(lno, "a field with no values");
            if (nf >= MAXF) cap("fields", nf + 1, MAXF);
            if (np - 2 > MAXV) cap("values in a field", np - 2, MAXV);
            tfn[t * MAXF + nf] = pstr(parts[1]);
            tnv[t][nf] = np - 2;
            for (i = 2; i < np; i = i + 1) tval[(t * MAXF + nf) * MAXV + i - 2] = pstr(parts[i]);
            nf = nf + 1;
            continue;
        }
        if (streq(parts[0], "#head")) {
            if (header) dieln(lno, "schema after the header");
            if (np < 4) dieln(lno, "a head with no classes");
            if (nh >= MAXH) cap("heads", nh + 1, MAXH);
            if (np - 3 > MAXC) cap("classes in a head", np - 3, MAXC);
            thn[t * MAXH + nh] = pstr(parts[1]);
            tncl[t][nh] = np - 3;
            for (i = 3; i < np; i = i + 1) tcls[(t * MAXH + nh) * MAXC + i - 3] = pstr(parts[i]);
            nh = nh + 1;
            continue;
        }
        if (s[0] == '#') dieln(lno, "unknown # line");
        if (!header) {
            if (nf == 0 || nh == 0) dieln(lno, "header before the schema");
            if (np != nf + nh) dieln(lno, "header is not the schema's");
            for (i = 0; i < nf; i = i + 1)
                if (!streq(parts[i], tfn[t * MAXF + i])) dieln(lno, "header is not the schema's");
            for (i = 0; i < nh; i = i + 1) {
                c = nf + i;
                if (!prefix(parts[c], "=> ") || !streq(parts[c] + 3, thn[t * MAXH + i])) dieln(lno, "header is not the schema's");
            }
            header = 1;
            nk = 1;
            for (i = nf - 1; i >= 0; i = i - 1) {
                stride[i] = nk;
                nk = nk * tnv[t][i];
                if (nk > MAXK) cap("keys", nk, MAXK);
            }
            if (dplen + nk * nh > MAXD) cap("DENSE bytes", dplen + nk * nh, MAXD);
            tdoff[t] = dplen;
            for (k = 0; k < nk; k = k + 1) seen[k] = 0;
            continue;
        }
        if (np != nf + nh) dieln(lno, "wrong number of columns");
        k = 0;
        for (i = 0; i < nf; i = i + 1) {
            v = -1;
            for (j = 0; j < tnv[t][i]; j = j + 1) if (streq(parts[i], tval[(t * MAXF + i) * MAXV + j])) { v = j; break; }
            if (v < 0) dieln(lno, "not a value of its field");
            k = k + v * stride[i];
        }
        if (seen[k]) dieln(lno, "key repeats");
        seen[k] = 1;
        for (i = 0; i < nh; i = i + 1) {
            c = -1;
            for (j = 0; j < tncl[t][i]; j = j + 1) if (streq(parts[nf + i], tcls[(t * MAXH + i) * MAXC + j])) { c = j; break; }
            if (c < 0) dieln(lno, "not a class of its head");
            dpool[tdoff[t] + k * nh + i] = (char)c;
        }
    }
    if (sn == 0 || nf == 0 || nh == 0 || !header) die("not a gold table with a schema");
    for (i = 0; i < nf; i = i + 1)
        for (j = 0; j < i; j = j + 1) if (streq(tfn[t * MAXF + i], tfn[t * MAXF + j])) die("field names repeat");
    for (i = 0; i < nh; i = i + 1)
        for (j = 0; j < i; j = j + 1) if (streq(thn[t * MAXH + i], thn[t * MAXH + j])) die("head names repeat");
    for (i = 0; i < nf; i = i + 1)
        for (j = 0; j < tnv[t][i]; j = j + 1)
            for (k = 0; k < j; k = k + 1) if (streq(tval[(t * MAXF + i) * MAXV + j], tval[(t * MAXF + i) * MAXV + k])) die("values repeat");
    for (i = 0; i < nh; i = i + 1)
        for (j = 0; j < tncl[t][i]; j = j + 1)
            for (k = 0; k < j; k = k + 1) if (streq(tcls[(t * MAXH + i) * MAXC + j], tcls[(t * MAXH + i) * MAXC + k])) die("classes repeat");
    for (k = 0; k < nk; k = k + 1) if (!seen[k]) die("keys do not cover the fields' product");
    for (i = 0; i < ntsv; i = i + 1)
        if (streq(tname[i], sn)) { printf("genmodel: %s: stage %s given twice\n", path, sn); exit(1); }
    strcpy(tname[t], sn);
    tnf[t] = nf; tnh[t] = nh; tnk[t] = nk;
    dplen = dplen + nk * nh;
    ntsv = ntsv + 1;
}

/* --------------------------------------------------------------- UNS2 -- */
char u2[U2SZ + 1];
int u2n;
char *u2path;
int nsec;
char uname[NST][17];
int ush0[NST]; int usH[NST]; int usnf[NST]; int usnh[NST];
int usfl[NST][MAXF];
int usw1[NST];           /* W1 bytes offset */
int ushoff[NST][MAXH];   /* head payload offset */
int ushlen[NST][MAXH];
int usncl[NST][MAXH];
int uswb[NST][MAXH];

int ub(int p) { return u2[p] & 255; }
void need(int p, int k, char *what) {
    if (p < 0 || k < 0 || p + k > u2n) {
        printf("genmodel: %s: truncated UNS2: %s needs %d B at offset %d, file is %d B\n", u2path, what, k, p, u2n);
        exit(1);
    }
}
int u16at(int p) { return ub(p) | (ub(p + 1) << 8); }
int u32at(int p, char *what) {
    if (ub(p + 3) > 127) { printf("genmodel: %s: %s at offset %d is too large\n", u2path, what, p); exit(1); }
    return ub(p) | (ub(p + 1) << 8) | (ub(p + 2) << 16) | (ub(p + 3) << 24);
}
int bitat(int base, int i) { return (ub(base + (i >> 3)) >> (7 - (i & 7))) & 1; }

/* a head payload's bit cursor */
int hb_base; int hb_len; int hb_i;
int hbits(int n) {
    int v = 0, k;
    if (hb_i + n > hb_len * 8) return -1;
    for (k = 0; k < n; k = k + 1) { v = (v << 1) | bitat(hb_base, hb_i); hb_i = hb_i + 1; }
    return v;
}
int wbits(int n) {   /* max(1, (n - 1).bit_length()) */
    int b = 0, x = n - 1;
    while (x > 0) { b = b + 1; x = x >> 1; }
    return b < 1 ? 1 : b;
}

void load_uns2(char *path) {
    int p, s, i, nu, tot, h0, H, nf, nh, w1len, hlen, j;
    char nm[17];
    u2path = path;
    u2n = slurp(path, u2, U2SZ);
    gpath = path;
    need(0, 16, "header");
    if (u2[0] != 'U' || u2[1] != 'N' || u2[2] != 'S' || u2[3] != '2') die("not UNS2 (magic)");
    if (ub(4) != 1) die("UNS2 version is not 1");
    if (ub(5) != 0) die("UNS2 flags are not 0");
    nsec = u16at(6);
    nu = u32at(8, "nUnits");
    if (u32at(12, "pad") != 0) die("UNS2 header pad is not 0");
    if (nsec != NST) { printf("genmodel: %s: %d stages, want exactly %d\n", path, nsec, NST); exit(1); }
    p = 16; tot = 0;
    for (s = 0; s < nsec; s = s + 1) {
        need(p, 24, "stage header");
        for (i = 0; i < 16; i = i + 1) nm[i] = u2[p + i];
        nm[16] = 0;
        if (nm[0] == 0) die("empty stage name");
        i = (int)strlen(nm);
        while (i < 16) { if (u2[p + i] != 0) die("stage name not NUL padded"); i = i + 1; }
        for (i = 0; i < s; i = i + 1)
            if (streq(uname[i], nm)) { printf("genmodel: %s: stage %s twice in UNS2\n", path, nm); exit(1); }
        strcpy(uname[s], nm);
        h0 = u16at(p + 16); H = u16at(p + 18); nf = ub(p + 20); nh = ub(p + 21);
        if (u16at(p + 22) != 0) die("stage pad is not 0");
        p = p + 24;
        if (nf < 1) die("a stage with no fields");
        if (nh < 1) die("a stage with no heads");
        if (nf > MAXF) cap("UNS2 fields", nf, MAXF);
        if (nh > MAXH) cap("UNS2 heads", nh, MAXH);
        if (H > MAXU) cap("units", H, MAXU);
        ush0[s] = h0; usH[s] = H; usnf[s] = nf; usnh[s] = nh;
        need(p, 2 * nf, "field lengths");
        j = 0;
        for (i = 0; i < nf; i = i + 1) { usfl[s][i] = u16at(p + 2 * i); j = j + usfl[s][i]; }
        p = p + 2 * nf;
        if (j != h0) { printf("genmodel: %s: stage %s: h0 %d is not the sum of its field lengths %d\n", path, nm, h0, j); exit(1); }
        need(p, 4, "W1 length");
        w1len = u32at(p, "W1 length");
        p = p + 4;
        if (w1len != (H * h0 + 7) / 8) { printf("genmodel: %s: stage %s: W1 is %d B, H %d x h0 %d wants %d\n", path, nm, w1len, H, h0, (H * h0 + 7) / 8); exit(1); }
        need(p, w1len, "W1");
        usw1[s] = p;
        p = p + w1len;
        for (i = 0; i < nh; i = i + 1) {
            need(p, 8, "head header");
            usncl[s][i] = u16at(p);
            uswb[s][i] = ub(p + 2);
            if (ub(p + 3) != 0) die("head pad is not 0");
            hlen = u32at(p + 4, "head payload length");
            p = p + 8;
            if (usncl[s][i] < 1) die("a head with no classes");
            if (uswb[s][i] < 1) die("a head with wBits 0");
            if (uswb[s][i] > 16) cap("wBits (MODEL holds w in u16)", uswb[s][i], 16);
            if (usncl[s][i] > MAXZ) cap("classes (the kernel's z[])", usncl[s][i], MAXZ);
            need(p, hlen, "head payload");
            ushoff[s][i] = p;
            ushlen[s][i] = hlen;
            p = p + hlen;
        }
        tot = tot + H;
    }
    if (p != u2n) { printf("genmodel: %s: %d trailing bytes after the last stage\n", path, u2n - p); exit(1); }
    if (tot != nu) { printf("genmodel: %s: header says %d units, the stages hold %d\n", path, nu, tot); exit(1); }
}

/* ------------------------------------------------------------- output -- */
char out[OUTSZ];
int on;
void oc(int c) { if (on >= OUTSZ) cap("output bytes", on + 1, OUTSZ); out[on] = (char)c; on = on + 1; }
void os_(char *s) { while (*s) { oc(*s); s = s + 1; } }
void od(int v) {
    char d[16];
    int n = 0;
    if (v < 0) { oc('-'); v = 0 - v; }
    if (v == 0) { oc('0'); return; }
    while (v > 0) { d[n] = (char)('0' + v % 10); v = v / 10; n = n + 1; }
    while (n > 0) { n = n - 1; oc(d[n]); }
}
/* ckernel._cstr: 20 bytes per line as \xNN, last line ends ';' */
void cstr(char *b, int n, char *name) {
    char *hx = "0123456789abcdef";
    int i, c;
    os_("char *"); os_(name); os_(" =\n");
    for (i = 0; i < n; i = i + 1) {
        if (i % 20 == 0) os_("  \"");
        c = b[i] & 255;
        oc('\\'); oc('x'); oc(hx[c >> 4]); oc(hx[c & 15]);
        if (i % 20 == 19 || i == n - 1) {
            oc('"');
            if (i == n - 1) oc(';');
            oc('\n');
        }
    }
}

char model[MAXM];
int mlen;
void mb(int c) { if (mlen >= MAXM) cap("MODEL bytes", mlen + 1, MAXM); model[mlen] = (char)c; mlen = mlen + 1; }
int ej[MAXE]; int eh[MAXE]; int ek[MAXE]; int ew[MAXE];
int sidx_u[NST]; int sidx_t[NST];
int soff[NST]; int smw[NST]; int sdoff[NST];
int smw_last;
char dense[MAXD];

/* one stage (UNS2 section u) into MODEL, as _blob */
void blob_stage(int u, char *nm) {
    int H = usH[u], h0 = ush0[u], nf = usnf[u], nh = usnh[u];
    int W, i, j, k, fo, cnt, full, b, ne, hi, cb, e;
    char mk[32];
    W = 1;
    for (i = 0; i < nf; i = i + 1) if ((usfl[u][i] + 63) / 64 > W) W = (usfl[u][i] + 63) / 64;
    if (W * 8 > 32) cap("mask words per field", W, 4);
    for (j = 0; j < H; j = j + 1) {
        fo = 0;
        for (i = 0; i < nf; i = i + 1) {
            for (b = 0; b < W * 8; b = b + 1) mk[b] = 0;
            cnt = 0;
            for (k = 0; k < usfl[u][i]; k = k + 1)
                if (bitat(usw1[u], j * h0 + fo + k)) { mk[k >> 3] = (char)(mk[k >> 3] | (1 << (k & 7))); cnt = cnt + 1; }
            full = cnt == usfl[u][i];
            for (b = 0; b < W * 8; b = b + 1) mb(full ? 0 : mk[b]);
            fo = fo + usfl[u][i];
        }
    }
    /* bits of W1 past H * h0 must be the zero padding */
    for (k = H * h0; k < ((H * h0 + 7) / 8) * 8; k = k + 1)
        if (bitat(usw1[u], k)) { printf("genmodel: %s: stage %s: W1 padding bits are not 0\n", u2path, nm); exit(1); }
    ne = 0;
    for (hi = 0; hi < nh; hi = hi + 1) {
        hb_base = ushoff[u][hi]; hb_len = ushlen[u][hi]; hb_i = 0;
        cb = wbits(usncl[u][hi]);
        for (j = 0; j < H; j = j + 1) {
            cnt = hbits(8);
            if (cnt < 0) { printf("genmodel: %s: stage %s head %d: payload ends inside unit %d\n", u2path, nm, hi, j); exit(1); }
            for (e = 0; e < cnt; e = e + 1) {
                if (ne >= MAXE) cap("W2 entries in a stage", ne + 1, MAXE);
                ek[ne] = hbits(cb);
                ew[ne] = hbits(uswb[u][hi]);
                if (ek[ne] < 0 || ew[ne] < 0) { printf("genmodel: %s: stage %s head %d: payload ends inside unit %d\n", u2path, nm, hi, j); exit(1); }
                if (ek[ne] >= usncl[u][hi]) { printf("genmodel: %s: stage %s head %d unit %d: class %d of %d\n", u2path, nm, hi, j, ek[ne], usncl[u][hi]); exit(1); }
                ej[ne] = j; eh[ne] = hi;
                ne = ne + 1;
            }
        }
        if ((hb_i + 7) / 8 != hb_len) { printf("genmodel: %s: stage %s head %d: payload is %d B, its units use %d\n", u2path, nm, hi, hb_len, (hb_i + 7) / 8); exit(1); }
        while (hb_i < hb_len * 8) {
            if (bitat(hb_base, hb_i)) { printf("genmodel: %s: stage %s head %d: payload padding bits are not 0\n", u2path, nm, hi); exit(1); }
            hb_i = hb_i + 1;
        }
    }
    mb(ne & 255); mb((ne >> 8) & 255); mb((ne >> 16) & 255); mb((ne >> 24) & 255);
    for (e = 0; e < ne; e = e + 1) {
        mb(ej[e] & 255); mb((ej[e] >> 8) & 255); mb(eh[e]);
        mb(ek[e] & 255); mb((ek[e] >> 8) & 255);
        mb(ew[e] & 255); mb((ew[e] >> 8) & 255);
    }
    smw_last = W;
}

/* ------------------------------------------------ vocab.tsv (slice 2) -- */
/* vocab.tsv is the mapping AND the order: 'vocab SYM stage field|head name',
   'typekw SYM' (its values from TYPEKW.tsv), 'bfbh stage'.  Rows are
   written in the file's order.  NVOCROW, NBFBH and the ABI list below are a
   CONSUMER ABI check -- the symbols kernel/unisa_core.c and src/ link against
   must all be declared -- not a second mapping: which stage, list and
   position a symbol comes from is only in vocab.tsv. */
#define NVOCROW 16      /* vocab + typekw rows */
#define NBFBH 11        /* bfbh rows */
#define MAXROW 64
#define MAXSYM 1024     /* written symbols, for the collision check */
/* packed as NUL-separated strings: the subset has no array-of-pointer initialiser */
char *ABI = "TOKV\0PRODV\0ACTV\0TYPEV\0DIRV\0PPACTV\0NTV\0TYSV\0TOPSV\0TYOUTV\0SCTXV\0SKINDV\0SACTV\0IRFAMV\0IRFLAV\0IRRECV\0";
char *ABI_TYPEKW = "TYPEV";
char *FIXED = "MODEL\0NSTAGE\0STAGE_M\0STAGE_H\0STAGE_OFF\0STAGE_NCLS\0STAGE_MW\0STAGE_VN\0act\0z\0DENSE\0DENSE_LEN\0STAGE_DOFF\0STAGE_NH\0model_dims\0";
#define NFIXED 15
char *KW = "auto\0break\0case\0char\0const\0continue\0default\0do\0double\0else\0enum\0extern\0float\0for\0goto\0if\0inline\0int\0long\0register\0restrict\0return\0short\0signed\0sizeof\0static\0struct\0switch\0typedef\0union\0unsigned\0void\0volatile\0while\0_Bool\0";
#define NKW 35
char *nth(char *p, int i) { while (i > 0) { p = p + strlen(p) + 1; i = i - 1; } return p; }

char vbuf[TSVSZ + 1];
int nrow;
int rkind[MAXROW];      /* 0 vocab, 1 typekw, 2 bfbh */
char *rsym[MAXROW];     /* vocab/typekw: the declared symbol */
char *rnsym[MAXROW];    /* vocab/typekw: N<symbol> */
int rtsv[MAXROW];       /* vocab/bfbh: the stage's TSV index */
int rhead[MAXROW];      /* vocab: 0 field, 1 head */
int ridx[MAXROW];       /* vocab: the field or head index in that TSV */
int rln[MAXROW];
char *vpath;
void okval(char *s, int octal0, char *sym, int j);
/* TYPEKW.tsv: 'kw<TAB>value' rows, the declared order; '#' lines are comments */
#define MAXTKW 256
char kbuf[TSVSZ + 1];
char *tkw[MAXTKW];
int ntkw;
int tkwrow;             /* the typekw row of vocab.tsv */
char *gsym[MAXSYM];
int ngsym;

void vdie(int ln, char *a, char *b) { printf("genmodel: %s: line %d: %s%s\n", vpath, ln, a, b); exit(1); }
int stage_tsv(char *nm) {
    int i;
    for (i = 0; i < ntsv; i = i + 1) if (streq(tname[i], nm)) return i;
    return -1;
}
void load_vocab(char *path) {
    int n, t, i, nv, nb, k;
    char *s;
    vpath = path; gpath = path;
    n = slurp(path, vbuf, TSVSZ);
    lp = vbuf; lend = vbuf + n; lno = 0; nrow = 0; nv = 0; nb = 0; tkwrow = -1;
    while ((s = nextline()) != 0) {
        if (s[0] == 0 || s[0] == '#') continue;
        split(s);
        if (nrow >= MAXROW) vdie(lno, "too many rows", "");
        rln[nrow] = lno;
        if (streq(parts[0], "vocab")) {
            if (np != 5) vdie(lno, "want: vocab SYM stage field|head name", "");
            t = stage_tsv(parts[2]);
            if (t < 0) vdie(lno, "unknown stage ", parts[2]);
            rkind[nrow] = 0; rsym[nrow] = pstr(parts[1]); rtsv[nrow] = t; ridx[nrow] = -1;
            if (streq(parts[3], "field")) {
                rhead[nrow] = 0;
                for (i = 0; i < tnf[t]; i = i + 1) if (streq(tfn[t * MAXF + i], parts[4])) ridx[nrow] = i;
                if (ridx[nrow] < 0) { printf("genmodel: %s: line %d: stage %s has no #field %s\n", path, lno, parts[2], parts[4]); exit(1); }
            } else if (streq(parts[3], "head")) {
                rhead[nrow] = 1;
                for (i = 0; i < tnh[t]; i = i + 1) if (streq(thn[t * MAXH + i], parts[4])) ridx[nrow] = i;
                if (ridx[nrow] < 0) { printf("genmodel: %s: line %d: stage %s has no #head %s\n", path, lno, parts[2], parts[4]); exit(1); }
            } else vdie(lno, "want field or head, not ", parts[3]);
            nv = nv + 1;
        } else if (streq(parts[0], "typekw")) {
            if (np != 2) vdie(lno, "want: typekw SYM", "");
            if (tkwrow >= 0) vdie(lno, "typekw row given twice", "");
            tkwrow = nrow;
            rkind[nrow] = 1; rsym[nrow] = pstr(parts[1]); rtsv[nrow] = -1;
            nv = nv + 1;
        } else if (streq(parts[0], "bfbh")) {
            if (np != 2) vdie(lno, "want: bfbh stage", "");
            t = stage_tsv(parts[1]);
            if (t < 0) vdie(lno, "unknown stage ", parts[1]);
            for (k = 0; k < nrow; k = k + 1)
                if (rkind[k] == 2 && rtsv[k] == t) vdie(lno, "bfbh stage given twice: ", parts[1]);
            rkind[nrow] = 2; rsym[nrow] = 0; rtsv[nrow] = t;
            nb = nb + 1;
        } else vdie(lno, "unknown row kind ", parts[0]);
        nrow = nrow + 1;
    }
    if (tkwrow < 0) { printf("genmodel: %s: no typekw row\n", path); exit(1); }
    for (i = 0; i < nrow; i = i + 1)
        for (k = 0; k < i; k = k + 1)
            if (rkind[i] != 2 && rkind[k] != 2 && streq(rsym[i], rsym[k])) vdie(rln[i], "duplicate symbol ", rsym[i]);
    if (nv < NVOCROW) { printf("genmodel: %s: missing row: %d vocab/typekw rows, want %d\n", path, nv, NVOCROW); exit(1); }
    if (nv > NVOCROW) { printf("genmodel: %s: extra row: %d vocab/typekw rows, want %d\n", path, nv, NVOCROW); exit(1); }
    if (nb < NBFBH) { printf("genmodel: %s: missing row: %d bfbh rows, want %d\n", path, nb, NBFBH); exit(1); }
    if (nb > NBFBH) { printf("genmodel: %s: extra row: %d bfbh rows, want %d\n", path, nb, NBFBH); exit(1); }
}

/* TYPEKW.tsv: the declared type keywords, in order.  Count and values come
   only from this file; every value is checked by okval (the vocab rules)
   and no value may repeat. */
void load_typekw(char *path) {
    int n, i, k;
    char *s;
    gpath = path;
    n = slurp(path, kbuf, TSVSZ);
    lp = kbuf; lend = kbuf + n; lno = 0; ntkw = 0;
    while ((s = nextline()) != 0) {
        if (s[0] == '#') continue;
        if (s[0] == 0) dieln(lno, "empty line (want: kw<TAB>value)");
        split(s);
        if (np != 2 || !streq(parts[0], "kw")) dieln(lno, "want: kw<TAB>value");
        if (ntkw >= MAXTKW) cap("typekw values", ntkw + 1, MAXTKW);
        tkw[ntkw] = pstr(parts[1]);
        for (k = 0; k < ntkw; k = k + 1)
            if (streq(tkw[k], tkw[ntkw])) { printf("genmodel: %s: line %d: duplicate value %s\n", path, lno, tkw[ntkw]); exit(1); }
        ntkw = ntkw + 1;
    }
    if (ntkw == 0) die("no kw rows");
    vpath = path;
    for (i = 0; i < ntkw; i = i + 1) okval(tkw[i], 1, "TYPEKW", i);
}

/* ---- names: every symbol written is built here and checked before output */
int isident(char *s) {
    int i = 0;
    if (!((s[0] >= 'A' && s[0] <= 'Z') || (s[0] >= 'a' && s[0] <= 'z') || s[0] == '_')) return 0;
    while (s[i]) {
        if (!((s[i] >= 'A' && s[i] <= 'Z') || (s[i] >= 'a' && s[i] <= 'z') || (s[i] >= '0' && s[i] <= '9') || s[i] == '_')) return 0;
        i = i + 1;
    }
    for (i = 0; i < NKW; i = i + 1) if (streq(s, nth(KW, i))) return 0;
    return 1;
}
int up(int c) { return (c >= 'a' && c <= 'z') ? c - 'a' + 'A' : c; }
int foldeq(char *a, char *b) {
    while (*a && *b) { if (up(*a) != up(*b)) return 0; a = a + 1; b = b + 1; }
    return *a == *b;
}
/* a + b (b upper-cased when ub) + c upper-cased + (d >= 0 ? decimal d : ""), into the pool */
char nbuf[600];
char *mkname(char *a, char *b, int ub, char *c, int d) {
    int n = 0, i, m = 0, len;
    char dg[16];
    len = (int)(strlen(a) + strlen(b) + strlen(c)) + 11;
    if (len > MAXSL) cap("generated name length", len, MAXSL);
    while (*a) { nbuf[n] = *a; n = n + 1; a = a + 1; }
    while (*b) { nbuf[n] = (char)(ub ? up(*b) : *b); n = n + 1; b = b + 1; }
    while (*c) { nbuf[n] = (char)up(*c); n = n + 1; c = c + 1; }
    if (d >= 0) {
        if (d == 0) { dg[0] = '0'; m = 1; }
        while (d > 0) { dg[m] = (char)('0' + d % 10); d = d / 10; m = m + 1; }
        for (i = m - 1; i >= 0; i = i - 1) { nbuf[n] = dg[i]; n = n + 1; }
    }
    nbuf[n] = 0;
    return pstr(nbuf);
}
void addsym(char *s) {
    int i;
    if (!isident(s)) { printf("genmodel: %s: symbol %s is not a legal C identifier\n", vpath, s); exit(1); }
    for (i = 0; i < ngsym; i = i + 1)
        if (foldeq(gsym[i], s)) {
            if (streq(gsym[i], s)) printf("genmodel: %s: symbol %s collides with %s\n", vpath, s, gsym[i]);
            else printf("genmodel: %s: symbol %s collides with %s (case-folded)\n", vpath, s, gsym[i]);
            exit(1);
        }
    if (ngsym >= MAXSYM) cap("written symbols", ngsym + 1, MAXSYM);
    gsym[ngsym] = s; ngsym = ngsym + 1;
}
char *bfn[NST * MAXF]; char *nbfn[NST * MAXF];
char *bhn[NST * MAXH]; char *nbhn[NST * MAXH]; char *hdn[NST * MAXH];

/* This tool's conservative serialization domain.  Bytes are written as they
   are, never escaped, so a value that would need an escape, could read as a
   trigraph, or could extend an octal escape is refused (exit 1) instead.
   Vocab strings end each value with \0, so a non-first value starting 0-7
   would extend it (8 and 9 are not octal digits); BF/BH use \000, three
   digits, so a leading digit is safe there. */
void okval(char *s, int octal0, char *sym, int j) {
    int i = 0, c;
    char *why = 0;
    if (s[0] == 0) why = "an empty value";
    while (!why && s[i]) {
        c = s[i] & 255;
        if (c == '"') why = "a quote";
        else if (c == '\\') why = "a backslash";
        else if (c < 32 || c > 126) why = "a control or non-ASCII byte";
        else if (c == '?' && s[i + 1] == '?') why = "\"??\" (a trigraph start)";
        i = i + 1;
    }
    if (!why && octal0 && j > 0 && s[0] >= '0' && s[0] <= '7') why = "a non-first vocab value starting 0-7 (after \\0)";
    if (why) { printf("genmodel: %s: %s value %d is outside this tool's input domain: %s\n", vpath, sym, j, why); exit(1); }
}
void commentok(char *s, char *what) {
    int i = 0;
    while (s[i]) {
        if ((s[i] & 255) < 32 || (s[i] & 255) > 126 || (s[i] == '*' && s[i + 1] == '/') || (s[i] == '/' && s[i + 1] == '*')) {
            printf("genmodel: %s: %s %s cannot be written in a comment\n", vpath, what, s);
            exit(1);
        }
        i = i + 1;
    }
}

void vocab_names(void) {
    int i, r, t, f, h, k;
    char *a;
    ngsym = 0;
    /* what the first slice writes */
    for (i = 0; i < NFIXED; i = i + 1) addsym(nth(FIXED, i));
    for (i = 0; i < NST; i = i + 1) addsym(mkname("S_", oname[i], 1, "", -1));
    for (r = 0; r < nrow; r = r + 1) {
        t = rtsv[r];
        if (rkind[r] == 2) {
            commentok(tname[t], "stage name");
            for (f = 0; f < tnf[t]; f = f + 1) {
                k = t * MAXF + f;
                commentok(tfn[k], "field name");
                bfn[k] = mkname("BF_", tname[t], 1, "_", f); addsym(bfn[k]);
                nbfn[k] = mkname("N", bfn[k], 0, "", -1); addsym(nbfn[k]);
            }
            for (h = 0; h < tnh[t]; h = h + 1) {
                k = t * MAXH + h;
                a = mkname("_", tname[t], 1, "_", -1);
                bhn[k] = mkname("BH", a, 0, thn[k], -1); addsym(bhn[k]);
                nbhn[k] = mkname("N", bhn[k], 0, "", -1); addsym(nbhn[k]);
                hdn[k] = mkname("HD", a, 0, thn[k], -1); addsym(hdn[k]);
            }
        } else {
            if (rkind[r] == 0) {
                commentok(tname[t], "stage name");
                commentok(rhead[r] ? thn[t * MAXH + ridx[r]] : tfn[t * MAXF + ridx[r]], "list name");
            }
            addsym(rsym[r]);                           /* kept exactly as declared */
            rnsym[r] = mkname("N", rsym[r], 0, "", -1);
            addsym(rnsym[r]);
        }
    }
    /* consumer ABI: every symbol the kernel links against is declared, and
       the TYPEKW one is the typekw row (its values from TYPEKW.tsv) */
    for (i = 0; i < NVOCROW; i = i + 1) {
        a = nth(ABI, i); f = -1;
        for (r = 0; r < nrow; r = r + 1) if (rkind[r] != 2 && streq(rsym[r], a)) f = r;
        if (f < 0) { printf("genmodel: %s: consumer ABI: symbol %s is not declared\n", vpath, a); exit(1); }
        if ((rkind[f] == 1) != streq(a, ABI_TYPEKW)) { printf("genmodel: %s: consumer ABI: %s is the wrong row kind\n", vpath, a); exit(1); }
    }
    /* the serialization domain, over every string that will be written */
    for (r = 0; r < nrow; r = r + 1) {
        t = rtsv[r];
        if (rkind[r] == 0 && rhead[r] == 0)
            for (i = 0; i < tnv[t][ridx[r]]; i = i + 1) okval(tval[(t * MAXF + ridx[r]) * MAXV + i], 1, rsym[r], i);
        if (rkind[r] == 0 && rhead[r] == 1)
            for (i = 0; i < tncl[t][ridx[r]]; i = i + 1) okval(tcls[(t * MAXH + ridx[r]) * MAXC + i], 1, rsym[r], i);
        if (rkind[r] == 2) {
            for (f = 0; f < tnf[t]; f = f + 1)
                for (i = 0; i < tnv[t][f]; i = i + 1) okval(tval[(t * MAXF + f) * MAXV + i], 0, bfn[t * MAXF + f], i);
            for (h = 0; h < tnh[t]; h = h + 1)
                for (i = 0; i < tncl[t][h]; i = i + 1) okval(tcls[(t * MAXH + h) * MAXC + i], 0, bhn[t * MAXH + h], i);
        }
    }
}

/* the vocab / BF / BH region, rows in vocab.tsv order, laid out as ckernel.py */
void emit_vocab(void) {
    int r, t, f, h, i, n, b, k;
    os_("\n");
    for (r = 0; r < nrow; r = r + 1) {
        t = rtsv[r];
        if (rkind[r] == 1) {
            os_("/* "); os_(rsym[r]); os_(": the typekw declaration (TYPEKW.tsv), in its order */\n");
            os_("char *"); os_(rsym[r]); os_(" = \"");
            for (i = 0; i < ntkw; i = i + 1) { os_(tkw[i]); os_("\\0"); }
            os_("\";\n#define "); os_(rnsym[r]); oc(' '); od(ntkw); os_("\n\n");
        } else if (rkind[r] == 0) {
            if (rhead[r]) { b = (t * MAXH + ridx[r]) * MAXC; n = tncl[t][ridx[r]]; }
            else { b = (t * MAXF + ridx[r]) * MAXV; n = tnv[t][ridx[r]]; }
            os_("/* "); os_(rsym[r]); os_(": gold TSV "); os_(tname[t]); os_(rhead[r] ? " #head " : " #field ");
            os_(rhead[r] ? thn[t * MAXH + ridx[r]] : tfn[t * MAXF + ridx[r]]); os_(" */\n");
            os_("char *"); os_(rsym[r]); os_(" = \"");
            for (i = 0; i < n; i = i + 1) { os_(rhead[r] ? tcls[b + i] : tval[b + i]); os_("\\0"); }
            os_("\";\n#define "); os_(rnsym[r]); oc(' '); od(n); os_("\n\n");
        } else {
            for (f = 0; f < tnf[t]; f = f + 1) {
                k = t * MAXF + f;
                os_("/* "); os_(bfn[k]); os_(": gold TSV "); os_(tname[t]); os_(" #field "); od(f);
                os_(" ("); os_(tfn[k]); os_(") */\n");
                os_("char *"); os_(bfn[k]); os_(" = \"");
                for (i = 0; i < tnv[t][f]; i = i + 1) { os_(tval[k * MAXV + i]); os_("\\000"); }
                os_("\";\n#define "); os_(nbfn[k]); oc(' '); od(tnv[t][f]); os_("\n\n");
            }
            for (h = 0; h < tnh[t]; h = h + 1) {
                k = t * MAXH + h;
                os_("/* "); os_(bhn[k]); os_(": gold TSV "); os_(tname[t]); os_(" #head "); os_(thn[k]);
                os_(", its class order */\n");
                os_("char *"); os_(bhn[k]); os_(" = \"");
                for (i = 0; i < tncl[t][h]; i = i + 1) { os_(tcls[k * MAXC + i]); os_("\\000"); }
                os_("\";\n#define "); os_(nbhn[k]); oc(' '); od(tncl[t][h]);
                os_("\n#define "); os_(hdn[k]); oc(' '); od(h); os_("\n\n");
            }
        }
    }
    os_("/* genmodel: END of the vocab / BF / BH region (ENC_* is not written here) */\n");
}

void upcase(char *s) {
    while (*s) { if (*s >= 'a' && *s <= 'z') oc(*s - 'a' + 'A'); else oc(*s); s = s + 1; }
}

int main(int argc, char **argv) {
    char *opath;
    int ai, i, j, s, u, t, f, h;
    FILE *fo;
    int n;
    int dl;
    if (argc < 7 || !streq(argv[1], "-o")) {
        printf("usage: genmodel -o OUT ORDER.tsv VOCAB.tsv TYPEKW.tsv BUILT.uns2 TSV...\n");
        exit(2);
    }
    opath = argv[2];
    load_order(argv[3]);
    load_uns2(argv[6]);
    for (ai = 7; ai < argc; ai = ai + 1) load_tsv(argv[ai]);
    if (ntsv != NST) { printf("genmodel: %d gold tables given, want exactly %d (one per stage)\n", ntsv, NST); exit(1); }
    /* match by name: every order.tsv stage is exactly one UNS2 section and
       exactly one TSV; the counts are all NST and each list has no repeat,
       so the three name sets are then equal */
    for (s = 0; s < NST; s = s + 1) {
        sidx_u[s] = -1; sidx_t[s] = -1;
        for (i = 0; i < nsec; i = i + 1) if (streq(uname[i], oname[s])) sidx_u[s] = i;
        for (i = 0; i < ntsv; i = i + 1) if (streq(tname[i], oname[s])) sidx_t[s] = i;
        if (sidx_u[s] < 0) { printf("genmodel: stage %s of order.tsv is not in UNS2\n", oname[s]); exit(1); }
        if (sidx_t[s] < 0) { printf("genmodel: stage %s of order.tsv has no gold table\n", oname[s]); exit(1); }
    }
    /* dimensions: UNS2 against the TSV, stage by stage */
    for (s = 0; s < NST; s = s + 1) {
        u = sidx_u[s]; t = sidx_t[s];
        if (usnf[u] != tnf[t]) { printf("genmodel: stage %s: UNS2 has %d fields, the TSV %d\n", oname[s], usnf[u], tnf[t]); exit(1); }
        for (f = 0; f < tnf[t]; f = f + 1)
            if (usfl[u][f] != tnv[t][f]) { printf("genmodel: stage %s: field %d has %d values in UNS2, %d in the TSV\n", oname[s], f, usfl[u][f], tnv[t][f]); exit(1); }
        if (usnh[u] != tnh[t]) { printf("genmodel: stage %s: UNS2 has %d heads, the TSV %d\n", oname[s], usnh[u], tnh[t]); exit(1); }
        if (tnh[t] > headsmax) { printf("genmodel: stage %s: %d heads, more than heads_max %d\n", oname[s], tnh[t], headsmax); exit(4); }
        for (h = 0; h < tnh[t]; h = h + 1)
            if (usncl[u][h] != tncl[t][h]) { printf("genmodel: stage %s: head %d has %d classes in UNS2, %d in the TSV\n", oname[s], h, usncl[u][h], tncl[t][h]); exit(1); }
    }
    /* second slice: the vocab mapping, every written name and string checked */
    load_vocab(argv[4]);
    vocab_names();
    load_typekw(argv[5]);
    /* MODEL and DENSE in order.tsv order */
    mlen = 0; dl = 0;
    for (s = 0; s < NST; s = s + 1) {
        u = sidx_u[s]; t = sidx_t[s];
        soff[s] = mlen;
        blob_stage(u, oname[s]);
        smw[s] = smw_last;
        sdoff[s] = dl;
        for (i = 0; i < tnk[t] * tnh[t]; i = i + 1) { dense[dl] = dpool[tdoff[t] + i]; dl = dl + 1; }
    }
    on = 0;
    os_("/* GENERATED by iterate/kernel/genmodel.c from order.tsv, built.uns2 and the\n");
    os_(" * gold TSVs named on its command line: the S_* defines, MODEL, DENSE and\n");
    os_(" * model_dims() of kernel/unisa_model.inc (J10 step 3, first slice). */\n\n");
    os_("/* S_<stage>: the stage order of order.tsv */\n");
    for (s = 0; s < NST; s = s + 1) { os_("#define S_"); upcase(oname[s]); oc(' '); od(s); oc('\n'); }
    os_("\n/* MODEL: every stage's units, packed: per stage [field masks][nw2][(unit, class, w2)...]\n");
    os_(" * -- decoded from built.uns2, in order.tsv order */\n");
    cstr(model, mlen, "MODEL");
    os_("\n#define NSTAGE "); od(NST); os_("\n\n");
    os_("int STAGE_M["); od(NST); os_("];\n");
    os_("int STAGE_H["); od(NST); os_("];\n");
    os_("int STAGE_OFF["); od(NST); os_("];\n");
    os_("int STAGE_NCLS["); od(NST * headsmax); os_("];\n");
    os_("int STAGE_MW["); od(NST); os_("];\n");
    os_("int STAGE_VN["); od(NST * MAXF); os_("];\n");
    os_("int act["); od(MAXU); os_("];\n");
    os_("int z["); od(MAXZ); os_("];\n\n");
    os_("/* DENSE: each stage's answers for every key (last field fastest), head-minor:\n");
    os_(" * the index of the key's gold TSV label in the head's class list */\n");
    cstr(dense, dl, "DENSE");
    os_("#define DENSE_LEN "); od(dl); oc('\n');
    os_("int STAGE_DOFF["); od(NST); os_("]; int STAGE_NH["); od(NST); os_("];\n\n");
    os_("int model_dims(void) {\n");
    for (s = 0; s < NST; s = s + 1) {
        os_("  STAGE_DOFF["); od(s); os_("] = "); od(sdoff[s]);
        os_("; STAGE_NH["); od(s); os_("] = "); od(tnh[sidx_t[s]]); os_(";\n");
    }
    for (s = 0; s < NST; s = s + 1) {
        u = sidx_u[s];
        os_("  STAGE_M["); od(s); os_("] = "); od(usnf[u]);
        os_("; STAGE_H["); od(s); os_("] = "); od(usH[u]);
        os_("; STAGE_OFF["); od(s); os_("] = "); od(soff[s]);
        os_(";  STAGE_MW["); od(s); os_("] = "); od(smw[s]); os_(";\n");
        for (h = 0; h < usnh[u]; h = h + 1) {
            os_("  STAGE_NCLS["); od(s * headsmax + h); os_("] = "); od(usncl[u][h]); os_(";\n");
        }
        for (f = 0; f < usnf[u]; f = f + 1) {
            os_("  STAGE_VN["); od(s * MAXF + f); os_("] = "); od(usfl[u][f]); os_(";\n");
        }
    }
    os_("  return "); od(NST); os_(";\n}\n");
    emit_vocab();
    fo = fopen(opath, "wb");
    if (!fo) { printf("genmodel: write: cannot open %s for writing\n", opath); exit(7); }
    n = (int)fwrite(out, 1, on, fo);
    if (n != on) { printf("genmodel: write: %s: short write, %d of %d B\n", opath, n, on); exit(7); }
    if (fclose(fo) != 0) { printf("genmodel: write: %s: close failed\n", opath); exit(7); }
    j = 0;
    printf("genmodel: %s: %d stages, MODEL %d B, DENSE %d B, %d B written\n", opath, NST, mlen, dl, on);
    return j;
}
