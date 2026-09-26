/* exec/c/run.c -- the generic delta executor in C: the machine of
   exec/pp/sim.py, action for action, on the integer table exec/c/tbl.py
   writes, or the integer threshold network exec/c/net.py constructs.
   It knows no language; transition decisions belong to the loaded model.

       run TABLE INPUT [SRCPATH] [INCLUDE_DIR]

   INCLUDE_DIR (where the bundled headers are) should be ABSOLUTE: a relative
   one read from another CWD is ENOENT, i.e. "absent", not an error.

   exit 0 accept (o on stdout); 1 reject (the reason, then e, on stderr);
   2 a bad table; 3 out of steps (UNISA_MAXSTEPS, default 2e8). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

typedef int64_t I;
enum { ADV, MARK, JUMP, LDI, COPYW, ALU, ALUI, CMP, CMPI, RLD, LDX, STX, OUT, OUTW, COPY, COPYT,
       SPAN, SPANT, SPAN2, OLAST, ODROP, OLEN, OCUT, ORES, OFILL, OCLR, OSEL, SETOT, XATTR, PUSH, POP,
       INTERN, BLOBSAVE, INPUSH, INPUSHX, INPUSHXE, INPOP, SBCLR, SBOUT, SBSPAN, SBBLOB, SBINTERN,
       SBSAVE, SBFIND, BLEN, BYTE, XLEN, DIVMOD10, SWAP, ACCEPT, REJECT, A64, A64I, C64, C64U, INC, NOP_ };
static const int ARITY[NOP_] = {0,1,1,2,2,4,4,2,2,1,3,3,1,1,0,0,1,1,2,0,0,1,2,2,3,0,1,1,1,1,0,3,3,1,1,2,0,
                                0,1,2,1,1,1,1,2,1,1,1,0,0,1,4,4,2,2,1};

static void die(const char *m) { fprintf(stderr, "run: %s\n", m); exit(2); }
static void *xrealloc(void *p, size_t n) { p = realloc(p, n ? n : 1); if (!p) die("out of memory"); return p; }

/* File operations are OS adaptation, not compiler actions. The native
   POSIX gates return -errno; the host wrapper normalises libc to that form.
   Windows native gates currently lack errno classification: an open failure
   remains an IO error, never silently accepted as an absent optional file. */
#ifdef __UNISA__
static long io_open(const char *path) {
#ifdef _WIN32
    return __open((char *)path, 0x80000000, 3);
#else
    return __open((char *)path, 0, 0);
#endif
}
static long io_read(long fd, void *p, long n) { return __read(fd, p, n); }
static long io_close(long fd) { return __close(fd); }
#else
#include <unistd.h>
#include <fcntl.h>
static long io_open(const char *path) { int fd = open(path, O_RDONLY); return fd < 0 ? -errno : fd; }
static long io_read(long fd, void *p, long n) { long r = read((int)fd, p, (size_t)n); return r < 0 ? -errno : r; }
static long io_close(long fd) { return close((int)fd); }
#endif
static unsigned char *readfile(const char *path, int *len, int optional) {
    long fd = io_open(path);
    if (fd < 0) {
        if (optional && fd == -ENOENT) return 0;
        fprintf(stderr, "run: cannot open %s\n", path); exit(2);
    }
    int n = 0, cap = 65536; unsigned char *b = xrealloc(0, cap);
    for (;;) {
        if (n == cap) { if (cap > INT32_MAX/2) die("file too large"); cap *= 2; b = xrealloc(b, cap); }
        long r = io_read(fd, b+n, cap-n);
        if (r < 0) { fprintf(stderr, "run: cannot read %s\n", path); exit(2); }
        if (!r) break;
        if (r > cap-n) die("invalid read length");
        n += (int)r;
    }
    if (io_close(fd) < 0) die("close failed");
    *len = n; return b;
}

/* Decimal model reader: no scanf dependency, and 64-bit arguments are
   checked before multiply/add (including the INT64_MIN magnitude). */
