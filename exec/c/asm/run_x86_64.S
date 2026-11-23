/* System V complete generic run lifecycle; no C execution fallback. */
#include "layout.h"
#include "../core.h"
#ifdef _WIN32
#error Windows_x64_binding_not_implemented
#endif
#ifdef __APPLE__
#define CSYM1(x) _##x
#define CSYM(x) CSYM1(x)
#else
#define CSYM(x) x
#endif
#if (MACHINE_SIZE % 8) != 0 || (RESULT_SIZE % 8) != 0 || PS_SIZE != 64
#error ABI_alignment_or_PS_SIZE_mismatch
#endif
.text
.p2align 2
Lxrfree_if:
    testq %rdi,%rdi
    je Lxrfree_skip
    jmp CSYM(free)
Lxrfree_skip:
    ret
.p2align 2
Lxrzero:
    movq $0,(%rdi)
    addq $8,%rdi
    subl $8,%ecx
    jne Lxrzero
    ret
.p2align 2
.macro FREE_MEMBER off
    movq \off(%r12),%rdi
    call Lxrfree_if
.endm
.globl CSYM(core_run)
CSYM(core_run):
    pushq %rbx
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    subq $360,%rsp
    movq %rsp,%r12
    movq %rdi,%rbx
    movq %r9,%rbp
    movq %r8,%r13
    movq %rsi,%r14
    movq %rcx,320(%rsp)
    movl %edx,328(%rsp)
    movq %r12,%rdi
    movl $MACHINE_SIZE,%ecx
    call Lxrzero
    movq %rbp,%rdi
    movl $RESULT_SIZE,%ecx
    call Lxrzero
    movq %rbx,M_MODEL(%r12)
    movq %rbp,M_RESULT(%r12)
    movq %r14,M_INPUT(%r12)
    movq %r14,M_X(%r12)
    movl CM_NRG(%rbx),%edi
    incl %edi
    movl $8,%esi
    call CSYM(calloc)
    testq %rax,%rax
    je Lxroom
    movq %rax,M_REGS(%r12)
    leaq M_BLOBS(%r12),%rdi
    leaq Lxrempty(%rip),%rsi
    xorl %edx,%edx
    call CSYM(core_blob_add)
    movq 320(%rsp),%rdi
    call CSYM(strlen)
    movl %eax,%edx
    movq 320(%rsp),%rsi
    leaq M_BLOBS(%r12),%rdi
    call CSYM(core_blob_add)
    movl 328(%rsp),%edi
    incl %edi
    movl $8,%esi
    call CSYM(calloc)
    testq %rax,%rax
    je Lxroom
    movq %rax,M_XATTR(%r12)
    movl 328(%rsp),%ecx
    movq %rcx,M_XN(%r12)
    movq %r14,288(%rsp)
    movq %rax,296(%rsp)
    movq $0,304(%rsp)
    movq %rcx,312(%rsp)
    leaq M_FRAMES(%r12),%rdi
    leaq 288(%rsp),%rsi
    call CSYM(core_frame_push)
    xorl %r14d,%r14d
    movl CM_START(%rbx),%r15d
.p2align 4
Lxrloop:
    cmpq %r13,%r14
    jge Lxrtimeout
    incq %r14
    movq CM_PACKED(%rbx),%rsi
    testq %rsi,%rsi
    je Lxrunpacked
    movslq %r15d,%rax
    shlq $6,%rax
    addq %rax,%rsi
    movl PS_MODE(%rsi),%eax
    cmpl $1,%eax
    je Lxrstack
    testl %eax,%eax
    jne Lxrresult
    movq M_FRAMES+STACK_ENTRIES(%r12),%rax
    movl M_FRAMES+STACK_COUNT(%r12),%ecx
    decl %ecx
    shlq $5,%rcx
    addq %rcx,%rax
    movq FRAME_CURSOR(%rax),%rcx
    movl $256,%edx
    cmpq FRAME_END(%rax),%rcx
    jge Lxrkey
    movq FRAME_BYTES(%rax),%rax
    movzbl (%rax,%rcx),%edx
    jmp Lxrkey
