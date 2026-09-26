/* Byte-string interning. Exact existing hash seed, binary lengths, stable
   first-insertion IDs, and pointer-preserving rehash. Empty bytes are valid. */
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
.globl CSYM(core_hash_bytes)
#ifndef __APPLE__
.type core_hash_bytes,%function
#endif
CSYM(core_hash_bytes):
    movz x2,#0x0383
    movk x2,#0x739d,lsl #16
    movk x2,#0x0fb0,lsl #32
    movk x2,#0x1465,lsl #48
    cmp w1,#0
    b.le Lhashdone
    movz x3,#0x01b3
    movk x3,#0x0100,lsl #32
Lhashloop:
    ldrb w4,[x0],#1
    eor x2,x2,x4
    mul x2,x2,x3
    subs w1,w1,#1
    b.ne Lhashloop
Lhashdone:
    mov x0,x2
    ret
#ifndef __APPLE__
.size core_hash_bytes,.-core_hash_bytes
#endif
.p2align 2
.globl CSYM(core_string_intern)
#ifndef __APPLE__
.type core_string_intern,%function
#endif
CSYM(core_string_intern):
    stp x29,x30,[sp,#-80]!
    mov x29,sp
    stp x19,x20,[sp,#16]
    stp x21,x22,[sp,#32]
    stp x23,x24,[sp,#48]
    stp x25,x26,[sp,#64]
    mov x19,x0
    mov x20,x1
    mov w21,w2
    ldr x23,[x19,#INTERN_CAP]
    ldr x3,[x19,#INTERN_COUNT]
    add x3,x3,#1
    lsl x3,x3,#1
    cmp x3,x23
    b.ls Lislookup
    movz x3,#0x5555
    movk x3,#0x5555,lsl #16
    movk x3,#0x5555,lsl #32
    movk x3,#0x0555,lsl #48
    cmp x23,x3
    b.hi Lisoverflow
    ldr x22,[x19,#INTERN_ENTRIES]
    lsl x0,x23,#1
    cbnz x0,Liscap
    mov x0,#1024
Liscap:
    str x0,[x19,#INTERN_CAP]
    mov x1,#ENTRY_SIZE
    bl CSYM(calloc)
    str x0,[x19,#INTERN_ENTRIES]
    cbz x0,Lisoom
    mov x24,#0
Lisrehash:
    cmp x24,x23
    b.hs Lisfree
    add x3,x24,x24,lsl #1
    add x26,x22,x3,lsl #3
    ldr x0,[x26,#ENTRY_BYTES]
    cbz x0,Lisnext
    ldr w1,[x26,#ENTRY_LENGTH]
    bl CSYM(core_hash_bytes)
    ldr x3,[x19,#INTERN_CAP]
    sub x3,x3,#1
    and x25,x0,x3
Lisfind:
    add x4,x25,x25,lsl #1
    ldr x5,[x19,#INTERN_ENTRIES]
    add x5,x5,x4,lsl #3
    ldr x4,[x5,#ENTRY_BYTES]
    cbz x4,Liscopy
    add x25,x25,#1
    and x25,x25,x3
    b Lisfind
Liscopy:
    ldp x6,x7,[x26]
    stp x6,x7,[x5]
    ldr x6,[x26,#ENTRY_VALUE]
    str x6,[x5,#ENTRY_VALUE]
Lisnext:
    add x24,x24,#1
    b Lisrehash
Lisfree:
    mov x0,x22
    bl CSYM(free)
Lislookup:
    mov x0,x20
    mov w1,w21
    bl CSYM(core_hash_bytes)
    ldr x3,[x19,#INTERN_CAP]
    sub x3,x3,#1
    and x25,x0,x3
Lisprobe:
    add x3,x25,x25,lsl #1
    ldr x26,[x19,#INTERN_ENTRIES]
    add x26,x26,x3,lsl #3
    ldr x0,[x26,#ENTRY_BYTES]
    cbz x0,Lisnew
    ldr w3,[x26,#ENTRY_LENGTH]
    cmp w3,w21
    b.ne Lisadvance
    mov x1,x20
    mov w2,w21
    bl CSYM(memcmp)
    cbz w0,Lisfound
Lisadvance:
    ldr x3,[x19,#INTERN_CAP]
    sub x3,x3,#1
    add x25,x25,#1
    and x25,x25,x3
    b Lisprobe
Lisnew:
    mov x0,#0
    add x1,x21,#1
    bl CSYM(realloc)
    cbz x0,Lisoom
    str x0,[x26,#ENTRY_BYTES]
    mov x1,x20
    mov w2,w21
    bl CSYM(memcpy)
    str w21,[x26,#ENTRY_LENGTH]
    ldr x0,[x19,#INTERN_COUNT]
    add x0,x0,#1
    str x0,[x19,#INTERN_COUNT]
    str x0,[x26,#ENTRY_VALUE]
    b Lisok
Lisfound:
    ldr x0,[x26,#ENTRY_VALUE]
Lisok:
    mov x1,#0
    b Lisreturn
Lisoom:
#ifdef __APPLE__
    adrp x0,Lisoom_text@PAGE
    add x0,x0,Lisoom_text@PAGEOFF
#else
    adrp x0,Lisoom_text
    add x0,x0,:lo12:Lisoom_text
#endif
    b Liserror
Lisoverflow:
#ifdef __APPLE__
    adrp x0,Lisoverflow_text@PAGE
    add x0,x0,Lisoverflow_text@PAGEOFF
#else
    adrp x0,Lisoverflow_text
    add x0,x0,:lo12:Lisoverflow_text
#endif
Liserror:
    mov x1,#1
Lisreturn:
    ldp x19,x20,[sp,#16]
    ldp x21,x22,[sp,#32]
    ldp x23,x24,[sp,#48]
    ldp x25,x26,[sp,#64]
    ldp x29,x30,[sp],#80
    cbnz x1,Lispanic
    ret
Lispanic:
    b CSYM(core_host_panic)
#ifndef __APPLE__
.size core_string_intern,.-core_string_intern
.section .rodata
#else
.section __TEXT,__cstring,cstring_literals
#endif
Lisoom_text: .asciz "out of memory"
Lisoverflow_text: .asciz "intern capacity overflow"
#ifndef __APPLE__
.section .note.GNU-stack,"",%progbits
#endif
