/* exec.c -- generic delta executor, option A (research/delta-framework.md).
 *
 * The machine: C = (q, r, i, s, W, o, h) over a fixed input x.
 * One step: obs = (q, r, b, t), b = x[i] or EOF(256), t = stack top or
 * BOT(NG); (q', acts) = delta(obs); q := q'; r := 0; run acts in order,
 * stopping at the first halt.  Nothing here knows any language or grammar:
 * all of that is in the delta table, loaded from a file at run time.
 *
 * Table format and action codes: exec/README.md.
 *
 *   exec delta.tbl [input]      run once (input from file, else stdin)
 *   exec -r N delta.tbl input   run N times (timing), output once
 *   exec -dump delta.tbl        print the dense delta on its whole domain
 *
 * Accept: o on stdout, exit 0.  Reject(k): "reject k at i" on stderr,
 * nothing on stdout, exit 1.  Bad table / usage: exit 2.
 */
#include <stdio.h>
#include <stdlib.h>

typedef unsigned int u32;

/* All storage is static and fixed: no allocator.  Running out of any of it
 * is exit 3 ("exhausted"), never a reject -- the abstract machine's stack,
 * output and W are unbounded, this executor's are not. */
#define MAXD (1 << 18)          /* dense delta entries: |Q|*|R|*257*(|G|+1) */
#define MAXP (1 << 16)          /* action pool ints */
#define MAXX (1 << 18)          /* input bytes, table bytes */
#define MAXS (1 << 16)          /* stack depth */
#define MAXO (1 << 18)          /* output bytes */
#define MAXW (1 << 12)          /* working-store keys (power of two) */

static int NQ, NR, NG, Q0;
static int dense[MAXD];         /* obs -> offset into pool */
static int pool[MAXP]; static int npool;
/* arity of each action code, 0..14 */
static const int arity[15] = {0,1,0,1,0,1,0,1,2,1,4,2,2,2,1};

static unsigned char x[MAXX]; static int nx;
static int stk[MAXS]; static int sp;
static unsigned char o[MAXO]; static int no;
static u32 wk[MAXW], wv[MAXW]; static unsigned char wu[MAXW]; static int wn;

static void die(const char *m) { fputs(m, stderr); fputs("\n", stderr); exit(2); }
static void full(void) { fputs("exhausted\n", stderr); exit(3); }

static int slurp(FILE *f, unsigned char *b) {
    int n = 0, k;
    while ((k = fread(b + n, 1, MAXX - n, f)) > 0) n += k;
    if (n == MAXX) full();
    return n;
}

/* ---- integer tokens of the table file ---- */
static unsigned char tb[MAXX]; static int tn, tp;
static int tok(void) {
    int v = 0, neg = 0, any = 0;
    for (;;) {
        if (tp >= tn) die("table: truncated");
        if (tb[tp] == '#') { while (tp < tn && tb[tp] != '\n') tp++; }
        else if (tb[tp] == ' ' || tb[tp] == '\n' || tb[tp] == '\t' || tb[tp] == '\r') tp++;
        else break;
    }
    if (tb[tp] == '-') { neg = 1; tp++; }
    while (tp < tn && tb[tp] >= '0' && tb[tp] <= '9') { v = v * 10 + (tb[tp] - '0'); tp++; any = 1; }
    if (!any) die("table: bad token");
    return neg ? -v : v;
}
static void put(int v) {
    if (npool == MAXP) full();
    pool[npool++] = v;
}
/* reads "q' n a..." into the pool; returns its offset */
static int entry(void) {
    int at = npool, n, k, j, a;
    put(tok()); if (pool[at] < 0 || pool[at] >= NQ) die("table: bad q'");
    n = tok(); put(n);
    for (k = 0; k < n; k++) {
        a = tok(); if (a < 0 || a > 14) die("table: bad action");
        put(a);
        for (j = 0; j < arity[a]; j++) put(tok());
        if (a == 3 && (pool[npool - 1] < 0 || pool[npool - 1] >= NG)) die("table: bad push");
        if (a == 7 && (pool[npool - 1] < 0 || pool[npool - 1] >= NR)) die("table: bad r");
    }
    return at;
}
static int lo(int w, int m) { return w < 0 ? 0 : w; }
static int hi(int w, int m) { return w < 0 ? m : w + 1; }

static void load(const char *path) {
    FILE *f = fopen(path, "rb"); int d, nd, rows, k, q, r, b, t, e, a[4], m[4];
    if (!f) die("table: cannot open");
    tn = slurp(f, tb); fclose(f); tp = 0;
    NQ = tok(); NR = tok(); NG = tok(); Q0 = tok();
    if (NQ < 1 || NR < 3 || NG < 0 || Q0 < 0 || Q0 >= NQ) die("table: bad header");
    nd = NQ * NR * 257 * (NG + 1);
    if (nd > MAXD) full();
    d = entry();
    for (k = 0; k < nd; k++) dense[k] = d;
    rows = tok();
    m[0] = NQ; m[1] = NR; m[2] = 257; m[3] = NG + 1;
    while (rows-- > 0) {
        for (k = 0; k < 4; k++) { a[k] = tok(); if (a[k] < -1 || a[k] >= m[k]) die("table: bad obs"); }
        e = entry();
        for (q = lo(a[0], 0); q < hi(a[0], m[0]); q++)
        for (r = lo(a[1], 0); r < hi(a[1], m[1]); r++)
        for (b = lo(a[2], 0); b < hi(a[2], m[2]); b++)
        for (t = lo(a[3], 0); t < hi(a[3], m[3]); t++)
            dense[((q * NR + r) * 257 + b) * (NG + 1) + t] = e;
    }
}