Lxrstack:
    movl $-1,%edx
    movl M_STACK+STACK_COUNT(%r12),%eax
    testl %eax,%eax
    je Lxrkey
    decl %eax
    movq M_STACK+STACK_ENTRIES(%r12),%rcx
    movl (%rcx,%rax,4),%edx
    jmp Lxrkey
Lxrresult:
    movq M_R(%r12),%rdx
    movl $256,%eax
    cmpq %rax,%rdx
    cmova %rax,%rdx
Lxrkey:
    /* Inlined transition reading from single 64-byte CorePackedState at (%rsi) */
    movslq %edx,%rdx
    movq PS_RET_OK(%rsi),%rax
    testq %rax,%rax
    je Lxt_noret
    testl %edx,%edx
    js Lxt_noret
    cmpl CM_NS(%rbx),%edx
    jge Lxt_noret
    cmpb $0,(%rax,%rdx)
    je Lxt_noret
    movl PS_RET_SEQ(%rsi),%ecx
    movl %edx,%r15d
    jmp Lxt_dispatch
Lxt_noret:
    cmpl $0,CM_ISNET(%rbx)
    je Lxt_table
    cmpl PS_LO(%rsi),%edx
    jl Lxt_domain
    cmpl PS_HI(%rsi),%edx
    jg Lxt_domain
    movslq PS_BASE_NEXT(%rsi),%r9
    movslq PS_BASE_SEQ(%rsi),%r10
    movl PS_COUNT(%rsi),%r8d
    testl %r8d,%r8d
    je Lxt_bounds
    movq PS_KEYS(%rsi),%rdi
    xorl %r11d,%r11d
Lxt_search:
    cmpl %r8d,%r11d
    jae Lxt_sum
    leal (%r11,%r8),%eax
    shrl $1,%eax
    leal 1(%rax),%ecx
    cmpl (%rdi,%rax,4),%edx
    cmovgel %ecx,%r11d
    cmovll %eax,%r8d
    jmp Lxt_search
Lxt_sum:
    testl %r11d,%r11d
    je Lxt_bounds
    movq PS_NEXT(%rsi),%rcx
    movq PS_SEQ(%rsi),%rdi
Lxt_unit:
    movslq (%rcx),%rax
    movslq (%rdi),%r8
    addq %rax,%r9
    addq %r8,%r10
    addq $4,%rcx
    addq $4,%rdi
    decl %r11d
    jne Lxt_unit
Lxt_bounds:
    cmpq $-1,%r9
    jl Lxt_invalid
    movslq CM_NS(%rbx),%rax
    cmpq %rax,%r9
    jge Lxt_invalid
    testq %r10,%r10
    js Lxt_invalid
    movslq CM_NQ(%rbx),%rax
    cmpq %rax,%r10
    jge Lxt_invalid
    movl %r9d,%r15d
    movl %r10d,%ecx
    jmp Lxt_dispatch
Lxt_table:
    movl PS_MODE(%rsi),%r11d
    movq PS_NEXT(%rsi),%rcx
    movq PS_SEQ(%rsi),%rdi
    cmpl $1,%r11d
    jne Lxt_direct
    movl PS_COUNT(%rsi),%r8d
    movq PS_KEYS(%rsi),%rax
    testl %r8d,%r8d
    je Lxrmissing
Lxt_find:
    cmpl (%rax),%edx
    je Lxt_found
    addq $4,%rax
    addq $4,%rcx
    addq $4,%rdi
    decl %r8d
    jne Lxt_find
    jmp Lxrmissing
Lxt_direct:
    leaq (%rcx,%rdx,4),%rcx
    leaq (%rdi,%rdx,4),%rdi
Lxt_found:
    movl (%rcx),%r15d
    movl (%rdi),%ecx
    jmp Lxt_dispatch
