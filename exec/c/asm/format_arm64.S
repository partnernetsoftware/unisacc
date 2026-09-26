/* Signed decimal and right-aligned field fill; no language operations. */
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
.globl CSYM(core_decimal)
CSYM(core_decimal):
    sub sp,sp,#32
    mov x2,x0
    tbz x1,#63,Ldpositive
    mov w3,#45
    strb w3,[x2],#1
    neg x1,x1
Ldpositive:
    mov x3,sp
    mov x4,#10
Lddigit:
    udiv x5,x1,x4
    msub x6,x5,x4,x1
    add w6,w6,#48
    strb w6,[x3],#1
    mov x1,x5
    cbnz x1,Lddigit
    mov x7,sp
Ldreverse:
    ldrb w6,[x3,#-1]!
    strb w6,[x2],#1
    cmp x3,x7
    b.ne Ldreverse
    sub x0,x2,x0
    add sp,sp,#32
    ret
.globl CSYM(core_field_fill)
CSYM(core_field_fill):
    stp x29,x30,[sp,#-80]!
    mov x29,sp
    stp x19,x20,[sp,#16]
    stp x21,x22,[sp,#32]
    mov x19,x0
    mov x20,x1
    mov x21,x2
    mov x1,x3
    add x0,sp,#48
    bl CSYM(core_decimal)
    cmp x0,x21
    b.gt Lfoverflow
    tbnz x20,#63,Lfpanic
    ldrsw x8,[x19,#BUF_LENGTH]
    cmp x20,x8
    b.gt Lfpanic
    sub x8,x8,x20
    cmp x21,x8
    b.gt Lfpanic
    sub x9,x21,x0
    ldr x10,[x19,#BUF_BYTES]
    add x10,x10,x20
    add x11,sp,#48
    mov x12,#0
Lfloop:
    cmp x12,x21
    b.ge Lfdone
    mov w13,#32
    cmp x12,x9
    b.lt Lfput
    ldrb w13,[x11],#1
Lfput:
    strb w13,[x10],#1
    add x12,x12,#1
    b Lfloop
Lfdone:
    mov w0,#0
    b Lfreturn
Lfoverflow:
    mov w0,#1
Lfreturn:
    ldp x21,x22,[sp,#32]
    ldp x19,x20,[sp,#16]
    ldp x29,x30,[sp],#80
    ret
Lfpanic:
#ifdef __APPLE__
    adrp x0,Lfmessage@PAGE
    add x0,x0,Lfmessage@PAGEOFF
#else
    adrp x0,Lfmessage
    add x0,x0,:lo12:Lfmessage
#endif
    ldp x21,x22,[sp,#32]
    ldp x19,x20,[sp,#16]
    ldp x29,x30,[sp],#80
    b CSYM(core_host_panic)
#ifdef __APPLE__
.section __TEXT,__cstring,cstring_literals
#else
.section .rodata
#endif
Lfmessage: .asciz "fill past the reservation"
#ifndef __APPLE__
.section .note.GNU-stack,"",%progbits
#endif