static unsigned char *LB; static int LP, LN;
static void lskip(void) { while (LP < LN && LB[LP] <= 32) LP++; }
static int lchar(void) {
    lskip(); if (LP == LN) die("truncated model");
    int c = LB[LP++]; if (LP < LN && LB[LP] > 32) die("bad model tag"); return c;
}
static void ltag(int c) { if (lchar() != c) die("bad model tag"); }
static I lnum(void) {
    lskip(); int neg = 0, count = 0;
    if (LP < LN && LB[LP] == '-') { neg = 1; LP++; }
    uint64_t v = 0, lim = neg ? 9223372036854775808ull : 9223372036854775807ull;
    while (LP < LN && LB[LP] > 32) {
        int d = LB[LP++] - '0'; if (d < 0 || d > 9) die("bad model number");
        if (v > (lim - d)/10) die("model number overflow");
        v = v*10+d; count++;
    }
    if (!count) die("missing model number");
    return neg ? (I)(0-v) : (I)v;
}
static int lint(void) { I n = lnum(); if (n < INT32_MIN || n > INT32_MAX) die("model index overflow"); return (int)n; }

/* ---- the table ---- */
static int NS, NQ, NRG, NSTR, START;
static char **STR; static int *STRL;
static int *QOFF, *QLEN; static I *QA; static int NQA;          /* actions, flattened */
static int ISNET, TOPMAX;
static int *NLO, *NHI, *BN, *BQ;
static int *SMODE; static int **ROWK, **ROWN, **ROWQ; static int *ROWC;

static int hexv(int c) { if (c >= '0' && c <= '9') return c-'0'; if (c >= 'a' && c <= 'f') return c-'a'+10; die("bad hex string"); return 0; }