Lxrunpacked:
    movq CM_MODE(%rbx),%rax
    movl (%rax,%r15,4),%eax
    cmpl $1,%eax
    je Lxru_stack
    testl %eax,%eax
    jne Lxru_result
    movq M_FRAMES+STACK_ENTRIES(%r12),%rax
    movl M_FRAMES+STACK_COUNT(%r12),%ecx
    decl %ecx
    shlq $5,%rcx
    addq %rcx,%rax
    movq FRAME_CURSOR(%rax),%rcx
    movl $256,%edx
    cmpq FRAME_END(%rax),%rcx
    jge Lxru_key
    movq FRAME_BYTES(%rax),%rax
    movzbl (%rax,%rcx),%edx
    jmp Lxru_key
Lxru_stack:
    movl $-1,%edx
    movl M_STACK+STACK_COUNT(%r12),%eax
    testl %eax,%eax
    je Lxru_key
    decl %eax
    movq M_STACK+STACK_ENTRIES(%r12),%rcx
    movl (%rcx,%rax,4),%edx
    jmp Lxru_key
Lxru_result:
    movq M_R(%r12),%rdx
    movl $256,%eax
    cmpq %rax,%rdx
    cmova %rax,%rdx
Lxru_key:
    movl $-1,280(%rsp)
    movl $0,284(%rsp)
    movq %rbx,%rdi
    movl %r15d,%esi
    leaq 280(%rsp),%rcx
    leaq 284(%rsp),%r8
    call CSYM(core_transition)
    movq %rax,RESULT_REASON(%rbp)
    testq %rax,%rax
    jne Lxrtransitionbad
    movl 280(%rsp),%r15d
    movl 284(%rsp),%ecx
Lxt_dispatch:
    testl %r15d,%r15d
    js Lxrmissing
    movq CM_QOFF(%rbx),%rax
    movslq (%rax,%rcx,4),%rax
    shlq $3,%rax
    addq CM_QA(%rbx),%rax
    movq %rax,336(%rsp)
    movq CM_QLEN(%rbx),%rax
    movl (%rax,%rcx,4),%eax
    movl %eax,348(%rsp)
    movl $0,344(%rsp)
Lxractions:
    movl 344(%rsp),%eax
    cmpl 348(%rsp),%eax
    jge Lxrloop
    movq %r12,%rdi
    movq 336(%rsp),%rsi
    call CSYM(core_action)
    testl %eax,%eax
    jne Lxrfinished
    movq 336(%rsp),%rdx
    movl (%rdx),%eax
    leaq Lxrarity(%rip),%rcx
    movzbl (%rcx,%rax),%eax
    incl %eax
    leaq (%rdx,%rax,8),%rax
    movq %rax,336(%rsp)
    incl 344(%rsp)
    jmp Lxractions
Lxt_domain:
    leaq Lxt_domain_text(%rip),%rax
    movq %rax,RESULT_REASON(%rbp)
    jmp Lxrtransitionbad
Lxt_invalid:
    leaq Lxt_invalid_text(%rip),%rax
    movq %rax,RESULT_REASON(%rbp)
    jmp Lxrtransitionbad
Lxrtimeout:
    leaq Lxrtime(%rip),%rax
    movq %rax,RESULT_REASON(%rbp)
    movl $3,M_STATUS(%r12)
    jmp Lxrfinished
Lxrmissing:
    leaq Lxrnone(%rip),%rax
    movq %rax,RESULT_REASON(%rbp)
Lxrtransitionbad:
    movl $2,M_STATUS(%r12)
Lxrfinished:
    movq %r14,RESULT_STEPS(%rbp)
    cmpl $0,M_STATUS(%r12)
    jne Lxrerrorout
    movq M_OUT+BUF_BYTES(%r12),%rax
    movl M_OUT+BUF_LENGTH(%r12),%ecx
    movq %rax,RESULT_OUT+BUF_BYTES(%rbp)
    movl %ecx,RESULT_OUT+BUF_LENGTH(%rbp)
    movq $0,M_OUT+BUF_BYTES(%r12)
