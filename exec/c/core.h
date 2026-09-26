/* Generic machine interface. No compiler, file format or OS semantics.
   Models are immutable, decoded arrays owned by the loader. One execution
   may be active at a time; all working state is discarded on return. */
#ifndef UNISA_EXEC_CORE_H
#define UNISA_EXEC_CORE_H
#include <stdint.h>
#include <stddef.h>
typedef int64_t I;
enum { ADV, MARK, JUMP, LDI, COPYW, ALU, ALUI, CMP, CMPI, RLD, LDX, STX, OUT, OUTW, COPY, COPYT,
       SPAN, SPANT, SPAN2, OLAST, ODROP, OLEN, OCUT, ORES, OFILL, OCLR, OSEL, SETOT, XATTR, PUSH, POP,
       INTERN, BLOBSAVE, INPUSH, INPUSHX, INPUSHXE, INPOP, SBCLR, SBOUT, SBSPAN, SBBLOB, SBINTERN,
       SBSAVE, SBFIND, BLEN, BYTE, XLEN, DIVMOD10, SWAP, ACCEPT, REJECT, A64, A64I, C64, C64U, INC, NOP_ };
static const int ARITY[NOP_] = {0,1,1,2,2,4,4,2,2,1,3,3,1,1,0,0,1,1,2,0,0,1,2,2,3,0,1,1,1,1,0,3,3,1,1,2,0,
                                0,1,2,1,1,1,1,2,1,1,1,0,0,1,4,4,2,2,1};

typedef struct { unsigned char *b; I *at; int n, cap; } Buf;
typedef struct {
    int ns, nq, nrg, start, isnet;
    char **str; int *strl;
    int *qoff, *qlen; I *qa;
    int *mode, *count, **keys, **next, **seq;
    int *lo, *hi, *base_next, *base_seq;
} CoreModel;
/* Host linkage: absent=0, borrowed=1, malloc-owned=2. No key syntax
   is interpreted by the core; even NUL bytes are ordinary key bytes. */
int core_host_fetch(const unsigned char *key,int n,unsigned char **bytes,int *len);
void core_host_panic(const char *reason); /* fatal; must not return */
typedef struct { Buf out, err; const char *reason; int reason_n; } CoreResult;
const char *core_transition(const CoreModel *m, int q, int key, int *nx, int *sq);
int core_run(const CoreModel *m, unsigned char *input,
             int inputn, const char *src, I maxsteps, CoreResult *result);
#endif
