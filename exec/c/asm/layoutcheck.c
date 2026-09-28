/* Build-time assertions use the actual C ABI, not a copied struct. */
#include "../core.h"
#include "layout.h"
#define CHECK(field,off) typedef char check_##field[(offsetof(CoreModel,field)==off)?1:-1]
CHECK(ns,CM_NS); CHECK(nq,CM_NQ); CHECK(isnet,CM_ISNET);
CHECK(mode,CM_MODE); CHECK(count,CM_COUNT); CHECK(keys,CM_KEYS);
CHECK(next,CM_NEXT); CHECK(seq,CM_SEQ); CHECK(lo,CM_LO); CHECK(hi,CM_HI);
CHECK(base_next,CM_BASE_NEXT); CHECK(base_seq,CM_BASE_SEQ);
CHECK(ret_seq,CM_RET_SEQ); CHECK(ret_ok,CM_RET_OK);
typedef char check_size[(sizeof(CoreModel)==CM_SIZE && sizeof(void*)==8 && sizeof(int)==4 && sizeof(I)==8)?1:-1];

#define BCHECK(field,off) typedef char bcheck_##field[(offsetof(Buf,field)==off)?1:-1]
BCHECK(b,BUF_BYTES); BCHECK(at,BUF_ATTR); BCHECK(n,BUF_LENGTH); BCHECK(cap,BUF_CAPACITY);
typedef char bcheck_size[(sizeof(Buf)==BUF_SIZE)?1:-1];

#define MCHECK(field,off) typedef char mcheck_##field[(offsetof(CoreMemory,field)==off)?1:-1]
MCHECK(keys,MEM_KEYS); MCHECK(values,MEM_VALUES); MCHECK(used,MEM_USED); MCHECK(cap,MEM_CAP); MCHECK(n,MEM_COUNT);
typedef char mcheck_size[(sizeof(CoreMemory)==MEM_SIZE && sizeof(size_t)==8)?1:-1];

#define ICHECK(field,off) typedef char icheck_##field[(offsetof(CoreIntern,field)==off)?1:-1]
ICHECK(entries,INTERN_ENTRIES); ICHECK(cap,INTERN_CAP); ICHECK(n,INTERN_COUNT);
#define ECHECK(field,off) typedef char echeck_##field[(offsetof(CoreInternEntry,field)==off)?1:-1]
ECHECK(b,ENTRY_BYTES); ECHECK(n,ENTRY_LENGTH); ECHECK(v,ENTRY_VALUE);
typedef char icheck_size[(sizeof(CoreIntern)==INTERN_SIZE && sizeof(CoreInternEntry)==ENTRY_SIZE)?1:-1];

#define BSCHECK(field,off) typedef char bscheck_##field[(offsetof(CoreBlobs,field)==off)?1:-1]
BSCHECK(entries,BLOBS_ENTRIES); BSCHECK(n,BLOBS_COUNT); BSCHECK(cap,BLOBS_CAP);
#define BECHECK(field,off) typedef char becheck_##field[(offsetof(CoreBlob,field)==off)?1:-1]
BECHECK(b,BLOB_BYTES); BECHECK(n,BLOB_LENGTH);
#define RSCHECK(field,off) typedef char rscheck_##field[(offsetof(CoreResources,field)==off)?1:-1]
RSCHECK(entries,RES_ENTRIES); RSCHECK(n,RES_COUNT);
#define RECHECK(field,off) typedef char recheck_##field[(offsetof(CoreResourceEntry,field)==off)?1:-1]
RECHECK(p,RES_KEY); RECHECK(n,RES_LENGTH); RECHECK(id,RES_ID);
typedef char blob_resource_sizes[(sizeof(CoreBlobs)==BLOBS_SIZE && sizeof(CoreBlob)==BLOB_SIZE && sizeof(CoreResources)==RES_SIZE && sizeof(CoreResourceEntry)==RES_ENTRY_SIZE)?1:-1];
#define SCHECK(t,f,o) typedef char check_##t##_##f[(offsetof(t,f)==o)?1:-1]
SCHECK(CoreStack,entries,STACK_ENTRIES); SCHECK(CoreStack,n,STACK_COUNT); SCHECK(CoreStack,cap,STACK_CAP);
SCHECK(CoreFrames,entries,STACK_ENTRIES); SCHECK(CoreFrames,n,STACK_COUNT); SCHECK(CoreFrames,cap,STACK_CAP);
SCHECK(CoreFrame,b,FRAME_BYTES); SCHECK(CoreFrame,at,FRAME_ATTR); SCHECK(CoreFrame,i,FRAME_CURSOR); SCHECK(CoreFrame,end,FRAME_END);
typedef char check_stacks[(sizeof(CoreStack)==STACK_SIZE && sizeof(CoreFrames)==STACK_SIZE && sizeof(CoreFrame)==FRAME_SIZE)?1:-1];