/* ---- working store: dictionary u32 -> u32, missing keys read 0 ---- */
static int wslot(u32 key, int make) {
    int h = (int)((key * 2654435761u) & (MAXW - 1));
    while (wu[h]) { if (wk[h] == key) return h; h = (h + 1) & (MAXW - 1); }
    if (!make) return -1;
    if (wn * 2 >= MAXW) full();
    wu[h] = 1; wk[h] = key; wv[h] = 0; wn++;
    return h;
}
static u32 wget(u32 k) { int h = wslot(k, 0); return h < 0 ? 0 : wv[h]; }
static void wset(u32 k, u32 v) { wv[wslot(k, 1)] = v; }

static void emit(int c) {
    if (no == MAXO) full();
    o[no++] = (unsigned char)c;
}
static u32 alu(int op, u32 a, u32 b) {
    switch (op) {
    case 0: return a + b;             case 1: return a - b;
    case 2: return a * b;             case 3: return b ? a / b : 0;
    case 4: return b ? a % b : a;     case 5: return a & b;
    case 6: return a | b;             case 7: return a ^ b;
    case 8: return a << (b & 31);     case 9: return a >> (b & 31);
    }
    return 0;
}

/* runs the machine once; returns -1 for accept, else k of reject(k) */
static int run(int *at) {
    int q = Q0, r = 0, i = 0, h = -2, b, t, n, a, *p;
    sp = 0; no = 0;
    if (wn) { for (b = 0; b < MAXW; b++) wu[b] = 0; wn = 0; }
    while (h == -2) {
        b = i < nx ? x[i] : 256;
        t = sp ? stk[sp - 1] : NG;
        p = pool + dense[((q * NR + r) * 257 + b) * (NG + 1) + t];
        q = p[0]; n = p[1]; p += 2; r = 0;
        while (n-- > 0 && h == -2) {
            a = *p++;
            switch (a) {
            case 0: h = -1; break;                                   /* ACC */
            case 1: h = p[0]; break;                                 /* REJ k */
            case 2: if (i < nx) i++; break;                          /* ADV */
            case 3: if (sp == MAXS) full();
                    stk[sp++] = p[0]; break;                         /* PUSH g */
            case 4: if (sp) sp--; else h = 254; break;               /* POP */
            case 5: emit(p[0]); break;                               /* EMIT c */
            case 6: if (i < nx) emit(x[i]); else h = 253; break;     /* COPY */
            case 7: r = p[0]; break;                                 /* SETR v */
            case 8: wset((u32)p[0], (u32)p[1]); break;               /* LDI k v */
            case 9: wset((u32)p[0], i < nx ? x[i] : 256); break;     /* BYTE k */
            case 10: wset((u32)p[1], alu(p[0], wget((u32)p[2]), wget((u32)p[3]))); break; /* ALU */
            case 11: { u32 u = wget((u32)p[0]), v = wget((u32)p[1]);
                       r = u < v ? 0 : u == v ? 1 : 2; } break;      /* CMP a b */
            case 12: wset((u32)p[0], wget(wget((u32)p[1]))); break;  /* LOAD d k */
            case 13: wset(wget((u32)p[0]), wget((u32)p[1])); break;  /* STORE k v */
            case 14: emit((int)(wget((u32)p[0]) & 255)); break;      /* OUTW k */
            }
            p += arity[a];
        }
    }
    *at = i;
    return h;
}

int main(int argc, char **argv) {
    int reps = 1, k, at, q, r, b, t, *p, n, j;
    FILE *f;
    if (argc >= 3 && argv[1][0] == '-' && argv[1][1] == 'd') {
        load(argv[2]);
        for (q = 0; q < NQ; q++) for (r = 0; r < NR; r++) for (b = 0; b < 257; b++) for (t = 0; t <= NG; t++) {
            p = pool + dense[((q * NR + r) * 257 + b) * (NG + 1) + t];
            printf("%d %d %d %d %d %d", q, r, b, t, p[0], p[1]);
            n = p[1]; p += 2;
            while (n-- > 0) { printf(" %d", *p); for (j = 0; j < arity[*p]; j++) printf(" %d", p[1 + j]); p += 1 + arity[*p]; }
            printf("\n");
        }
        return 0;
    }
    if (argc >= 4 && argv[1][0] == '-' && argv[1][1] == 'r') { reps = atoi(argv[2]); argv += 2; argc -= 2; }
    if (argc < 2) die("usage: exec [-r N] delta.tbl [input] | exec -dump delta.tbl");
    load(argv[1]);
    if (argc >= 3) { f = fopen(argv[2], "rb"); if (!f) die("cannot open input"); nx = slurp(f, x); fclose(f); }
    else nx = slurp(stdin, x);
    k = -1;
    while (reps-- > 0) k = run(&at);
    if (k >= 0) { fprintf(stderr, "reject %d at %d\n", k, at); return 1; }
    fwrite(o, 1, no, stdout);
    return 0;
}
