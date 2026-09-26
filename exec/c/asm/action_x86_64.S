/* System V generic action engine; lifecycle remains in the driver. */
#include "layout.h"
#include "actions.h"
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
.globl CSYM(core_action)
CSYM(core_action):
    pushq %rbx
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    subq $72,%rsp
    movq %rdi,%r12
    movq %rsi,%r13
    movq M_REGS(%r12),%r14
    movq M_FRAMES+STACK_ENTRIES(%r12),%r15
    movl M_FRAMES+STACK_COUNT(%r12),%eax
    decl %eax
    shlq $5,%rax
    addq %rax,%r15
    movl (%r13),%ebx
    cmpl $OP_COUNT-1,%ebx
    ja Lxabad
    leaq Lxatable(%rip),%rcx
    movslq (%rcx,%rbx,4),%rax
    addq %rcx,%rax
    jmp *%rax
LxaADV:
    incq FRAME_CURSOR(%r15)
    jmp Lxacontinue
LxaMARK:
    movq FRAME_CURSOR(%r15),%rax
    jmp Lxawrite
LxaJUMP:
    movq 8(%r13),%rax
    movq (%r14,%rax,8),%rax
    movq %rax,FRAME_CURSOR(%r15)
    jmp Lxacontinue
LxaLDI:
    movq 16(%r13),%rax
    jmp Lxawrite
LxaCOPYW:
    movq 16(%r13),%rax
    movq (%r14,%rax,8),%rax
    jmp Lxawrite
LxaALU:
    movl 8(%r13),%edi
    movq 24(%r13),%rax
    movq (%r14,%rax,8),%rsi
    movq 32(%r13),%rdx
    cmpl $OP_ALUI,%ebx
    je Lxaaluargs
    cmpl $OP_A64I,%ebx
    je Lxaaluargs
    movq (%r14,%rdx,8),%rdx
Lxaaluargs:
    cmpl $OP_A64,%ebx
    jge Lxaalu64
    call CSYM(core_alu32)
    jmp Lxawrite2
Lxaalu64:
    leaq 64(%rsp),%rcx
    call CSYM(core_alu64)
    movslq 64(%rsp),%rcx
    movq %rcx,M_R(%r12)
Lxawrite2:
    movq 16(%r13),%rcx
    movq %rax,(%r14,%rcx,8)
    jmp Lxacontinue
LxaCMP:
    movq 8(%r13),%rcx
    movq (%r14,%rcx,8),%r8
    movq 16(%r13),%r9
    cmpl $OP_CMPI,%ebx
    je Lxacmpvalues
    movq (%r14,%r9,8),%r9
Lxacmpvalues:
    xorl %eax,%eax
    xorl %ecx,%ecx
    cmpl $OP_C64U,%ebx
    je Lxacmpunsigned
    cmpq %r9,%r8
    setge %al
    setg %cl
    addq %rcx,%rax
    jmp Lxarwrite
Lxacmpunsigned:
    cmpq %r9,%r8
    setae %al
    seta %cl
    addq %rcx,%rax
    jmp Lxarwrite
LxaRLD:
    movq 8(%r13),%rax
    movq (%r14,%rax,8),%rax
    movl $256,%ecx
    cmpq %rcx,%rax
    cmova %rcx,%rax
    jmp Lxarwrite
LxaINC:
    movq 8(%r13),%rax
    movl (%r14,%rax,8),%eax
    incl %eax
    jmp Lxawrite
LxaLDX:
    movq 16(%r13),%rax
    movq (%r14,%rax,8),%rsi
    addq 24(%r13),%rsi
    leaq M_MEMORY(%r12),%rdi
    call CSYM(core_memory_get)
    jmp Lxawrite
LxaSTX:
    movq 8(%r13),%rax
    movq (%r14,%rax,8),%rsi
    addq 16(%r13),%rsi
    movq 24(%r13),%rax
    movq (%r14,%rax,8),%rdx
    leaq M_MEMORY(%r12),%rdi
    call CSYM(core_memory_set)
    jmp Lxacontinue
LxaOUT:
    movq 8(%r13),%rsi
    cmpl $OP_OUT,%ebx
    je Lxaoutvalue
    movq (%r14,%rsi,8),%rsi
Lxaoutvalue:
    movq M_OT(%r12),%rdx
    jmp Lxaout
