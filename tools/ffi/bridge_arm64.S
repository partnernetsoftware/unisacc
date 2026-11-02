.text
.globl _us_library_bridge_call
_us_library_bridge_call:
    stp x29, x30, [sp, #-48]!
    stp x19, x20, [sp, #16]
    str x21, [sp, #32]
    mov x19, x0
    ldr x9, [x19, #0]
    ldr x0, [x19, #8]
    ldr x1, [x19, #16]
    ldr x2, [x19, #24]
    ldr x3, [x19, #32]
    ldr x4, [x19, #40]
    ldr x5, [x19, #48]
    mov x6, #0
    mov x7, #0
    ldr x10, [x19, #56]
    fmov d0, x10
    ldr x10, [x19, #64]
    fmov d1, x10
    ldr x10, [x19, #72]
    fmov d2, x10
    ldr x10, [x19, #80]
    fmov d3, x10
    ldr x10, [x19, #88]
    fmov d4, x10
    ldr x10, [x19, #96]
    fmov d5, x10
    ldr x10, [x19, #104]
    fmov d6, x10
    ldr x10, [x19, #112]
    fmov d7, x10
    ldr x12, [x19, #128]
    cmp x12, #16
    b.hi too_many_
    sub sp, sp, #128
    add x9, sp, #0
    ldr x11, [x19, #120]
    mov x13, #0
loop2_:
    cmp x13, x12
    b.eq done2_
    ldr x14, [x11, x13, lsl #3]
    str x14, [x9, x13, lsl #3]
    add x13, x13, #1
    b loop2_
done2_:
    ldr x10, [x19, #0]
    blr x10
    add sp, sp, #128
    str x0, [x19, #144]
    fmov x10, d0
    str x10, [x19, #152]
    b exit_
too_many_:
    mov x10, #-1
    str x10, [x19, #144]
exit_:
    ldr x21, [sp, #32]
    ldp x19, x20, [sp, #16]
    ldp x29, x30, [sp], #48
    ret
