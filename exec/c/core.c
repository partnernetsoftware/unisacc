/* The complete C execution kernel: inference, actions and their working
   storage. Loader, IO, route selection and diagnostic printing are outside.
   libc memory primitives are linked dependencies, not hidden compiler rules. */
#include "core.h"
#include <stdlib.h>
#include <string.h>
#if defined(UNISA_CORE_ASM_ACTION) && (!defined(UNISA_CORE_ASM_ALU) || !defined(UNISA_CORE_ASM_BUFFER) || !defined(UNISA_CORE_ASM_MEMORY) || !defined(UNISA_CORE_ASM_INTERN) || !defined(UNISA_CORE_ASM_BYTES) || !defined(UNISA_CORE_ASM_FORMAT) || !defined(UNISA_CORE_ASM_STACK))
#error Assembly_actions_require_all_assembly_primitives
#endif
#ifndef UNISA_CORE_ASM_ACTION
static void core_die(const char *s) { core_host_panic(s); abort(); }
static void *core_alloc(void *p,size_t n) { p=realloc(p,n ? n : 1); if (!p) core_die("out of memory"); return p; }
#endif
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
#ifndef CORE_TRANSITION_NAME
#define CORE_TRANSITION_NAME core_transition
#endif
#ifndef UNISA_CORE_ASM_TRANSITION
const char *CORE_TRANSITION_NAME(const CoreModel *m, int q, int key, int *nx, int *sq) {
    *nx = -1; *sq = 0;
    if (m->isnet) {
        if (key < m->lo[q] || key > m->hi[q]) return "observation outside network domain";
        I n = m->base_next[q], s = m->base_seq[q];
        /* Thresholds ascend (the loader rejects any other bank): the active
           units are a prefix. Find its length, then add exactly those weights. */
        const int *k = m->keys[q]; int cut = 0, end = m->count[q];
        while (cut < end) { int mid = (cut + end) >> 1; if (key >= k[mid]) cut = mid + 1; else end = mid; }
        for (int j = 0; j < cut; j++) { n += (I)m->next[q][j]; s += (I)m->seq[q][j]; }
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

/* ---- blobs and interning ---- */
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
/* SBFIND: opaque resource key -> blob id (0: absent), cached. Blob zero
   is reserved by core_run before a resource can be queried. */
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

#ifdef UNISA_CORE_ASM_ACTION
int core_action(CoreMachine *s,const I *a);
#define action_run core_action
#else
#ifndef CORE_ACTION_LINKAGE
#define CORE_ACTION_LINKAGE static
#endif
CORE_ACTION_LINKAGE int action_run(CoreMachine *s,const I *a) {
    const CoreModel *m=s->model; CoreResult *result=s->result; I *R=s->regs;
    CoreFrame *F=&s->frames.entries[s->frames.n-1]; int op=(int)a[0];
    switch (op) {
    case ADV: F->i++; break;
    case MARK: R[a[1]] = F->i; break;
    case JUMP: F->i = R[a[1]]; break;
    case LDI: R[a[1]] = a[2]; break;
    case COPYW: R[a[1]] = R[a[2]]; break;
    case ALU: R[a[2]] = alu32((int)a[1], R[a[3]], R[a[4]]); break;
    case ALUI: R[a[2]] = alu32((int)a[1], R[a[3]], a[4]); break;
    case CMP: case CMPI: { I u = R[a[1]], v = op == CMP ? R[a[2]] : a[2]; s->r = u < v ? 0 : u == v ? 1 : 2; } break;
    case A64: case A64I: { int z; R[a[2]] = alu64((int)a[1], R[a[3]], op == A64 ? R[a[4]] : a[4], &z); s->r = z; } break;
    case C64: { I u = R[a[1]], v = R[a[2]]; s->r = u < v ? 0 : u == v ? 1 : 2; } break;
    case C64U: { uint64_t u = (uint64_t)R[a[1]], v = (uint64_t)R[a[2]]; s->r = u < v ? 0 : u == v ? 1 : 2; } break;
    case INC: R[a[1]] = (I)(((uint64_t)R[a[1]] + 1) & 0xFFFFFFFFull); break;
    case RLD: { I v = R[a[1]]; s->r = v >= 0 && v <= 256 ? v : 256; } break;
    case LDX: R[a[1]] = core_mget(&s->memory,R[a[2]] + a[3]); break;
    case STX: core_mset(&s->memory,R[a[1]] + a[2], R[a[3]]); break;
    case OUT: if (s->osel == 0) core_put(&s->out, (int)a[1], s->ot); else core_put(&s->err, (int)a[1], 0); break;
    case OUTW: if (s->osel == 0) core_put(&s->out, (int)(R[a[1]] & 255), s->ot); else core_put(&s->err, (int)(R[a[1]] & 255), 0); break;
    case COPYT: if (F->i < F->end) { if (s->osel == 0) core_put(&s->out, F->b[F->i], s->ot); else core_put(&s->err, F->b[F->i], 0); } break;
    case COPY: if (F->i < F->end) { if (s->osel == 0) core_put(&s->out, F->b[F->i], F->at ? F->at[F->i] : s->ot); else core_put(&s->err, F->b[F->i], 0); } break;
    case SPAN: case SPANT: case SPAN2: {
        I s0 = R[a[1]], s1 = op == SPAN2 ? R[a[2]] : F->i; if (s1 > F->end) s1 = F->end;
        for (I j = s0 < 0 ? 0 : s0; j < s1; j++) {
            if (s->osel == 1) core_put(&s->err, F->b[j], 0);
            else core_put(&s->out, F->b[j], (op == SPANT || !F->at) ? s->ot : F->at[j]);
        }
    } break;
    case OLAST: s->r = s->out.n ? s->out.b[s->out.n - 1] : 256; break;
    case ODROP: if (s->out.n) s->out.n--; break;
    case OLEN: R[a[1]] = s->out.n; break;
    case OCUT: { I s0 = R[a[2]]; if (s0 < 0) s0 = 0; if (s0 > s->out.n) s0 = s->out.n;
                 R[a[1]] = core_badd(&s->blobs,s->out.b + s0, (int)(s->out.n - s0)); s->out.n = (int)s0; } break;
    case ORES: R[a[1]] = s->out.n; for (I j = 0; j < a[2]; j++) core_put(&s->out, ' ', s->ot); break;
    case OFILL: if (field_fill(&s->out,R[a[1]],a[3],R[a[2]])) {
                    result->reason="field overflow"; result->reason_n=14; s->status=1; return 1;
                } break;
    case OCLR: s->out.n = 0; break;
    case OSEL: s->osel = (int)a[1]; break;
    case SETOT: s->ot = R[a[1]]; break;
    case XATTR: R[a[1]] = (F->at && F->i < F->end) ? F->at[F->i] : 0; break;
    case PUSH: stack_push(&s->stack,(int)a[1]); break;
    case POP: stack_pop(&s->stack); break;
    case INTERN: case SBINTERN: {
        if (op == INTERN) { I s0 = R[a[2]] < 0 ? 0 : R[a[2]], s1 = R[a[3]]; if (s1 > F->end) s1 = F->end;
                            R[a[1]] = s0 < s1 ? core_intern(&s->strings,F->b + s0, (int)(s1 - s0)) : core_intern(&s->strings,(const unsigned char *)"", 0); }
        else R[a[1]] = core_intern(&s->strings,s->scratch.b ? s->scratch.b : (unsigned char *)"", s->scratch.n);
    } break;
    case BLOBSAVE: { I s0 = R[a[2]] < 0 ? 0 : R[a[2]], s1 = R[a[3]]; if (s1 > F->end) s1 = F->end;
                     R[a[1]] = s0 < s1 ? core_badd(&s->blobs,F->b + s0, (int)(s1 - s0)) : core_badd(&s->blobs,(const unsigned char *)"", 0); } break;
    case SBSAVE: R[a[1]] = core_badd(&s->blobs,s->scratch.b ? s->scratch.b : (unsigned char *)"", s->scratch.n); break;
    case INPUSH: case INPUSHX: case INPUSHXE: {
        CoreFrame added; CoreFrame *G=&added;
        if (op == INPUSH) { CoreBlob *B = &s->blobs.entries[R[a[1]]]; G->b = B->b; G->at = 0; G->i = 0; G->end = B->n; }
        else { G->b = s->x; G->at = s->xattr; G->i = R[a[1]]; G->end = s->xn;
               if (op == INPUSHXE && R[a[2]] < s->xn) G->end = R[a[2]]; }
        frame_push(&s->frames,G);
    } break;
    case INPOP: frame_pop(&s->frames); break;
    case SBCLR: s->scratch.n = 0; break;
    case SBOUT: core_put(&s->scratch, (int)a[1], 0); break;
    case SBSPAN: { I s0 = R[a[1]] < 0 ? 0 : R[a[1]], s1 = R[a[2]]; if (s1 > F->end) s1 = F->end; for (I j = s0; j < s1; j++) core_put(&s->scratch, F->b[j], 0); } break;
    case SBBLOB: { CoreBlob *B = &s->blobs.entries[R[a[1]]]; for (int j = 0; j < B->n; j++) core_put(&s->scratch, B->b[j], 0); } break;
    case SBFIND: R[a[1]] = core_rfind(&s->resources,&s->blobs,s->scratch.b ? s->scratch.b : (unsigned char *)"", s->scratch.n); break;
    case BYTE: R[a[1]] = F->i < F->end ? F->b[F->i] : 0; break;
    case XLEN: R[a[1]] = F->end; break;
    case BLEN: R[a[1]] = s->blobs.entries[R[a[2]]].n; break;
    case DIVMOD10: { uint64_t v = (uint32_t)(uint64_t)R[a[1]]; R[a[1]] = (I)(v / 10); s->r = (I)(v % 10) + (v / 10 == 0 ? 10 : 0); } break;
    case SWAP: { unsigned char *nb = core_alloc(0, s->out.n + 1); I *na = core_alloc(0, sizeof(I) * (s->out.n + 1));
                 memcpy(nb, s->out.b, s->out.n); memcpy(na, s->out.at, sizeof(I) * s->out.n);
                 if (s->x != s->input) free(s->x); free(s->xattr);
                 s->x = nb; s->xattr = na; s->xn = s->out.n; s->out.n = 0; s->frames.n=1; s->frames.entries[0].b=s->x; s->frames.entries[0].at=s->xattr; s->frames.entries[0].i=0; s->frames.entries[0].end=s->xn; } break;
    case ACCEPT: return 1;
    case REJECT: result->reason=m->str[a[1]]; result->reason_n=m->strl[a[1]]; s->status = 1; return 1;
    default: core_die("bad action");
    }
    return 0;
}
#endif

/* Run one model; all mutable ownership is explicit and invocation-local. */
#ifndef UNISA_CORE_ASM_RUN
#ifndef CORE_RUN_NAME
#define CORE_RUN_NAME core_run
#endif
int CORE_RUN_NAME(const CoreModel *m,unsigned char *input,
             int inputn,const char *src,I maxsteps,CoreResult *result) {
    memset(result,0,sizeof *result);
    CoreMachine s={0}; s.model=m; s.result=result; s.input=input; s.x=input;
    s.regs=calloc(m->nrg+1,sizeof(I));
    if (!s.regs) core_host_panic("out of memory");
    core_badd(&s.blobs,(const unsigned char *)"",0);
    core_badd(&s.blobs,(const unsigned char *)src,(int)strlen(src));
    s.xattr=calloc(inputn+1,sizeof(I)); s.xn=inputn;
    if (!s.xattr) core_host_panic("out of memory");
    CoreFrame first; first.b=s.x; first.at=s.xattr; first.i=0; first.end=s.xn;
    frame_push(&s.frames,&first);
    I steps=0; int q=m->start;
    for (;;) {
        if (steps>=maxsteps) { result->reason="timeout"; s.status=3; break; }
        steps++;
        CoreFrame *F=&s.frames.entries[s.frames.n-1]; int nx=-1,sq=-1;
        int key=m->mode[q]==1 ? (s.stack.n ? s.stack.entries[s.stack.n-1] : -1) :
            m->mode[q]==0 ? (F->i<F->end ? F->b[F->i] : 256) : (s.r>=0 && s.r<=256 ? (int)s.r : 256);
        result->reason=CORE_TRANSITION_NAME(m,q,key,&nx,&sq);
        if (result->reason) { s.status=2; break; }
        if (nx<0) { result->reason="no transition"; s.status=2; break; }
        q=nx; const I *a=m->qa+m->qoff[sq];
        for (int k=0;k<m->qlen[sq];k++) {
            if (action_run(&s,a)) goto finished;
            a+=1+ARITY[(int)a[0]];
        }
    }
finished:
    if (!s.status) { result->out.b=s.out.b; result->out.n=s.out.n; s.out.b=0; }
    result->err.b=s.err.b; result->err.n=s.err.n; s.err.b=0;
    free(s.out.b);free(s.out.at);free(s.err.b);free(s.err.at);free(s.scratch.b);free(s.scratch.at);
    free(s.frames.entries);free(s.stack.entries);if (s.x!=input) free(s.x);free(s.xattr);free(s.regs);
    free(s.memory.keys);free(s.memory.values);free(s.memory.used);
    for (int i=0;i<s.blobs.n;i++) free(s.blobs.entries[i].b);
    free(s.blobs.entries);
    for (size_t i=0;i<s.strings.cap;i++) if (s.strings.entries[i].b) free(s.strings.entries[i].b);
    free(s.strings.entries);
    for (int i=0;i<s.resources.n;i++) free(s.resources.entries[i].p);
    free(s.resources.entries);
    return s.status;
}

#endif
