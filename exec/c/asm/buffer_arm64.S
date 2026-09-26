/* Append one byte and its 64-bit attribute. Buf's fields are asserted in
   layoutcheck.c. realloc owns allocation; both arrays grow in lockstep.
   Valid input: 0 <= n <= cap with storage for cap elements (or cap=0).
   Growth beyond the signed 32-bit capacity representation is rejected. */
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
.globl CSYM(core_buffer_put)
#ifndef __APPLE__
.type core_buffer_put,%function
#endif
CSYM(core_buffer_put):
    ldp w8,w9,[x0,#BUF_LENGTH]
    cmp w8,w9
    b.lt Lbput
    mov w10,#0x3fffffff
    cmp w9,w10
    b.gt Lboverflow
    cbz w9,Lbfirst
    lsl w9,w9,#1
    b Lbgrow
Lbfirst:
    mov w9,#256
Lbgrow:
    str w9,[x0,#BUF_CAPACITY]
    stp x29,x30,[sp,#-48]!
    mov x29,sp
    stp x19,x20,[sp,#16]
    stp x21,x22,[sp,#32]
    mov x19,x0
    mov x20,x2
    mov w21,w1
    mov w22,w9
    ldr x0,[x19,#BUF_BYTES]
    mov w1,w22
    bl CSYM(realloc)
    cbz x0,Lboom
    str x0,[x19,#BUF_BYTES]
    ldr x0,[x19,#BUF_ATTR]
    lsl x1,x22,#3
    bl CSYM(realloc)
    cbz x0,Lboom
    str x0,[x19,#BUF_ATTR]
    mov x0,x19
    mov x2,x20
    mov w1,w21
    ldp x19,x20,[sp,#16]
    ldp x21,x22,[sp,#32]
    ldp x29,x30,[sp],#48
    ldr w8,[x0,#BUF_LENGTH]
Lbput:
    ldr x10,[x0,#BUF_BYTES]
    strb w1,[x10,x8]
    ldr x10,[x0,#BUF_ATTR]
    str x2,[x10,x8,lsl #3]
    add w8,w8,#1
    str w8,[x0,#BUF_LENGTH]
    ret
Lboom:
    ldp x19,x20,[sp,#16]
    ldp x21,x22,[sp,#32]
    ldp x29,x30,[sp],#48
#ifdef __APPLE__
    adrp x0,Lboom_text@PAGE
    add x0,x0,Lboom_text@PAGEOFF
#else
    adrp x0,Lboom_text
    add x0,x0,:lo12:Lboom_text
#endif
    b CSYM(core_host_panic)
Lboverflow:
#ifdef __APPLE__
    adrp x0,Lboverflow_text@PAGE
    add x0,x0,Lboverflow_text@PAGEOFF
#else
    adrp x0,Lboverflow_text
    add x0,x0,:lo12:Lboverflow_text
#endif
    b CSYM(core_host_panic)
#ifndef __APPLE__
.size core_buffer_put,.-core_buffer_put
.section .rodata
#else
.section __TEXT,__cstring,cstring_literals
#endif
Lboom_text: .asciz "out of memory"
Lboverflow_text: .asciz "buffer capacity overflow"
#ifndef __APPLE__
.section .note.GNU-stack,"",%progbits
#endif
