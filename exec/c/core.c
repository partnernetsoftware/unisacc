/* The complete C execution kernel: inference, actions and their working
   storage. Loader, IO, route selection and diagnostic printing are outside.
   libc memory primitives are linked dependencies, not hidden compiler rules. */
#include "core.h"
#include <stdlib.h>
#include <string.h>
static void core_die(const char *s) { core_host_panic(s); abort(); }
static void *core_alloc(void *p,size_t n) { p=realloc(p,n ? n : 1); if (!p) core_die("out of memory"); return p; }
static int decimal(char *s,I v) {
    char t[20]; int n=0,k=0; uint64_t u=(uint64_t)v;
    if (v<0) { s[k++]='-'; u=0-u; }
    do { t[n++]=(char)('0'+u%10); u/=10; } while (u);
    while (n) s[k++]=t[--n];
    return k;
}
/* The selected bank is sparse evaluation of a one-hot state-conditioned net.
   Each hidden activation is H(key-threshold); output weights are signed.
   No answer table is materialised. int weights/count bound sums within int64. */
const char *core_transition(const CoreModel *m, int q, int key, int *nx, int *sq) {
    *nx = -1; *sq = 0;
    if (m->isnet) {
        if (key < m->lo[q] || key > m->hi[q]) return "observation outside network domain";
        I n = m->base_next[q], s = m->base_seq[q];
        for (int j = 0; j < m->count[q]; j++) {
            int h = key >= m->keys[q][j];
            n += (I)m->next[q][j] * h; s += (I)m->seq[q][j] * h;
        }
        if (n < -1 || n >= m->ns || s < 0 || s >= m->nq) return "invalid network output";
        *nx = (int)n; *sq = (int)s;
    } else if (m->mode[q] == 1) {
        for (int j = 0; j < m->count[q]; j++) if (m->keys[q][j] == key) { *nx = m->next[q][j]; *sq = m->seq[q][j]; break; }
    } else { *nx = m->next[q][key]; *sq = m->seq[q][key]; }
    return 0;
}

/* ---- byte buffers ---- */
static void core_put(Buf *o, int c, I at) {
    if (o->n >= o->cap) { o->cap = o->cap ? o->cap * 2 : 256; o->b = core_alloc(o->b, o->cap); o->at = core_alloc(o->at, sizeof(I) * o->cap); }
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
        if (!MK || !MV || !MU) core_die("out of memory");
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
    if (NBL >= CBL) { CBL = CBL ? CBL * 2 : 64; BL = core_alloc(BL, sizeof(Blob) * CBL); }
    BL[NBL].b = core_alloc(0, n + 1); memcpy(BL[NBL].b, b, n); BL[NBL].n = n; return NBL++;
}
typedef struct { unsigned char *b; int n; I v; } Ent;
static Ent *IT; static size_t ICAP, IN_;
static uint64_t hbytes(const unsigned char *b, int n) { uint64_t h = 1469598103934665603ull; for (int j = 0; j < n; j++) h = (h ^ b[j]) * 1099511628211ull; return h; }
static I intern(const unsigned char *b, int n) {
    if ((IN_ + 1) * 2 > ICAP) {
        size_t oc = ICAP; Ent *o = IT; ICAP = oc ? oc * 2 : 1024; IT = calloc(ICAP, sizeof(Ent)); if (!IT) core_die("out of memory");
        for (size_t j = 0; j < oc; j++) if (o[j].b) { size_t s = hbytes(o[j].b, o[j].n) & (ICAP - 1); while (IT[s].b) s = (s + 1) & (ICAP - 1); IT[s] = o[j]; }
        free(o);
    }
    size_t s = hbytes(b, n) & (ICAP - 1);
    while (IT[s].b) { if (IT[s].n == n && !memcmp(IT[s].b, b, n)) return IT[s].v; s = (s + 1) & (ICAP - 1); }
    IT[s].b = core_alloc(0, n + 1); memcpy(IT[s].b, b, n); IT[s].n = n; IT[s].v = (I)++IN_;
    return IT[s].v;
}
/* SBFIND: file path -> blob id (0: absent), cached */
typedef struct { unsigned char *p; int n; int id; } FEnt;
static FEnt *FC; static int NFC;
static int sbfind(const unsigned char *p, int n) {
    for (int j = 0; j < NFC; j++) if (FC[j].n == n && !memcmp(FC[j].p,p,n)) return FC[j].id;
    unsigned char *b=0; int len=0, id=0;
    int owned=core_host_fetch(p,n,&b,&len);
    if (owned) id=blob_add(b,len);
    if (owned==2) free(b);
    FC=core_alloc(FC,sizeof(FEnt)*(NFC+1)); FC[NFC].p=core_alloc(0,n+1);
    memcpy(FC[NFC].p,p,n); FC[NFC].n=n; FC[NFC].id=id; NFC++;
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
    core_die("bad alu op"); return 0;
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
    core_die("bad alu64 op"); return 0;
}