LxaCOPY:
    movq FRAME_CURSOR(%r15),%rax
    cmpq FRAME_END(%r15),%rax
    jge Lxacontinue
    movq FRAME_BYTES(%r15),%rcx
    movzbl (%rcx,%rax),%esi
    movq M_OT(%r12),%rdx
    cmpl $OP_COPYT,%ebx
    je Lxaout
    movq FRAME_ATTR(%r15),%rcx
    testq %rcx,%rcx
    je Lxaout
    movq (%rcx,%rax,8),%rdx
Lxaout:
    leaq M_OUT(%r12),%rdi
    cmpl $0,M_OSEL(%r12)
    je Lxaoutcall
    leaq M_ERR(%r12),%rdi
    xorl %edx,%edx
Lxaoutcall:
    call CSYM(core_buffer_put)
    jmp Lxacontinue
LxaSPAN:
    movq 8(%r13),%rax
    movq (%r14,%rax,8),%rbp
    movq FRAME_CURSOR(%r15),%rax
    cmpl $OP_SPAN2,%ebx
    jne Lxaspanend
    movq 16(%r13),%rax
    movq (%r14,%rax,8),%rax
Lxaspanend:
    leaq M_OUT(%r12),%rcx
    cmpl $1,M_OSEL(%r12)
    jne Lxaspanclamp
    leaq M_ERR(%r12),%rcx
    jmp Lxaspanclamp
LxaSBSPAN:
    movq 8(%r13),%rax
    movq (%r14,%rax,8),%rbp
    movq 16(%r13),%rax
    movq (%r14,%rax,8),%rax
    leaq M_SCRATCH(%r12),%rcx
Lxaspanclamp:
    movq %rcx,48(%rsp)
    xorq %rdx,%rdx
    testq %rbp,%rbp
    cmovs %rdx,%rbp
    cmpq FRAME_END(%r15),%rax
    cmovg FRAME_END(%r15),%rax
    movq %rax,40(%rsp)
Lxaspanloop:
    cmpq 40(%rsp),%rbp
    jge Lxacontinue
    movq FRAME_BYTES(%r15),%rax
    movzbl (%rax,%rbp),%esi
    xorl %edx,%edx
    movq 48(%rsp),%rdi
    leaq M_OUT(%r12),%rax
    cmpq %rax,%rdi
    jne Lxaspanput
    movq M_OT(%r12),%rdx
    cmpl $OP_SPANT,%ebx
    je Lxaspanput
    movq FRAME_ATTR(%r15),%rax
    testq %rax,%rax
    je Lxaspanput
    movq (%rax,%rbp,8),%rdx
Lxaspanput:
    call CSYM(core_buffer_put)
    incq %rbp
    jmp Lxaspanloop
LxaOLAST:
    movl $256,%eax
    movl M_OUT+BUF_LENGTH(%r12),%ecx
    testl %ecx,%ecx
    je Lxarwrite
    decl %ecx
    movq M_OUT+BUF_BYTES(%r12),%rdx
    movzbl (%rdx,%rcx),%eax
    jmp Lxarwrite
LxaODROP:
    cmpl $0,M_OUT+BUF_LENGTH(%r12)
    je Lxacontinue
    decl M_OUT+BUF_LENGTH(%r12)
    jmp Lxacontinue
LxaOLEN:
    movslq M_OUT+BUF_LENGTH(%r12),%rax
    jmp Lxawrite
LxaOCUT:
    movq 16(%r13),%rax
    movq (%r14,%rax,8),%rbp
    xorl %eax,%eax
    testq %rbp,%rbp
    cmovs %rax,%rbp
    movslq M_OUT+BUF_LENGTH(%r12),%rdx
    cmpq %rdx,%rbp
    cmovg %rdx,%rbp
    subq %rbp,%rdx
    movq M_OUT+BUF_BYTES(%r12),%rsi
    addq %rbp,%rsi
    leaq M_BLOBS(%r12),%rdi
    call CSYM(core_blob_add)
    movl %ebp,M_OUT+BUF_LENGTH(%r12)
    jmp Lxawrite
LxaORES:
    movslq M_OUT+BUF_LENGTH(%r12),%rax
    movq 8(%r13),%rcx
    movq %rax,(%r14,%rcx,8)
    xorl %ebp,%ebp
Lxaresloop:
    cmpq 16(%r13),%rbp
    jge Lxacontinue
    leaq M_OUT(%r12),%rdi
    movl $32,%esi
    movq M_OT(%r12),%rdx
    call CSYM(core_buffer_put)
    incq %rbp
    jmp Lxaresloop
