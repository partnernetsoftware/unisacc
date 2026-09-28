/* Windows x64 normal-return host bridge. uint64_t(entry,args6,softtop).
   ABI: https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention
   Unwind: https://learn.microsoft.com/en-us/cpp/build/exception-handling-x64
   Prolog: https://learn.microsoft.com/en-us/cpp/build/prolog-and-epilog
   Save every Win64 nonvolatile GPR and XMM6..15. The host frame reserves
   32 shadow bytes and is 16-aligned. Script uses its own rsp ABI, not Win64.
   Host rsp lives in a guest-stack sentinel, never in a clobberable GPR.
   The prolog has valid COFF unwind metadata. Dynamic guest-stack switching
   is NOT covered by that metadata: no SEH propagation through guest frames
   is supported by this normal-return bridge. softtop must be 16-aligned. */
.text
.p2align 4
.globl us_library_bridge_raw
.def us_library_bridge_raw; .scl 2; .type 32; .endef
us_library_bridge_raw:
.seh_proc us_library_bridge_raw
    pushq %rbp
    .seh_pushreg %rbp
    pushq %rbx
    .seh_pushreg %rbx
    pushq %rdi
    .seh_pushreg %rdi
    pushq %rsi
    .seh_pushreg %rsi
    pushq %r12
    .seh_pushreg %r12
    pushq %r13
    .seh_pushreg %r13
    pushq %r14
    .seh_pushreg %r14
    pushq %r15
    .seh_pushreg %r15
    subq $200, %rsp
    .seh_stackalloc 200
    movdqu %xmm6, 32(%rsp)
    .seh_savexmm %xmm6, 32
    movdqu %xmm7, 48(%rsp)
    .seh_savexmm %xmm7, 48
    movdqu %xmm8, 64(%rsp)
    .seh_savexmm %xmm8, 64
    movdqu %xmm9, 80(%rsp)
    .seh_savexmm %xmm9, 80
    movdqu %xmm10, 96(%rsp)
    .seh_savexmm %xmm10, 96
    movdqu %xmm11, 112(%rsp)
    .seh_savexmm %xmm11, 112
    movdqu %xmm12, 128(%rsp)
    .seh_savexmm %xmm12, 128
    movdqu %xmm13, 144(%rsp)
    .seh_savexmm %xmm13, 144
    movdqu %xmm14, 160(%rsp)
    .seh_savexmm %xmm14, 160
    movdqu %xmm15, 176(%rsp)
    .seh_savexmm %xmm15, 176
    .seh_endprologue
    movq %rsp, %r9
    movq %rcx, %r11
    movq %rdx, %r10
    leaq -16(%r8), %rsp
    movq %r9, (%rsp)
    movq 0(%r10), %rax
    movq 8(%r10), %rdi
    movq 16(%r10), %rsi
    movq 24(%r10), %rdx
    movq 32(%r10), %rcx
    movq 40(%r10), %r8
    xorq %r9, %r9
    callq *%r11
    movq (%rsp), %rsp
    movdqu 32(%rsp), %xmm6
    movdqu 48(%rsp), %xmm7
    movdqu 64(%rsp), %xmm8
    movdqu 80(%rsp), %xmm9
    movdqu 96(%rsp), %xmm10
    movdqu 112(%rsp), %xmm11
    movdqu 128(%rsp), %xmm12
    movdqu 144(%rsp), %xmm13
    movdqu 160(%rsp), %xmm14
    movdqu 176(%rsp), %xmm15
    addq $200, %rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rsi
    popq %rdi
    popq %rbx
    popq %rbp
    retq
    .seh_endproc
