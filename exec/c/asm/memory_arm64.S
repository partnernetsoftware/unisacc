/* Generic sparse 64-bit indexed memory; zero for absent keys. The same
   power-of-two hash, linear probe, half-load growth and overwrite as C. */
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
.macro HASH dst,key,tmp
    movz \tmp,#0x7c15
    movk \tmp,#0x7f4a,lsl #16
    movk \tmp,#0x79b9,lsl #32
    movk \tmp,#0x9e37,lsl #48
    mul \dst,\key,\tmp
    lsr \dst,\dst,#20
.endm
.text
.p2align 2
.globl CSYM(core_memory_get)
#ifndef __APPLE__
.type core_memory_get,%function
#endif
CSYM(core_memory_get):
    ldr x2,[x0,#MEM_CAP]
    cbz x2,Lmgzero
    sub x2,x2,#1
    HASH x4,x1,x3
    and x4,x4,x2
    ldr x5,[x0,#MEM_USED]
    ldr x6,[x0,#MEM_KEYS]
Lmgprobe:
    ldrb w7,[x5,x4]
    cbz w7,Lmgzero
    ldr x7,[x6,x4,lsl #3]
    cmp x7,x1
    b.eq Lmgfound
    add x4,x4,#1
    and x4,x4,x2
    b Lmgprobe
Lmgfound:
    ldr x0,[x0,#MEM_VALUES]
    ldr x0,[x0,x4,lsl #3]
    ret
Lmgzero:
    mov x0,#0
    ret
#ifndef __APPLE__
.size core_memory_get,.-core_memory_get
#endif
.p2align 2
.globl CSYM(core_memory_set)
#ifndef __APPLE__
.type core_memory_set,%function
#endif
CSYM(core_memory_set):
    stp x29,x30,[sp,#-80]!
    mov x29,sp
    stp x19,x20,[sp,#16]
    stp x21,x22,[sp,#32]
    stp x23,x24,[sp,#48]
    stp x25,x26,[sp,#64]
    mov x19,x0
    mov x20,x1
    mov x21,x2
    ldr x22,[x19,#MEM_CAP]
    ldr x3,[x19,#MEM_COUNT]
    add x3,x3,#1
    lsl x3,x3,#1
    cmp x3,x22
    b.ls Lmsinsert
    lsr x3,x22,#60
    cbnz x3,Lmsoverflow
    ldr x23,[x19,#MEM_KEYS]
    ldr x24,[x19,#MEM_VALUES]
    ldr x25,[x19,#MEM_USED]
    lsl x0,x22,#1
    cbnz x0,Lmscap
    mov x0,#65536
Lmscap:
    str x0,[x19,#MEM_CAP]
    mov x1,#8
    bl CSYM(calloc)
    str x0,[x19,#MEM_KEYS]
    ldr x0,[x19,#MEM_CAP]
    mov x1,#8
    bl CSYM(calloc)
    str x0,[x19,#MEM_VALUES]
    ldr x0,[x19,#MEM_CAP]
    mov x1,#1
    bl CSYM(calloc)
    str x0,[x19,#MEM_USED]
    str xzr,[x19,#MEM_COUNT]
    cbz x0,Lmsoom
    ldr x0,[x19,#MEM_KEYS]
    cbz x0,Lmsoom
    ldr x0,[x19,#MEM_VALUES]
    cbz x0,Lmsoom
    mov x26,#0
Lmsrehash:
    cmp x26,x22
    b.hs Lmsfree
    ldrb w3,[x25,x26]
    cbz w3,Lmsnext
    mov x0,x19
    ldr x1,[x23,x26,lsl #3]
    ldr x2,[x24,x26,lsl #3]
    bl CSYM(core_memory_set)
Lmsnext:
    add x26,x26,#1
    b Lmsrehash
Lmsfree:
    mov x0,x23
    bl CSYM(free)
    mov x0,x24
    bl CSYM(free)
    mov x0,x25
    bl CSYM(free)
Lmsinsert:
    ldr x3,[x19,#MEM_CAP]
    sub x3,x3,#1
    HASH x5,x20,x4
    and x5,x5,x3
    ldr x6,[x19,#MEM_USED]
    ldr x7,[x19,#MEM_KEYS]
Lmsprobe:
    ldrb w8,[x6,x5]
    cbz w8,Lmsnew
    ldr x8,[x7,x5,lsl #3]
    cmp x8,x20
    b.eq Lmsvalue
    add x5,x5,#1
    and x5,x5,x3
    b Lmsprobe
Lmsnew:
    mov w8,#1
    strb w8,[x6,x5]
    str x20,[x7,x5,lsl #3]
    ldr x8,[x19,#MEM_COUNT]
    add x8,x8,#1
    str x8,[x19,#MEM_COUNT]
Lmsvalue:
    ldr x7,[x19,#MEM_VALUES]
    str x21,[x7,x5,lsl #3]
    mov x0,#0
    b Lmsreturn
Lmsoom:
#ifdef __APPLE__
    adrp x0,Lmsoom_text@PAGE
    add x0,x0,Lmsoom_text@PAGEOFF
#else
    adrp x0,Lmsoom_text
    add x0,x0,:lo12:Lmsoom_text
#endif
    b Lmsreturn
Lmsoverflow:
#ifdef __APPLE__
    adrp x0,Lmsoverflow_text@PAGE
    add x0,x0,Lmsoverflow_text@PAGEOFF
#else
    adrp x0,Lmsoverflow_text
    add x0,x0,:lo12:Lmsoverflow_text
#endif
Lmsreturn:
    ldp x19,x20,[sp,#16]
    ldp x21,x22,[sp,#32]
    ldp x23,x24,[sp,#48]
    ldp x25,x26,[sp,#64]
    ldp x29,x30,[sp],#80
    cbnz x0,Lmspanic
    ret
Lmspanic:
    b CSYM(core_host_panic)
#ifndef __APPLE__
.size core_memory_set,.-core_memory_set
.section .rodata
#else
.section __TEXT,__cstring,cstring_literals
#endif
Lmsoom_text: .asciz "out of memory"
Lmsoverflow_text: .asciz "memory capacity overflow"
#ifndef __APPLE__
.section .note.GNU-stack,"",%progbits
#endif
