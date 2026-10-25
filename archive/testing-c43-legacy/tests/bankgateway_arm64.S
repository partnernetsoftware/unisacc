/* AAPCS64 register-bank gateway (macOS / Linux arm64), R12-1 ① second slice.
   void us_bank_call_arm64(const BankFrame *f, void *fn)
   Same frame layout as the x86_64 gateway (see tests/bankplan.h):
     0   gp[8]        x0..x7
     64  fp[8][16]    v0..v7 full 128-bit lanes
     192 hidden       x8 indirect-result address when non-zero
     200 stack_bytes  multiple of 16
     208 stack_image  copied to [sp, sp+stack_bytes) before the call
     216 al           unused on arm64
     224 captured     x0 x1 (8 each) then v0 v1 (16 each) — the header also stores v2 v3
                      at 256+16.. for HFA results of up to four members. */
#if defined(__aarch64__)
#if defined(__APPLE__)
#define SYM(x) _##x
#else
#define SYM(x) x
#endif
.text
.p2align 2
.globl SYM(us_bank_call_arm64)
#if !defined(__APPLE__)
.type SYM(us_bank_call_arm64), %function
#endif
SYM(us_bank_call_arm64):
    stp x29, x30, [sp, #-32]!
    mov x29, sp
    stp x19, x20, [sp, #16]
    mov x19, x0                 /* frame */
    mov x20, x1                 /* callee */
    ldr x9, [x19, #200]         /* stack bytes (multiple of 16) */
    sub sp, sp, x9
    ldr x10, [x19, #208]        /* stack image */
    mov x11, sp
    cbz x9, 2f
1:  ldp x12, x13, [x10], #16
    stp x12, x13, [x11], #16
    subs x9, x9, #16
    b.ne 1b
2:  add x9, x19, #64
    ldp q0, q1, [x9, #0]
    ldp q2, q3, [x9, #32]
    ldp q4, q5, [x9, #64]
    ldp q6, q7, [x9, #96]
    ldr x8, [x19, #192]
    ldp x0, x1, [x19, #0]
    ldp x2, x3, [x19, #16]
    ldp x4, x5, [x19, #32]
    ldp x6, x7, [x19, #48]
    blr x20
    stp x0, x1, [x19, #224]
    add x9, x19, #240
    stp q0, q1, [x9, #0]
    stp q2, q3, [x9, #32]
    mov sp, x29
    ldp x19, x20, [sp, #16]
    ldp x29, x30, [sp], #32
    ret
#endif
