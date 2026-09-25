/* genmodel.c -- the model region of kernel/unisa_model.inc, from declared
 * inputs only, in the C subset unisacc compiles. [J10 step 3, first slice]
 *
 *   genmodel -o OUT ORDER.tsv BUILT.uns2 TSV...     (exactly one TSV per stage)
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
 * Not written here (out of this slice): the provenance header, the
 * vocabularies, BF_/BH_, ENC_.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NST 18          /* stages: exactly, order.tsv and UNS2 and the TSVs */
#define MAXF 4          /* fields per stage: the kernel's STAGE_VN stride */
#define MAXV 256        /* values per field */
#define MAXH 16         /* heads per stage (heads_max may not exceed it) */
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
char *val[MAXF][MAXV];
char *cls[MAXH][MAXC];
char *fname[MAXF];
char *hname[MAXH];
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
            fname[nf] = parts[1];
            tnv[t][nf] = np - 2;
            for (i = 2; i < np; i = i + 1) val[nf][i - 2] = parts[i];
            nf = nf + 1;
            continue;
        }
        if (streq(parts[0], "#head")) {
            if (header) dieln(lno, "schema after the header");
            if (np < 4) dieln(lno, "a head with no classes");
            if (nh >= MAXH) cap("heads", nh + 1, MAXH);
            if (np - 3 > MAXC) cap("classes in a head", np - 3, MAXC);
            hname[nh] = parts[1];
            tncl[t][nh] = np - 3;
            for (i = 3; i < np; i = i + 1) cls[nh][i - 3] = parts[i];
            nh = nh + 1;
            continue;
        }
        if (s[0] == '#') dieln(lno, "unknown # line");
        if (!header) {
            if (nf == 0 || nh == 0) dieln(lno, "header before the schema");
            if (np != nf + nh) dieln(lno, "header is not the schema's");
            for (i = 0; i < nf; i = i + 1)
                if (!streq(parts[i], fname[i])) dieln(lno, "header is not the schema's");
            for (i = 0; i < nh; i = i + 1) {
                c = nf + i;
                if (!prefix(parts[c], "=> ") || !streq(parts[c] + 3, hname[i])) dieln(lno, "header is not the schema's");
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
            for (j = 0; j < tnv[t][i]; j = j + 1) if (streq(parts[i], val[i][j])) { v = j; break; }
            if (v < 0) dieln(lno, "not a value of its field");
            k = k + v * stride[i];
        }
        if (seen[k]) dieln(lno, "key repeats");
        seen[k] = 1;
        for (i = 0; i < nh; i = i + 1) {
            c = -1;
            for (j = 0; j < tncl[t][i]; j = j + 1) if (streq(parts[nf + i], cls[i][j])) { c = j; break; }
            if (c < 0) dieln(lno, "not a class of its head");
            dpool[tdoff[t] + k * nh + i] = (char)c;
        }
    }
    if (sn == 0 || nf == 0 || nh == 0 || !header) die("not a gold table with a schema");
    for (i = 0; i < nf; i = i + 1)
        for (j = 0; j < i; j = j + 1) if (streq(fname[i], fname[j])) die("field names repeat");
    for (i = 0; i < nh; i = i + 1)
        for (j = 0; j < i; j = j + 1) if (streq(hname[i], hname[j])) die("head names repeat");
    for (i = 0; i < nf; i = i + 1)
        for (j = 0; j < tnv[t][i]; j = j + 1)
            for (k = 0; k < j; k = k + 1) if (streq(val[i][j], val[i][k])) die("values repeat");
    for (i = 0; i < nh; i = i + 1)
        for (j = 0; j < tncl[t][i]; j = j + 1)
            for (k = 0; k < j; k = k + 1) if (streq(cls[i][j], cls[i][k])) die("classes repeat");
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

void upcase(char *s) {
    while (*s) { if (*s >= 'a' && *s <= 'z') oc(*s - 'a' + 'A'); else oc(*s); s = s + 1; }
}

int main(int argc, char **argv) {
    char *opath;
    int ai, i, j, s, u, t, f, h;
    FILE *fo;
    int n;
    int dl;
    if (argc < 5 || !streq(argv[1], "-o")) {
        printf("usage: genmodel -o OUT ORDER.tsv BUILT.uns2 TSV...\n");
        exit(2);
    }
    opath = argv[2];
    load_order(argv[3]);
    load_uns2(argv[4]);
    for (ai = 5; ai < argc; ai = ai + 1) load_tsv(argv[ai]);
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
    fo = fopen(opath, "wb");
    if (!fo) { printf("genmodel: write: cannot open %s for writing\n", opath); exit(7); }
    n = (int)fwrite(out, 1, on, fo);
    if (n != on) { printf("genmodel: write: %s: short write, %d of %d B\n", opath, n, on); exit(7); }
    if (fclose(fo) != 0) { printf("genmodel: write: %s: close failed\n", opath); exit(7); }
    j = 0;
    printf("genmodel: %s: %d stages, MODEL %d B, DENSE %d B, %d B written\n", opath, NST, mlen, dl, on);
    return j;
}