#include "actions.h"
typedef char action_ADV[(ADV==OP_ADV)?1:-1];
typedef char action_MARK[(MARK==OP_MARK)?1:-1];
typedef char action_JUMP[(JUMP==OP_JUMP)?1:-1];
typedef char action_LDI[(LDI==OP_LDI)?1:-1];
typedef char action_COPYW[(COPYW==OP_COPYW)?1:-1];
typedef char action_ALU[(ALU==OP_ALU)?1:-1];
typedef char action_ALUI[(ALUI==OP_ALUI)?1:-1];
typedef char action_CMP[(CMP==OP_CMP)?1:-1];
typedef char action_CMPI[(CMPI==OP_CMPI)?1:-1];
typedef char action_RLD[(RLD==OP_RLD)?1:-1];
typedef char action_LDX[(LDX==OP_LDX)?1:-1];
typedef char action_STX[(STX==OP_STX)?1:-1];
typedef char action_OUT[(OUT==OP_OUT)?1:-1];
typedef char action_OUTW[(OUTW==OP_OUTW)?1:-1];
typedef char action_COPY[(COPY==OP_COPY)?1:-1];
typedef char action_COPYT[(COPYT==OP_COPYT)?1:-1];
typedef char action_SPAN[(SPAN==OP_SPAN)?1:-1];
typedef char action_SPANT[(SPANT==OP_SPANT)?1:-1];
typedef char action_SPAN2[(SPAN2==OP_SPAN2)?1:-1];
typedef char action_OLAST[(OLAST==OP_OLAST)?1:-1];
typedef char action_ODROP[(ODROP==OP_ODROP)?1:-1];
typedef char action_OLEN[(OLEN==OP_OLEN)?1:-1];
typedef char action_OCUT[(OCUT==OP_OCUT)?1:-1];
typedef char action_ORES[(ORES==OP_ORES)?1:-1];
typedef char action_OFILL[(OFILL==OP_OFILL)?1:-1];
typedef char action_OCLR[(OCLR==OP_OCLR)?1:-1];
typedef char action_OSEL[(OSEL==OP_OSEL)?1:-1];
typedef char action_SETOT[(SETOT==OP_SETOT)?1:-1];
typedef char action_XATTR[(XATTR==OP_XATTR)?1:-1];
typedef char action_PUSH[(PUSH==OP_PUSH)?1:-1];
typedef char action_POP[(POP==OP_POP)?1:-1];
typedef char action_INTERN[(INTERN==OP_INTERN)?1:-1];
typedef char action_BLOBSAVE[(BLOBSAVE==OP_BLOBSAVE)?1:-1];
typedef char action_INPUSH[(INPUSH==OP_INPUSH)?1:-1];
typedef char action_INPUSHX[(INPUSHX==OP_INPUSHX)?1:-1];
typedef char action_INPUSHXE[(INPUSHXE==OP_INPUSHXE)?1:-1];
typedef char action_INPOP[(INPOP==OP_INPOP)?1:-1];
typedef char action_SBCLR[(SBCLR==OP_SBCLR)?1:-1];
typedef char action_SBOUT[(SBOUT==OP_SBOUT)?1:-1];
typedef char action_SBSPAN[(SBSPAN==OP_SBSPAN)?1:-1];
typedef char action_SBBLOB[(SBBLOB==OP_SBBLOB)?1:-1];
typedef char action_SBINTERN[(SBINTERN==OP_SBINTERN)?1:-1];
typedef char action_SBSAVE[(SBSAVE==OP_SBSAVE)?1:-1];
typedef char action_SBFIND[(SBFIND==OP_SBFIND)?1:-1];
typedef char action_BLEN[(BLEN==OP_BLEN)?1:-1];
typedef char action_BYTE[(BYTE==OP_BYTE)?1:-1];
typedef char action_XLEN[(XLEN==OP_XLEN)?1:-1];
typedef char action_DIVMOD10[(DIVMOD10==OP_DIVMOD10)?1:-1];
typedef char action_SWAP[(SWAP==OP_SWAP)?1:-1];
typedef char action_ACCEPT[(ACCEPT==OP_ACCEPT)?1:-1];
typedef char action_REJECT[(REJECT==OP_REJECT)?1:-1];
typedef char action_A64[(A64==OP_A64)?1:-1];
typedef char action_A64I[(A64I==OP_A64I)?1:-1];
typedef char action_C64[(C64==OP_C64)?1:-1];
typedef char action_C64U[(C64U==OP_C64U)?1:-1];
typedef char action_INC[(INC==OP_INC)?1:-1];
typedef char action_count[(NOP_==OP_COUNT)?1:-1];
SCHECK(CoreMachine,model,M_MODEL);
SCHECK(CoreMachine,result,M_RESULT);
SCHECK(CoreMachine,regs,M_REGS);
SCHECK(CoreMachine,input,M_INPUT);
SCHECK(CoreMachine,x,M_X);
SCHECK(CoreMachine,xattr,M_XATTR);
SCHECK(CoreMachine,xn,M_XN);
SCHECK(CoreMachine,r,M_R);
SCHECK(CoreMachine,ot,M_OT);
SCHECK(CoreMachine,osel,M_OSEL);
SCHECK(CoreMachine,status,M_STATUS);
SCHECK(CoreMachine,out,M_OUT);
SCHECK(CoreMachine,err,M_ERR);
SCHECK(CoreMachine,scratch,M_SCRATCH);
SCHECK(CoreMachine,stack,M_STACK);
SCHECK(CoreMachine,frames,M_FRAMES);
SCHECK(CoreMachine,memory,M_MEMORY);
SCHECK(CoreMachine,blobs,M_BLOBS);
SCHECK(CoreMachine,strings,M_STRINGS);
SCHECK(CoreMachine,resources,M_RESOURCES);
SCHECK(CoreResult,reason,RESULT_REASON); SCHECK(CoreResult,reason_n,RESULT_REASON_N);
SCHECK(CoreModel,str,CM_STR); SCHECK(CoreModel,strl,CM_STRL);
typedef char machine_size[(sizeof(CoreMachine)==MACHINE_SIZE)?1:-1];

SCHECK(CoreModel,nrg,CM_NRG); SCHECK(CoreModel,start,CM_START);
SCHECK(CoreModel,qoff,CM_QOFF); SCHECK(CoreModel,qlen,CM_QLEN); SCHECK(CoreModel,qa,CM_QA);
SCHECK(CoreResult,out,RESULT_OUT); SCHECK(CoreResult,err,RESULT_ERR);
typedef char result_size[(sizeof(CoreResult)==RESULT_SIZE)?1:-1];
