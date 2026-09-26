/* System V byte blocks and resource cache; same allocation/fetch/copy/free
   order as the C implementation. Cache miss zero is stored, not re-fetched. */
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
.globl CSYM(core_blob_add)
#ifndef __APPLE__
.type core_blob_add,@function
#endif
CSYM(core_blob_add):
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    movq %rdi,%r12
    movq %rsi,%r13
    movl %edx,%r14d
    movl BLOBS_COUNT(%r12),%eax
    movl BLOBS_CAP(%r12),%ecx
    cmpl %ecx,%eax
    jl Lxblslot
    cmpl $0x3fffffff,%ecx
    jg Lxbloverflow
    addl %ecx,%ecx
    jne Lxblgrow
    movl $64,%ecx
Lxblgrow:
    movl %ecx,BLOBS_CAP(%r12)
    movq BLOBS_ENTRIES(%r12),%rdi
    movl %ecx,%esi
    shlq $4,%rsi
    call CSYM(realloc)
    testq %rax,%rax
    je Lxblnomem
    movq %rax,BLOBS_ENTRIES(%r12)
Lxblslot:
    movl BLOBS_COUNT(%r12),%ebx
    movq %rbx,%rax
    shlq $4,%rax
    addq BLOBS_ENTRIES(%r12),%rax
    movq %rax,%r15
    xorl %edi,%edi
    leaq 1(%r14),%rsi
    call CSYM(realloc)
    testq %rax,%rax
    je Lxblnomem
    movq %rax,BLOB_BYTES(%r15)
    movq %rax,%rdi
    movq %r13,%rsi
    movq %r14,%rdx
    call CSYM(memcpy)
    movl %r14d,BLOB_LENGTH(%r15)
    movl %ebx,%eax
    incl %ebx
    movl %ebx,BLOBS_COUNT(%r12)
    xorl %ecx,%ecx
    jmp Lxblreturn
Lxblnomem:
    leaq Lxbytesoom(%rip),%rax
    jmp Lxblerror
Lxbloverflow:
    leaq Lxbloboverflow(%rip),%rax
Lxblerror:
    movl $1,%ecx
Lxblreturn:
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    testl %ecx,%ecx
    jne Lxbytespanic
    ret
#ifndef __APPLE__
.size core_blob_add,.-core_blob_add
#endif
.p2align 2
.globl CSYM(core_resource_find)
#ifndef __APPLE__
.type core_resource_find,@function
#endif
CSYM(core_resource_find):
    pushq %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    subq $40,%rsp
    movq %rdi,%r12
    movq %rsi,%r13
    movq %rdx,%r14
    movl %ecx,%r15d
    xorl %ebx,%ebx
Lxrfscan:
    cmpl RES_COUNT(%r12),%ebx
    jge Lxrfmiss
    movq %rbx,%rbp
    shlq $4,%rbp
    addq RES_ENTRIES(%r12),%rbp
    cmpl %r15d,RES_LENGTH(%rbp)
    jne Lxrfnext
    movq RES_KEY(%rbp),%rdi
    movq %r14,%rsi
    movq %r15,%rdx
    call CSYM(memcmp)
    testl %eax,%eax
    je Lxrffound
Lxrfnext:
    incl %ebx
    jmp Lxrfscan
Lxrffound:
    movl RES_ID(%rbp),%eax
    xorl %ecx,%ecx
    jmp Lxrfreturn
Lxrfmiss:
    cmpl $0x7fffffff,%ebx
    je Lxrfoverflow
    movq $0,0(%rsp)
    movl $0,8(%rsp)
    movl $0,16(%rsp)
    movq %r14,%rdi
    movl %r15d,%esi
    leaq 0(%rsp),%rdx
    leaq 8(%rsp),%rcx
    call CSYM(core_host_fetch)
    movl %eax,12(%rsp)
    testl %eax,%eax
    je Lxrfcache
    movq %r13,%rdi
    movq 0(%rsp),%rsi
    movl 8(%rsp),%edx
    call CSYM(core_blob_add)
    movl %eax,16(%rsp)
    cmpl $2,12(%rsp)
    jne Lxrfcache
    movq 0(%rsp),%rdi
    call CSYM(free)
Lxrfcache:
    movq RES_ENTRIES(%r12),%rdi
    leaq 1(%rbx),%rsi
    shlq $4,%rsi
    call CSYM(realloc)
    testq %rax,%rax
    je Lxrfnomem
    movq %rax,RES_ENTRIES(%r12)
    movq %rbx,%rbp
    shlq $4,%rbp
    addq %rax,%rbp
    xorl %edi,%edi
    leaq 1(%r15),%rsi
    call CSYM(realloc)
    testq %rax,%rax
    je Lxrfnomem
    movq %rax,RES_KEY(%rbp)
    movq %rax,%rdi
    movq %r14,%rsi
    movq %r15,%rdx
    call CSYM(memcpy)
    movl %r15d,RES_LENGTH(%rbp)
    movl 16(%rsp),%eax
    movl %eax,RES_ID(%rbp)
    incl %ebx
    movl %ebx,RES_COUNT(%r12)
    xorl %ecx,%ecx
    jmp Lxrfreturn
Lxrfnomem:
    leaq Lxbytesoom(%rip),%rax
    jmp Lxrferror
Lxrfoverflow:
    leaq Lxresoverflow(%rip),%rax
Lxrferror:
    movl $1,%ecx
Lxrfreturn:
    addq $40,%rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    testl %ecx,%ecx
    jne Lxbytespanic
    ret
Lxbytespanic:
    movq %rax,%rdi
    jmp CSYM(core_host_panic)
#ifndef __APPLE__
.size core_resource_find,.-core_resource_find
.section .rodata
#else
.section __TEXT,__cstring,cstring_literals
#endif
Lxbytesoom: .asciz "out of memory"
Lxbloboverflow: .asciz "blob capacity overflow"
Lxresoverflow: .asciz "resource cache capacity overflow"
#ifndef __APPLE__
.section .note.GNU-stack,"",@progbits
#endif
