/* System V signed decimal and bounded field fill. */
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
.globl CSYM(core_decimal)
CSYM(core_decimal):
    subq $32,%rsp
    movq %rdi,%r8
    movq %rsi,%rax
    testq %rax,%rax
    jns Lxdpositive
    movb $45,(%r8)
    incq %r8
    negq %rax
Lxdpositive:
    movq %rsp,%r9
    movl $10,%ecx
Lxddigit:
    xorl %edx,%edx
    divq %rcx
    addb $48,%dl
    movb %dl,(%r9)
    incq %r9
    testq %rax,%rax
    jne Lxddigit
Lxdreverse:
    decq %r9
    movb (%r9),%dl
    movb %dl,(%r8)
    incq %r8
    cmpq %rsp,%r9
    jne Lxdreverse
    movq %r8,%rax
    subq %rdi,%rax
    addq $32,%rsp
    ret
.globl CSYM(core_field_fill)
CSYM(core_field_fill):
    pushq %r12
    pushq %r13
    pushq %r14
    subq $32,%rsp
    movq %rdi,%r12
    movq %rsi,%r13
    movq %rdx,%r14
    movq %rcx,%rsi
    movq %rsp,%rdi
    call CSYM(core_decimal)
    cmpq %r14,%rax
    jg Lxfoverflow
    testq %r13,%r13
    js Lxfpanic
    movslq BUF_LENGTH(%r12),%rcx
    cmpq %rcx,%r13
    jg Lxfpanic
    subq %r13,%rcx
    cmpq %rcx,%r14
    jg Lxfpanic
    movq %r14,%r8
    subq %rax,%r8
    movq BUF_BYTES(%r12),%rdi
    addq %r13,%rdi
    movq %rsp,%rsi
    xorl %ecx,%ecx
Lxfloop:
    cmpq %r14,%rcx
    jge Lxfdone
    movb $32,%dl
    cmpq %r8,%rcx
    jl Lxfput
    movb (%rsi),%dl
    incq %rsi
Lxfput:
    movb %dl,(%rdi)
    incq %rdi
    incq %rcx
    jmp Lxfloop
Lxfdone:
    xorl %eax,%eax
    jmp Lxfreturn
Lxfoverflow:
    movl $1,%eax
Lxfreturn:
    addq $32,%rsp
    popq %r14
    popq %r13
    popq %r12
    ret
Lxfpanic:
    leaq Lxfmessage(%rip),%rdi
    addq $32,%rsp
    popq %r14
    popq %r13
    popq %r12
    jmp CSYM(core_host_panic)
#ifdef __APPLE__
.section __TEXT,__cstring,cstring_literals
#else
.section .rodata
#endif
Lxfmessage: .asciz "fill past the reservation"
#ifndef __APPLE__
.section .note.GNU-stack,"",@progbits
#endif
