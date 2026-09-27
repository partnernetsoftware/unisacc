/* unisacc tape ABI <-> internal AAPCS kernel. The hardware stack must move
   below the live x7 tape stack before any conventional function is called. */
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
    mov x9,sp
    and x10,x7,#-16
    mov sp,x10
    sub sp,sp,#48
    stp x9,x7,[sp]
    stp x6,x30,[sp,#16]
    stp x29,x28,[sp,#32]
    mov x10,x0
    ldr x11,[x10]
    ldp x0,x1,[x10,#8]
    ldp x2,x3,[x10,#24]
    ldp x4,x5,[x10,#40]
    cbnz x11,Ltransition
    bl CSYM(core_run)
    b Lentrydone
Ltransition:
    bl CSYM(core_transition)
Lentrydone:
    ldp x9,x7,[sp]
    ldp x6,x30,[sp,#16]
    ldp x29,x28,[sp,#32]
    mov sp,x9
    ldr x17,[x7],#8
    ret x17

.macro SERVICE name,number
.globl _bridge_\name
_bridge_\name:
    mov x4,x3
    mov x3,x2
    mov x2,x1
    mov x1,x0
    mov x0,#\number
    b Lservice
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
    stp x29,x30,[sp,#-16]!
    adrp x16,CSYM(kernel_service)@PAGE
    ldr x16,[x16,CSYM(kernel_service)@PAGEOFF]
    adr x17,Lreturned
    sub x7,sp,#8
    str x17,[x7]
    br x16
Lreturned:
    ldp x29,x30,[sp],#16
    ret
.section __TEXT,__const
.p2align 3
.globl CSYM(kernel_service)
CSYM(kernel_service): .quad 0