Lxrerrorout:
    movq M_ERR+BUF_BYTES(%r12),%rax
    movl M_ERR+BUF_LENGTH(%r12),%ecx
    movq %rax,RESULT_ERR+BUF_BYTES(%rbp)
    movl %ecx,RESULT_ERR+BUF_LENGTH(%rbp)
    movq $0,M_ERR+BUF_BYTES(%r12)
    FREE_MEMBER M_OUT+BUF_BYTES
    FREE_MEMBER M_OUT+BUF_ATTR
    FREE_MEMBER M_ERR+BUF_BYTES
    FREE_MEMBER M_ERR+BUF_ATTR
    FREE_MEMBER M_SCRATCH+BUF_BYTES
    FREE_MEMBER M_SCRATCH+BUF_ATTR
    FREE_MEMBER M_FRAMES+STACK_ENTRIES
    FREE_MEMBER M_STACK+STACK_ENTRIES
    movq M_X(%r12),%rdi
    cmpq M_INPUT(%r12),%rdi
    je Lxrkeepinput
    call Lxrfree_if
Lxrkeepinput:
    FREE_MEMBER M_XATTR
    FREE_MEMBER M_REGS
    FREE_MEMBER M_MEMORY+MEM_KEYS
    FREE_MEMBER M_MEMORY+MEM_VALUES
    FREE_MEMBER M_MEMORY+MEM_USED
    movl M_BLOBS+BLOBS_COUNT(%r12),%r15d
    movq M_BLOBS+BLOBS_ENTRIES(%r12),%r14
    testl %r15d,%r15d
    je Lxrblobsdone
Lxrblobs:
    movq BLOB_BYTES(%r14),%rdi
    call Lxrfree_if
    addq $BLOB_SIZE,%r14
    decl %r15d
    jne Lxrblobs
Lxrblobsdone:
    FREE_MEMBER M_BLOBS+BLOBS_ENTRIES
    movq M_STRINGS+INTERN_CAP(%r12),%r15
    movq M_STRINGS+INTERN_ENTRIES(%r12),%r14
    testq %r15,%r15
    je Lxrstringsdone
Lxrstrings:
    movq ENTRY_BYTES(%r14),%rdi
    testq %rdi,%rdi
    je Lxrstringnext
    call CSYM(free)
Lxrstringnext:
    addq $ENTRY_SIZE,%r14
    decq %r15
    jne Lxrstrings
Lxrstringsdone:
    FREE_MEMBER M_STRINGS+INTERN_ENTRIES
    movl M_RESOURCES+RES_COUNT(%r12),%r15d
    movq M_RESOURCES+RES_ENTRIES(%r12),%r14
    testl %r15d,%r15d
    je Lxrresourcesdone
Lxrresources:
    movq RES_KEY(%r14),%rdi
    call Lxrfree_if
    addq $RES_ENTRY_SIZE,%r14
    decl %r15d
    jne Lxrresources
Lxrresourcesdone:
    FREE_MEMBER M_RESOURCES+RES_ENTRIES
    movl M_STATUS(%r12),%eax
    addq $360,%rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    popq %rbx
    ret
Lxroom:
    leaq Lxroommsg(%rip),%rdi
    call CSYM(core_host_panic)
    ud2
.p2align 2
Lxrarity:
    .byte CORE_ACTION_ARITIES
#ifdef __APPLE__
.section __TEXT,__cstring,cstring_literals
#else
.section .rodata
#endif
Lxrempty: .asciz ""
Lxrtime: .asciz "timeout"
Lxrnone: .asciz "no transition"
Lxroommsg: .asciz "out of memory"
Lxt_domain_text: .asciz "observation outside network domain"
Lxt_invalid_text: .asciz "invalid network output"
#ifndef __APPLE__
.section .note.GNU-stack,"",@progbits
#endif