static void load(const char *path) {
    LB = readfile(path, &LN, 0); LP = 0;
    int kind = lchar(); NS=lint(); NQ=lint(); NRG=lint(); NSTR=lint(); START=lint();
    if ((kind != 'T' && kind != 'N') || NS <= 0 || NQ <= 0 || NRG < 0 || NSTR < 0 || START < 0 || START >= NS) die("bad header");
    ISNET = kind == 'N'; TOPMAX = NS - 1;
    if (ISNET) { TOPMAX=lint(); if (TOPMAX < NS-1 || TOPMAX == INT32_MAX) die("bad network domain"); }
    STR = xrealloc(0, sizeof *STR * (NSTR + 1)); STRL = xrealloc(0, sizeof *STRL * (NSTR + 1));
    for (int k = 0; k < NSTR; k++) {
        ltag('S'); lskip(); int begin = LP;
        while (LP < LN && LB[LP] > 32) LP++;
        int size = LP-begin; unsigned char *buf = LB+begin;
        int n = size == 1 && buf[0] == '-' ? 0 : size/2;
        if (size == 0 || ((size & 1) && !(size == 1 && buf[0] == '-'))) die("bad string");
        STR[k] = xrealloc(0, n + 1); STRL[k] = n;
        for (int j = 0; j < n; j++) STR[k][j] = (char)(hexv(buf[2 * j]) * 16 + hexv(buf[2 * j + 1]));
        STR[k][n] = 0;
    }
    QOFF = xrealloc(0, sizeof(int) * NQ); QLEN = xrealloc(0, sizeof(int) * NQ);
    int cap = 1 << 16; QA = xrealloc(0, sizeof(I) * cap); NQA = 0;
    for (int k = 0; k < NQ; k++) {
        ltag('Q'); int n = lint(); if (n < 0) die("bad sequence");
        QOFF[k] = NQA; QLEN[k] = n;
        for (int a = 0; a < n; a++) {
            int op = lint(); if (op < 0 || op >= NOP_) die("bad opcode");
            if (NQA + 6 > cap) { cap *= 2; QA = xrealloc(QA, sizeof(I) * cap); }
            QA[NQA++] = op;
            for (int j = 0; j < ARITY[op]; j++) QA[NQA++] = lnum();
        }
    }
    SMODE = xrealloc(0, sizeof(int) * NS); ROWC = xrealloc(0, sizeof(int) * NS);
    ROWK = xrealloc(0, sizeof(int *) * NS); ROWN = xrealloc(0, sizeof(int *) * NS); ROWQ = xrealloc(0, sizeof(int *) * NS);
    NLO = xrealloc(0, sizeof(int) * NS); NHI = xrealloc(0, sizeof(int) * NS);
    BN = xrealloc(0, sizeof(int) * NS); BQ = xrealloc(0, sizeof(int) * NS);
    for (int s = 0; s < NS; s++) {
        if (ISNET) {
            ltag('H'); int m=lint(), lo=lint(), hi=lint(), n=lint(), bn=lint(), bq=lint();
            if (m < 0 || m > 2 ||
                lo != (m == 1 ? -1 : 0) || hi != (m == 1 ? TOPMAX : 256) || n < 0 || (I)n > (I)hi - lo) die("bad network bank");
            SMODE[s] = m; NLO[s] = lo; NHI[s] = hi; ROWC[s] = n; BN[s] = bn; BQ[s] = bq;
            ROWK[s] = xrealloc(0, sizeof(int) * n); ROWN[s] = xrealloc(0, sizeof(int) * n); ROWQ[s] = xrealloc(0, sizeof(int) * n);
            int prev = lo;
            for (int j = 0; j < n; j++) {
                ROWK[s][j]=lint(); ROWN[s][j]=lint(); ROWQ[s][j]=lint();
                if (ROWK[s][j] <= prev || ROWK[s][j] > hi) die("bad network unit");
                prev = ROWK[s][j];
            }
            continue;
        }
        ltag('R'); int m=lint(), n=lint(), dn=lint(), dq=lint();
        if (m < 0 || m > 2 || n < 0) die("bad state");
        SMODE[s] = m; ROWC[s] = n;
        if (m == 1) {                                   /* keyed by the stack top: a short list */
            ROWK[s] = xrealloc(0, sizeof(int) * n); ROWN[s] = xrealloc(0, sizeof(int) * n); ROWQ[s] = xrealloc(0, sizeof(int) * n);
            for (int j = 0; j < n; j++) { ROWK[s][j]=lint(); ROWN[s][j]=lint(); ROWQ[s][j]=lint(); }
        } else {                                        /* keyed 0..256: direct */
            ROWN[s] = xrealloc(0, sizeof(int) * 257); ROWQ[s] = xrealloc(0, sizeof(int) * 257); ROWK[s] = 0;
            for (int j = 0; j < 257; j++) { ROWN[s][j] = dn; ROWQ[s][j] = dq; }
            for (int j = 0; j < n; j++) { int k=lint(), nx=lint(), q=lint(); if (k < 0 || k > 256 || nx < -1 || nx >= NS) die("bad row"); ROWN[s][k] = nx; ROWQ[s][k] = q; }
        }
    }
    lskip(); if (LP != LN) die("trailing model data"); free(LB); LB=0;
    if (!ISNET) {
        for (int s = 0; s < NS; s++) if (SMODE[s] == 1)
            for (int j = 0; j < ROWC[s]; j++) if (ROWK[s][j] > TOPMAX) TOPMAX = ROWK[s][j];
        for (int i = 0; i < NQA; i += 1 + ARITY[QA[i]])
            if (QA[i] == PUSH && QA[i+1] > TOPMAX) TOPMAX = (int)QA[i+1];
    }
}

/* The selected bank is sparse evaluation of a one-hot state-conditioned net.
   Each hidden activation is H(key-threshold); output weights are signed.
   No answer table is materialised. int weights/count bound sums within int64. */
