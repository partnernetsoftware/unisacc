/* construct.c -- the weight constructor, in the C subset unisacc compiles. [J10]
 *
 * Reads one gold table, weights/gold/<stage>.tsv, and prints the net that
 * unisa/construct.py build_net builds from it, in the canonical text form
 * that tools/netdump.py prints from the Python side.  Standalone; it is not
 * part of the compiler.
 *
 *   construct [-d] weights/gold/prec.tsv
 *
 * -d adds the intermediate dumps (quotient groups, decision list with ranks,
 * chosen units) so a divergence can be located at its first step.
 *
 * Scope of this slice: single-head stages with at most 3 fields and at most
 * 62 quotient keys (every key set is one long bitmask).  That is prec and
 * reloc.  Cross-head sharing (T4) and the multi-head pick are not ported.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXF 3
#define MAXV 62
#define MAXH 4
#define MAXC 32
#define MAXOK 4096
#define MAXQ 62
#define MAXR 62
#define MAXU 128
#define MAXCAND 48
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
            if (nh >= MAXH) dieln(ln, "more heads than this constructor holds");
            if (np - 3 > MAXC) dieln(ln, "more classes than this constructor holds");
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
                if (nok > MAXOK) die("more keys than this constructor holds");
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
int qlab[MAXQ];
long qmask[MAXF][MAXV];   /* field, group -> keys having it */
long ALL;
long FULL[MAXF];
int crank[MAXC];          /* class -> its position in name order */

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

int popc(long m) {
    int c = 0;
    while (m) { m = m & (m - 1); c = c + 1; }
    return c;
}

void domain(void) {
    int i, v, g, j, t, k;
    for (i = 0; i < nf; i = i + 1) {
        ng[i] = 0;
        for (v = 0; v < nv[i]; v = v + 1) {
            grp[i][v] = -1;
            for (g = 0; g < ng[i]; g = g + 1)
                if (sameslice(i, gfirst[i][g], v)) { grp[i][v] = g; break; }
            if (grp[i][v] < 0) { gfirst[i][ng[i]] = v; grp[i][v] = ng[i]; ng[i] = ng[i] + 1; }
        }
    }
    nq = 1;
    for (i = 0; i < nf; i = i + 1) {
        nq = nq * ng[i];
        if (nq > MAXQ) die("more quotient keys than this constructor holds");
    }
    for (j = 0; j < nq; j = j + 1) {
        t = j;
        for (i = nf - 1; i >= 0; i = i - 1) { qk[j][i] = t % ng[i]; t = t / ng[i]; }
        k = 0;
        for (i = 0; i < nf; i = i + 1) k = k + gfirst[i][qk[j][i]] * ostride[i];
        qlab[j] = olab[k][0];
    }
    ALL = bit(nq) - 1;
    for (i = 0; i < nf; i = i + 1) {
        FULL[i] = bit(ng[i]) - 1;
        for (g = 0; g < ng[i]; g = g + 1) qmask[i][g] = 0;
    }
    for (j = 0; j < nq; j = j + 1)
        for (i = 0; i < nf; i = i + 1) qmask[i][qk[j][i]] = qmask[i][qk[j][i]] | bit(j);
    for (i = 0; i < ncl[0]; i = i + 1) {
        crank[i] = 0;
        for (j = 0; j < ncl[0]; j = j + 1) if (strcmp(cls[0][j], cls[0][i]) < 0) crank[i] = crank[i] + 1;
    }
}

/* ----------------------------------------------------------------- cubes -- */
/* a cube is long c[MAXF]: per field, a bitset of groups; FULL = don't care */

long cubemask(long *c) {
    long m = ALL, o;
    int i, g;
    for (i = 0; i < nf; i = i + 1) {
        if (c[i] == FULL[i]) continue;
        o = 0;
        for (g = 0; g < ng[i]; g = g + 1) if ((c[i] >> g) & 1) o = o | qmask[i][g];
        m = m & o;
    }
    return m;
}

