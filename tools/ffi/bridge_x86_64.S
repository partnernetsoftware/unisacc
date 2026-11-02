.text
.globl us_library_bridge_call
us_library_bridge_call:
    pushq %rbx
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    movq %rdi, %rbx
    movq 0(%rbx), %r11
    movq 8(%rbx), %rdi
    movq 16(%rbx), %rsi
    movq 24(%rbx), %rdx
    movq 32(%rbx), %rcx
    movq 40(%rbx), %r8
    movq 48(%rbx), %r9
    movq 120(%rbx), %r13
    movq 128(%rbx), %r12
    movq %r12, %rax
    andq $1, %rax
    addq %rax, %r12
    shlq $3, %r12
    subq $8, %rsp
    subq %r12, %rsp
    movq $0, %r14
loop_:
    cmpq 128(%rbx), %r14
    jae done_
    movq (%r13,%r14,8), %r15
    movq %r15, (%rsp,%r14,8)
    incq %r14
    jmp loop_
done_:
    movq 56(%rbx), %rax
    movq %rax, %xmm0
    movq 64(%rbx), %rax
    movq %rax, %xmm1
    movq 72(%rbx), %rax
    movq %rax, %xmm2
    movq 80(%rbx), %rax
    movq %rax, %xmm3
    movq 88(%rbx), %rax
    movq %rax, %xmm4
    movq 96(%rbx), %rax
    movq %rax, %xmm5
    movq 104(%rbx), %rax
    movq %rax, %xmm6
    movq 112(%rbx), %rax
    movq %rax, %xmm7
    movq 136(%rbx), %rax
    call *%r11
    movq %rax, 144(%rbx)
    movq %xmm0, %rax
    movq %rax, 152(%rbx)
    addq %r12, %rsp
    addq $8, %rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    popq %rbx
    ret
