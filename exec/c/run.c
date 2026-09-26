/* exec/c/run.c -- the generic delta executor in C: the machine of
   exec/pp/sim.py, action for action, on the integer table exec/c/tbl.py
   writes.  It knows no language; every decision is the table's.

       run TABLE INPUT [SRCPATH] [INCLUDE_DIR]

   exit 0 accept (o on stdout); 1 reject (the reason, then e, on stderr);
   2 a bad table; 3 out of steps (UNISA_MAXSTEPS, default 2e8). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef int64_t I;
enum { ADV, MARK, JUMP, LDI, COPYW, ALU, ALUI, CMP, CMPI, RLD, LDX, STX, OUT, OUTW, COPY, COPYT,
       SPAN, SPANT, SPAN2, OLAST, ODROP, OLEN, OCUT, ORES, OFILL, OCLR, OSEL, SETOT, XATTR, PUSH, POP,
       INTERN, BLOBSAVE, INPUSH, INPUSHX, INPUSHXE, INPOP, SBCLR, SBOUT, SBSPAN, SBBLOB, SBINTERN,
       SBSAVE, SBFIND, BLEN, BYTE, XLEN, DIVMOD10, SWAP, ACCEPT, REJECT, A64, A64I, C64, C64U, NOP_ };
static const int ARITY[NOP_] = {0,1,1,2,2,4,4,2,2,1,3,3,1,1,0,0,1,1,2,0,0,1,2,2,3,0,1,1,1,1,0,3,3,1,1,2,0,
                                0,1,2,1,1,1,1,2,1,1,1,0,0,1,4,4,2,2};

static void die(const char *m) { fprintf(stderr, "run: %s\n", m); exit(2); }
static void *xrealloc(void *p, size_t n) { p = realloc(p, n ? n : 1); if (!p) die("out of memory"); return p; }

/* ---- the table ---- */
static int NS, NQ, NRG, NSTR, START;
static char **STR; static int *STRL;
static int *QOFF, *QLEN; static I *QA; static int NQA;          /* actions, flattened */
static int *SMODE; static int **ROWK, **ROWN, **ROWQ; static int *ROWC;

static int hexv(int c) { return c <= '9' ? c - '0' : c - 'a' + 10; }