int nlits(long *c) {
    int i, n = 0;
    for (i = 0; i < nf; i = i + 1) if (c[i] != FULL[i]) n = n + 1;
    return n;
}

/* Python compares sorted tuples of set members lexicographically. */
int cmpset(long a, long b) {
    long d = a ^ b, hi;
    int p = 0;
    if (d == 0) return 0;
    while (!((d >> p) & 1)) p = p + 1;
    if ((a >> p) & 1) {
        hi = b >> p;
        return hi ? -1 : 1;
    }
    hi = a >> p;
    return hi ? 1 : -1;
}

int cmpcube(long *a, long *b) {
    int i, r;
    for (i = 0; i < nf; i = i + 1) { r = cmpset(a[i], b[i]); if (r) return r; }
    return 0;
}

int samecube(long *a, long *b) {
    int i;
    for (i = 0; i < nf; i = i + 1) if (a[i] != b[i]) return 0;
    return 1;
}

void cpcube(long *d, long *s) {
    int i;
    for (i = 0; i < nf; i = i + 1) d[i] = s[i];
}

void prcube(long *c) {
    int i, g, first;
    for (i = 0; i < nf; i = i + 1) {
        printf(" ");
        if (c[i] == FULL[i]) { printf("*"); continue; }
        printf("{");
        first = 1;
        for (g = 0; g < ng[i]; g = g + 1)
            if ((c[i] >> g) & 1) { if (!first) printf(","); printf("%d", g); first = 0; }
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

long expand(int *seed, long badmask, int *ord, long *cube) {
    long cur[MAXF], others, nm;
    int t, i, j, v;
    for (i = 0; i < nf; i = i + 1) { cur[i] = qmask[i][seed[i]]; cube[i] = bit(seed[i]); }
    for (t = 0; t < nf; t = t + 1) {
        i = ord[t];
        others = ALL;
        for (j = 0; j < nf; j = j + 1) if (j != i) others = others & cur[j];
        if (!(others & badmask)) { cube[i] = FULL[i]; cur[i] = ALL; continue; }
        for (v = 0; v < ng[i]; v = v + 1) {
            if ((cube[i] >> v) & 1) continue;
            nm = cur[i] | qmask[i][v];
            if (!(others & nm & badmask)) { cube[i] = cube[i] | bit(v); cur[i] = nm; }
        }
    }
    return cubemask(cube);
}

int nr;
long rc[MAXR][MAXF];
int rl[MAXR];
int lv[MAXR];

void decision_list(void) {
    long labmask[MAXC], rem, bad, cm, bcm, newly;
    long cube[MAXF], bcube[MAXF];
    int j, o, L, bL, cnt, bcnt, nl, bnl, have, better, r, i, g;
    for (j = 0; j < ncl[0]; j = j + 1) labmask[j] = 0;
    for (j = 0; j < nq; j = j + 1) labmask[qlab[j]] = labmask[qlab[j]] | bit(j);
    rem = ALL;
    nr = 0;
    bL = 0; bcnt = 0; bnl = 0; bcm = 0;
    while (rem) {
        have = 0;
        for (j = 0; j < nq; j = j + 1) {
            if (!((rem >> j) & 1)) continue;
            L = qlab[j];
            bad = rem & ~labmask[L];
            for (o = 0; o < nord; o = o + 1) {
                cm = expand(qk[j], bad, ORD[o], cube);
                cnt = popc(cm & rem);
                nl = nlits(cube);
                better = 0;
                if (!have || cnt > bcnt) better = 1;
                else if (cnt == bcnt) {
                    if (nl < bnl) better = 1;
                    else if (nl == bnl) {
                        r = cmpcube(cube, bcube);
                        if (r < 0 || (r == 0 && strcmp(cls[0][L], cls[0][bL]) < 0)) better = 1;
                    }
                }
                if (better) { have = 1; bcnt = cnt; bnl = nl; bL = L; bcm = cm; cpcube(bcube, cube); }
            }
        }
        newly = bcm & rem;
        if (nr >= MAXR) die("more rules than this constructor holds");
        /* REDUCE: the supercube of the keys this rule claims */
        for (i = 0; i < nf; i = i + 1) {
            rc[nr][i] = 0;
            for (g = 0; g < ng[i]; g = g + 1) if (qmask[i][g] & newly) rc[nr][i] = rc[nr][i] | bit(g);
        }
        rl[nr] = bL;
        nr = nr + 1;
        rem = rem & ~newly;
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

void ranks(void) {
    long m, z[MAXC];
    int zs[MAXC];
    int i, j, t, it, mx, b, grew, w, cstar, c, a, bb, ng2;
    int grpi[MAXR];
    for (j = 0; j < nq; j = j + 1) nfire[j] = 0;
    for (i = 0; i < nr; i = i + 1) {
        m = cubemask(rc[i]);
        for (j = 0; j < nq; j = j + 1)
            if ((m >> j) & 1) { fire[j][nfire[j]] = i; nfire[j] = nfire[j] + 1; }
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
        for (j = 0; j < nq; j = j + 1) {
            if (nfire[j] < 2) continue;
            w = fire[j][0];
            cstar = rl[w];
            for (c = 0; c < ncl[0]; c = c + 1) { z[c] = 0; zs[c] = 0; }
            for (t = 0; t < nfire[j]; t = t + 1) {
                c = rl[fire[j][t]];
                z[c] = z[c] + bit(lv[fire[j][t]]);
                zs[c] = 1;
            }
            for (c = 0; c < ncl[0]; c = c + 1) {
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
   Candidate ci, unit u lives at row ci * MAXU + u: flat 2-D, because unisacc
   (at 8d4011c) crashes on a 3-D array indexed by variables. */
int ncand;
int cn[MAXCAND];
long ccube[MAXCAND * MAXU][MAXF];
long cw[MAXCAND * MAXU][MAXC];

int newunit(int ci, long *cube) {
    int u = cn[ci], c;
    if (u >= MAXU) die("more units than this constructor holds");
    cpcube(ccube[ci * MAXU + u], cube);
    for (c = 0; c < ncl[0]; c = c + 1) cw[ci * MAXU + u][c] = 0;
    cn[ci] = u + 1;
    return u;
}

void rep_from_dl(int ci) {
    int r, u, found;
    cn[ci] = 0;
    for (r = 0; r < nr; r = r + 1) {
        found = -1;
        for (u = 0; u < cn[ci]; u = u + 1) if (samecube(ccube[ci * MAXU + u], rc[r])) { found = u; break; }
        if (found < 0) found = newunit(ci, rc[r]);
        cw[ci * MAXU + found][rl[r]] = cw[ci * MAXU + found][rl[r]] + bit(lv[r]);
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

int headfail(int ci, int *badj) {
    long m[MAXU], z[MAXC], mx;
    int u, j, c, nb = 0, cntmx, arg;
    for (u = 0; u < cn[ci]; u = u + 1) m[u] = cubemask(ccube[ci * MAXU + u]);
    for (j = 0; j < nq; j = j + 1) {
        for (c = 0; c < ncl[0]; c = c + 1) z[c] = 0;
        for (u = 0; u < cn[ci]; u = u + 1)
            if ((m[u] >> j) & 1)
                for (c = 0; c < ncl[0]; c = c + 1) z[c] = z[c] + cw[ci * MAXU + u][c];
        mx = z[0]; arg = 0;
        for (c = 1; c < ncl[0]; c = c + 1) if (z[c] > mx) { mx = z[c]; arg = c; }
        cntmx = 0;
        for (c = 0; c < ncl[0]; c = c + 1) if (z[c] == mx) cntmx = cntmx + 1;
        if (cntmx != 1 || arg != qlab[j]) { badj[nb] = j; nb = nb + 1; }
    }
    return nb;
}

/* class set -> the same set over name ranks, so cmpset orders by sorted names */
long rankset(long s) {
    long r = 0;
    int c;
    for (c = 0; c < ncl[0]; c = c + 1) if ((s >> c) & 1) r = r | bit(crank[c]);
    return r;
}

int gcode[MAXQ];
long gcls[MAXQ];
long bcls[MAXQ];
int bn[MAXQ];
int bmem[MAXQ][MAXQ];
int border[MAXQ];

int codedigit(int code, int pi, int pp, int fi) {
    int t = psz[pi][pp] - 1;
    while (t > fi) { code = code / ng[pf[pi * 4 + pp][t]]; t = t - 1; }
    return code % ng[pf[pi * 4 + pp][fi]];
}

/* T5; returns 1 and fills candidate ci, or 0 for None */
int rep_factored(int ci, int pi, int merge, int k) {
    long covered = 0, resid, sets[MAXF], wmask, pr;
    int i, u, pp, j, code, fi, f, x, ngv, nbk, b, t, a, tmp, cap, it, nb, prod, cnt, s, e;
    int badj[MAXQ];
    cn[ci] = 0;
    if (k) {
        if (k > nr) return 0;
        for (i = 0; i < k; i = i + 1) {
            u = newunit(ci, rc[i]);
            cw[ci * MAXU + u][rl[i]] = bit(k + 1 - i);
            covered = covered | cubemask(rc[i]);
        }
    }
    resid = ALL & ~covered;
    if (resid == 0) return 0;
    for (pp = 0; pp < npart[pi]; pp = pp + 1) {
        ngv = 0;
        for (j = 0; j < nq; j = j + 1) {
            if (!((resid >> j) & 1)) continue;
            code = 0;
            for (fi = 0; fi < psz[pi][pp]; fi = fi + 1) {
                f = pf[pi * 4 + pp][fi];
                code = code * ng[f] + qk[j][f];
            }
            x = -1;
            for (t = 0; t < ngv; t = t + 1) if (gcode[t] == code) { x = t; break; }
            if (x < 0) { x = ngv; gcode[x] = code; gcls[x] = 0; ngv = ngv + 1; }
            gcls[x] = gcls[x] | bit(qlab[j]);
        }
        if (ngv == 0) return 0;
        nbk = 0;
        for (t = 0; t < ngv; t = t + 1) {
            x = -1;
            for (b = 0; b < nbk; b = b + 1) if (bcls[b] == gcls[t]) { x = b; break; }
            if (x < 0) { x = nbk; bcls[x] = gcls[t]; bn[x] = 0; nbk = nbk + 1; }
            bmem[x][bn[x]] = gcode[t];
            bn[x] = bn[x] + 1;
        }
        /* buckets sorted by the sorted names of their classes (stable) */
        for (b = 0; b < nbk; b = b + 1) border[b] = b;
        for (a = 1; a < nbk; a = a + 1) {
            t = a;
            while (t > 0 && cmpset(rankset(bcls[border[t - 1]]), rankset(bcls[border[t]])) > 0) {
                tmp = border[t]; border[t] = border[t - 1]; border[t - 1] = tmp;
                t = t - 1;
            }
        }
        for (a = 0; a < nbk; a = a + 1) {
            b = border[a];
            /* gvs = sorted(bucket): codes are mixed-radix, first field most significant */
            for (s = 1; s < bn[b]; s = s + 1) {
                t = s;
                while (t > 0 && bmem[b][t - 1] > bmem[b][t]) {
                    tmp = bmem[b][t]; bmem[b][t] = bmem[b][t - 1]; bmem[b][t - 1] = tmp;
                    t = t - 1;
                }
            }
            prod = 0;
            if (merge && bn[b] > 1) {
                prod = 1;
                for (fi = 0; fi < psz[pi][pp]; fi = fi + 1) {
                    pr = 0;
                    for (s = 0; s < bn[b]; s = s + 1) pr = pr | bit(codedigit(bmem[b][s], pi, pp, fi));
                    prod = prod * popc(pr);
                }
                prod = (prod == bn[b]);
            }
            s = 0;
            while (s < bn[b]) {
                e = prod ? bn[b] : s + 1;
                for (i = 0; i < nf; i = i + 1) sets[i] = FULL[i];
                for (fi = 0; fi < psz[pi][pp]; fi = fi + 1) {
                    f = pf[pi * 4 + pp][fi];
                    sets[f] = 0;
                    for (t = s; t < e; t = t + 1) sets[f] = sets[f] | bit(codedigit(bmem[b][t], pi, pp, fi));
                }
                u = newunit(ci, sets);
                wmask = bcls[b];
                for (i = 0; i < ncl[0]; i = i + 1) if ((wmask >> i) & 1) cw[ci * MAXU + u][i] = 1;
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
            for (i = 0; i < nf; i = i + 1) sets[i] = bit(qk[j][i]);
            cnt = 0;
            for (u = 0; u < cn[ci]; u = u + 1) if (samecube(ccube[ci * MAXU + u], sets)) cnt = 1;
            if (cnt) return 0;
            u = newunit(ci, sets);
            cw[ci * MAXU + u][qlab[j]] = bit(k + 2);
        }
    }
    return 0;
}

int total_units(int ci) {
    int u, v, n = 0, dup;
    for (u = 0; u < cn[ci]; u = u + 1) {
        dup = 0;
        for (v = 0; v < u; v = v + 1) if (samecube(ccube[ci * MAXU + u], ccube[ci * MAXU + v])) { dup = 1; break; }
        if (!dup) n = n + 1;
    }
    return n;
}

int pick(int chosen) {
    int pass, bi, bv, i, v;
    for (pass = 0; pass < 5; pass = pass + 1) {
        bi = chosen; bv = total_units(chosen);
        for (i = 0; i < ncand; i = i + 1) {
            if (i == chosen) continue;
            v = total_units(i);
            if (v < bv || (v == bv && cn[i] < cn[bi])) { bi = i; bv = v; }
        }
        if (bi == chosen) break;
        chosen = bi;
    }
    return chosen;
}

/* ------------------------------------------------------------- the net ---- */
int H;
long ucube[MAXU][MAXF];
int offs[MAXF];
int h0;
int b1[MAXU];
long W2[MAXU][MAXC];

int w1(int i, int v, int j) {
    if (ucube[j][i] == FULL[i]) return 0;
    return (int)((ucube[j][i] >> grp[i][v]) & 1);
}

int main(int argc, char **argv) {
    int ai, k, j, pi, mg, i, u, c, s0, s1, a, b, chosen, found, v, arg, cntmx, bad, lim;
    long z[MAXC], mx, mn, mxlog, hv;
    char *path = 0;
    dbg = 0;
    for (ai = 1; ai < argc; ai = ai + 1) {
        if (streq(argv[ai], "-d")) dbg = 1;
        else path = argv[ai];
    }
    if (!path) { printf("usage: construct [-d] <stage>.tsv\n"); return 2; }
    load(path);
    if (nh != 1) die("multi-head stages are not in this slice");
    domain();
    orders();
    decision_list();
    ranks();
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
        for (j = 0; j < nr; j = j + 1) {
            printf("rule %d:", j);
            prcube(rc[j]);
            printf(" => %s rank %d\n", cls[0][rl[j]], lv[j]);
        }
    }
    ncand = 1;
    rep_from_dl(0);
    partitions();
    lim = nr < 9 ? nr : 9;
    for (pi = 0; pi < nparts; pi = pi + 1)
        for (mg = 0; mg < 2; mg = mg + 1)
            for (k = 0; k < lim; k = k + 1) {
                if (ncand >= MAXCAND) die("more candidates than this constructor holds");
                if (rep_factored(ncand, pi, mg, k) && cn[ncand] < cn[0]) ncand = ncand + 1;
            }
    /* starts: all dlist, then the shortest candidate; min by (units, length) */
    s1 = 0;
    for (i = 1; i < ncand; i = i + 1) if (cn[i] < cn[s1]) s1 = i;
    a = pick(0);
    b = pick(s1);
    chosen = a;
    if (total_units(b) < total_units(a) || (total_units(b) == total_units(a) && cn[b] < cn[a])) chosen = b;
    s0 = chosen;
    H = 0;
    for (u = 0; u < cn[s0]; u = u + 1) {
        found = -1;
        for (j = 0; j < H; j = j + 1) if (samecube(ucube[j], ccube[s0 * MAXU + u])) { found = j; break; }
        if (found < 0) {
            found = H;
            cpcube(ucube[H], ccube[s0 * MAXU + u]);
            for (c = 0; c < ncl[0]; c = c + 1) W2[H][c] = 0;
            b1[H] = -(nlits(ucube[H]) - 1);
            H = H + 1;
        }
        for (c = 0; c < ncl[0]; c = c + 1) if (cw[s0 * MAXU + u][c]) W2[found][c] = cw[s0 * MAXU + u][c];
    }
    h0 = 0;
    for (i = 0; i < nf; i = i + 1) { offs[i] = h0; h0 = h0 + nv[i]; }
    if (dbg) {
        printf("kind %s %s\n", hname[0], s0 ? "factored" : "dlist");
        for (j = 0; j < H; j = j + 1) { printf("unit %d:", j); prcube(ucube[j]); printf("\n"); }
    }
    /* verify over the FULL original domain; measure maxlogit as verify_int does */
    mxlog = 0;
    bad = 0;
    for (k = 0; k < nok; k = k + 1) {
        for (c = 0; c < ncl[0]; c = c + 1) z[c] = 0;
        for (j = 0; j < H; j = j + 1) {
            hv = b1[j];
            for (i = 0; i < nf; i = i + 1) hv = hv + w1(i, odigit(k, i), j);
            if (hv > 0) for (c = 0; c < ncl[0]; c = c + 1) z[c] = z[c] + hv * W2[j][c];
        }
        mx = z[0]; mn = z[0]; arg = 0;
        for (c = 1; c < ncl[0]; c = c + 1) {
            if (z[c] > mx) { mx = z[c]; arg = c; }
            if (z[c] < mn) mn = z[c];
        }
        if ((mx < 0 ? -mx : mx) > mxlog) mxlog = mx < 0 ? -mx : mx;
        if (mn < -mxlog) mxlog = -mn;
        cntmx = 0;
        for (c = 0; c < ncl[0]; c = c + 1) if (z[c] == mx) cntmx = cntmx + 1;
        if (cntmx != 1 || arg != olab[k][0]) bad = bad + 1;
    }
    if (bad) { printf("construct: %s: construction not exact (%d wrong)\n", path, bad); return 1; }
    /* the canonical form: intnet.to_dict's fields, in its order */
    printf("stage %s\n", sname);
    printf("H %d\n", H);
    printf("offs");
    for (i = 0; i < nf; i = i + 1) printf(" %d", offs[i]);
    printf("\nb1");
    for (j = 0; j < H; j = j + 1) printf(" %d", b1[j]);
    printf("\nhead %s", hname[0]);
    for (c = 0; c < ncl[0]; c = c + 1) printf(" %s", cls[0][c]);
    printf("\nfeeds %d\n", h0);
    for (i = 0; i < nf; i = i + 1)
        for (v = 0; v < nv[i]; v = v + 1) {
            printf("f %d:", offs[i] + v);
            for (j = 0; j < H; j = j + 1) if (w1(i, v, j)) printf(" %d", j);
            printf("\n");
        }
    printf("w2 %s\n", hname[0]);
    for (j = 0; j < H; j = j + 1) {
        printf("r %d:", j);
        for (c = 0; c < ncl[0]; c = c + 1) if (W2[j][c]) printf(" %d,%ld", c, W2[j][c]);
        printf("\n");
    }
    printf("maxlogit %ld\n", mxlog);
    printf("exact %d\n", nok);
    return 0;
}
