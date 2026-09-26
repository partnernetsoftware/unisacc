/* System V sparse memory. Calls use a 16-byte-aligned stack. Rehash calls
   this same insertion routine, preserving all six used callee-saved GPRs. */
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
.globl CSYM(core_memory_get)
#ifndef __APPLE__
.type core_memory_get,@function
#endif
CSYM(core_memory_get):
    movq MEM_CAP(%rdi),%rdx
    testq %rdx,%rdx
    je Lxmgzero
    decq %rdx
    movabsq $0x9e3779b97f4a7c15,%rax
    imulq %rsi,%rax
    shrq $20,%rax
    andq %rdx,%rax
    movq MEM_USED(%rdi),%r8
    movq MEM_KEYS(%rdi),%r9
Lxmgprobe:
    cmpb $0,(%r8,%rax)
    je Lxmgzero
    cmpq %rsi,(%r9,%rax,8)
    je Lxmgfound
    incq %rax
    andq %rdx,%rax
    jmp Lxmgprobe
Lxmgfound:
    movq MEM_VALUES(%rdi),%rcx
    movq (%rcx,%rax,8),%rax
    ret
Lxmgzero:
    xorl %eax,%eax
    ret
#ifndef __APPLE__
.size core_memory_get,.-core_memory_get
#endif
.p2align 2
.globl CSYM(core_memory_set)
#ifndef __APPLE__
.type core_memory_set,@function
#endif
CSYM(core_memory_set):
    pushq %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    subq $24,%rsp
    movq %rdi,%r12
    movq %rsi,%r13
    movq %rdx,%r14
    movq MEM_CAP(%r12),%r15
    movq MEM_COUNT(%r12),%rax
    incq %rax
    addq %rax,%rax
    cmpq %r15,%rax
    jbe Lxmsinsert
    movq %r15,%rax
    shrq $60,%rax
    jne Lxmsoverflow
    movq MEM_KEYS(%r12),%rbp
    movq MEM_VALUES(%r12),%rax
    movq %rax,0(%rsp)
    movq MEM_USED(%r12),%rax
    movq %rax,8(%rsp)
    leaq (%r15,%r15),%rdi
    testq %rdi,%rdi
    jne Lxmscap
    movl $65536,%edi
Lxmscap:
    movq %rdi,MEM_CAP(%r12)
    movl $8,%esi
    call CSYM(calloc)
    movq %rax,MEM_KEYS(%r12)
    movq MEM_CAP(%r12),%rdi
    movl $8,%esi
    call CSYM(calloc)
    movq %rax,MEM_VALUES(%r12)
    movq MEM_CAP(%r12),%rdi
    movl $1,%esi
    call CSYM(calloc)
    movq %rax,MEM_USED(%r12)
    movq $0,MEM_COUNT(%r12)
    testq %rax,%rax
    je Lxmsoom
    cmpq $0,MEM_KEYS(%r12)
    je Lxmsoom
    cmpq $0,MEM_VALUES(%r12)
    je Lxmsoom
    xorl %ebx,%ebx
Lxmsrehash:
    cmpq %r15,%rbx
    jae Lxmsfree
    movq 8(%rsp),%rax
    cmpb $0,(%rax,%rbx)
    je Lxmsnext
    movq %r12,%rdi
    movq (%rbp,%rbx,8),%rsi
    movq 0(%rsp),%rax
    movq (%rax,%rbx,8),%rdx
    call CSYM(core_memory_set)
Lxmsnext:
    incq %rbx
    jmp Lxmsrehash
Lxmsfree:
    movq %rbp,%rdi
    call CSYM(free)
    movq 0(%rsp),%rdi
    call CSYM(free)
    movq 8(%rsp),%rdi
    call CSYM(free)
Lxmsinsert:
    movq MEM_CAP(%r12),%rcx
    decq %rcx
    movabsq $0x9e3779b97f4a7c15,%rax
    imulq %r13,%rax
    shrq $20,%rax
    andq %rcx,%rax
    movq MEM_USED(%r12),%rdx
    movq MEM_KEYS(%r12),%rsi
Lxmsprobe:
    cmpb $0,(%rdx,%rax)
    je Lxmsnew
    cmpq %r13,(%rsi,%rax,8)
    je Lxmsvalue
    incq %rax
    andq %rcx,%rax
    jmp Lxmsprobe
Lxmsnew:
    movb $1,(%rdx,%rax)
    movq %r13,(%rsi,%rax,8)
    incq MEM_COUNT(%r12)
Lxmsvalue:
    movq MEM_VALUES(%r12),%rdx
    movq %r14,(%rdx,%rax,8)
    xorl %eax,%eax
    jmp Lxmsreturn
Lxmsoom:
    leaq Lxmsoom_text(%rip),%rax
    jmp Lxmsreturn
Lxmsoverflow:
    leaq Lxmsoverflow_text(%rip),%rax
Lxmsreturn:
    addq $24,%rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    testq %rax,%rax
    jne Lxmspanic
    ret
Lxmspanic:
    movq %rax,%rdi
    jmp CSYM(core_host_panic)
#ifndef __APPLE__
.size core_memory_set,.-core_memory_set
.section .rodata
#else
.section __TEXT,__cstring,cstring_literals
#endif
Lxmsoom_text: .asciz "out of memory"
Lxmsoverflow_text: .asciz "memory capacity overflow"
#ifndef __APPLE__
.section .note.GNU-stack,"",@progbits
#endif