LxaOFILL:
    movq 8(%r13),%rax
    movq (%r14,%rax,8),%rsi
    movq 24(%r13),%rdx
    movq 16(%r13),%rax
    movq (%r14,%rax,8),%rcx
    leaq M_OUT(%r12),%rdi
    call CSYM(core_field_fill)
    testl %eax,%eax
    je Lxacontinue
    movq M_RESULT(%r12),%rax
    leaq Lxafield(%rip),%rcx
    movq %rcx,RESULT_REASON(%rax)
    movl $14,RESULT_REASON_N(%rax)
    jmp Lxarejected
LxaOCLR:
    movl $0,M_OUT+BUF_LENGTH(%r12)
    jmp Lxacontinue
LxaOSEL:
    movl 8(%r13),%eax
    movl %eax,M_OSEL(%r12)
    jmp Lxacontinue
LxaSETOT:
    movq 8(%r13),%rax
    movq (%r14,%rax,8),%rax
    movq %rax,M_OT(%r12)
    jmp Lxacontinue
LxaXATTR:
    xorl %eax,%eax
    movq FRAME_ATTR(%r15),%rcx
    testq %rcx,%rcx
    je Lxawrite
    movq FRAME_CURSOR(%r15),%rdx
    cmpq FRAME_END(%r15),%rdx
    jge Lxawrite
    movq (%rcx,%rdx,8),%rax
    jmp Lxawrite
LxaPUSH:
    movl 8(%r13),%esi
    leaq M_STACK(%r12),%rdi
    call CSYM(core_stack_push)
    jmp Lxacontinue
LxaPOP:
    leaq M_STACK(%r12),%rdi
    call CSYM(core_stack_pop)
    jmp Lxacontinue
LxaSAVE:
    movq 16(%r13),%rax
    movq (%r14,%rax,8),%rcx
    xorl %edx,%edx
    testq %rcx,%rcx
    cmovs %rdx,%rcx
    movq 24(%r13),%rax
    movq (%r14,%rax,8),%rax
    cmpq FRAME_END(%r15),%rax
    cmovg FRAME_END(%r15),%rax
    leaq Lxaempty(%rip),%rsi
    cmpq %rax,%rcx
    jge Lxasavecall
    movq FRAME_BYTES(%r15),%rsi
    addq %rcx,%rsi
    movq %rax,%rdx
    subq %rcx,%rdx
    jmp Lxasavecall
LxaSBSAVE:
    movq M_SCRATCH+BUF_BYTES(%r12),%rsi
    leaq Lxaempty(%rip),%rax
    testq %rsi,%rsi
    cmove %rax,%rsi
    movl M_SCRATCH+BUF_LENGTH(%r12),%edx
Lxasavecall:
    cmpl $OP_INTERN,%ebx
    je Lxaintern
    cmpl $OP_SBINTERN,%ebx
    je Lxaintern
    leaq M_BLOBS(%r12),%rdi
    call CSYM(core_blob_add)
    jmp Lxawrite
Lxaintern:
    leaq M_STRINGS(%r12),%rdi
    call CSYM(core_string_intern)
    jmp Lxawrite
LxaINPUSH:
    cmpl $OP_INPUSH,%ebx
    jne Lxainx
    movq 8(%r13),%rax
    movq (%r14,%rax,8),%rax
    shlq $4,%rax
    addq M_BLOBS+BLOBS_ENTRIES(%r12),%rax
    movq BLOB_BYTES(%rax),%rcx
    movslq BLOB_LENGTH(%rax),%r9
    xorl %edx,%edx
    xorl %r8d,%r8d
    jmp Lxaincopy
Lxainx:
    movq M_X(%r12),%rcx
    movq M_XATTR(%r12),%rdx
    movq 8(%r13),%rax
    movq (%r14,%rax,8),%r8
    movq M_XN(%r12),%r9
    cmpl $OP_INPUSHXE,%ebx
    jne Lxaincopy
    movq 16(%r13),%rax
    movq (%r14,%rax,8),%rax
    cmpq %rax,%r9
    cmovg %rax,%r9
