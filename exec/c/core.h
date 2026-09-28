/* Generic machine interface. No compiler, file format or OS semantics.
   Models are immutable, decoded arrays owned by the loader. One execution
   may be active at a time; all working state is discarded on return. */
#ifndef UNISA_EXEC_CORE_H
#define UNISA_EXEC_CORE_H
#define CORE_ACTION_ARITIES 0,1,1,2,2,4,4,2,2,1,3,3,1,1,0,0,1,1,2,0,0,1,2,2,3,0,1,1,1,1,0,3,3,1,1,2,0,0,1,2,1,1,1,1,2,1,1,1,0,0,1,4,4,2,2,1
#ifndef __ASSEMBLER__
#include <stdint.h>
#include <stddef.h>
typedef int64_t I;
enum { ADV, MARK, JUMP, LDI, COPYW, ALU, ALUI, CMP, CMPI, RLD, LDX, STX, OUT, OUTW, COPY, COPYT,
       SPAN, SPANT, SPAN2, OLAST, ODROP, OLEN, OCUT, ORES, OFILL, OCLR, OSEL, SETOT, XATTR, PUSH, POP,
       INTERN, BLOBSAVE, INPUSH, INPUSHX, INPUSHXE, INPOP, SBCLR, SBOUT, SBSPAN, SBBLOB, SBINTERN,
       SBSAVE, SBFIND, BLEN, BYTE, XLEN, DIVMOD10, SWAP, ACCEPT, REJECT, A64, A64I, C64, C64U, INC, NOP_ };
static const int ARITY[NOP_] = {CORE_ACTION_ARITIES};

typedef struct { unsigned char *b; I *at; int n, cap; } Buf;
typedef struct { const unsigned char *b; const I *at; I i,end; } CoreFrame;
typedef struct { CoreFrame *entries; int n,cap; } CoreFrames;
typedef struct { int *entries; int n,cap; } CoreStack;
typedef struct { I *keys, *values; unsigned char *used; size_t cap, n; } CoreMemory;
typedef struct { unsigned char *b; int n; I v; } CoreInternEntry;
typedef struct { CoreInternEntry *entries; size_t cap, n; } CoreIntern;
typedef struct { unsigned char *b; int n; } CoreBlob;
typedef struct { CoreBlob *entries; int n, cap; } CoreBlobs;
typedef struct { unsigned char *p; int n, id; } CoreResourceEntry;
typedef struct { CoreResourceEntry *entries; int n; } CoreResources;
typedef struct {
    int ns, nq, nrg, start, isnet;
    char **str; int *strl;
    int *qoff, *qlen; I *qa;
    int *mode, *count, **keys, **next, **seq;
    int *lo, *hi, *base_next, *base_seq;
    int *ret_seq; unsigned char **ret_ok;   /* declared returns; ret_ok[q] may be NULL */
} CoreModel;
/* Host linkage: absent=0, borrowed=1, malloc-owned=2. No key syntax
   is interpreted by the core; even NUL bytes are ordinary key bytes. */
int core_host_fetch(const unsigned char *key,int n,unsigned char **bytes,int *len);
void core_host_panic(const char *reason); /* fatal; must not return */
typedef struct { Buf out, err; const char *reason; int reason_n; } CoreResult;
/* Mutable state shared with the action engine. Byte owners are cleaned up by
   core_run; model/result and the original input are borrowed. */
typedef struct {
    const CoreModel *model; CoreResult *result; I *regs;
    unsigned char *input,*x; I *xattr; I xn,r,ot;
    int osel,status;
    Buf out,err,scratch;
    CoreStack stack; CoreFrames frames; CoreMemory memory;
    CoreBlobs blobs; CoreIntern strings; CoreResources resources;
} CoreMachine;
const char *core_transition(const CoreModel *m, int q, int key, int *nx, int *sq);
int core_run(const CoreModel *m, unsigned char *input,
             int inputn, const char *src, I maxsteps, CoreResult *result);
#endif /* C interface */
#endif