static void transition(int q, int key, int *nx, int *sq) {
    *nx = -1; *sq = 0;
    if (ISNET) {
        if (key < NLO[q] || key > NHI[q]) die("observation outside network domain");
        I n = BN[q], s = BQ[q];
        for (int j = 0; j < ROWC[q]; j++) {
            int h = key >= ROWK[q][j];
            n += (I)ROWN[q][j] * h; s += (I)ROWQ[q][j] * h;
        }
        if (n < -1 || n >= NS || s < 0 || s >= NQ) die("invalid network output");
        *nx = (int)n; *sq = (int)s;
    } else if (SMODE[q] == 1) {
        for (int j = 0; j < ROWC[q]; j++) if (ROWK[q][j] == key) { *nx = ROWN[q][j]; *sq = ROWQ[q][j]; break; }
    } else { *nx = ROWN[q][key]; *sq = ROWQ[q][key]; }
}

static int checknet(const char *table, const char *net) {
    load(table); if (ISNET) die("expected reference table");
    int ns = NS, nq = NQ, nr = NRG, nstr = NSTR, start = START, top = TOPMAX, nqa = NQA;
    int *modes = SMODE, *qlen = QLEN, *sl = STRL; I *qa = QA; char **str = STR;
    size_t total = 0;
    for (int q = 0; q < ns; q++) total += modes[q] == 1 ? (size_t)top + 2 : 257;
    int *en = xrealloc(0, total * sizeof(int)), *es = xrealloc(0, total * sizeof(int));
    size_t at = 0;
    for (int q = 0; q < ns; q++) for (int k = modes[q] == 1 ? -1 : 0, hi = modes[q] == 1 ? top : 256; k <= hi; k++)
        transition(q, k, &en[at], &es[at]), at++;
    load(net);
    if (!ISNET || NS != ns || NQ != nq || NRG != nr || NSTR != nstr || START != start || TOPMAX != top || NQA != nqa ||
        memcmp(qa, QA, sizeof(I)*nqa) || memcmp(qlen, QLEN, sizeof(int)*nq)) die("network changed action declarations");
    for (int i = 0; i < nstr; i++) if (sl[i] != STRL[i] || memcmp(str[i], STR[i], sl[i])) die("network changed strings");
    at = 0;
    for (int q = 0; q < ns; q++) {
        if (modes[q] != SMODE[q]) die("network changed observation mode");
        for (int k = NLO[q]; k <= NHI[q]; k++) {
            int n, s; transition(q, k, &n, &s);
            if (n != en[at] || s != es[at]) { fprintf(stderr, "network differs: state %d key %d\n", q, k); return 1; }
            at++;
        }
    }
    printf("network = table: %zu observations, %d states; actions/strings identical\n", at, ns);
    return 0;
}

/* ---- byte buffers ---- */
typedef struct { unsigned char *b; I *at; int n, cap; } Buf;
static void bput(Buf *o, int c, I at) {
    if (o->n >= o->cap) { o->cap = o->cap ? o->cap * 2 : 256; o->b = xrealloc(o->b, o->cap); o->at = xrealloc(o->at, sizeof(I) * o->cap); }
    o->b[o->n] = (unsigned char)c; o->at[o->n] = at; o->n++;
}

/* ---- W: registers and the indexed memory (a hash map) ---- */
static I *R;
static I *MK, *MV; static char *MU; static size_t MCAP, MN;
static size_t mslot(I k) { uint64_t h = (uint64_t)k * 0x9E3779B97F4A7C15ull; return (size_t)(h >> 20) & (MCAP - 1); }
static I mget(I k) { if (!MCAP) return 0; size_t s = mslot(k); while (MU[s]) { if (MK[s] == k) return MV[s]; s = (s + 1) & (MCAP - 1); } return 0; }
static void mset(I k, I v) {
    if ((MN + 1) * 2 > MCAP) {
        size_t oc = MCAP; I *ok = MK, *ov = MV; char *ou = MU;
        MCAP = oc ? oc * 2 : 1 << 16; MK = calloc(MCAP, sizeof(I)); MV = calloc(MCAP, sizeof(I)); MU = calloc(MCAP, 1); MN = 0;
        if (!MK || !MV || !MU) die("out of memory");
        for (size_t j = 0; j < oc; j++) if (ou[j]) mset(ok[j], ov[j]);
        free(ok); free(ov); free(ou);
    }
    size_t s = mslot(k); while (MU[s] && MK[s] != k) s = (s + 1) & (MCAP - 1);
    if (!MU[s]) { MU[s] = 1; MK[s] = k; MN++; }
    MV[s] = v;
}

