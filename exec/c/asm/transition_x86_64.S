/* Handwritten x86-64 System V core_transition. Signed 64-bit accumulation.
   Thresholds ascend (the loader rejects any other bank), so the active units
   are a prefix: a branchless binary search finds its length, then exactly
   those weights are added. No table answers are consulted. A declared return
   (ret_ok[q][key]) answers (key, ret_seq[q]) first: control flow, not a
   network decision.
   Windows x64 has a different calling convention and is not selected here. */
#include "layout.h"
#ifdef _WIN32
#error Windows_x64_binding_not_implemented
#endif
#ifdef __APPLE__
#define CNAME(x) _##x
#else
#define CNAME(x) x
#endif
.text
.p2align 4
.globl CNAME(core_transition)
#ifndef __APPLE__
.type core_transition,@function
#endif
CNAME(core_transition):
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    movslq %esi,%rsi
    movslq %edx,%rdx
    movl $-1,(%rcx)
    movl $0,(%r8)
    movq CM_RET_OK(%rdi),%rax
    testq %rax,%rax
    je Lxnoret
    movq (%rax,%rsi,8),%rax
    testq %rax,%rax
    je Lxnoret
    testl %edx,%edx
    js Lxnoret
    cmpl CM_NS(%rdi),%edx
    jge Lxnoret
    cmpb $0,(%rax,%rdx)
    je Lxnoret
    movq CM_RET_SEQ(%rdi),%rax
    movl (%rax,%rsi,4),%r10d
    movl %edx,%r9d
    jmp Lxstore
Lxnoret:
    cmpl $0,CM_ISNET(%rdi)
    je Lxtable
    movq CM_LO(%rdi),%rax
    cmpl (%rax,%rsi,4),%edx
    jl Lxdomain
    movq CM_HI(%rdi),%rax
    cmpl (%rax,%rsi,4),%edx
    jg Lxdomain
    movq CM_BASE_NEXT(%rdi),%rax
    movslq (%rax,%rsi,4),%r9
    movq CM_BASE_SEQ(%rdi),%rax
    movslq (%rax,%rsi,4),%r10
    movq CM_COUNT(%rdi),%rax
    movl (%rax,%rsi,4),%r15d
    movq CM_KEYS(%rdi),%rax
    movq (%rax,%rsi,8),%r12
    movq CM_NEXT(%rdi),%rax
    movq (%rax,%rsi,8),%r13
    movq CM_SEQ(%rdi),%rax
    movq (%rax,%rsi,8),%r14
    testl %r15d,%r15d
    je Lxbounds
    xorl %ebx,%ebx
Lxsearch:
    cmpl %r15d,%ebx
    jae Lxsum
    leal (%rbx,%r15),%eax
    shrl $1,%eax
    leal 1(%rax),%r11d
    cmpl (%r12,%rax,4),%edx
    cmovgel %r11d,%ebx
    cmovll %eax,%r15d
    jmp Lxsearch
Lxsum:
    testl %ebx,%ebx
    je Lxbounds
Lxunit:
    movslq (%r13),%rax
    movslq (%r14),%r11
    addq %rax,%r9
    addq %r11,%r10
    addq $4,%r13
    addq $4,%r14
    decl %ebx
    jne Lxunit
Lxbounds:
    cmpq $-1,%r9
    jl Lxinvalid
    movslq CM_NS(%rdi),%rax
    cmpq %rax,%r9
    jge Lxinvalid
    testq %r10,%r10
    js Lxinvalid
    movslq CM_NQ(%rdi),%rax
    cmpq %rax,%r10
    jge Lxinvalid
Lxstore:
    movl %r9d,(%rcx)
    movl %r10d,(%r8)
Lxdone:
    xorl %eax,%eax
Lxreturn:
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    ret
Lxtable:
    movq CM_MODE(%rdi),%rax
    movl (%rax,%rsi,4),%r11d
    movq CM_NEXT(%rdi),%rax
    movq (%rax,%rsi,8),%r13
    movq CM_SEQ(%rdi),%rax
    movq (%rax,%rsi,8),%r14
    cmpl $1,%r11d
    jne Lxdirect
    movq CM_COUNT(%rdi),%rax
    movl (%rax,%rsi,4),%r15d
    movq CM_KEYS(%rdi),%rax
    movq (%rax,%rsi,8),%r12
    testl %r15d,%r15d
    je Lxdone
Lxfind:
    cmpl (%r12),%edx
    je Lxfound
    addq $4,%r12
    addq $4,%r13
    addq $4,%r14
    decl %r15d
    jne Lxfind
    jmp Lxdone
Lxdirect:
    leaq (%r13,%rdx,4),%r13
    leaq (%r14,%rdx,4),%r14
Lxfound:
    movl (%r13),%r9d
    movl (%r14),%r10d
    jmp Lxstore
Lxdomain:
    leaq Lxdomain_text(%rip),%rax
    jmp Lxreturn
Lxinvalid:
    leaq Lxinvalid_text(%rip),%rax
    jmp Lxreturn
#ifndef __APPLE__
.size core_transition,.-core_transition
.section .rodata
#else
.section __TEXT,__cstring,cstring_literals
#endif
Lxdomain_text:
    .asciz "observation outside network domain"
Lxinvalid_text:
    .asciz "invalid network output"
#ifndef __APPLE__
.section .note.GNU-stack,"",@progbits
#endif
