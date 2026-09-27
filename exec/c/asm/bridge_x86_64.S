/* One unisacc tape argument (rax) points to the six core_run values. Tape r9 is
   the frame pointer. The tape stack is rsp but is not necessarily aligned. */
#ifdef __APPLE__
#define CSYM1(x) _##x
#define CSYM(x) CSYM1(x)
#else
#error Build_the_portable_kernel_blob_with_the_declared_MachO_seed_linker
#endif
.text
.p2align 2
.globl CSYM(kernel_entry)
CSYM(kernel_entry):
    pushq %rbp
    movq %rsp,%rbp
    pushq %r9
    andq $-16,%rsp
    movq %rax,%r10
    movq 8(%r10),%rdi
    movq 16(%r10),%rsi
    movq 24(%r10),%rdx
    movq 32(%r10),%rcx
    movq 40(%r10),%r8
    movq 48(%r10),%r9
    cmpq $0,(%r10)
    jne Ltransition
    call CSYM(core_run)
    jmp Lentrydone
Ltransition:
    call CSYM(core_transition)
Lentrydone:
    movq -8(%rbp),%r9
    movq %rbp,%rsp
    popq %rbp
    ret

.macro SERVICE name,number
.globl _bridge_\name
_bridge_\name:
    movq $\number,%rax
    jmp Lservice
.endm
SERVICE calloc,0
SERVICE realloc,1
SERVICE free,2
SERVICE memcpy,3
SERVICE memcmp,4
SERVICE memset,5
SERVICE strlen,6
SERVICE core_host_fetch,7
SERVICE core_host_panic,8
Lservice:
    /* Product code does not promise the System V callee-saved set. Keep
       it here; no floating point registers are callee-saved in this ABI. */
    pushq %rbx
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    call *CSYM(kernel_service)(%rip)
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    popq %rbx
    ret
.section __TEXT,__const
.p2align 3
.globl CSYM(kernel_service)
CSYM(kernel_service): .quad 0