/* ---- blobs and interning ---- */
typedef struct { unsigned char *b; int n; } Blob;
static Blob *BL; static int NBL, CBL;
static int blob_add(const unsigned char *b, int n) {
    if (NBL >= CBL) { CBL = CBL ? CBL * 2 : 64; BL = xrealloc(BL, sizeof(Blob) * CBL); }
    BL[NBL].b = xrealloc(0, n + 1); memcpy(BL[NBL].b, b, n); BL[NBL].n = n; return NBL++;
}
typedef struct { unsigned char *b; int n; I v; } Ent;
static Ent *IT; static size_t ICAP, IN_;
static uint64_t hbytes(const unsigned char *b, int n) { uint64_t h = 1469598103934665603ull; for (int j = 0; j < n; j++) h = (h ^ b[j]) * 1099511628211ull; return h; }
static I intern(const unsigned char *b, int n) {
    if ((IN_ + 1) * 2 > ICAP) {
        size_t oc = ICAP; Ent *o = IT; ICAP = oc ? oc * 2 : 1024; IT = calloc(ICAP, sizeof(Ent)); if (!IT) die("out of memory");
        for (size_t j = 0; j < oc; j++) if (o[j].b) { size_t s = hbytes(o[j].b, o[j].n) & (ICAP - 1); while (IT[s].b) s = (s + 1) & (ICAP - 1); IT[s] = o[j]; }
        free(o);
    }
    size_t s = hbytes(b, n) & (ICAP - 1);
    while (IT[s].b) { if (IT[s].n == n && !memcmp(IT[s].b, b, n)) return IT[s].v; s = (s + 1) & (ICAP - 1); }
    IT[s].b = xrealloc(0, n + 1); memcpy(IT[s].b, b, n); IT[s].n = n; IT[s].v = (I)++IN_;
    return IT[s].v;
}
/* SBFIND: file path -> blob id (0: absent), cached */
typedef struct { unsigned char *p; int n; int id; } FEnt;
static FEnt *FC; static int NFC;
static const char *INCDIR = 0;   /* the 4th argument; SBFIND of a bundled header without it is an error */
static int sbfind(const unsigned char *p, int n) {
    for (int j = 0; j < NFC; j++) if (FC[j].n == n && !memcmp(FC[j].p, p, n)) return FC[j].id;
    char path[4096]; int id = 0;
    if (n >= 5 && !memcmp(p, "\0hdr/", 5) && !INCDIR) die("a bundled header was asked for and no include directory was given");
    if (n >= 5 && !memcmp(p, "\0hdr/", 5)) snprintf(path, sizeof path, "%s/%.*s", INCDIR, n - 5, p + 5);
    else snprintf(path, sizeof path, "%.*s", n, p);
    if ((int)strlen(path) == (n >= 5 && !memcmp(p, "\0hdr/", 5) ? (int)strlen(INCDIR) + 1 + n - 5 : n)) {   /* else truncated: absent */
        int m = 0; unsigned char *b = readfile(path, &m, 1);
        if (b) { id=blob_add(b,m); free(b); }
    }
    FC = xrealloc(FC, sizeof(FEnt) * (NFC + 1)); FC[NFC].p = xrealloc(0, n + 1); memcpy(FC[NFC].p, p, n); FC[NFC].n = n; FC[NFC].id = id; NFC++;
    return id;
}

