/* System V byte-string interning, with exact C hash and insertion order.
   All library calls use a 16-byte-aligned stack; x86 callee-saved GPRs survive. */
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
.globl CSYM(core_hash_bytes)
#ifndef __APPLE__
.type core_hash_bytes,@function
#endif
CSYM(core_hash_bytes):
    movabsq $0x14650fb0739d0383,%rax
    testl %esi,%esi
    jle Lxhashdone
    movabsq $0x100000001b3,%rcx
Lxhashloop:
    movzbl (%rdi),%edx
    incq %rdi
    xorq %rdx,%rax
    imulq %rcx,%rax
    decl %esi
    jne Lxhashloop
Lxhashdone:
    ret
#ifndef __APPLE__
.size core_hash_bytes,.-core_hash_bytes
#endif
.p2align 2
.globl CSYM(core_string_intern)
#ifndef __APPLE__
.type core_string_intern,@function
#endif
CSYM(core_string_intern):
    pushq %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    subq $24,%rsp
    movq %rdi,%r12
    movq %rsi,%r13
    movl %edx,%r14d
    movq INTERN_CAP(%r12),%rbp
    movq INTERN_COUNT(%r12),%rax
    incq %rax
    addq %rax,%rax
    cmpq %rbp,%rax
    jbe Lxislookup
    movabsq $0x0555555555555555,%rax
    cmpq %rax,%rbp
    ja Lxisoverflow
    movq INTERN_ENTRIES(%r12),%r15
    leaq (%rbp,%rbp),%rdi
    testq %rdi,%rdi
    jne Lxiscap
    movl $1024,%edi
Lxiscap:
    movq %rdi,INTERN_CAP(%r12)
    movl $ENTRY_SIZE,%esi
    call CSYM(calloc)
    movq %rax,INTERN_ENTRIES(%r12)
    testq %rax,%rax
    je Lxisoom
    xorl %ebx,%ebx
Lxisrehash:
    cmpq %rbp,%rbx
    jae Lxisfree
    leaq (%rbx,%rbx,2),%rax
    leaq (%r15,%rax,8),%rcx
    movq %rcx,0(%rsp)
    movq ENTRY_BYTES(%rcx),%rdi
    testq %rdi,%rdi
    je Lxisnext
    movl ENTRY_LENGTH(%rcx),%esi
    call CSYM(core_hash_bytes)
    movq INTERN_CAP(%r12),%rcx
    decq %rcx
    andq %rcx,%rax
Lxisfind:
    leaq (%rax,%rax,2),%rdx
    movq INTERN_ENTRIES(%r12),%rsi
    leaq (%rsi,%rdx,8),%rdx
    cmpq $0,ENTRY_BYTES(%rdx)
    je Lxiscopy
    incq %rax
    andq %rcx,%rax
    jmp Lxisfind
Lxiscopy:
    movq 0(%rsp),%rsi
    movq 0(%rsi),%rdi
    movq 8(%rsi),%r8
    movq 16(%rsi),%r9
    movq %rdi,0(%rdx)
    movq %r8,8(%rdx)
    movq %r9,16(%rdx)
Lxisnext:
    incq %rbx
    jmp Lxisrehash
Lxisfree:
    movq %r15,%rdi
    call CSYM(free)
Lxislookup:
    movq %r13,%rdi
    movl %r14d,%esi
    call CSYM(core_hash_bytes)
    movq INTERN_CAP(%r12),%rcx
    decq %rcx
    andq %rcx,%rax
    movq %rax,%rbx
Lxisprobe:
    leaq (%rbx,%rbx,2),%rax
    movq INTERN_ENTRIES(%r12),%rcx
    leaq (%rcx,%rax,8),%rcx
    movq %rcx,0(%rsp)
    movq ENTRY_BYTES(%rcx),%rdi
    testq %rdi,%rdi
    je Lxisnew
    cmpl %r14d,ENTRY_LENGTH(%rcx)
    jne Lxisadvance
    movq %r13,%rsi
    movq %r14,%rdx
    call CSYM(memcmp)
    testl %eax,%eax
    je Lxisfound
Lxisadvance:
    movq INTERN_CAP(%r12),%rcx
    decq %rcx
    incq %rbx
    andq %rcx,%rbx
    jmp Lxisprobe
Lxisnew:
    xorl %edi,%edi
    leaq 1(%r14),%rsi
    call CSYM(realloc)
    testq %rax,%rax
    je Lxisoom
    movq 0(%rsp),%rcx
    movq %rax,ENTRY_BYTES(%rcx)
    movq %rax,%rdi
    movq %r13,%rsi
    movq %r14,%rdx
    call CSYM(memcpy)
    movq 0(%rsp),%rcx
    movl %r14d,ENTRY_LENGTH(%rcx)
    movq INTERN_COUNT(%r12),%rax
    incq %rax
    movq %rax,INTERN_COUNT(%r12)
    movq %rax,ENTRY_VALUE(%rcx)
    jmp Lxisok
Lxisfound:
    movq 0(%rsp),%rcx
    movq ENTRY_VALUE(%rcx),%rax
Lxisok:
    xorl %ecx,%ecx
    jmp Lxisreturn
Lxisoom:
    leaq Lxisoom_text(%rip),%rax
    jmp Lxiserror
Lxisoverflow:
    leaq Lxisoverflow_text(%rip),%rax
Lxiserror:
    movl $1,%ecx
Lxisreturn:
    addq $24,%rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    testl %ecx,%ecx
    jne Lxispanic
    ret
Lxispanic:
    movq %rax,%rdi
    jmp CSYM(core_host_panic)
#ifndef __APPLE__
.size core_string_intern,.-core_string_intern
.section .rodata
#else
.section __TEXT,__cstring,cstring_literals
#endif
Lxisoom_text: .asciz "out of memory"
Lxisoverflow_text: .asciz "intern capacity overflow"
#ifndef __APPLE__
.section .note.GNU-stack,"",@progbits
#endif
