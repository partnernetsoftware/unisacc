/* Prototype v0.0.97 x86-64 core_run: 64-byte CorePackedState + inlined transition.
 * When m->packed != NULL (formal host-built path), zero extra calloc at entry.
 * When m->packed == NULL (prototype harness on 0.0.93 binary), builds temporary table and frees on exit. */
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
#if (MACHINE_SIZE % 8) != 0 || (RESULT_SIZE % 8) != 0
#error MACHINE_SIZE_and_RESULT_SIZE_must_be_multiples_of_8
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
    subq $392,%rsp
    movq %rsp,%r12
    movq %rdi,%rbx
    movq %r9,%rbp
    movq %r8,%r13
    movq %rsi,%r14
    movq %rcx,320(%rsp)
    movl %edx,328(%rsp)
    movq $0,352(%rsp)
    movq $0,360(%rsp)
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
    /* Resolve CorePackedState table: use host-built CM_PACKED when present, else build prototype table */
    movq CM_PACKED(%rbx),%rax
    testq %rax,%rax
    jne Lxpack_ready
    movl CM_NS(%rbx),%edi
    testl %edi,%edi
    jle Lxpack_ready
    movl $64,%esi
    call CSYM(calloc)
    testq %rax,%rax
    je Lxroom
    movq $1,360(%rsp)
    movq %rax,%rdi
    xorl %ecx,%ecx
    movl CM_NS(%rbx),%r8d
Lxpack_fill:
    cmpl %r8d,%ecx
    jge Lxpack_done
    movslq %ecx,%rdx
    movq CM_MODE(%rbx),%r9
    movl (%r9,%rdx,4),%r10d
    movl %r10d,0(%rdi)
    movq CM_COUNT(%rbx),%r9
    testq %r9,%r9
    je Lxp_nc
    movl (%r9,%rdx,4),%r10d
    movl %r10d,4(%rdi)
Lxp_nc:
    movq CM_LO(%rbx),%r9
    testq %r9,%r9
    je Lxp_nl
    movl (%r9,%rdx,4),%r10d
    movl %r10d,8(%rdi)
Lxp_nl:
    movq CM_HI(%rbx),%r9
    testq %r9,%r9
    je Lxp_nh
    movl (%r9,%rdx,4),%r10d
    movl %r10d,12(%rdi)
Lxp_nh:
    movq CM_BASE_NEXT(%rbx),%r9
    testq %r9,%r9
    je Lxp_nbn
    movl (%r9,%rdx,4),%r10d
    movl %r10d,16(%rdi)
Lxp_nbn:
    movq CM_BASE_SEQ(%rbx),%r9
    testq %r9,%r9
    je Lxp_nbs
    movl (%r9,%rdx,4),%r10d
    movl %r10d,20(%rdi)
Lxp_nbs:
    movq CM_RET_SEQ(%rbx),%r9
    testq %r9,%r9
    je Lxp_nrs
    movl (%r9,%rdx,4),%r10d
    movl %r10d,24(%rdi)
Lxp_nrs:
    movq CM_RET_OK(%rbx),%r9
    testq %r9,%r9
    je Lxp_nro
    movq (%r9,%rdx,8),%r10
    movq %r10,32(%rdi)
Lxp_nro:
    movq CM_KEYS(%rbx),%r9
    testq %r9,%r9
    je Lxp_nk
    movq (%r9,%rdx,8),%r10
    movq %r10,40(%rdi)
Lxp_nk:
    movq CM_NEXT(%rbx),%r9
    testq %r9,%r9
    je Lxp_nn
    movq (%r9,%rdx,8),%r10
    movq %r10,48(%rdi)
Lxp_nn:
    movq CM_SEQ(%rbx),%r9
    testq %r9,%r9
    je Lxp_nsq
    movq (%r9,%rdx,8),%r10
    movq %r10,56(%rdi)
Lxp_nsq:
    addq $64,%rdi
    incl %ecx
    jmp Lxpack_fill
Lxpack_done:
Lxpack_ready:
    movq %rax,352(%rsp)
    xorl %r14d,%r14d
    movl CM_START(%rbx),%r15d
.p2align 4
Lxrloop:
    cmpq %r13,%r14
    jge Lxrtimeout
    incq %r14
    movslq %r15d,%rsi
    shlq $6,%rsi
    addq 352(%rsp),%rsi
    movl 0(%rsi),%eax
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
    movq 32(%rsi),%rax
    testq %rax,%rax
    je Lxt_noret
    testl %edx,%edx
    js Lxt_noret
    cmpl CM_NS(%rbx),%edx
    jge Lxt_noret
    cmpb $0,(%rax,%rdx)
    je Lxt_noret
    movl 24(%rsi),%ecx
    movl %edx,%r15d
    jmp Lxt_dispatch
Lxt_noret:
    cmpl $0,CM_ISNET(%rbx)
    je Lxt_table
    cmpl 8(%rsi),%edx
    jl Lxt_domain
    cmpl 12(%rsi),%edx
    jg Lxt_domain
    movslq 16(%rsi),%r9
    movslq 20(%rsi),%r10
    movl 4(%rsi),%r8d
    testl %r8d,%r8d
    je Lxt_bounds
    movq 40(%rsi),%rdi
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
    movq 48(%rsi),%rcx
    movq 56(%rsi),%rdi
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
    movl 0(%rsi),%r11d
    movq 48(%rsi),%rcx
    movq 56(%rsi),%rdi
    cmpl $1,%r11d
    jne Lxt_direct
    movl 4(%rsi),%r8d
    movq 40(%rsi),%rax
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
    cmpq $0,360(%rsp)
    je Lxpack_nofree
    movq 352(%rsp),%rdi
    call Lxrfree_if
Lxpack_nofree:
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
    addq $392,%rsp
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
