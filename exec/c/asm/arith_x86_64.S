/* System V generic 32/64-bit word operations. Explicit checks prevent IDIV
   traps for the machine's defined zero and MIN/-1 cases. No callee-saved
   register is touched. The panic hook is a non-returning host contract. */
#ifdef _WIN32
#error Windows_x64_binding_not_implemented
#endif
#ifdef __APPLE__
#define CNAME1(x) _##x
#define CNAME(x) CNAME1(x)
#else
#define CNAME(x) x
#endif
.text
.p2align 2
.globl CNAME(core_alu32)
#ifndef __APPLE__
.type core_alu32,@function
#endif
CNAME(core_alu32):
    cmpl $9,%edi
    ja Lx32bad
    movl %edi,%edi
    leaq Lx32table(%rip),%r8
    movslq (%r8,%rdi,4),%rax
    addq %r8,%rax
    jmp *%rax
Lx32table:
    .long Lx32add-Lx32table, Lx32sub-Lx32table, Lx32mul-Lx32table
    .long Lx32divide-Lx32table, Lx32divide-Lx32table, Lx32and-Lx32table
    .long Lx32or-Lx32table, Lx32xor-Lx32table, Lx32shl-Lx32table, Lx32sar-Lx32table
Lx32add: leal (%rsi,%rdx),%eax
    jmp Lx32sign
Lx32sub: movl %esi,%eax
    subl %edx,%eax
    jmp Lx32sign
Lx32mul: movl %esi,%eax
    imull %edx,%eax
    jmp Lx32sign
Lx32and: movl %esi,%eax
    andl %edx,%eax
    jmp Lx32sign
Lx32or: movl %esi,%eax
    orl %edx,%eax
    jmp Lx32sign
Lx32xor: movl %esi,%eax
    xorl %edx,%eax
    jmp Lx32sign
Lx32shl: movl %esi,%eax
    movl %edx,%ecx
    shll %cl,%eax
    jmp Lx32sign
Lx32sar: movl %esi,%eax
    movl %edx,%ecx
    sarl %cl,%eax
Lx32sign: cltq
    ret
Lx32divide:
    movl %edx,%r8d
    testl %r8d,%r8d
    je Lx32zero
    movl %esi,%eax
    cmpl $-1,%r8d
    jne Lx32idiv
    cmpl $0x80000000,%eax
    jne Lx32idiv
    cmpl $4,%edi
    je Lx32zero
    jmp Lx32sign
Lx32idiv:
    cltd
    idivl %r8d
    cmpl $4,%edi
    jne Lx32sign
    movl %edx,%eax
    jmp Lx32sign
Lx32zero: xorl %eax,%eax
    ret
Lx32bad: leaq Lx32message(%rip),%rdi
    jmp CNAME(core_host_panic)
#ifndef __APPLE__
.size core_alu32,.-core_alu32
#endif
.p2align 2
.globl CNAME(core_alu64)
#ifndef __APPLE__
.type core_alu64,@function
#endif
CNAME(core_alu64):
    movl $0,(%rcx)
    cmpl $15,%edi
    ja Lx64bad
    movl %edi,%edi
    leaq Lx64table(%rip),%r8
    movslq (%r8,%rdi,4),%rax
    addq %r8,%rax
    jmp *%rax
Lx64table:
    .long Lx64add-Lx64table, Lx64sub-Lx64table, Lx64mul-Lx64table, Lx64bad-Lx64table
    .long Lx64bad-Lx64table, Lx64and-Lx64table, Lx64or-Lx64table, Lx64xor-Lx64table
    .long Lx64shl-Lx64table, Lx64sar-Lx64table, Lx64signed-Lx64table, Lx64signed-Lx64table
    .long Lx64unsigned-Lx64table, Lx64unsigned-Lx64table, Lx64not-Lx64table, Lx64shr-Lx64table
Lx64add: leaq (%rsi,%rdx),%rax
    ret
Lx64sub: movq %rsi,%rax
    subq %rdx,%rax
    ret
Lx64mul: movq %rsi,%rax
    imulq %rdx,%rax
    ret
Lx64and: movq %rsi,%rax
    andq %rdx,%rax
    ret
Lx64or: movq %rsi,%rax
    orq %rdx,%rax
    ret
Lx64xor: movq %rsi,%rax
    xorq %rdx,%rax
    ret
Lx64not: movq %rsi,%rax
    notq %rax
    ret
Lx64shl: movq %rsi,%rax
    movl %edx,%ecx
    shlq %cl,%rax
    ret
Lx64sar: movq %rsi,%rax
    movl %edx,%ecx
    sarq %cl,%rax
    ret
Lx64shr: movq %rsi,%rax
    movl %edx,%ecx
    shrq %cl,%rax
    ret
Lx64signed:
    movq %rdx,%r8
    testq %r8,%r8
    je Lx64zero
    movq %rsi,%rax
    cmpq $-1,%r8
    jne Lx64idiv
    movabsq $0x8000000000000000,%r9
    cmpq %r9,%rax
    jne Lx64idiv
    cmpl $11,%edi
    je Lx64resultzero
    ret
Lx64idiv:
    cqto
    idivq %r8
    cmpl $11,%edi
    jne Lx64return
    movq %rdx,%rax
    ret
Lx64unsigned:
    movq %rdx,%r8
    testq %r8,%r8
    je Lx64zero
    movq %rsi,%rax
    xorl %edx,%edx
    divq %r8
    cmpl $13,%edi
    jne Lx64return
    movq %rdx,%rax
Lx64return: ret
Lx64zero:
    movl $1,(%rcx)
Lx64resultzero:
    xorl %eax,%eax
    ret
Lx64bad: leaq Lx64message(%rip),%rdi
    jmp CNAME(core_host_panic)
#ifndef __APPLE__
.size core_alu64,.-core_alu64
.section .rodata
#else
.section __TEXT,__cstring,cstring_literals
#endif
Lx32message: .asciz "bad alu op"
Lx64message: .asciz "bad alu64 op"
#ifndef __APPLE__
.section .note.GNU-stack,"",@progbits
#endif
