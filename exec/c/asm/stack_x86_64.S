/* System V control/input stacks. Frame push source cannot alias backing store. */
#include "layout.h"
#ifdef _WIN32
#error Windows_x64_binding_not_implemented
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
    xorl %edx,%edx
    jmp Lxspush
.globl CSYM(core_frame_push)
CSYM(core_frame_push):
    movl $1,%edx
Lxspush:
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    movq %rdi,%r12
    movq %rsi,%r13
    movl %edx,%r14d
    movl STACK_COUNT(%r12),%r15d
    movl STACK_CAP(%r12),%eax
    cmpl %eax,%r15d
    jl Lxsstore
    cmpl $0x3fffffff,%eax
    jg Lxscap
    testl %eax,%eax
    jne Lxsdouble
    movl $1024,%eax
    testl %r14d,%r14d
    je Lxsalloc
    movl $16,%eax
    jmp Lxsalloc
Lxsdouble:
    addl %eax,%eax
Lxsalloc:
    movl %eax,STACK_CAP(%r12)
    shlq $2,%rax
    testl %r14d,%r14d
    je Lxssize
    shlq $3,%rax
Lxssize:
    movq %rax,%rsi
    movq STACK_ENTRIES(%r12),%rdi
    call CSYM(realloc)
    testq %rax,%rax
    je Lxsoom
    movq %rax,STACK_ENTRIES(%r12)
Lxsstore:
    movq STACK_ENTRIES(%r12),%rax
    testl %r14d,%r14d
    jne Lxsframe
    movl %r13d,(%rax,%r15,4)
    jmp Lxsdone
Lxsframe:
    movq %r15,%rcx
    shlq $5,%rcx
    addq %rcx,%rax
    movq 0(%r13),%rdx
    movq %rdx,0(%rax)
    movq 8(%r13),%rdx
    movq %rdx,8(%rax)
    movq 16(%r13),%rdx
    movq %rdx,16(%rax)
    movq 24(%r13),%rdx
    movq %rdx,24(%rax)
Lxsdone:
    incl %r15d
    movl %r15d,STACK_COUNT(%r12)
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    ret
Lxscap:
    leaq Lxscapmsg(%rip),%rdi
    leaq Lxfcapmsg(%rip),%rax
    testl %r14d,%r14d
    cmovne %rax,%rdi
    jmp Lxspanic
Lxsoom:
    leaq Lxsoommsg(%rip),%rdi
Lxspanic:
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    jmp CSYM(core_host_panic)
.globl CSYM(core_stack_pop)
CSYM(core_stack_pop):
    movl STACK_COUNT(%rdi),%eax
    testl %eax,%eax
    je Lxsempty
    decl %eax
    movl %eax,STACK_COUNT(%rdi)
    ret
Lxsempty:
    leaq Lxsemptymsg(%rip),%rdi
    jmp CSYM(core_host_panic)
.globl CSYM(core_frame_pop)
CSYM(core_frame_pop):
    movl STACK_COUNT(%rdi),%eax
    cmpl $1,%eax
    jle Lxsfret
    decl %eax
    movl %eax,STACK_COUNT(%rdi)
Lxsfret:
    ret
#ifdef __APPLE__
.section __TEXT,__cstring,cstring_literals
#else
.section .rodata
#endif
Lxscapmsg: .asciz "stack capacity overflow"
Lxfcapmsg: .asciz "frame capacity overflow"
Lxsoommsg: .asciz "out of memory"
Lxsemptymsg: .asciz "pop of an empty stack"
#ifndef __APPLE__
.section .note.GNU-stack,"",@progbits
#endif
