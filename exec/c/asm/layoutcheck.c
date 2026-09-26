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