static int32_t w32(I v) { return (int32_t)(uint32_t)(uint64_t)v; }
static I alu32(int op, I a64, I b64) {
    int32_t a = w32(a64), b = w32(b64);
    switch (op) {
    case 0: return (int32_t)((uint32_t)a + (uint32_t)b);
    case 1: return (int32_t)((uint32_t)a - (uint32_t)b);
    case 2: return (int32_t)((uint32_t)a * (uint32_t)b);
    case 3: if (!b) return 0; if (a == INT32_MIN && b == -1) return INT32_MIN; return a / b;
    case 4: if (!b) return 0; if (a == INT32_MIN && b == -1) return 0; return a % b;
    case 5: return a & b; case 6: return a | b; case 7: return a ^ b;
    case 8: return (int32_t)((uint32_t)a << (b & 31));
    case 9: return a >> (b & 31);
    }
    die("bad alu op"); return 0;
}
static I alu64(int op, I a, I b, int *z) {
    uint64_t ua = (uint64_t)a, ub = (uint64_t)b; *z = 0;
    switch (op) {
    case 0: return (I)(ua + ub); case 1: return (I)(ua - ub); case 2: return (I)(ua * ub);
    case 10: if (!b) { *z = 1; return 0; } if (a == INT64_MIN && b == -1) return INT64_MIN; return a / b;
    case 11: if (!b) { *z = 1; return 0; } if (a == INT64_MIN && b == -1) return 0; return a % b;
    case 12: if (!b) { *z = 1; return 0; } return (I)(ua / ub);
    case 13: if (!b) { *z = 1; return 0; } return (I)(ua % ub);
    case 5: return a & b; case 6: return a | b; case 7: return a ^ b;
    case 14: return ~a;
    case 8: return (I)(ua << (b & 63));
    case 15: return (I)(ua >> (b & 63));
    case 9: return a >> (b & 63);
    }
    die("bad alu64 op"); return 0;
}

typedef struct { const unsigned char *b; const I *at; I i, end; } Frame;

