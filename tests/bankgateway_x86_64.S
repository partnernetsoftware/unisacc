/* SysV x86_64 register-bank gateway (R12-1 ①, design research/r11-bank-design.md §3).
   void us_bank_call_x86_64(const BankFrame *f, void *fn)
   BankFrame layout (bytes):
     0   gp[8]       (rdi rsi rdx rcx r8 r9 used; 6..7 ignored)
     64  fp[8][16]   xmm0..xmm7 full 128-bit lanes
     192 hidden      pointer loaded into rdi when non-zero (result in memory)
     200 stack_bytes multiple of 16
     208 stack_image pointer to stack_bytes bytes copied below the return address
     216 al          number of vector registers used (SysV varargs convention)
     224 captured    rax rdx (8 each), xmm0 xmm1 (16 each) -> 48 bytes
   Only the plan decides what is loaded: registers not named by the plan are still
   loaded from the frame, so the host zero-fills the frame first. */
#if defined(__x86_64__)
#if defined(__APPLE__)
#define SYM(x) _##x
#else
#define SYM(x) x
#endif
.text
.p2align 4
.globl SYM(us_bank_call_x86_64)
#if !defined(__APPLE__)
.type SYM(us_bank_call_x86_64), @function
#endif
SYM(us_bank_call_x86_64):
    pushq %rbp
    movq %rsp, %rbp
    pushq %rbx
    pushq %r12               /* rsp is 16-aligned again here */
    movq %rdi, %rbx          /* frame */
    movq %rsi, %r12          /* callee */
    movq 200(%rbx), %rcx     /* stack bytes */
    subq %rcx, %rsp          /* stack_bytes is a multiple of 16: rsp stays aligned */
    movq %rsp, %rdi
    movq 208(%rbx), %rsi
    rep movsb                /* copy the stack image to [rsp, rsp+stack_bytes) */
    movdqu 64(%rbx), %xmm0
    movdqu 80(%rbx), %xmm1
    movdqu 96(%rbx), %xmm2
    movdqu 112(%rbx), %xmm3
    movdqu 128(%rbx), %xmm4
    movdqu 144(%rbx), %xmm5
    movdqu 160(%rbx), %xmm6
    movdqu 176(%rbx), %xmm7
    movq 0(%rbx), %rdi
    movq 8(%rbx), %rsi
    movq 16(%rbx), %rdx
    movq 24(%rbx), %rcx
    movq 32(%rbx), %r8
    movq 40(%rbx), %r9
    movq 192(%rbx), %rax
    testq %rax, %rax
    jz 1f
    movq %rax, %rdi          /* hidden result pointer (the plan reserved GP[0]) */
1:  movq 216(%rbx), %rax     /* al = vector registers used */
    call *%r12
    movq %rax, 224(%rbx)
    movq %rdx, 232(%rbx)
    movdqu %xmm0, 240(%rbx)
    movdqu %xmm1, 256(%rbx)
    leaq -16(%rbp), %rsp
    popq %r12
    popq %rbx
    popq %rbp
    ret
#endif
