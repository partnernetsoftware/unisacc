/* Prototype v0.0.97 AArch64 core_run: 64-byte CorePackedState (PS_* from core-abi.tsv) + inlined transition.
 * When m->packed != NULL (formal host-built path), zero extra calloc at entry.
 * When m->packed == NULL (prototype harness on 0.0.93 binary), builds temporary table and frees on exit.
 * x18 is never touched; x28 is reserved by the product bridge. */
#include "layout.h"
#include "../core.h"
#ifdef _WIN32
#error Windows_ARM64_object_binding_not_implemented
#endif
#ifdef __APPLE__
#define CSYM1(x) _##x
#define CSYM(x) CSYM1(x)
#else
#define CSYM(x) x
#endif
#if (MACHINE_SIZE % 8) != 0 || (RESULT_SIZE % 8) != 0 || PS_SIZE != 64
#error ABI_alignment_or_PS_SIZE_mismatch
#endif
.text
.p2align 2
Lrfree_if:
    cbz x0,Lrfree_skip
    b CSYM(free)
Lrfree_skip:
    ret
.p2align 2
Lrzero:
    str xzr,[x0]
    add x0,x0,#8
    subs w1,w1,#8
    b.ne Lrzero
    ret
.p2align 2
.macro FREE_MEMBER off
    ldr x0,[x19,#\off]
    bl Lrfree_if
.endm
.globl CSYM(core_run)
CSYM(core_run):
    stp x29,x30,[sp,#-448]!
    mov x29,sp
    stp x19,x20,[sp,#16]
    stp x21,x22,[sp,#32]
    stp x23,x24,[sp,#48]
    stp x25,x26,[sp,#64]
    stp x27,x28,[sp,#80]
    add x19,sp,#96
    mov x20,x0
    mov x21,x5
    mov x22,x4
    mov x23,x1
    mov w24,w2
    mov x25,x3
    str xzr,[sp,#424]
    str xzr,[sp,#432]
    mov x0,x19
    mov w1,#MACHINE_SIZE
    bl Lrzero
    mov x0,x21
    mov w1,#RESULT_SIZE
    bl Lrzero
    stp x20,x21,[x19,#M_MODEL]
    str x23,[x19,#M_INPUT]
    str x23,[x19,#M_X]
    ldr w0,[x20,#CM_NRG]
    add w0,w0,#1
    mov x1,#8
    bl CSYM(calloc)
    cbz x0,Lroom
    str x0,[x19,#M_REGS]
    add x0,x19,#M_BLOBS
#ifdef __APPLE__
    adrp x1,Lrempty@PAGE
    add x1,x1,Lrempty@PAGEOFF
#else
    adrp x1,Lrempty
    add x1,x1,:lo12:Lrempty
#endif
    mov w2,#0
    bl CSYM(core_blob_add)
    mov x0,x25
    bl CSYM(strlen)
    mov w2,w0
    mov x1,x25
    add x0,x19,#M_BLOBS
    bl CSYM(core_blob_add)
    add w0,w24,#1
    mov x1,#8
    bl CSYM(calloc)
    cbz x0,Lroom
    str x0,[x19,#M_XATTR]
    str x24,[x19,#M_XN]
    stp x23,x0,[sp,#384]
    stp xzr,x24,[sp,#400]
    add x0,x19,#M_FRAMES
    add x1,sp,#384
    bl CSYM(core_frame_push)
    /* Resolve CorePackedState table: use host CM_PACKED if present, else build prototype table */
    ldr x0,[x20,#CM_PACKED]
    cbnz x0,Lpack_ready
    ldr w0,[x20,#CM_NS]
    cmp w0,#0
    b.le Lpack_ready
    mov x1,#PS_SIZE
    bl CSYM(calloc)
    cbz x0,Lroom
    mov x1,#1
    str x1,[sp,#432]
    mov x2,x0
    mov w3,#0
    ldr w4,[x20,#CM_NS]
Lpack_fill:
    cmp w3,w4
    b.ge Lpack_done
    uxtw x5,w3
    ldr x6,[x20,#CM_MODE]
    ldr w7,[x6,x5,lsl #2]
    str w7,[x2,#PS_MODE]
    ldr x6,[x20,#CM_COUNT]
    cbz x6,Lp_nc
    ldr w7,[x6,x5,lsl #2]
    str w7,[x2,#PS_COUNT]
Lp_nc:
    ldr x6,[x20,#CM_LO]
    cbz x6,Lp_nl
    ldr w7,[x6,x5,lsl #2]
    str w7,[x2,#PS_LO]
Lp_nl:
    ldr x6,[x20,#CM_HI]
    cbz x6,Lp_nh
    ldr w7,[x6,x5,lsl #2]
    str w7,[x2,#PS_HI]
Lp_nh:
    ldr x6,[x20,#CM_BASE_NEXT]
    cbz x6,Lp_nbn
    ldr w7,[x6,x5,lsl #2]
    str w7,[x2,#PS_BASE_NEXT]
Lp_nbn:
    ldr x6,[x20,#CM_BASE_SEQ]
    cbz x6,Lp_nbs
    ldr w7,[x6,x5,lsl #2]
    str w7,[x2,#PS_BASE_SEQ]
Lp_nbs:
    ldr x6,[x20,#CM_RET_SEQ]
    cbz x6,Lp_nrs
    ldr w7,[x6,x5,lsl #2]
    str w7,[x2,#PS_RET_SEQ]
Lp_nrs:
    ldr x6,[x20,#CM_RET_OK]
    cbz x6,Lp_nro
    ldr x7,[x6,x5,lsl #3]
    str x7,[x2,#PS_RET_OK]
Lp_nro:
    ldr x6,[x20,#CM_KEYS]
    cbz x6,Lp_nk
    ldr x7,[x6,x5,lsl #3]
    str x7,[x2,#PS_KEYS]
Lp_nk:
    ldr x6,[x20,#CM_NEXT]
    cbz x6,Lp_nn
    ldr x7,[x6,x5,lsl #3]
    str x7,[x2,#PS_NEXT]
Lp_nn:
    ldr x6,[x20,#CM_SEQ]
    cbz x6,Lp_nsq
    ldr x7,[x6,x5,lsl #3]
    str x7,[x2,#PS_SEQ]
Lp_nsq:
    add x2,x2,#PS_SIZE
    add w3,w3,#1
    b Lpack_fill
Lpack_done:
Lpack_ready:
    str x0,[sp,#424]
    mov x23,#0
    ldr w24,[x20,#CM_START]
.p2align 4
Lrloop:
    cmp x23,x22
    b.ge Lrtimeout
    add x23,x23,#1
    ldr x5,[sp,#424]
    uxtw x1,w24
    add x5,x5,x1,lsl #6
    ldr w8,[x5,#PS_MODE]
    cmp w8,#1
    b.eq Lrstack
    cbnz w8,Lrresult
    ldr x8,[x19,#M_FRAMES+STACK_ENTRIES]
    ldr w9,[x19,#M_FRAMES+STACK_COUNT]
    sub w9,w9,#1
    add x8,x8,x9,lsl #5
    ldp x9,x10,[x8,#FRAME_CURSOR]
    mov w2,#256
    cmp x9,x10
    b.ge Lrkey
    ldr x8,[x8,#FRAME_BYTES]
    ldrb w2,[x8,x9]
    b Lrkey
Lrstack:
    mov w2,#-1
    ldr w8,[x19,#M_STACK+STACK_COUNT]
    cbz w8,Lrkey
    sub w8,w8,#1
    ldr x9,[x19,#M_STACK+STACK_ENTRIES]
    ldr w2,[x9,x8,lsl #2]
    b Lrkey
Lrresult:
    ldr x8,[x19,#M_R]
    mov x9,#256
    cmp x8,x9
    csel x2,x8,x9,ls
Lrkey:
    /* Inlined transition reading from single 64-byte CorePackedState at [x5] */
    sxtw x2,w2
    ldr x6,[x5,#PS_RET_OK]
    cbz x6,Ltnoret
    tbnz w2,#31,Ltnoret
    ldr w7,[x20,#CM_NS]
    cmp w2,w7
    b.ge Ltnoret
    ldrb w7,[x6,x2]
    cbz w7,Ltnoret
    ldr w25,[x5,#PS_RET_SEQ]
    mov w24,w2
    b Ltdispatch
Ltnoret:
    ldr w6,[x20,#CM_ISNET]
    cbz w6,Lttable
    ldr w6,[x5,#PS_LO]
    cmp w2,w6
    b.lt Ltdomain
    ldr w6,[x5,#PS_HI]
    cmp w2,w6
    b.gt Ltdomain
    ldrsw x9,[x5,#PS_BASE_NEXT]
    ldrsw x10,[x5,#PS_BASE_SEQ]
    ldr w8,[x5,#PS_COUNT]
    cbz w8,Ltbounds
    ldr x11,[x5,#PS_KEYS]
    mov w6,#0
Ltsearch:
    cmp w6,w8
    b.hs Ltsum
    add w7,w6,w8
    lsr w7,w7,#1
    ldr w14,[x11,w7,uxtw #2]
    add w15,w7,#1
    cmp w2,w14
    csel w6,w15,w6,ge
    csel w8,w8,w7,ge
    b Ltsearch
Ltsum:
    cbz w6,Ltbounds
    ldr x12,[x5,#PS_NEXT]
    ldr x13,[x5,#PS_SEQ]
Ltunit:
    ldrsw x15,[x12],#4
    ldrsw x16,[x13],#4
    add x9,x9,x15
    add x10,x10,x16
    subs w6,w6,#1
    b.ne Ltunit
Ltbounds:
    cmn x9,#1
    b.lt Ltinvalid
    ldrsw x6,[x20,#CM_NS]
    cmp x9,x6
    b.ge Ltinvalid
    cmp x10,#0
    b.lt Ltinvalid
    ldrsw x6,[x20,#CM_NQ]
    cmp x10,x6
    b.ge Ltinvalid
    mov w24,w9
    mov w25,w10
    b Ltdispatch
Lttable:
    ldr w6,[x5,#PS_MODE]
    ldr x12,[x5,#PS_NEXT]
    ldr x13,[x5,#PS_SEQ]
    cmp w6,#1
    b.ne Ltdirect
    ldr w8,[x5,#PS_COUNT]
    ldr x11,[x5,#PS_KEYS]
    mov x7,#0
    cbz w8,Lrmissing
Ltfind:
    ldr w6,[x11,x7,lsl #2]
    cmp w2,w6
    b.eq Ltfound
    add x7,x7,#1
    subs w8,w8,#1
    b.ne Ltfind
    b Lrmissing
Ltdirect:
    mov x7,x2
Ltfound:
    ldr w24,[x12,x7,lsl #2]
    ldr w25,[x13,x7,lsl #2]
Ltdispatch:
    tbnz w24,#31,Lrmissing
    ldr x8,[x20,#CM_QOFF]
    ldrsw x8,[x8,x25,lsl #2]
    ldr x26,[x20,#CM_QA]
    add x26,x26,x8,lsl #3
    ldr x8,[x20,#CM_QLEN]
    ldr w8,[x8,x25,lsl #2]
    str w8,[sp,#416]
    mov w27,#0
Lractions:
    ldr w8,[sp,#416]
    cmp w27,w8
    b.ge Lrloop
    mov x0,x19
    mov x1,x26
    bl CSYM(core_action)
    cbnz w0,Lrfinished
    ldr w8,[x26]
    adr x9,Lrarity
    ldrb w8,[x9,x8]
    add w8,w8,#1
    add x26,x26,x8,lsl #3
    add w27,w27,#1
    b Lractions
Ltdomain:
#ifdef __APPLE__
    adrp x8,Ltdomain_text@PAGE
    add x8,x8,Ltdomain_text@PAGEOFF
#else
    adrp x8,Ltdomain_text
    add x8,x8,:lo12:Ltdomain_text
#endif
    str x8,[x21,#RESULT_REASON]
    b Lrtransitionbad
Ltinvalid:
#ifdef __APPLE__
    adrp x8,Ltinvalid_text@PAGE
    add x8,x8,Ltinvalid_text@PAGEOFF
#else
    adrp x8,Ltinvalid_text
    add x8,x8,:lo12:Ltinvalid_text
#endif
    str x8,[x21,#RESULT_REASON]
    b Lrtransitionbad
Lrtimeout:
#ifdef __APPLE__
    adrp x8,Lrtime@PAGE
    add x8,x8,Lrtime@PAGEOFF
#else
    adrp x8,Lrtime
    add x8,x8,:lo12:Lrtime
#endif
    str x8,[x21,#RESULT_REASON]
    mov w8,#3
    str w8,[x19,#M_STATUS]
    b Lrfinished
Lrmissing:
#ifdef __APPLE__
    adrp x8,Lrnone@PAGE
    add x8,x8,Lrnone@PAGEOFF
#else
    adrp x8,Lrnone
    add x8,x8,:lo12:Lrnone
#endif
    str x8,[x21,#RESULT_REASON]
Lrtransitionbad:
    mov w8,#2
    str w8,[x19,#M_STATUS]
Lrfinished:
    ldr x8,[sp,#432]
    cbz x8,Lpack_nofree
    ldr x0,[sp,#424]
    bl Lrfree_if
Lpack_nofree:
    str x23,[x21,#RESULT_STEPS]
    ldr w8,[x19,#M_STATUS]
    cbnz w8,Lrerrorout
    ldr x8,[x19,#M_OUT+BUF_BYTES]
    ldr w9,[x19,#M_OUT+BUF_LENGTH]
    str x8,[x21,#RESULT_OUT+BUF_BYTES]
    str w9,[x21,#RESULT_OUT+BUF_LENGTH]
    str xzr,[x19,#M_OUT+BUF_BYTES]
Lrerrorout:
    ldr x8,[x19,#M_ERR+BUF_BYTES]
    ldr w9,[x19,#M_ERR+BUF_LENGTH]
    str x8,[x21,#RESULT_ERR+BUF_BYTES]
    str w9,[x21,#RESULT_ERR+BUF_LENGTH]
    str xzr,[x19,#M_ERR+BUF_BYTES]
    FREE_MEMBER M_OUT+BUF_BYTES
    FREE_MEMBER M_OUT+BUF_ATTR
    FREE_MEMBER M_ERR+BUF_BYTES
    FREE_MEMBER M_ERR+BUF_ATTR
    FREE_MEMBER M_SCRATCH+BUF_BYTES
    FREE_MEMBER M_SCRATCH+BUF_ATTR
    FREE_MEMBER M_FRAMES+STACK_ENTRIES
    FREE_MEMBER M_STACK+STACK_ENTRIES
    ldr x0,[x19,#M_X]
    ldr x8,[x19,#M_INPUT]
    cmp x0,x8
    b.eq Lrkeepinput
    bl Lrfree_if
Lrkeepinput:
    FREE_MEMBER M_XATTR
    FREE_MEMBER M_REGS
    FREE_MEMBER M_MEMORY+MEM_KEYS
    FREE_MEMBER M_MEMORY+MEM_VALUES
    FREE_MEMBER M_MEMORY+MEM_USED
    ldr w24,[x19,#M_BLOBS+BLOBS_COUNT]
    ldr x26,[x19,#M_BLOBS+BLOBS_ENTRIES]
    cbz w24,Lrblobsdone
Lrblobs:
    ldr x0,[x26,#BLOB_BYTES]
    bl Lrfree_if
    add x26,x26,#BLOB_SIZE
    subs w24,w24,#1
    b.ne Lrblobs
Lrblobsdone:
    FREE_MEMBER M_BLOBS+BLOBS_ENTRIES
    ldr x24,[x19,#M_STRINGS+INTERN_CAP]
    ldr x26,[x19,#M_STRINGS+INTERN_ENTRIES]
    cbz x24,Lrstringsdone
Lrstrings:
    ldr x0,[x26,#ENTRY_BYTES]
    cbz x0,Lrstringnext
    bl CSYM(free)
Lrstringnext:
    add x26,x26,#ENTRY_SIZE
    subs x24,x24,#1
    b.ne Lrstrings
Lrstringsdone:
    FREE_MEMBER M_STRINGS+INTERN_ENTRIES
    ldr w24,[x19,#M_RESOURCES+RES_COUNT]
    ldr x26,[x19,#M_RESOURCES+RES_ENTRIES]
    cbz w24,Lrresourcesdone
Lrresources:
    ldr x0,[x26,#RES_KEY]
    bl Lrfree_if
    add x26,x26,#RES_ENTRY_SIZE
    subs w24,w24,#1
    b.ne Lrresources
Lrresourcesdone:
    FREE_MEMBER M_RESOURCES+RES_ENTRIES
    ldr w0,[x19,#M_STATUS]
Lrreturn:
    ldp x27,x28,[sp,#80]
    ldp x25,x26,[sp,#64]
    ldp x23,x24,[sp,#48]
    ldp x21,x22,[sp,#32]
    ldp x19,x20,[sp,#16]
    ldp x29,x30,[sp],#448
    ret
Lroom:
#ifdef __APPLE__
    adrp x0,Lroommsg@PAGE
    add x0,x0,Lroommsg@PAGEOFF
#else
    adrp x0,Lroommsg
    add x0,x0,:lo12:Lroommsg
#endif
    bl CSYM(core_host_panic)
    brk #0
.p2align 2
Lrarity:
    .byte CORE_ACTION_ARITIES
#ifdef __APPLE__
.section __TEXT,__cstring,cstring_literals
#else
.section .rodata
#endif
Lrempty: .asciz ""
Lrtime: .asciz "timeout"
Lrnone: .asciz "no transition"
Lroommsg: .asciz "out of memory"
Ltdomain_text: .asciz "observation outside network domain"
Ltinvalid_text: .asciz "invalid network output"
#ifndef __APPLE__
.section .note.GNU-stack,"",%progbits
#endif