static void load(const char *path) {
    FILE *f = fopen(path, "r"); if (!f) die("cannot open the table");
    if (fscanf(f, " T %d %d %d %d %d", &NS, &NQ, &NRG, &NSTR, &START) != 5) die("bad header");
    STR = xrealloc(0, sizeof *STR * (NSTR + 1)); STRL = xrealloc(0, sizeof *STRL * (NSTR + 1));
    for (int k = 0; k < NSTR; k++) {
        static char buf[1 << 16];
        if (fscanf(f, " S %65535s", buf) != 1) die("bad string");
        int n = buf[0] == '-' ? 0 : (int)strlen(buf) / 2;
        STR[k] = xrealloc(0, n + 1); STRL[k] = n;
        for (int j = 0; j < n; j++) STR[k][j] = (char)(hexv(buf[2 * j]) * 16 + hexv(buf[2 * j + 1]));
        STR[k][n] = 0;
    }
    QOFF = xrealloc(0, sizeof(int) * NQ); QLEN = xrealloc(0, sizeof(int) * NQ);
    int cap = 1 << 16; QA = xrealloc(0, sizeof(I) * cap); NQA = 0;
    for (int k = 0; k < NQ; k++) {
        int n; if (fscanf(f, " Q %d", &n) != 1) die("bad sequence");
        QOFF[k] = NQA; QLEN[k] = n;
        for (int a = 0; a < n; a++) {
            long long op; if (fscanf(f, "%lld", &op) != 1 || op < 0 || op >= NOP_) die("bad opcode");
            if (NQA + 6 > cap) { cap *= 2; QA = xrealloc(QA, sizeof(I) * cap); }
            QA[NQA++] = op;
            for (int j = 0; j < ARITY[op]; j++) { long long v; if (fscanf(f, "%lld", &v) != 1) die("bad argument"); QA[NQA++] = v; }
        }
    }
    SMODE = xrealloc(0, sizeof(int) * NS); ROWC = xrealloc(0, sizeof(int) * NS);
    ROWK = xrealloc(0, sizeof(int *) * NS); ROWN = xrealloc(0, sizeof(int *) * NS); ROWQ = xrealloc(0, sizeof(int *) * NS);
    for (int s = 0; s < NS; s++) {
        int m, n; if (fscanf(f, " R %d %d", &m, &n) != 2) die("bad state");
        SMODE[s] = m; ROWC[s] = n;
        if (m == 1) {                                   /* keyed by the stack top: a short list */
            ROWK[s] = xrealloc(0, sizeof(int) * n); ROWN[s] = xrealloc(0, sizeof(int) * n); ROWQ[s] = xrealloc(0, sizeof(int) * n);
            for (int j = 0; j < n; j++) if (fscanf(f, "%d %d %d", &ROWK[s][j], &ROWN[s][j], &ROWQ[s][j]) != 3) die("bad row");
        } else {                                        /* keyed 0..256: direct */
            ROWN[s] = xrealloc(0, sizeof(int) * 257); ROWQ[s] = xrealloc(0, sizeof(int) * 257); ROWK[s] = 0;
            for (int j = 0; j < 257; j++) ROWN[s][j] = -1;
            for (int j = 0; j < n; j++) { int k, nx, q; if (fscanf(f, "%d %d %d", &k, &nx, &q) != 3 || k < 0 || k > 256) die("bad row"); ROWN[s][k] = nx; ROWQ[s][k] = q; }
        }
    }
    fclose(f);
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
static const char *INCDIR = "include";
static int sbfind(const unsigned char *p, int n) {
    for (int j = 0; j < NFC; j++) if (FC[j].n == n && !memcmp(FC[j].p, p, n)) return FC[j].id;
    char path[4096]; int id = 0;
    if (n >= 5 && !memcmp(p, "\0hdr/", 5)) snprintf(path, sizeof path, "%s/%.*s", INCDIR, n - 5, p + 5);
    else snprintf(path, sizeof path, "%.*s", n, p);
    if ((int)strlen(path) == (n >= 5 && !memcmp(p, "\0hdr/", 5) ? (int)strlen(INCDIR) + 1 + n - 5 : n)) {
        FILE *f = fopen(path, "rb");
        if (f) {
            unsigned char *b = 0; int m = 0, c = 0; int ch;
            while ((ch = fgetc(f)) != EOF) { if (m >= c) { c = c ? c * 2 : 4096; b = xrealloc(b, c); } b[m++] = (unsigned char)ch; }
            fclose(f); id = blob_add(b ? b : (unsigned char *)"", m); free(b);
        }
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
    if (argc < 3) { fprintf(stderr, "usage: run TABLE INPUT [SRCPATH] [INCLUDE_DIR]\n"); return 2; }
    load(argv[1]);
    if (argc > 4) INCDIR = argv[4];
    FILE *f = fopen(argv[2], "rb"); if (!f) die("cannot open the input");
    Buf xin = {0};
    { int ch; while ((ch = fgetc(f)) != EOF) bput(&xin, ch, 0); fclose(f); }
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
    long long maxsteps = getenv("UNISA_MAXSTEPS") ? atoll(getenv("UNISA_MAXSTEPS")) : 200000000LL, steps = 0;
    int q = START; I r = 0;
    for (;;) {
        if (++steps > maxsteps) { fprintf(stderr, "timeout\n"); return 3; }
        Frame *F = &fr[NFR - 1];
        int nx = -1, sq = -1;
        if (SMODE[q] == 1) {
            int top = nst ? stk[nst - 1] : -1;
            for (int j = 0; j < ROWC[q]; j++) if (ROWK[q][j] == top) { nx = ROWN[q][j]; sq = ROWQ[q][j]; break; }
        } else {
            int key = SMODE[q] == 0 ? (F->i < F->end ? F->b[F->i] : 256) : (r >= 0 && r <= 256 ? (int)r : 256);
            nx = ROWN[q][key]; sq = ROWQ[q][key];
        }
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
                          for (I j = 0; j < w; j++) o.b[at + j] = j < w - n ? ' ' : (unsigned char)t[j - (w - n)]; } break;
            case OCLR: o.n = 0; break;
            case OSEL: osel = (int)a[1]; break;
            case SETOT: OT = R[a[1]]; break;
            case XATTR: R[a[1]] = (F->at && F->i < F->end) ? F->at[F->i] : 0; break;
            case PUSH: if (nst >= cst) { cst = cst ? cst * 2 : 1024; stk = xrealloc(stk, sizeof(int) * cst); } stk[nst++] = (int)a[1]; break;
            case POP: if (nst) nst--; break;
            case INTERN: case SBINTERN: {
                if (op == INTERN) { I s0 = R[a[2]], s1 = R[a[3]]; if (s1 > F->end) s1 = F->end;
                                    R[a[1]] = s0 < s1 ? intern(F->b + s0, (int)(s1 - s0)) : intern((const unsigned char *)"", 0); }
                else R[a[1]] = intern(sb.b ? sb.b : (unsigned char *)"", sb.n);
            } break;
            case BLOBSAVE: { I s0 = R[a[2]], s1 = R[a[3]]; if (s1 > F->end) s1 = F->end;
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
            case SBSPAN: { I s0 = R[a[1]], s1 = R[a[2]]; if (s1 > F->end) s1 = F->end; for (I j = s0; j < s1; j++) bput(&sb, F->b[j], 0); } break;
            case SBBLOB: { Blob *B = &BL[R[a[1]]]; for (int j = 0; j < B->n; j++) bput(&sb, B->b[j], 0); } break;
            case SBFIND: R[a[1]] = sbfind(sb.b ? sb.b : (unsigned char *)"", sb.n); break;
            case BYTE: R[a[1]] = F->i < F->end ? F->b[F->i] : 0; break;
            case XLEN: R[a[1]] = F->end; break;
            case BLEN: R[a[1]] = BL[R[a[2]]].n; break;
            case DIVMOD10: { uint64_t v = (uint32_t)(uint64_t)R[a[1]]; R[a[1]] = (I)(v / 10); r = (I)(v % 10) + (v / 10 == 0 ? 10 : 0); } break;
            case SWAP: { unsigned char *nb = xrealloc(0, o.n + 1); I *na = xrealloc(0, sizeof(I) * (o.n + 1));
                         memcpy(nb, o.b, o.n); memcpy(na, o.at, sizeof(I) * o.n);
                         x = nb; xattr = na; xn = o.n; o.n = 0; NFR = 1; fr[0].b = x; fr[0].at = xattr; fr[0].i = 0; fr[0].end = xn; } break;
            case ACCEPT: fwrite(o.b, 1, o.n, stdout); return 0;
            case REJECT: fprintf(stderr, "reject: %.*s\n", STRL[a[1]], STR[a[1]]); fwrite(e.b, 1, e.n, stderr); return 1;
            default: die("bad action");
            }
            a += 1 + ARITY[op];
        }
    }
}
