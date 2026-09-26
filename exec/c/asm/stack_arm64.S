/* Two generic stacks. Frame values borrow bytes/attributes; push input must
   not alias the frame backing array, which realloc may move. */
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
.globl CSYM(core_stack_push)
CSYM(core_stack_push):
    mov x2,#0
    b Lspush
.globl CSYM(core_frame_push)
CSYM(core_frame_push):
    mov x2,#1
Lspush:
    stp x29,x30,[sp,#-48]!
    mov x29,sp
    stp x19,x20,[sp,#16]
    stp x21,x22,[sp,#32]
    mov x19,x0
    mov x20,x1
    mov x21,x2
    ldp w22,w8,[x19,#STACK_COUNT]
    cmp w22,w8
    b.lt Lsstore
    mov w9,#0x3fffffff
    cmp w8,w9
    b.gt Lscap
    cbnz w8,Lsdouble
    mov w8,#1024
    cbz x21,Lsalloc
    mov w8,#16
    b Lsalloc
Lsdouble:
    lsl w8,w8,#1
Lsalloc:
    str w8,[x19,#STACK_CAP]
    lsl x1,x8,#2
    cbz x21,Lssize
    lsl x1,x8,#5
Lssize:
    ldr x0,[x19,#STACK_ENTRIES]
    bl CSYM(realloc)
    cbz x0,Lsoom
    str x0,[x19,#STACK_ENTRIES]
Lsstore:
    ldr x8,[x19,#STACK_ENTRIES]
    cbnz x21,Lsframe
    str w20,[x8,x22,lsl #2]
    b Lsdone
Lsframe:
    add x8,x8,x22,lsl #5
    ldp x9,x10,[x20]
    stp x9,x10,[x8]
    ldp x9,x10,[x20,#16]
    stp x9,x10,[x8,#16]
Lsdone:
    add w22,w22,#1
    str w22,[x19,#STACK_COUNT]
    ldp x21,x22,[sp,#32]
    ldp x19,x20,[sp,#16]
    ldp x29,x30,[sp],#48
    ret
Lscap:
#ifdef __APPLE__
    adrp x0,Lscapmsg@PAGE
    add x0,x0,Lscapmsg@PAGEOFF
    adrp x8,Lfcapmsg@PAGE
    add x8,x8,Lfcapmsg@PAGEOFF
#else
    adrp x0,Lscapmsg
    add x0,x0,:lo12:Lscapmsg
    adrp x8,Lfcapmsg
    add x8,x8,:lo12:Lfcapmsg
#endif
    cmp x21,#0
    csel x0,x0,x8,eq
    b Lspanic
Lsoom:
#ifdef __APPLE__
    adrp x0,Lsoommsg@PAGE
    add x0,x0,Lsoommsg@PAGEOFF
#else
    adrp x0,Lsoommsg
    add x0,x0,:lo12:Lsoommsg
#endif
Lspanic:
    ldp x21,x22,[sp,#32]
    ldp x19,x20,[sp,#16]
    ldp x29,x30,[sp],#48
    b CSYM(core_host_panic)
.globl CSYM(core_stack_pop)
CSYM(core_stack_pop):
    ldr w8,[x0,#STACK_COUNT]
    cbz w8,Lsempty
    sub w8,w8,#1
    str w8,[x0,#STACK_COUNT]
    ret
Lsempty:
#ifdef __APPLE__
    adrp x0,Lsemptymsg@PAGE
    add x0,x0,Lsemptymsg@PAGEOFF
#else
    adrp x0,Lsemptymsg
    add x0,x0,:lo12:Lsemptymsg
#endif
    b CSYM(core_host_panic)
.globl CSYM(core_frame_pop)
CSYM(core_frame_pop):
    ldr w8,[x0,#STACK_COUNT]
    cmp w8,#1
    b.le Lsfret
    sub w8,w8,#1
    str w8,[x0,#STACK_COUNT]
Lsfret:
    ret
#ifdef __APPLE__
.section __TEXT,__cstring,cstring_literals
#else
.section .rodata
#endif
Lscapmsg: .asciz "stack capacity overflow"
Lfcapmsg: .asciz "frame capacity overflow"
Lsoommsg: .asciz "out of memory"
Lsemptymsg: .asciz "pop of an empty stack"
#ifndef __APPLE__
.section .note.GNU-stack,"",%progbits
#endif
