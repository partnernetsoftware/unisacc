/* The complete C execution kernel: inference, actions and their working
   storage. Loader, IO, route selection and diagnostic printing are outside.
   libc memory primitives are linked dependencies, not hidden compiler rules. */
#include "core.h"
#include <stdlib.h>
#include <string.h>
static void core_die(const char *s) { core_host_panic(s); abort(); }
static void *core_alloc(void *p,size_t n) { p=realloc(p,n ? n : 1); if (!p) core_die("out of memory"); return p; }
#ifdef UNISA_CORE_ASM_FORMAT
int core_decimal(char *s,I v);
int core_field_fill(Buf *o,I at,I width,I value);
#define decimal core_decimal
#define field_fill core_field_fill
#else
#ifndef CORE_FORMAT_LINKAGE
#define CORE_FORMAT_LINKAGE static
#endif
CORE_FORMAT_LINKAGE int decimal(char *s,I v) {
    char t[20]; int n=0,k=0; uint64_t u=(uint64_t)v;
    if (v<0) { s[k++]='-'; u=0-u; }
    do { t[n++]=(char)('0'+u%10); u/=10; } while (u);
    while (n) s[k++]=t[--n];
    return k;
}
CORE_FORMAT_LINKAGE int field_fill(Buf *o,I at,I width,I value) {
    char t[32]; int n=decimal(t,value);
    if (n>width) return 1;
    /* Subtraction after the range check cannot overflow. */
    if (at<0 || at>o->n || width>o->n-at) core_die("fill past the reservation");
    for (I j=0;j<width;j++) o->b[at+j]=j<width-n ? ' ' : (unsigned char)t[j-(width-n)];
    return 0;
}
#endif
/* The selected bank is sparse evaluation of a one-hot state-conditioned net.
   Each hidden activation is H(key-threshold); output weights are signed.
   No answer table is materialised. int weights/count bound sums within int64. */
