/* Handwritten AArch64 implementation of core_transition. No calls, stack,
   model-specific constants or precomputed answer rows. Network mode evaluates
   every H(key-threshold) and accumulates signed weights in 64 bits.
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
    cbz w8,Lbounds
Lunit:
    ldr w14,[x11],#4
    cmp w2,w14
    ldrsw x15,[x12],#4
    ldrsw x16,[x13],#4
    csel x15,x15,xzr,ge
    csel x16,x16,xzr,ge
    add x9,x9,x15
    add x10,x10,x16
    subs w8,w8,#1
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