typedef struct { const unsigned char *b; const I *at; I i, end; } Frame;

/* Run one model over a byte stream. The caller owns input and receives
   only accepted output. Registers, indexed memory, blobs and file cache are
   stage-local; the include directory is an explicit process configuration. */
int core_run(const CoreModel *m, unsigned char *input,
             int inputn, const char *src, I maxsteps, CoreResult *result) {
    memset(result,0,sizeof *result);
    int status = 0;
    R = calloc(m->nrg + 1, sizeof(I));
    blob_add((const unsigned char *)"", 0);
    blob_add((const unsigned char *)src, (int)strlen(src));
    unsigned char *x = input; I *xattr = calloc(inputn + 1, sizeof(I)); I xn = inputn;
    int NFR = 1, CFR = 16; Frame *fr = core_alloc(0, sizeof(Frame) * CFR);
    fr[0].b = x; fr[0].at = xattr; fr[0].i = 0; fr[0].end = xn;
    Buf o = {0}, e = {0}; int osel = 0; I OT = 0;
    Buf sb = {0};
    int *stk = 0; int nst = 0, cst = 0;
    I steps=0;
    int q = m->start; I r = 0;
    for (;;) {
        if (++steps > maxsteps) { result->reason="timeout"; status = 3; goto finished; }
        Frame *F = &fr[NFR - 1];
        int nx = -1, sq = -1;
        int key = m->mode[q] == 1 ? (nst ? stk[nst - 1] : -1) :
            m->mode[q] == 0 ? (F->i < F->end ? F->b[F->i] : 256) : (r >= 0 && r <= 256 ? (int)r : 256);
        result->reason=core_transition(m,q,key,&nx,&sq);
        if (result->reason) { status=2; goto finished; }
        if (nx < 0) { result->reason="no transition"; status = 2; goto finished; }
        q = nx;
        const I *a = m->qa + m->qoff[sq];
        for (int k = 0; k < m->qlen[sq]; k++) {
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
            case OUT: if (osel == 0) core_put(&o, (int)a[1], OT); else core_put(&e, (int)a[1], 0); break;
            case OUTW: if (osel == 0) core_put(&o, (int)(R[a[1]] & 255), OT); else core_put(&e, (int)(R[a[1]] & 255), 0); break;
            case COPYT: if (F->i < F->end) { if (osel == 0) core_put(&o, F->b[F->i], OT); else core_put(&e, F->b[F->i], 0); } break;
            case COPY: if (F->i < F->end) { if (osel == 0) core_put(&o, F->b[F->i], F->at ? F->at[F->i] : OT); else core_put(&e, F->b[F->i], 0); } break;
            case SPAN: case SPANT: case SPAN2: {
                I s0 = R[a[1]], s1 = op == SPAN2 ? R[a[2]] : F->i; if (s1 > F->end) s1 = F->end;
                for (I j = s0 < 0 ? 0 : s0; j < s1; j++) {
                    if (osel == 1) core_put(&e, F->b[j], 0);
                    else core_put(&o, F->b[j], (op == SPANT || !F->at) ? OT : F->at[j]);
                }
            } break;
            case OLAST: r = o.n ? o.b[o.n - 1] : 256; break;
            case ODROP: if (o.n) o.n--; break;
            case OLEN: R[a[1]] = o.n; break;
            case OCUT: { I s0 = R[a[2]]; if (s0 < 0) s0 = 0; if (s0 > o.n) s0 = o.n;
                         R[a[1]] = blob_add(o.b + s0, (int)(o.n - s0)); o.n = (int)s0; } break;
            case ORES: R[a[1]] = o.n; for (I j = 0; j < a[2]; j++) core_put(&o, ' ', OT); break;
            case OFILL: { char t[32]; int n = decimal(t,R[a[2]]); I w = a[3], at = R[a[1]];
                          if (n > w) { result->reason="field overflow"; result->reason_n=14; status = 1; goto finished; }
                          if (at < 0 || at + w > o.n) core_die("fill past the reservation");
                          for (I j = 0; j < w; j++) o.b[at + j] = j < w - n ? ' ' : (unsigned char)t[j - (w - n)]; } break;
            case OCLR: o.n = 0; break;
            case OSEL: osel = (int)a[1]; break;
            case SETOT: OT = R[a[1]]; break;
            case XATTR: R[a[1]] = (F->at && F->i < F->end) ? F->at[F->i] : 0; break;
            case PUSH: if (nst >= cst) { cst = cst ? cst * 2 : 1024; stk = core_alloc(stk, sizeof(int) * cst); } stk[nst++] = (int)a[1]; break;
            case POP: if (!nst) core_die("pop of an empty stack"); nst--; break;
            case INTERN: case SBINTERN: {
                if (op == INTERN) { I s0 = R[a[2]] < 0 ? 0 : R[a[2]], s1 = R[a[3]]; if (s1 > F->end) s1 = F->end;
                                    R[a[1]] = s0 < s1 ? intern(F->b + s0, (int)(s1 - s0)) : intern((const unsigned char *)"", 0); }
                else R[a[1]] = intern(sb.b ? sb.b : (unsigned char *)"", sb.n);
            } break;
            case BLOBSAVE: { I s0 = R[a[2]] < 0 ? 0 : R[a[2]], s1 = R[a[3]]; if (s1 > F->end) s1 = F->end;
                             R[a[1]] = s0 < s1 ? blob_add(F->b + s0, (int)(s1 - s0)) : blob_add((const unsigned char *)"", 0); } break;
            case SBSAVE: R[a[1]] = blob_add(sb.b ? sb.b : (unsigned char *)"", sb.n); break;
            case INPUSH: case INPUSHX: case INPUSHXE: {
                if (NFR >= CFR) { CFR *= 2; fr = core_alloc(fr, sizeof(Frame) * CFR); }
                Frame *G = &fr[NFR++];
                if (op == INPUSH) { Blob *B = &BL[R[a[1]]]; G->b = B->b; G->at = 0; G->i = 0; G->end = B->n; }
                else { G->b = x; G->at = xattr; G->i = R[a[1]]; G->end = xn;
                       if (op == INPUSHXE && R[a[2]] < xn) G->end = R[a[2]]; }
            } break;
            case INPOP: if (NFR > 1) NFR--; break;
            case SBCLR: sb.n = 0; break;
            case SBOUT: core_put(&sb, (int)a[1], 0); break;
            case SBSPAN: { I s0 = R[a[1]] < 0 ? 0 : R[a[1]], s1 = R[a[2]]; if (s1 > F->end) s1 = F->end; for (I j = s0; j < s1; j++) core_put(&sb, F->b[j], 0); } break;
            case SBBLOB: { Blob *B = &BL[R[a[1]]]; for (int j = 0; j < B->n; j++) core_put(&sb, B->b[j], 0); } break;
            case SBFIND: R[a[1]] = sbfind(sb.b ? sb.b : (unsigned char *)"", sb.n); break;
            case BYTE: R[a[1]] = F->i < F->end ? F->b[F->i] : 0; break;
            case XLEN: R[a[1]] = F->end; break;
            case BLEN: R[a[1]] = BL[R[a[2]]].n; break;
            case DIVMOD10: { uint64_t v = (uint32_t)(uint64_t)R[a[1]]; R[a[1]] = (I)(v / 10); r = (I)(v % 10) + (v / 10 == 0 ? 10 : 0); } break;
            case SWAP: { unsigned char *nb = core_alloc(0, o.n + 1); I *na = core_alloc(0, sizeof(I) * (o.n + 1));
                         memcpy(nb, o.b, o.n); memcpy(na, o.at, sizeof(I) * o.n);
                         if (x != input) free(x); free(xattr);
                         x = nb; xattr = na; xn = o.n; o.n = 0; NFR = 1; fr[0].b = x; fr[0].at = xattr; fr[0].i = 0; fr[0].end = xn; } break;
            case ACCEPT: goto finished;
            case REJECT: result->reason=m->str[a[1]]; result->reason_n=m->strl[a[1]]; status = 1; goto finished;
            default: core_die("bad action");
            }
            a += 1 + ARITY[op];
        }
    }
finished:
    if (!status) { result->out.b=o.b; result->out.n=o.n; o.b=0; }
    result->err.b=e.b; result->err.n=e.n; e.b=0;
    free(o.b); free(o.at); free(e.b); free(e.at); free(sb.b); free(sb.at);
    free(fr); free(stk); if (x != input) free(x); free(xattr); free(R);
    free(MK); free(MV); free(MU); MK = 0; MV = 0; MU = 0; MCAP = 0; MN = 0;
    for (int i = 0; i < NBL; i++) free(BL[i].b);
    free(BL); BL = 0; NBL = 0; CBL = 0;
    for (size_t i = 0; i < ICAP; i++) if (IT[i].b) free(IT[i].b);
    free(IT); IT = 0; ICAP = 0; IN_ = 0;
    for (int i = 0; i < NFC; i++) free(FC[i].p);
    free(FC); FC = 0; NFC = 0;
    return status;
}

