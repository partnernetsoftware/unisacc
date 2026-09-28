/* macOS/Linux SysV host ABI -> script rax,rdi,rsi,rdx,rcx,r8 / rsp ABI.
   The host SP is saved in a stack sentinel, not a register the script may use. */
#if defined(_WIN32)
#error "librarycall bridge does not implement the Windows host ABI"
#endif
#if defined(__APPLE__)
#define BRIDGE_SYMBOL _us_library_bridge_raw
#else
#define BRIDGE_SYMBOL us_library_bridge_raw
#endif
.text
.p2align 4
.globl BRIDGE_SYMBOL
#if !defined(__APPLE__)
.type BRIDGE_SYMBOL, @function
#endif
BRIDGE_SYMBOL:
    pushq %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    movq %rsp, %r9
    movq %rdi, %r11
    movq %rsi, %r10
    leaq -16(%rdx), %rsp
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
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    retq

#if !defined(__APPLE__)
.size BRIDGE_SYMBOL, .-BRIDGE_SYMBOL
#endif

/* Model-declared ALL_STACK entry. Normal script functions save/restore r6
   (r9), so it carries the host frame across the script call. Every raw slot
   starts at callee FP+16+8*k; padding follows the last argument. */
#if defined(__APPLE__)
#define STACK_BRIDGE_SYMBOL _us_library_bridge_stack_raw
#else
#define STACK_BRIDGE_SYMBOL us_library_bridge_stack_raw
#endif
.p2align 4
.globl STACK_BRIDGE_SYMBOL
#if !defined(__APPLE__)
.type STACK_BRIDGE_SYMBOL, @function
#endif
STACK_BRIDGE_SYMBOL:
    pushq %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    movq %rsp, %r9
    movq %rdi, %r11
    movq %rsi, %r10
    movq %rdx, %rsp
    leaq 15(,%rcx,8), %rax
    andq $-16, %rax
    subq %rax, %rsp
    xorq %rdx, %rdx
.Lus_stack_copy:
    cmpq %rcx, %rdx
    jae .Lus_stack_ready
    movq (%r10,%rdx,8), %rax
    movq %rax, (%rsp,%rdx,8)
    incq %rdx
    jmp .Lus_stack_copy
.Lus_stack_ready:
    callq *%r11
    movq %r9, %rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    retq
#if !defined(__APPLE__)
.size STACK_BRIDGE_SYMBOL, .-STACK_BRIDGE_SYMBOL
.section .note.GNU-stack,"",@progbits
#endif