Lxaincopy:
    movq %rcx,0(%rsp)
    movq %rdx,8(%rsp)
    movq %r8,16(%rsp)
    movq %r9,24(%rsp)
    leaq M_FRAMES(%r12),%rdi
    movq %rsp,%rsi
    call CSYM(core_frame_push)
    jmp Lxacontinue
LxaINPOP:
    leaq M_FRAMES(%r12),%rdi
    call CSYM(core_frame_pop)
    jmp Lxacontinue
LxaSBCLR:
    movl $0,M_SCRATCH+BUF_LENGTH(%r12)
    jmp Lxacontinue
LxaSBOUT:
    leaq M_SCRATCH(%r12),%rdi
    movl 8(%r13),%esi
    xorl %edx,%edx
    call CSYM(core_buffer_put)
    jmp Lxacontinue
LxaSBBLOB:
    movq 8(%r13),%rax
    movq (%r14,%rax,8),%rax
    shlq $4,%rax
    addq M_BLOBS+BLOBS_ENTRIES(%r12),%rax
    movq BLOB_BYTES(%rax),%rcx
    movslq BLOB_LENGTH(%rax),%rax
    movq %rcx,0(%rsp)
    movq $0,8(%rsp)
    movq $0,16(%rsp)
    movq %rax,24(%rsp)
    movq %rax,40(%rsp)
    movq %rsp,%r15
    xorl %ebp,%ebp
    leaq M_SCRATCH(%r12),%rax
    movq %rax,48(%rsp)
    jmp Lxaspanloop
LxaSBFIND:
    leaq M_RESOURCES(%r12),%rdi
    leaq M_BLOBS(%r12),%rsi
    movq M_SCRATCH+BUF_BYTES(%r12),%rdx
    leaq Lxaempty(%rip),%rax
    testq %rdx,%rdx
    cmove %rax,%rdx
    movl M_SCRATCH+BUF_LENGTH(%r12),%ecx
    call CSYM(core_resource_find)
    jmp Lxawrite
LxaBYTE:
    xorl %eax,%eax
    movq FRAME_CURSOR(%r15),%rcx
    cmpq FRAME_END(%r15),%rcx
    jge Lxawrite
    movq FRAME_BYTES(%r15),%rdx
    movzbl (%rdx,%rcx),%eax
    jmp Lxawrite
LxaXLEN:
    movq FRAME_END(%r15),%rax
    jmp Lxawrite
LxaBLEN:
    movq 16(%r13),%rax
    movq (%r14,%rax,8),%rax
    shlq $4,%rax
    addq M_BLOBS+BLOBS_ENTRIES(%r12),%rax
    movslq BLOB_LENGTH(%rax),%rax
    jmp Lxawrite
LxaDIVMOD10:
    movq 8(%r13),%rax
    movl (%r14,%rax,8),%eax
    xorl %edx,%edx
    movl $10,%ecx
    divq %rcx
    testq %rax,%rax
    jne Lxadmwrite
    addq $10,%rdx
Lxadmwrite:
    movq %rdx,M_R(%r12)
    jmp Lxawrite
LxaSWAP:
    movslq M_OUT+BUF_LENGTH(%r12),%rbp
    xorl %edi,%edi
    leaq 1(%rbp),%rsi
    call CSYM(realloc)
    testq %rax,%rax
    je Lxaoom
    movq %rax,32(%rsp)
    xorl %edi,%edi
    leaq 8(,%rbp,8),%rsi
    call CSYM(realloc)
    testq %rax,%rax
    je Lxaoom
    movq %rax,40(%rsp)
    movq 32(%rsp),%rdi
    movq M_OUT+BUF_BYTES(%r12),%rsi
    movq %rbp,%rdx
    call CSYM(memcpy)
    movq 40(%rsp),%rdi
    movq M_OUT+BUF_ATTR(%r12),%rsi
    leaq 0(,%rbp,8),%rdx
    call CSYM(memcpy)
    movq M_X(%r12),%rdi
    cmpq M_INPUT(%r12),%rdi
    je Lxaswapattr
    call CSYM(free)
Lxaswapattr:
    movq M_XATTR(%r12),%rdi
    call CSYM(free)
    movq 32(%rsp),%rax
    movq 40(%rsp),%rcx
    movq %rax,M_X(%r12)
    movq %rcx,M_XATTR(%r12)
    movq %rbp,M_XN(%r12)
    movl $0,M_OUT+BUF_LENGTH(%r12)
    movl $1,M_FRAMES+STACK_COUNT(%r12)
    movq M_FRAMES+STACK_ENTRIES(%r12),%rdx
    movq %rax,FRAME_BYTES(%rdx)
    movq %rcx,FRAME_ATTR(%rdx)
    movq $0,FRAME_CURSOR(%rdx)
    movq %rbp,FRAME_END(%rdx)
    jmp Lxacontinue