int main(int argc, char **argv) {
    if (argc == 4 && !strcmp(argv[1], "--check-net")) return checknet(argv[2], argv[3]);
    if (argc < 3) { fprintf(stderr, "usage: run TABLE INPUT [SRCPATH] [INCLUDE_DIR, absolute]\n"); return 2; }
    load(argv[1]);
    if (argc > 4) INCDIR = argv[4];
    Buf xin = {0}; xin.b=readfile(argv[2], &xin.n, 0);
    const char *src = argc > 3 ? argv[3] : argv[2];
    R = calloc(NRG + 1, sizeof(I));
    blob_add((const unsigned char *)"", 0);
    blob_add((const unsigned char *)src, (int)strlen(src));
    unsigned char *x = xin.b ? xin.b : (unsigned char *)""; I *xattr = calloc(xin.n + 1, sizeof(I)); I xn = xin.n;
    int NFR = 1, CFR = 16; Frame *fr = xrealloc(0, sizeof(Frame) * CFR);
    fr[0].b = x; fr[0].at = xattr; fr[0].i = 0; fr[0].end = xn;
    Buf o = {0}, e = {0}; int osel = 0; I OT = 0;
    Buf sb = {0};
    int *stk = 0; int nst = 0, cst = 0;
    long long maxsteps = getenv("UNISA_MAXSTEPS") ? strtol(getenv("UNISA_MAXSTEPS"), 0, 10) : 200000000LL, steps = 0;
    int q = START; I r = 0;
    for (;;) {
        if (++steps > maxsteps) { fprintf(stderr, "timeout\n"); return 3; }
        Frame *F = &fr[NFR - 1];
        int nx = -1, sq = -1;
        int key = SMODE[q] == 1 ? (nst ? stk[nst - 1] : -1) :
            SMODE[q] == 0 ? (F->i < F->end ? F->b[F->i] : 256) : (r >= 0 && r <= 256 ? (int)r : 256);
        transition(q, key, &nx, &sq);
        if (nx < 0) { fprintf(stderr, "run: no transition\n"); return 2; }
        q = nx;
        const I *a = QA + QOFF[sq];
        for (int k = 0; k < QLEN[sq]; k++) {
            int op = (int)a[0];
            F = &fr[NFR - 1];
            switch (op) {
            case ADV: F->i++; break;
            case MARK: R[a[1]] = F->i; break;
            case JUMP: F->i = R[a[1]]; break;
            case LDI: R[a[1]] = a[2]; break;
            case COPYW: R[a[1]] = R[a[2]]; break;
            case ALU: R[a[2]] = alu32((int)a[1], R[a[3]], R[a[4]]); break;
            case ALUI: R[a[2]] = alu32((int)a[1], R[a[3]], a[4]); break;
            case CMP: case CMPI: { I u = R[a[1]], v = op == CMP ? R[a[2]] : a[2]; r = u < v ? 0 : u == v ? 1 : 2; } break;
            case A64: case A64I: { int z; R[a[2]] = alu64((int)a[1], R[a[3]], op == A64 ? R[a[4]] : a[4], &z); r = z; } break;
            case C64: { I u = R[a[1]], v = R[a[2]]; r = u < v ? 0 : u == v ? 1 : 2; } break;
            case C64U: { uint64_t u = (uint64_t)R[a[1]], v = (uint64_t)R[a[2]]; r = u < v ? 0 : u == v ? 1 : 2; } break;
            case INC: R[a[1]] = (I)(((uint64_t)R[a[1]] + 1) & 0xFFFFFFFFull); break;
            case RLD: { I v = R[a[1]]; r = v >= 0 && v <= 256 ? v : 256; } break;
            case LDX: R[a[1]] = mget(R[a[2]] + a[3]); break;
            case STX: mset(R[a[1]] + a[2], R[a[3]]); break;
            case OUT: if (osel == 0) bput(&o, (int)a[1], OT); else bput(&e, (int)a[1], 0); break;
            case OUTW: if (osel == 0) bput(&o, (int)(R[a[1]] & 255), OT); else bput(&e, (int)(R[a[1]] & 255), 0); break;
            case COPYT: if (F->i < F->end) { if (osel == 0) bput(&o, F->b[F->i], OT); else bput(&e, F->b[F->i], 0); } break;
            case COPY: if (F->i < F->end) { if (osel == 0) bput(&o, F->b[F->i], F->at ? F->at[F->i] : OT); else bput(&e, F->b[F->i], 0); } break;
            case SPAN: case SPANT: case SPAN2: {
                I s0 = R[a[1]], s1 = op == SPAN2 ? R[a[2]] : F->i; if (s1 > F->end) s1 = F->end;
                for (I j = s0 < 0 ? 0 : s0; j < s1; j++) {
                    if (osel == 1) bput(&e, F->b[j], 0);
                    else bput(&o, F->b[j], (op == SPANT || !F->at) ? OT : F->at[j]);
                }
            } break;
            case OLAST: r = o.n ? o.b[o.n - 1] : 256; break;
            case ODROP: if (o.n) o.n--; break;
            case OLEN: R[a[1]] = o.n; break;
            case OCUT: { I s0 = R[a[2]]; if (s0 < 0) s0 = 0; if (s0 > o.n) s0 = o.n;
                         R[a[1]] = blob_add(o.b + s0, (int)(o.n - s0)); o.n = (int)s0; } break;
            case ORES: R[a[1]] = o.n; for (I j = 0; j < a[2]; j++) bput(&o, ' ', OT); break;
            case OFILL: { char t[32]; int n = snprintf(t, sizeof t, "%lld", (long long)R[a[2]]); I w = a[3], at = R[a[1]];
                          if (n > w) { fprintf(stderr, "reject: field overflow\n"); fwrite(e.b, 1, e.n, stderr); return 1; }
                          if (at < 0 || at + w > o.n) die("fill past the reservation");
                          for (I j = 0; j < w; j++) o.b[at + j] = j < w - n ? ' ' : (unsigned char)t[j - (w - n)]; } break;
            case OCLR: o.n = 0; break;
            case OSEL: osel = (int)a[1]; break;
            case SETOT: OT = R[a[1]]; break;
            case XATTR: R[a[1]] = (F->at && F->i < F->end) ? F->at[F->i] : 0; break;
            case PUSH: if (nst >= cst) { cst = cst ? cst * 2 : 1024; stk = xrealloc(stk, sizeof(int) * cst); } stk[nst++] = (int)a[1]; break;
            case POP: if (!nst) die("pop of an empty stack"); nst--; break;
            case INTERN: case SBINTERN: {
                if (op == INTERN) { I s0 = R[a[2]] < 0 ? 0 : R[a[2]], s1 = R[a[3]]; if (s1 > F->end) s1 = F->end;
                                    R[a[1]] = s0 < s1 ? intern(F->b + s0, (int)(s1 - s0)) : intern((const unsigned char *)"", 0); }
                else R[a[1]] = intern(sb.b ? sb.b : (unsigned char *)"", sb.n);
            } break;
            case BLOBSAVE: { I s0 = R[a[2]] < 0 ? 0 : R[a[2]], s1 = R[a[3]]; if (s1 > F->end) s1 = F->end;
                             R[a[1]] = s0 < s1 ? blob_add(F->b + s0, (int)(s1 - s0)) : blob_add((const unsigned char *)"", 0); } break;
            case SBSAVE: R[a[1]] = blob_add(sb.b ? sb.b : (unsigned char *)"", sb.n); break;
            case INPUSH: case INPUSHX: case INPUSHXE: {
                if (NFR >= CFR) { CFR *= 2; fr = xrealloc(fr, sizeof(Frame) * CFR); }
                Frame *G = &fr[NFR++];
                if (op == INPUSH) { Blob *B = &BL[R[a[1]]]; G->b = B->b; G->at = 0; G->i = 0; G->end = B->n; }
                else { G->b = x; G->at = xattr; G->i = R[a[1]]; G->end = xn;
                       if (op == INPUSHXE && R[a[2]] < xn) G->end = R[a[2]]; }
            } break;
            case INPOP: if (NFR > 1) NFR--; break;
            case SBCLR: sb.n = 0; break;
            case SBOUT: bput(&sb, (int)a[1], 0); break;
            case SBSPAN: { I s0 = R[a[1]] < 0 ? 0 : R[a[1]], s1 = R[a[2]]; if (s1 > F->end) s1 = F->end; for (I j = s0; j < s1; j++) bput(&sb, F->b[j], 0); } break;
            case SBBLOB: { Blob *B = &BL[R[a[1]]]; for (int j = 0; j < B->n; j++) bput(&sb, B->b[j], 0); } break;
            case SBFIND: R[a[1]] = sbfind(sb.b ? sb.b : (unsigned char *)"", sb.n); break;
            case BYTE: R[a[1]] = F->i < F->end ? F->b[F->i] : 0; break;
            case XLEN: R[a[1]] = F->end; break;
            case BLEN: R[a[1]] = BL[R[a[2]]].n; break;
            case DIVMOD10: { uint64_t v = (uint32_t)(uint64_t)R[a[1]]; R[a[1]] = (I)(v / 10); r = (I)(v % 10) + (v / 10 == 0 ? 10 : 0); } break;
            case SWAP: { unsigned char *nb = xrealloc(0, o.n + 1); I *na = xrealloc(0, sizeof(I) * (o.n + 1));
                         memcpy(nb, o.b, o.n); memcpy(na, o.at, sizeof(I) * o.n);
                         x = nb; xattr = na; xn = o.n; o.n = 0; NFR = 1; fr[0].b = x; fr[0].at = xattr; fr[0].i = 0; fr[0].end = xn; } break;
            case ACCEPT: if (fwrite(o.b, 1, o.n, stdout) != (size_t)o.n || fclose(stdout)) die("cannot write output"); return 0;
            case REJECT: fprintf(stderr, "reject: %.*s\n", STRL[a[1]], STR[a[1]]); fwrite(e.b, 1, e.n, stderr); return 1;
            default: die("bad action");
            }
            a += 1 + ARITY[op];
        }
    }
}
