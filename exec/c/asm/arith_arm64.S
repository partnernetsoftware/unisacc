/* Generic machine word operations, not C-language arithmetic policy.
   32-bit results are sign extended. Shifts use the low 5/6 count bits.
   32-bit div/rem by zero return 0; 64-bit division sets *z=1 and returns 0.
   MIN/-1 gives MIN (division) or 0 (remainder) on both ISAs. */
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
.globl CNAME(core_alu32)
#ifndef __APPLE__
.type core_alu32,%function
#endif
CNAME(core_alu32):
    cmp w0,#9
    b.hi La32bad
    adr x4,La32table
    ldrsw x5,[x4,w0,uxtw #2]
    add x4,x4,x5
    br x4
La32table:
    .word La32add-La32table, La32sub-La32table, La32mul-La32table
    .word La32div-La32table, La32rem-La32table, La32and-La32table
    .word La32or-La32table, La32xor-La32table, La32shl-La32table, La32sar-La32table
La32add: add w0,w1,w2
    b La32sign
La32sub: sub w0,w1,w2
    b La32sign
La32mul: mul w0,w1,w2
    b La32sign
La32div:
    cbz w2,La32zero
    sdiv w0,w1,w2
    b La32sign
La32rem:
    cbz w2,La32zero
    sdiv w4,w1,w2
    msub w0,w4,w2,w1
    b La32sign
La32and: and w0,w1,w2
    b La32sign
La32or: orr w0,w1,w2
    b La32sign
La32xor: eor w0,w1,w2
    b La32sign
La32shl: lslv w0,w1,w2
    b La32sign
La32sar: asrv w0,w1,w2
La32sign:
    sxtw x0,w0
    ret
La32zero:
    mov x0,#0
    ret
La32bad:
#ifdef __APPLE__
    adrp x0,La32message@PAGE
    add x0,x0,La32message@PAGEOFF
#else
    adrp x0,La32message
    add x0,x0,:lo12:La32message
#endif
    b CNAME(core_host_panic)
#ifndef __APPLE__
.size core_alu32,.-core_alu32
#endif
.p2align 2
.globl CNAME(core_alu64)
#ifndef __APPLE__
.type core_alu64,%function
#endif
CNAME(core_alu64):
    str wzr,[x3]
    cmp w0,#15
    b.hi La64bad
    adr x4,La64table
    ldrsw x5,[x4,w0,uxtw #2]
    add x4,x4,x5
    br x4
La64table:
    .word La64add-La64table, La64sub-La64table, La64mul-La64table, La64bad-La64table
    .word La64bad-La64table, La64and-La64table, La64or-La64table, La64xor-La64table
    .word La64shl-La64table, La64sar-La64table, La64div-La64table, La64rem-La64table
    .word La64udiv-La64table, La64urem-La64table, La64not-La64table, La64shr-La64table
La64add: add x0,x1,x2
    ret
La64sub: sub x0,x1,x2
    ret
La64mul: mul x0,x1,x2
    ret
La64and: and x0,x1,x2
    ret
La64or: orr x0,x1,x2
    ret
La64xor: eor x0,x1,x2
    ret
La64shl: lslv x0,x1,x2
    ret
La64sar: asrv x0,x1,x2
    ret
La64shr: lsrv x0,x1,x2
    ret
La64not: mvn x0,x1
    ret
La64div:
    cbz x2,La64zero
    sdiv x0,x1,x2
    ret
La64rem:
    cbz x2,La64zero
    sdiv x4,x1,x2
    msub x0,x4,x2,x1
    ret
La64udiv:
    cbz x2,La64zero
    udiv x0,x1,x2
    ret
La64urem:
    cbz x2,La64zero
    udiv x4,x1,x2
    msub x0,x4,x2,x1
    ret
La64zero:
    mov w4,#1
    str w4,[x3]
    mov x0,#0
    ret
La64bad:
#ifdef __APPLE__
    adrp x0,La64message@PAGE
    add x0,x0,La64message@PAGEOFF
#else
    adrp x0,La64message
    add x0,x0,:lo12:La64message
#endif
    b CNAME(core_host_panic)
#ifndef __APPLE__
.size core_alu64,.-core_alu64
.section .rodata
#else
.section __TEXT,__cstring,cstring_literals
#endif
La32message: .asciz "bad alu op"
La64message: .asciz "bad alu64 op"
#ifndef __APPLE__
.section .note.GNU-stack,"",%progbits
#endif