LxaREJECT:
    movq M_MODEL(%r12),%rax
    movq 8(%r13),%rcx
    movq CM_STR(%rax),%rdx
    movq (%rdx,%rcx,8),%rdx
    movq CM_STRL(%rax),%rax
    movl (%rax,%rcx,4),%ecx
    movq M_RESULT(%r12),%rax
    movq %rdx,RESULT_REASON(%rax)
    movl %ecx,RESULT_REASON_N(%rax)
Lxarejected:
    movl $1,M_STATUS(%r12)
LxaACCEPT:
    movl $1,%eax
    jmp Lxareturn
Lxawrite:
    movq 8(%r13),%rcx
    movq %rax,(%r14,%rcx,8)
    jmp Lxacontinue
Lxarwrite:
    movq %rax,M_R(%r12)
Lxacontinue:
    xorl %eax,%eax
Lxareturn:
    addq $72,%rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    popq %rbx
    ret
Lxabad:
    leaq Lxabadmsg(%rip),%rdi
    jmp Lxapanic
Lxaoom:
    leaq Lxaoommsg(%rip),%rdi
Lxapanic:
    addq $72,%rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    popq %rbx
    jmp CSYM(core_host_panic)
.p2align 2
Lxatable:
/* Table order is checked against the public action enum at build time. */
    .long LxaADV-Lxatable
    .long LxaMARK-Lxatable
    .long LxaJUMP-Lxatable
    .long LxaLDI-Lxatable
    .long LxaCOPYW-Lxatable
    .long LxaALU-Lxatable
    .long LxaALU-Lxatable
    .long LxaCMP-Lxatable
    .long LxaCMP-Lxatable
    .long LxaRLD-Lxatable
    .long LxaLDX-Lxatable
    .long LxaSTX-Lxatable
    .long LxaOUT-Lxatable
    .long LxaOUT-Lxatable
    .long LxaCOPY-Lxatable
    .long LxaCOPY-Lxatable
    .long LxaSPAN-Lxatable
    .long LxaSPAN-Lxatable
    .long LxaSPAN-Lxatable
    .long LxaOLAST-Lxatable
    .long LxaODROP-Lxatable
    .long LxaOLEN-Lxatable
    .long LxaOCUT-Lxatable
    .long LxaORES-Lxatable
    .long LxaOFILL-Lxatable
    .long LxaOCLR-Lxatable
    .long LxaOSEL-Lxatable
    .long LxaSETOT-Lxatable
    .long LxaXATTR-Lxatable
    .long LxaPUSH-Lxatable
    .long LxaPOP-Lxatable
    .long LxaSAVE-Lxatable
    .long LxaSAVE-Lxatable
    .long LxaINPUSH-Lxatable
    .long LxaINPUSH-Lxatable
    .long LxaINPUSH-Lxatable
    .long LxaINPOP-Lxatable
    .long LxaSBCLR-Lxatable
    .long LxaSBOUT-Lxatable
    .long LxaSBSPAN-Lxatable
    .long LxaSBBLOB-Lxatable
    .long LxaSBSAVE-Lxatable
    .long LxaSBSAVE-Lxatable
    .long LxaSBFIND-Lxatable
    .long LxaBLEN-Lxatable
    .long LxaBYTE-Lxatable
    .long LxaXLEN-Lxatable
    .long LxaDIVMOD10-Lxatable
    .long LxaSWAP-Lxatable
    .long LxaACCEPT-Lxatable
    .long LxaREJECT-Lxatable
    .long LxaALU-Lxatable
    .long LxaALU-Lxatable
    .long LxaCMP-Lxatable
    .long LxaCMP-Lxatable
    .long LxaINC-Lxatable
#ifdef __APPLE__
.section __TEXT,__cstring,cstring_literals
#else
.section .rodata
#endif
Lxaempty: .asciz ""
Lxafield: .asciz "field overflow"
Lxabadmsg: .asciz "bad action"
Lxaoommsg: .asciz "out of memory"
#ifndef __APPLE__
.section .note.GNU-stack,"",@progbits
#endif
