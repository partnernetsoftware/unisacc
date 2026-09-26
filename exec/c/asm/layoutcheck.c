/* Build-time assertions use the actual C ABI, not a copied struct. */
#include "../core.h"
#include "layout.h"
#define CHECK(field,off) typedef char check_##field[(offsetof(CoreModel,field)==off)?1:-1]
CHECK(ns,CM_NS); CHECK(nq,CM_NQ); CHECK(isnet,CM_ISNET);
CHECK(mode,CM_MODE); CHECK(count,CM_COUNT); CHECK(keys,CM_KEYS);
CHECK(next,CM_NEXT); CHECK(seq,CM_SEQ); CHECK(lo,CM_LO); CHECK(hi,CM_HI);
CHECK(base_next,CM_BASE_NEXT); CHECK(base_seq,CM_BASE_SEQ);
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
