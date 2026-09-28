/* Handwritten AArch64 implementation of core_transition. No calls, stack,
   model-specific constants or precomputed answer rows. Thresholds ascend (the
   loader rejects any other bank), so the active units H(key-threshold)=1 are a
   prefix: a branchless binary search finds its length, then exactly those
   signed weights accumulate in 64 bits. A declared return (ret_ok[q][key])
   answers (key, ret_seq[q]) first: control flow, not a network decision.
   x0=model, w1=state, w2=key, x3=&next, x4=&sequence; x0=error or NULL.
   Only caller-saved x0-x17 are touched; x18 is never used. */
#include "layout.h"
#ifdef _WIN32
#error Windows_ARM64_object_binding_not_implemented
#endif
#ifdef __APPLE__
#define CNAME(x) _##x
#else
#define CNAME(x) x
#endif
.text
.p2align 2
.globl CNAME(core_transition)
#ifndef __APPLE__
.type core_transition,%function
#endif
CNAME(core_transition):
    uxtw x1,w1
    sxtw x2,w2
    mov w5,#-1
    str w5,[x3]
    str wzr,[x4]
    ldr x5,[x0,#CM_RET_OK]
    cbz x5,Lnoret
    ldr x5,[x5,x1,lsl #3]
    cbz x5,Lnoret
    tbnz w2,#31,Lnoret
    ldr w6,[x0,#CM_NS]
    cmp w2,w6
    b.ge Lnoret
    ldrb w6,[x5,x2]
    cbz w6,Lnoret
    ldr x5,[x0,#CM_RET_SEQ]
    ldr w10,[x5,x1,lsl #2]
    mov w9,w2
    b Lstore
Lnoret:
    ldr w5,[x0,#CM_ISNET]
    cbz w5,Ltable
    ldr x5,[x0,#CM_LO]
    ldr w6,[x5,x1,lsl #2]
    cmp w2,w6
    b.lt Ldomain
    ldr x5,[x0,#CM_HI]
    ldr w6,[x5,x1,lsl #2]
    cmp w2,w6
    b.gt Ldomain
    ldr x5,[x0,#CM_BASE_NEXT]
    ldrsw x9,[x5,x1,lsl #2]
    ldr x5,[x0,#CM_BASE_SEQ]
    ldrsw x10,[x5,x1,lsl #2]
    ldr x5,[x0,#CM_COUNT]
    ldr w8,[x5,x1,lsl #2]
    ldr x5,[x0,#CM_KEYS]
    ldr x11,[x5,x1,lsl #3]
    ldr x5,[x0,#CM_NEXT]
    ldr x12,[x5,x1,lsl #3]
    ldr x5,[x0,#CM_SEQ]
    ldr x13,[x5,x1,lsl #3]
    mov w6,#0
Lsearch:
    cmp w6,w8
    b.hs Lsum
    add w7,w6,w8
    lsr w7,w7,#1
    ldr w14,[x11,w7,uxtw #2]
    add w15,w7,#1
    cmp w2,w14
    csel w6,w15,w6,ge
    csel w8,w8,w7,ge
    b Lsearch
Lsum:
    cbz w6,Lbounds
Lunit:
    ldrsw x15,[x12],#4
    ldrsw x16,[x13],#4
    add x9,x9,x15
    add x10,x10,x16
    subs w6,w6,#1
    b.ne Lunit
Lbounds:
    cmn x9,#1
    b.lt Linvalid
    ldrsw x5,[x0,#CM_NS]
    cmp x9,x5
    b.ge Linvalid
    cmp x10,#0
    b.lt Linvalid
    ldrsw x5,[x0,#CM_NQ]
    cmp x10,x5
    b.ge Linvalid
Lstore:
    str w9,[x3]
    str w10,[x4]
Ldone:
    mov x0,#0
    ret
Ltable:
    ldr x5,[x0,#CM_MODE]
    ldr w6,[x5,x1,lsl #2]
    ldr x5,[x0,#CM_NEXT]
    ldr x12,[x5,x1,lsl #3]
    ldr x5,[x0,#CM_SEQ]
    ldr x13,[x5,x1,lsl #3]
    cmp w6,#1
    b.ne Ldirect
    ldr x5,[x0,#CM_COUNT]
    ldr w8,[x5,x1,lsl #2]
    ldr x5,[x0,#CM_KEYS]
    ldr x11,[x5,x1,lsl #3]
    mov x7,#0
    cbz w8,Ldone
Lfind:
    ldr w6,[x11,x7,lsl #2]
    cmp w2,w6
    b.eq Lfound
    add x7,x7,#1
    subs w8,w8,#1
    b.ne Lfind
    b Ldone
Ldirect:
    mov x7,x2
Lfound:
    ldr w9,[x12,x7,lsl #2]
    ldr w10,[x13,x7,lsl #2]
    b Lstore
Ldomain:
#ifdef __APPLE__
    adrp x0,Ldomain_text@PAGE
    add x0,x0,Ldomain_text@PAGEOFF
#else
    adrp x0,Ldomain_text
    add x0,x0,:lo12:Ldomain_text
#endif
    ret
Linvalid:
#ifdef __APPLE__
    adrp x0,Linvalid_text@PAGE
    add x0,x0,Linvalid_text@PAGEOFF
#else
    adrp x0,Linvalid_text
    add x0,x0,:lo12:Linvalid_text
#endif
    ret
#ifndef __APPLE__
.size core_transition,.-core_transition
.section .rodata
#else
.section __TEXT,__cstring,cstring_literals
#endif
Ldomain_text:
    .asciz "observation outside network domain"
Linvalid_text:
    .asciz "invalid network output"
#ifndef __APPLE__
.section .note.GNU-stack,"",%progbits
#endif
