/* System V Buf append, matching core_put including allocation order.
   Preserve r12/r13/rbx across realloc and align the stack at each call. */
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
.globl CSYM(core_buffer_put)
#ifndef __APPLE__
.type core_buffer_put,@function
#endif
CSYM(core_buffer_put):
    movl BUF_LENGTH(%rdi),%eax
    cmpl BUF_CAPACITY(%rdi),%eax
    jl Lxbput
    movl BUF_CAPACITY(%rdi),%ecx
    cmpl $0x3fffffff,%ecx
    jg Lxboverflow
    testl %ecx,%ecx
    je Lxbfirst
    addl %ecx,%ecx
    jmp Lxbgrow
Lxbfirst:
    movl $256,%ecx
Lxbgrow:
    movl %ecx,BUF_CAPACITY(%rdi)
    pushq %r12
    pushq %r13
    pushq %rbx
    movq %rdi,%r12
    movq %rdx,%r13
    movl %esi,%ebx
    movq BUF_BYTES(%r12),%rdi
    movl BUF_CAPACITY(%r12),%esi
    call CSYM(realloc)
    testq %rax,%rax
    je Lxboom
    movq %rax,BUF_BYTES(%r12)
    movq BUF_ATTR(%r12),%rdi
    movl BUF_CAPACITY(%r12),%esi
    shlq $3,%rsi
    call CSYM(realloc)
    testq %rax,%rax
    je Lxboom
    movq %rax,BUF_ATTR(%r12)
    movq %r12,%rdi
    movq %r13,%rdx
    movl %ebx,%esi
    popq %rbx
    popq %r13
    popq %r12
    movl BUF_LENGTH(%rdi),%eax
Lxbput:
    movq BUF_BYTES(%rdi),%rcx
    movb %sil,(%rcx,%rax)
    movq BUF_ATTR(%rdi),%rcx
    movq %rdx,(%rcx,%rax,8)
    incl %eax
    movl %eax,BUF_LENGTH(%rdi)
    ret
Lxboom:
    popq %rbx
    popq %r13
    popq %r12
    leaq Lxboom_text(%rip),%rdi
    jmp CSYM(core_host_panic)
Lxboverflow:
    leaq Lxboverflow_text(%rip),%rdi
    jmp CSYM(core_host_panic)
#ifndef __APPLE__
.size core_buffer_put,.-core_buffer_put
.section .rodata
#else
.section __TEXT,__cstring,cstring_literals
#endif
Lxboom_text: .asciz "out of memory"
Lxboverflow_text: .asciz "buffer capacity overflow"
#ifndef __APPLE__
.section .note.GNU-stack,"",@progbits
#endif
