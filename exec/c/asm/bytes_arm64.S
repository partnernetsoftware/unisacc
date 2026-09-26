/* Owned byte blocks and opaque resource cache. Blob 0 is reserved by the
   caller. Host fetch returns absent=0, borrowed=1 or malloc-owned=2. */
#include "layout.h"
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
.globl CSYM(core_blob_add)
#ifndef __APPLE__
.type core_blob_add,%function
#endif
CSYM(core_blob_add):
    stp x29,x30,[sp,#-48]!
    mov x29,sp
    stp x19,x20,[sp,#16]
    stp x21,x22,[sp,#32]
    mov x19,x0
    mov x20,x1
    mov w21,w2
    ldp w3,w4,[x19,#BLOBS_COUNT]
    cmp w3,w4
    b.lt Lblslot
    mov w5,#0x3fffffff
    cmp w4,w5
    b.gt Lbloverflow
    lsl w4,w4,#1
    cbnz w4,Lblgrow
    mov w4,#64
Lblgrow:
    str w4,[x19,#BLOBS_CAP]
    ldr x0,[x19,#BLOBS_ENTRIES]
    lsl x1,x4,#4
    bl CSYM(realloc)
    cbz x0,Lblnomem
    str x0,[x19,#BLOBS_ENTRIES]
Lblslot:
    ldr w3,[x19,#BLOBS_COUNT]
    ldr x22,[x19,#BLOBS_ENTRIES]
    add x22,x22,x3,lsl #4
    mov x0,#0
    add x1,x21,#1
    bl CSYM(realloc)
    cbz x0,Lblnomem
    str x0,[x22,#BLOB_BYTES]
    mov x1,x20
    mov w2,w21
    bl CSYM(memcpy)
    str w21,[x22,#BLOB_LENGTH]
    ldr w0,[x19,#BLOBS_COUNT]
    add w3,w0,#1
    str w3,[x19,#BLOBS_COUNT]
    mov x1,#0
    b Lblreturn
Lblnomem:
#ifdef __APPLE__
    adrp x0,Lbytesoom@PAGE
    add x0,x0,Lbytesoom@PAGEOFF
#else
    adrp x0,Lbytesoom
    add x0,x0,:lo12:Lbytesoom
#endif
    b Lblerror
Lbloverflow:
#ifdef __APPLE__
    adrp x0,Lbloboverflow@PAGE
    add x0,x0,Lbloboverflow@PAGEOFF
#else
    adrp x0,Lbloboverflow
    add x0,x0,:lo12:Lbloboverflow
#endif
Lblerror:
    mov x1,#1
Lblreturn:
    ldp x19,x20,[sp,#16]
    ldp x21,x22,[sp,#32]
    ldp x29,x30,[sp],#48
    cbnz x1,Lbytespanic
    ret
#ifndef __APPLE__
.size core_blob_add,.-core_blob_add
#endif
.p2align 2
.globl CSYM(core_resource_find)
#ifndef __APPLE__
.type core_resource_find,%function
#endif
CSYM(core_resource_find):
    stp x29,x30,[sp,#-96]!
    mov x29,sp
    stp x19,x20,[sp,#16]
    stp x21,x22,[sp,#32]
    stp x23,x24,[sp,#48]
    stp x25,x26,[sp,#64]
    mov x19,x0
    mov x20,x1
    mov x21,x2
    mov w22,w3
    mov w23,#0
Lrfscan:
    ldr w3,[x19,#RES_COUNT]
    cmp w23,w3
    b.ge Lrfmiss
    ldr x24,[x19,#RES_ENTRIES]
    add x24,x24,x23,lsl #4
    ldr w3,[x24,#RES_LENGTH]
    cmp w3,w22
    b.ne Lrfnext
    ldr x0,[x24,#RES_KEY]
    mov x1,x21
    mov w2,w22
    bl CSYM(memcmp)
    cbz w0,Lrffound
Lrfnext:
    add w23,w23,#1
    b Lrfscan
Lrffound:
    ldr w0,[x24,#RES_ID]
    mov x1,#0
    b Lrfreturn
Lrfmiss:
    mov w3,#0x7fffffff
    cmp w23,w3
    b.eq Lrfoverflow
    str xzr,[sp,#80]
    str wzr,[sp,#88]
    mov x0,x21
    mov w1,w22
    add x2,sp,#80
    add x3,sp,#88
    bl CSYM(core_host_fetch)
    mov w25,w0
    mov w26,#0
    cbz w25,Lrfcache
    mov x0,x20
    ldr x1,[sp,#80]
    ldr w2,[sp,#88]
    bl CSYM(core_blob_add)
    mov w26,w0
    cmp w25,#2
    b.ne Lrfcache
    ldr x0,[sp,#80]
    bl CSYM(free)
Lrfcache:
    ldr x0,[x19,#RES_ENTRIES]
    add x1,x23,#1
    lsl x1,x1,#4
    bl CSYM(realloc)
    cbz x0,Lrfnomem
    str x0,[x19,#RES_ENTRIES]
    add x24,x0,x23,lsl #4
    mov x0,#0
    add x1,x22,#1
    bl CSYM(realloc)
    cbz x0,Lrfnomem
    str x0,[x24,#RES_KEY]
    mov x1,x21
    mov w2,w22
    bl CSYM(memcpy)
    str w22,[x24,#RES_LENGTH]
    str w26,[x24,#RES_ID]
    add w3,w23,#1
    str w3,[x19,#RES_COUNT]
    mov w0,w26
    mov x1,#0
    b Lrfreturn
Lrfnomem:
#ifdef __APPLE__
    adrp x0,Lbytesoom@PAGE
    add x0,x0,Lbytesoom@PAGEOFF
#else
    adrp x0,Lbytesoom
    add x0,x0,:lo12:Lbytesoom
#endif
    b Lrferror
Lrfoverflow:
#ifdef __APPLE__
    adrp x0,Lresoverflow@PAGE
    add x0,x0,Lresoverflow@PAGEOFF
#else
    adrp x0,Lresoverflow
    add x0,x0,:lo12:Lresoverflow
#endif
Lrferror:
    mov x1,#1
Lrfreturn:
    ldp x19,x20,[sp,#16]
    ldp x21,x22,[sp,#32]
    ldp x23,x24,[sp,#48]
    ldp x25,x26,[sp,#64]
    ldp x29,x30,[sp],#96
    cbnz x1,Lbytespanic
    ret
Lbytespanic:
    b CSYM(core_host_panic)
#ifndef __APPLE__
.size core_resource_find,.-core_resource_find
.section .rodata
#else
.section __TEXT,__cstring,cstring_literals
#endif
Lbytesoom: .asciz "out of memory"
Lbloboverflow: .asciz "blob capacity overflow"
Lresoverflow: .asciz "resource cache capacity overflow"
#ifndef __APPLE__
.section .note.GNU-stack,"",%progbits
#endif