#ifndef UNISA_CORE_ASM_TRANSITION
#ifndef CORE_TRANSITION_NAME
#define CORE_TRANSITION_NAME core_transition
#endif
const char *CORE_TRANSITION_NAME(const CoreModel *m, int q, int key, int *nx, int *sq) {
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

#endif

/* ---- byte buffers ---- */
#ifdef UNISA_CORE_ASM_BUFFER
void core_buffer_put(Buf *o,int c,I at);
#define core_put core_buffer_put
#else
#ifndef CORE_BUFFER_LINKAGE
#define CORE_BUFFER_LINKAGE static
#endif
CORE_BUFFER_LINKAGE void core_put(Buf *o, int c, I at) {
    if (o->n >= o->cap) {
        if (o->cap > INT32_MAX/2) core_die("buffer capacity overflow");
        o->cap = o->cap ? o->cap * 2 : 256;
        o->b = core_alloc(o->b, o->cap); o->at = core_alloc(o->at, sizeof(I) * o->cap);
    }
    o->b[o->n] = (unsigned char)c; o->at[o->n] = at; o->n++;
}
#endif

/* ---- W: registers and the indexed memory (a hash map) ---- */
static I *R;
static CoreMemory memory;
#ifdef UNISA_CORE_ASM_MEMORY
I core_memory_get(const CoreMemory *m,I k);
void core_memory_set(CoreMemory *m,I k,I v);
#define core_mget core_memory_get
#define core_mset core_memory_set
#else
#ifndef CORE_MEMORY_LINKAGE
#define CORE_MEMORY_LINKAGE static
#endif
static size_t mslot(const CoreMemory *m,I k) { uint64_t h = (uint64_t)k * 0x9E3779B97F4A7C15ull; return (size_t)(h >> 20) & (m->cap - 1); }
CORE_MEMORY_LINKAGE I core_mget(const CoreMemory *m,I k) {
    if (!m->cap) return 0;
    size_t s=mslot(m,k);
    while (m->used[s]) { if (m->keys[s]==k) return m->values[s]; s=(s+1)&(m->cap-1); }
    return 0;
}
CORE_MEMORY_LINKAGE void core_mset(CoreMemory *m,I k,I v) {
    if ((m->n+1)*2 > m->cap) {
        size_t oc=m->cap; I *ok=m->keys, *ov=m->values; unsigned char *ou=m->used;
        /* 64-bit storage ABI: doubled capacity times eight must fit. */
        if (oc > 0x0fffffffffffffffull) core_die("memory capacity overflow");
        m->cap=oc ? oc*2 : 1<<16;
        m->keys=calloc(m->cap,sizeof(I)); m->values=calloc(m->cap,sizeof(I)); m->used=calloc(m->cap,1); m->n=0;
        if (!m->keys || !m->values || !m->used) core_die("out of memory");
        for (size_t j=0;j<oc;j++) if (ou[j]) core_mset(m,ok[j],ov[j]);
        free(ok);free(ov);free(ou);
    }
    size_t s=mslot(m,k);
    while (m->used[s] && m->keys[s]!=k) s=(s+1)&(m->cap-1);
    if (!m->used[s]) { m->used[s]=1; m->keys[s]=k; m->n++; }
    m->values[s]=v;
}
#endif
#define mget(k) core_mget(&memory,(k))
#define mset(k,v) core_mset(&memory,(k),(v))

/* ---- blobs and interning ---- */
static CoreBlobs blobs;
#ifdef UNISA_CORE_ASM_BYTES
int core_blob_add(CoreBlobs *t,const unsigned char *b,int n);
#define core_badd core_blob_add
#else
#ifndef CORE_BYTES_LINKAGE
#define CORE_BYTES_LINKAGE static
#endif
CORE_BYTES_LINKAGE int core_badd(CoreBlobs *t,const unsigned char *b,int n) {
    if (t->n>=t->cap) {
        if (t->cap>INT32_MAX/2) core_die("blob capacity overflow");
        t->cap=t->cap ? t->cap*2 : 64;
        t->entries=core_alloc(t->entries,sizeof(CoreBlob)*t->cap);
    }
    t->entries[t->n].b=core_alloc(0,(size_t)n+1);
    memcpy(t->entries[t->n].b,b,n); t->entries[t->n].n=n;
    return t->n++;
}
#endif
#define blob_add(b,n) core_badd(&blobs,(b),(n))
static CoreIntern strings;
#ifdef UNISA_CORE_ASM_INTERN
I core_string_intern(CoreIntern *t,const unsigned char *b,int n);
#define core_intern core_string_intern
#else
#ifndef CORE_INTERN_LINKAGE
#define CORE_INTERN_LINKAGE static
#endif
CORE_INTERN_LINKAGE uint64_t core_hbytes(const unsigned char *b,int n) {
    uint64_t h=1469598103934665603ull;
    for (int j=0;j<n;j++) h=(h^b[j])*1099511628211ull;
    return h;
}
CORE_INTERN_LINKAGE I core_intern(CoreIntern *t,const unsigned char *b,int n) {
    if ((t->n+1)*2 > t->cap) {
        size_t oc=t->cap; CoreInternEntry *o=t->entries;
        /* 64-bit storage ABI, 24-byte entries, doubled capacity. */
        if (oc > 0x0555555555555555ull) core_die("intern capacity overflow");
        t->cap=oc ? oc*2 : 1024;
        t->entries=calloc(t->cap,sizeof(CoreInternEntry));
        if (!t->entries) core_die("out of memory");
        for (size_t j=0;j<oc;j++) if (o[j].b) {
            size_t k=core_hbytes(o[j].b,o[j].n)&(t->cap-1);
            while (t->entries[k].b) k=(k+1)&(t->cap-1);
            t->entries[k]=o[j];
        }
        free(o);
    }
    size_t k=core_hbytes(b,n)&(t->cap-1);
    while (t->entries[k].b) {
        if (t->entries[k].n==n && !memcmp(t->entries[k].b,b,n)) return t->entries[k].v;
        k=(k+1)&(t->cap-1);
    }
    t->entries[k].b=core_alloc(0,(size_t)n+1);
    memcpy(t->entries[k].b,b,n); t->entries[k].n=n; t->entries[k].v=(I)++t->n;
    return t->entries[k].v;
}
#endif
#define intern(b,n) core_intern(&strings,(b),(n))
/* SBFIND: opaque resource key -> blob id (0: absent), cached. Blob zero
   is reserved by core_run before a resource can be queried. */
static CoreResources resources;
#ifdef UNISA_CORE_ASM_BYTES
int core_resource_find(CoreResources *t,CoreBlobs *bs,const unsigned char *p,int n);
#define core_rfind core_resource_find
#else
CORE_BYTES_LINKAGE int core_rfind(CoreResources *t,CoreBlobs *bs,const unsigned char *p,int n) {
    for (int j=0;j<t->n;j++) if (t->entries[j].n==n && !memcmp(t->entries[j].p,p,n)) return t->entries[j].id;
    if (t->n==INT32_MAX) core_die("resource cache capacity overflow");
    unsigned char *b=0; int len=0,id=0;
    int owned=core_host_fetch(p,n,&b,&len);
    if (owned) id=core_badd(bs,b,len);
    if (owned==2) free(b);
    t->entries=core_alloc(t->entries,sizeof(CoreResourceEntry)*((size_t)t->n+1));
    t->entries[t->n].p=core_alloc(0,(size_t)n+1);
    memcpy(t->entries[t->n].p,p,n); t->entries[t->n].n=n; t->entries[t->n].id=id; t->n++;
    return id;
}
#endif
#define sbfind(p,n) core_rfind(&resources,&blobs,(p),(n))

#ifdef UNISA_CORE_ASM_ALU
I core_alu32(int op,I a,I b);
I core_alu64(int op,I a,I b,int *z);
#define alu32 core_alu32
#define alu64 core_alu64
#else
#ifndef CORE_ALU_LINKAGE
#define CORE_ALU_LINKAGE static
#endif
static int32_t w32(I v) { return (int32_t)(uint32_t)(uint64_t)v; }
CORE_ALU_LINKAGE I alu32(int op, I a64, I b64) {
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
CORE_ALU_LINKAGE I alu64(int op, I a, I b, int *z) {
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

#endif

#ifdef UNISA_CORE_ASM_STACK
void core_stack_push(CoreStack *s,int value);
void core_stack_pop(CoreStack *s);
void core_frame_push(CoreFrames *s,const CoreFrame *value);
void core_frame_pop(CoreFrames *s);
#define stack_push core_stack_push
#define stack_pop core_stack_pop
#define frame_push core_frame_push
#define frame_pop core_frame_pop
#else
#ifndef CORE_STACK_LINKAGE
#define CORE_STACK_LINKAGE static
#endif
CORE_STACK_LINKAGE void stack_push(CoreStack *s,int value) {
    if (s->n>=s->cap) {
        if (s->cap>INT32_MAX/2) core_die("stack capacity overflow");
        s->cap=s->cap ? s->cap*2 : 1024;
        s->entries=core_alloc(s->entries,sizeof(int)*s->cap);
    }
    s->entries[s->n++]=value;
}
CORE_STACK_LINKAGE void stack_pop(CoreStack *s) {
    if (!s->n) core_die("pop of an empty stack");
    s->n--;
}
CORE_STACK_LINKAGE void frame_push(CoreFrames *s,const CoreFrame *value) {
    if (s->n>=s->cap) {
        if (s->cap>INT32_MAX/2) core_die("frame capacity overflow");
        s->cap=s->cap ? s->cap*2 : 16;
        s->entries=core_alloc(s->entries,sizeof(CoreFrame)*s->cap);
    }
    s->entries[s->n++]=*value;
}
CORE_STACK_LINKAGE void frame_pop(CoreFrames *s) { if (s->n>1) s->n--; }
#endif

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
    CoreFrames frames={0}; CoreFrame first;
    first.b=x; first.at=xattr; first.i=0; first.end=xn; frame_push(&frames,&first);
    Buf o = {0}, e = {0}; int osel = 0; I OT = 0;
    Buf sb = {0};
    CoreStack stack={0};
    I steps=0;
    int q = m->start; I r = 0;
    for (;;) {
        if (++steps > maxsteps) { result->reason="timeout"; status = 3; goto finished; }
        CoreFrame *F = &frames.entries[frames.n-1];
        int nx = -1, sq = -1;
        int key = m->mode[q] == 1 ? (stack.n ? stack.entries[stack.n-1] : -1) :
            m->mode[q] == 0 ? (F->i < F->end ? F->b[F->i] : 256) : (r >= 0 && r <= 256 ? (int)r : 256);
        result->reason=core_transition(m,q,key,&nx,&sq);
        if (result->reason) { status=2; goto finished; }
        if (nx < 0) { result->reason="no transition"; status = 2; goto finished; }
        q = nx;
        const I *a = m->qa + m->qoff[sq];
        for (int k = 0; k < m->qlen[sq]; k++) {
            int op = (int)a[0];
            F = &frames.entries[frames.n-1];
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
            case OFILL: if (field_fill(&o,R[a[1]],a[3],R[a[2]])) {
                            result->reason="field overflow"; result->reason_n=14; status=1; goto finished;
                        } break;
            case OCLR: o.n = 0; break;
            case OSEL: osel = (int)a[1]; break;
            case SETOT: OT = R[a[1]]; break;
            case XATTR: R[a[1]] = (F->at && F->i < F->end) ? F->at[F->i] : 0; break;
            case PUSH: stack_push(&stack,(int)a[1]); break;
            case POP: stack_pop(&stack); break;
            case INTERN: case SBINTERN: {
                if (op == INTERN) { I s0 = R[a[2]] < 0 ? 0 : R[a[2]], s1 = R[a[3]]; if (s1 > F->end) s1 = F->end;
                                    R[a[1]] = s0 < s1 ? intern(F->b + s0, (int)(s1 - s0)) : intern((const unsigned char *)"", 0); }
                else R[a[1]] = intern(sb.b ? sb.b : (unsigned char *)"", sb.n);
            } break;
            case BLOBSAVE: { I s0 = R[a[2]] < 0 ? 0 : R[a[2]], s1 = R[a[3]]; if (s1 > F->end) s1 = F->end;
                             R[a[1]] = s0 < s1 ? blob_add(F->b + s0, (int)(s1 - s0)) : blob_add((const unsigned char *)"", 0); } break;
            case SBSAVE: R[a[1]] = blob_add(sb.b ? sb.b : (unsigned char *)"", sb.n); break;
            case INPUSH: case INPUSHX: case INPUSHXE: {
                CoreFrame added; CoreFrame *G=&added;
                if (op == INPUSH) { CoreBlob *B = &blobs.entries[R[a[1]]]; G->b = B->b; G->at = 0; G->i = 0; G->end = B->n; }
                else { G->b = x; G->at = xattr; G->i = R[a[1]]; G->end = xn;
                       if (op == INPUSHXE && R[a[2]] < xn) G->end = R[a[2]]; }
                frame_push(&frames,G);
            } break;
            case INPOP: frame_pop(&frames); break;
            case SBCLR: sb.n = 0; break;
            case SBOUT: core_put(&sb, (int)a[1], 0); break;
            case SBSPAN: { I s0 = R[a[1]] < 0 ? 0 : R[a[1]], s1 = R[a[2]]; if (s1 > F->end) s1 = F->end; for (I j = s0; j < s1; j++) core_put(&sb, F->b[j], 0); } break;
            case SBBLOB: { CoreBlob *B = &blobs.entries[R[a[1]]]; for (int j = 0; j < B->n; j++) core_put(&sb, B->b[j], 0); } break;
            case SBFIND: R[a[1]] = sbfind(sb.b ? sb.b : (unsigned char *)"", sb.n); break;
            case BYTE: R[a[1]] = F->i < F->end ? F->b[F->i] : 0; break;
            case XLEN: R[a[1]] = F->end; break;
            case BLEN: R[a[1]] = blobs.entries[R[a[2]]].n; break;
            case DIVMOD10: { uint64_t v = (uint32_t)(uint64_t)R[a[1]]; R[a[1]] = (I)(v / 10); r = (I)(v % 10) + (v / 10 == 0 ? 10 : 0); } break;
            case SWAP: { unsigned char *nb = core_alloc(0, o.n + 1); I *na = core_alloc(0, sizeof(I) * (o.n + 1));
                         memcpy(nb, o.b, o.n); memcpy(na, o.at, sizeof(I) * o.n);
                         if (x != input) free(x); free(xattr);
                         x = nb; xattr = na; xn = o.n; o.n = 0; frames.n=1; frames.entries[0].b=x; frames.entries[0].at=xattr; frames.entries[0].i=0; frames.entries[0].end=xn; } break;
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
    free(frames.entries); free(stack.entries); if (x != input) free(x); free(xattr); free(R);
    free(memory.keys); free(memory.values); free(memory.used); memset(&memory,0,sizeof memory);
    for (int i=0;i<blobs.n;i++) free(blobs.entries[i].b);
    free(blobs.entries); memset(&blobs,0,sizeof blobs);
    for (size_t i=0;i<strings.cap;i++) if (strings.entries[i].b) free(strings.entries[i].b);
    free(strings.entries); memset(&strings,0,sizeof strings);
    for (int i=0;i<resources.n;i++) free(resources.entries[i].p);
    free(resources.entries); memset(&resources,0,sizeof resources);
    return status;
}

