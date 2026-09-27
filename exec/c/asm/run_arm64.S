/* Complete generic execution lifecycle and transition loop. Models, original
   input and result storage are borrowed; accepted/error bytes transfer out. */
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
.text
.p2align 2
.macro FREE_MEMBER off
    ldr x0,[x19,#\off]
    bl CSYM(free)
.endm
.globl CSYM(core_run)
CSYM(core_run):
    /* x28 is reserved by the product bridge for its separate tape stack.
       Keep the sequence length in this frame across action/service calls. */
    stp x29,x30,[sp,#-432]!
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
    mov x0,x19
    mov w1,#0
    mov x2,#MACHINE_SIZE
    bl CSYM(memset)
    mov x0,x21
    mov w1,#0
    mov x2,#RESULT_SIZE
    bl CSYM(memset)
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
    mov x23,#0
    ldr w24,[x20,#CM_START]
Lrloop:
    cmp x23,x22
    b.ge Lrtimeout
    add x23,x23,#1
    ldr x8,[x20,#CM_MODE]
    ldr w8,[x8,x24,lsl #2]
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
    mov w8,#-1
    str w8,[sp,#376]
    str wzr,[sp,#380]
    mov x0,x20
    mov w1,w24
    add x3,sp,#376
    add x4,sp,#380
    bl CSYM(core_transition)
    str x0,[x21,#RESULT_REASON]
    cbnz x0,Lrtransitionbad
    ldr w24,[sp,#376]
    tbnz w24,#31,Lrmissing
    ldr w25,[sp,#380]
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
    bl CSYM(free)
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
    bl CSYM(free)
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
    bl CSYM(free)
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
    ldp x29,x30,[sp],#432
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
#ifndef __APPLE__
.section .note.GNU-stack,"",%progbits
#endif
