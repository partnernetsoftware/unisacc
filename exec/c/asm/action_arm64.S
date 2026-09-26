/* Generic model-action execution. The driver owns initialization and cleanup.
   All action helpers selected here are assembly; no C action fallback. */
#include "layout.h"
#include "actions.h"
#ifdef _WIN32
#error Windows_ARM64_object_binding_not_implemented
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
    stp x29,x30,[sp,#-128]!
    mov x29,sp
    stp x19,x20,[sp,#16]
    stp x21,x22,[sp,#32]
    stp x23,x24,[sp,#48]
    stp x25,x26,[sp,#64]
    mov x19,x0
    mov x20,x1
    ldr x21,[x19,#M_REGS]
    ldr x22,[x19,#M_FRAMES+STACK_ENTRIES]
    ldr w8,[x19,#M_FRAMES+STACK_COUNT]
    sub w8,w8,#1
    add x22,x22,x8,lsl #5
    ldr w23,[x20]
    cmp w23,#OP_COUNT-1
    b.hi Labad
    adr x8,Latable
    ldrsw x9,[x8,x23,lsl #2]
    add x8,x8,x9
    br x8
LaADV:
    ldr x8,[x22,#FRAME_CURSOR]
    add x8,x8,#1
    str x8,[x22,#FRAME_CURSOR]
    b Lacontinue
LaMARK:
    ldr x0,[x22,#FRAME_CURSOR]
    b Lawrite
LaJUMP:
    ldr x8,[x20,#8]
    ldr x8,[x21,x8,lsl #3]
    str x8,[x22,#FRAME_CURSOR]
    b Lacontinue
LaLDI:
    ldr x0,[x20,#16]
    b Lawrite
LaCOPYW:
    ldr x8,[x20,#16]
    ldr x0,[x21,x8,lsl #3]
    b Lawrite
LaALU:
    ldr w0,[x20,#8]
    ldr x8,[x20,#24]
    ldr x1,[x21,x8,lsl #3]
    ldr x2,[x20,#32]
    cmp w23,#OP_ALUI
    b.eq Laaluargs
    cmp w23,#OP_A64I
    b.eq Laaluargs
    ldr x2,[x21,x2,lsl #3]
Laaluargs:
    cmp w23,#OP_A64
    b.ge Laalu64
    bl CSYM(core_alu32)
    b Lawrite2
Laalu64:
    add x3,sp,#112
    bl CSYM(core_alu64)
    ldrsw x8,[sp,#112]
    str x8,[x19,#M_R]
Lawrite2:
    ldr x8,[x20,#16]
    str x0,[x21,x8,lsl #3]
    b Lacontinue
LaCMP:
    ldr x8,[x20,#8]
    ldr x8,[x21,x8,lsl #3]
    ldr x9,[x20,#16]
    cmp w23,#OP_CMPI
    b.eq Lacmpvalues
    ldr x9,[x21,x9,lsl #3]
Lacmpvalues:
    cmp w23,#OP_C64U
    b.eq Lacmpunsigned
    cmp x8,x9
    cset x0,ge
    cinc x0,x0,gt
    b Larwrite
Lacmpunsigned:
    cmp x8,x9
    cset x0,hs
    cinc x0,x0,hi
    b Larwrite
LaRLD:
    ldr x8,[x20,#8]
    ldr x0,[x21,x8,lsl #3]
    cmp x0,#256
    mov x9,#256
    csel x0,x0,x9,ls
    b Larwrite
LaINC:
    ldr x8,[x20,#8]
    ldr x0,[x21,x8,lsl #3]
    add w0,w0,#1
    b Lawrite
LaLDX:
    ldr x8,[x20,#16]
    ldr x1,[x21,x8,lsl #3]
    ldr x8,[x20,#24]
    add x1,x1,x8
    add x0,x19,#M_MEMORY
    bl CSYM(core_memory_get)
    b Lawrite
LaSTX:
    ldr x8,[x20,#8]
    ldr x1,[x21,x8,lsl #3]
    ldr x8,[x20,#16]
    add x1,x1,x8
    ldr x8,[x20,#24]
    ldr x2,[x21,x8,lsl #3]
    add x0,x19,#M_MEMORY
    bl CSYM(core_memory_set)
    b Lacontinue
LaOUT:
    ldr x1,[x20,#8]
    cmp w23,#OP_OUT
    b.eq Laoutvalue
    ldr x1,[x21,x1,lsl #3]
Laoutvalue:
    ldr x2,[x19,#M_OT]
    b Laout
LaCOPY:
    ldr x8,[x22,#FRAME_CURSOR]
    ldr x9,[x22,#FRAME_END]
    cmp x8,x9
    b.ge Lacontinue
    ldr x9,[x22,#FRAME_BYTES]
    ldrb w1,[x9,x8]
    ldr x2,[x19,#M_OT]
    cmp w23,#OP_COPYT
    b.eq Laout
    ldr x9,[x22,#FRAME_ATTR]
    cbz x9,Laout
    ldr x2,[x9,x8,lsl #3]
Laout:
    add x0,x19,#M_OUT
    ldr w8,[x19,#M_OSEL]
    cbz w8,Laoutcall
    add x0,x19,#M_ERR
    mov x2,#0
Laoutcall:
    bl CSYM(core_buffer_put)
    b Lacontinue
LaSPAN:
    ldr x8,[x20,#8]
    ldr x24,[x21,x8,lsl #3]
    ldr x25,[x22,#FRAME_CURSOR]
    cmp w23,#OP_SPAN2
    b.ne Laspanend
    ldr x8,[x20,#16]
    ldr x25,[x21,x8,lsl #3]
Laspanend:
    add x26,x19,#M_OUT
    ldr w8,[x19,#M_OSEL]
    cmp w8,#1
    b.ne Laspanclamp
    add x26,x19,#M_ERR
    b Laspanclamp
LaSBSPAN:
    ldr x8,[x20,#8]
    ldr x24,[x21,x8,lsl #3]
    ldr x8,[x20,#16]
    ldr x25,[x21,x8,lsl #3]
    add x26,x19,#M_SCRATCH
Laspanclamp:
    cmp x24,#0
    csel x24,x24,xzr,ge
    ldr x8,[x22,#FRAME_END]
    cmp x25,x8
    csel x25,x25,x8,le
Laspanloop:
    cmp x24,x25
    b.ge Lacontinue
    ldr x8,[x22,#FRAME_BYTES]
    ldrb w1,[x8,x24]
    mov x2,#0
    add x8,x19,#M_OUT
    cmp x26,x8
    b.ne Laspanput
    ldr x2,[x19,#M_OT]
    cmp w23,#OP_SPANT
    b.eq Laspanput
    ldr x8,[x22,#FRAME_ATTR]
    cbz x8,Laspanput
    ldr x2,[x8,x24,lsl #3]
Laspanput:
    mov x0,x26
    bl CSYM(core_buffer_put)
    add x24,x24,#1
    b Laspanloop
LaOLAST:
    mov x0,#256
    ldr w8,[x19,#M_OUT+BUF_LENGTH]
    cbz w8,Larwrite
    sub w8,w8,#1
    ldr x9,[x19,#M_OUT+BUF_BYTES]
    ldrb w0,[x9,x8]
    b Larwrite
LaODROP:
    ldr w8,[x19,#M_OUT+BUF_LENGTH]
    cbz w8,Lacontinue
    sub w8,w8,#1
    str w8,[x19,#M_OUT+BUF_LENGTH]
    b Lacontinue
LaOLEN:
    ldrsw x0,[x19,#M_OUT+BUF_LENGTH]
    b Lawrite
LaOCUT:
    ldr x8,[x20,#16]
    ldr x24,[x21,x8,lsl #3]
    cmp x24,#0
    csel x24,x24,xzr,ge
    ldrsw x2,[x19,#M_OUT+BUF_LENGTH]
    cmp x24,x2
    csel x24,x24,x2,le
    sub x2,x2,x24
    ldr x1,[x19,#M_OUT+BUF_BYTES]
    add x1,x1,x24
    add x0,x19,#M_BLOBS
    bl CSYM(core_blob_add)
    str w24,[x19,#M_OUT+BUF_LENGTH]
    b Lawrite
LaORES:
    ldrsw x0,[x19,#M_OUT+BUF_LENGTH]
    ldr x8,[x20,#8]
    str x0,[x21,x8,lsl #3]
    mov x24,#0
    ldr x25,[x20,#16]
Laresloop:
    cmp x24,x25
    b.ge Lacontinue
    add x0,x19,#M_OUT
    mov w1,#32
    ldr x2,[x19,#M_OT]
    bl CSYM(core_buffer_put)
    add x24,x24,#1
    b Laresloop
LaOFILL:
    ldr x8,[x20,#8]
    ldr x1,[x21,x8,lsl #3]
    ldr x2,[x20,#24]
    ldr x8,[x20,#16]
    ldr x3,[x21,x8,lsl #3]
    add x0,x19,#M_OUT
    bl CSYM(core_field_fill)
    cbz w0,Lacontinue
    ldr x8,[x19,#M_RESULT]
#ifdef __APPLE__
    adrp x9,Lafield@PAGE
    add x9,x9,Lafield@PAGEOFF
#else
    adrp x9,Lafield
    add x9,x9,:lo12:Lafield
#endif
    str x9,[x8,#RESULT_REASON]
    mov w9,#14
    str w9,[x8,#RESULT_REASON_N]
    b Larejected
LaOCLR:
    str wzr,[x19,#M_OUT+BUF_LENGTH]
    b Lacontinue
LaOSEL:
    ldr w8,[x20,#8]
    str w8,[x19,#M_OSEL]
    b Lacontinue
LaSETOT:
    ldr x8,[x20,#8]
    ldr x8,[x21,x8,lsl #3]
    str x8,[x19,#M_OT]
    b Lacontinue
LaXATTR:
    mov x0,#0
    ldr x8,[x22,#FRAME_ATTR]
    cbz x8,Lawrite
    ldr x9,[x22,#FRAME_CURSOR]
    ldr x10,[x22,#FRAME_END]
    cmp x9,x10
    b.ge Lawrite
    ldr x0,[x8,x9,lsl #3]
    b Lawrite
LaPUSH:
    ldr w1,[x20,#8]
    add x0,x19,#M_STACK
    bl CSYM(core_stack_push)
    b Lacontinue
LaPOP:
    add x0,x19,#M_STACK
    bl CSYM(core_stack_pop)
    b Lacontinue
LaSAVE:
    ldr x8,[x20,#16]
    ldr x8,[x21,x8,lsl #3]
    cmp x8,#0
    csel x8,x8,xzr,ge
    ldr x9,[x20,#24]
    ldr x9,[x21,x9,lsl #3]
    ldr x10,[x22,#FRAME_END]
    cmp x9,x10
    csel x9,x9,x10,le
#ifdef __APPLE__
    adrp x1,Laempty@PAGE
    add x1,x1,Laempty@PAGEOFF
#else
    adrp x1,Laempty
    add x1,x1,:lo12:Laempty
#endif
    mov x2,#0
    cmp x8,x9
    b.ge Lasavecall
    ldr x1,[x22,#FRAME_BYTES]
    add x1,x1,x8
    sub x2,x9,x8
    b Lasavecall
LaSBSAVE:
    ldr x1,[x19,#M_SCRATCH+BUF_BYTES]
#ifdef __APPLE__
    adrp x8,Laempty@PAGE
    add x8,x8,Laempty@PAGEOFF
#else
    adrp x8,Laempty
    add x8,x8,:lo12:Laempty
#endif
    cmp x1,#0
    csel x1,x1,x8,ne
    ldr w2,[x19,#M_SCRATCH+BUF_LENGTH]
Lasavecall:
    cmp w23,#OP_INTERN
    b.eq Laintern
    cmp w23,#OP_SBINTERN
    b.eq Laintern
    add x0,x19,#M_BLOBS
    bl CSYM(core_blob_add)
    b Lawrite
Laintern:
    add x0,x19,#M_STRINGS
    bl CSYM(core_string_intern)
    b Lawrite
LaINPUSH:
    cmp w23,#OP_INPUSH
    b.ne Lainx
    ldr x8,[x20,#8]
    ldr x8,[x21,x8,lsl #3]
    ldr x9,[x19,#M_BLOBS+BLOBS_ENTRIES]
    add x9,x9,x8,lsl #4
    ldr x8,[x9,#BLOB_BYTES]
    ldrsw x11,[x9,#BLOB_LENGTH]
    mov x9,#0
    mov x10,#0
    b Laincopy
Lainx:
    ldr x8,[x19,#M_X]
    ldr x9,[x19,#M_XATTR]
    ldr x10,[x20,#8]
    ldr x10,[x21,x10,lsl #3]
    ldr x11,[x19,#M_XN]
    cmp w23,#OP_INPUSHXE
    b.ne Laincopy
    ldr x12,[x20,#16]
    ldr x12,[x21,x12,lsl #3]
    cmp x11,x12
    csel x11,x11,x12,le
Laincopy:
    stp x8,x9,[sp,#80]
    stp x10,x11,[sp,#96]
    add x0,x19,#M_FRAMES
    add x1,sp,#80
    bl CSYM(core_frame_push)
    b Lacontinue
LaINPOP:
    add x0,x19,#M_FRAMES
    bl CSYM(core_frame_pop)
    b Lacontinue
LaSBCLR:
    str wzr,[x19,#M_SCRATCH+BUF_LENGTH]
    b Lacontinue
LaSBOUT:
    add x0,x19,#M_SCRATCH
    ldr w1,[x20,#8]
    mov x2,#0
    bl CSYM(core_buffer_put)
    b Lacontinue
LaSBBLOB:
    ldr x8,[x20,#8]
    ldr x8,[x21,x8,lsl #3]
    ldr x9,[x19,#M_BLOBS+BLOBS_ENTRIES]
    add x9,x9,x8,lsl #4
    ldr x8,[x9,#BLOB_BYTES]
    ldrsw x25,[x9,#BLOB_LENGTH]
    stp x8,xzr,[sp,#80]
    stp xzr,x25,[sp,#96]
    add x22,sp,#80
    mov x24,#0
    add x26,x19,#M_SCRATCH
    b Laspanloop
LaSBFIND:
    add x0,x19,#M_RESOURCES
    add x1,x19,#M_BLOBS
    ldr x2,[x19,#M_SCRATCH+BUF_BYTES]
#ifdef __APPLE__
    adrp x8,Laempty@PAGE
    add x8,x8,Laempty@PAGEOFF
#else
    adrp x8,Laempty
    add x8,x8,:lo12:Laempty
#endif
    cmp x2,#0
    csel x2,x2,x8,ne
    ldr w3,[x19,#M_SCRATCH+BUF_LENGTH]
    bl CSYM(core_resource_find)
    b Lawrite
LaBYTE:
    mov x0,#0
    ldr x8,[x22,#FRAME_CURSOR]
    ldr x9,[x22,#FRAME_END]
    cmp x8,x9
    b.ge Lawrite
    ldr x9,[x22,#FRAME_BYTES]
    ldrb w0,[x9,x8]
    b Lawrite
LaXLEN:
    ldr x0,[x22,#FRAME_END]
    b Lawrite
LaBLEN:
    ldr x8,[x20,#16]
    ldr x8,[x21,x8,lsl #3]
    ldr x9,[x19,#M_BLOBS+BLOBS_ENTRIES]
    add x9,x9,x8,lsl #4
    ldrsw x0,[x9,#BLOB_LENGTH]
    b Lawrite
LaDIVMOD10:
    ldr x8,[x20,#8]
    ldr x9,[x21,x8,lsl #3]
    mov w9,w9
    mov x10,#10
    udiv x0,x9,x10
    msub x9,x0,x10,x9
    cmp x0,#0
    csel x10,x10,xzr,eq
    add x9,x9,x10
    str x9,[x19,#M_R]
    b Lawrite
LaSWAP:
    ldrsw x26,[x19,#M_OUT+BUF_LENGTH]
    mov x0,#0
    add x1,x26,#1
    bl CSYM(realloc)
    cbz x0,Laoom
    mov x24,x0
    mov x0,#0
    add x1,x26,#1
    lsl x1,x1,#3
    bl CSYM(realloc)
    cbz x0,Laoom
    mov x25,x0
    mov x0,x24
    ldr x1,[x19,#M_OUT+BUF_BYTES]
    mov x2,x26
    bl CSYM(memcpy)
    mov x0,x25
    ldr x1,[x19,#M_OUT+BUF_ATTR]
    lsl x2,x26,#3
    bl CSYM(memcpy)
    ldr x0,[x19,#M_X]
    ldr x8,[x19,#M_INPUT]
    cmp x0,x8
    b.eq Laswapattr
    bl CSYM(free)
Laswapattr:
    ldr x0,[x19,#M_XATTR]
    bl CSYM(free)
    stp x24,x25,[x19,#M_X]
    str x26,[x19,#M_XN]
    str wzr,[x19,#M_OUT+BUF_LENGTH]
    mov w8,#1
    str w8,[x19,#M_FRAMES+STACK_COUNT]
    ldr x8,[x19,#M_FRAMES+STACK_ENTRIES]
    stp x24,x25,[x8]
    stp xzr,x26,[x8,#16]
    b Lacontinue
LaREJECT:
    ldr x8,[x19,#M_MODEL]
    ldr x9,[x20,#8]
    ldr x10,[x8,#CM_STR]
    ldr x10,[x10,x9,lsl #3]
    ldr x11,[x8,#CM_STRL]
    ldr w11,[x11,x9,lsl #2]
    ldr x8,[x19,#M_RESULT]
    str x10,[x8,#RESULT_REASON]
    str w11,[x8,#RESULT_REASON_N]
Larejected:
    mov w8,#1
    str w8,[x19,#M_STATUS]
LaACCEPT:
    mov w0,#1
    b Lareturn
Lawrite:
    ldr x8,[x20,#8]
    str x0,[x21,x8,lsl #3]
    b Lacontinue
Larwrite:
    str x0,[x19,#M_R]
Lacontinue:
    mov w0,#0
Lareturn:
    ldp x25,x26,[sp,#64]
    ldp x23,x24,[sp,#48]
    ldp x21,x22,[sp,#32]
    ldp x19,x20,[sp,#16]
    ldp x29,x30,[sp],#128
    ret
Labad:
#ifdef __APPLE__
    adrp x0,Labadmsg@PAGE
    add x0,x0,Labadmsg@PAGEOFF
#else
    adrp x0,Labadmsg
    add x0,x0,:lo12:Labadmsg
#endif
    b Lapanic
Laoom:
#ifdef __APPLE__
    adrp x0,Laoommsg@PAGE
    add x0,x0,Laoommsg@PAGEOFF
#else
    adrp x0,Laoommsg
    add x0,x0,:lo12:Laoommsg
#endif
Lapanic:
    ldp x25,x26,[sp,#64]
    ldp x23,x24,[sp,#48]
    ldp x21,x22,[sp,#32]
    ldp x19,x20,[sp,#16]
    ldp x29,x30,[sp],#128
    b CSYM(core_host_panic)
.p2align 2
Latable:
/* Table order is checked against the public action enum at build time. */
    .long LaADV-Latable
    .long LaMARK-Latable
    .long LaJUMP-Latable
    .long LaLDI-Latable
    .long LaCOPYW-Latable
    .long LaALU-Latable
    .long LaALU-Latable
    .long LaCMP-Latable
    .long LaCMP-Latable
    .long LaRLD-Latable
    .long LaLDX-Latable
    .long LaSTX-Latable
    .long LaOUT-Latable
    .long LaOUT-Latable
    .long LaCOPY-Latable
    .long LaCOPY-Latable
    .long LaSPAN-Latable
    .long LaSPAN-Latable
    .long LaSPAN-Latable
    .long LaOLAST-Latable
    .long LaODROP-Latable
    .long LaOLEN-Latable
    .long LaOCUT-Latable
    .long LaORES-Latable
    .long LaOFILL-Latable
    .long LaOCLR-Latable
    .long LaOSEL-Latable
    .long LaSETOT-Latable
    .long LaXATTR-Latable
    .long LaPUSH-Latable
    .long LaPOP-Latable
    .long LaSAVE-Latable
    .long LaSAVE-Latable
    .long LaINPUSH-Latable
    .long LaINPUSH-Latable
    .long LaINPUSH-Latable
    .long LaINPOP-Latable
    .long LaSBCLR-Latable
    .long LaSBOUT-Latable
    .long LaSBSPAN-Latable
    .long LaSBBLOB-Latable
    .long LaSBSAVE-Latable
    .long LaSBSAVE-Latable
    .long LaSBFIND-Latable
    .long LaBLEN-Latable
    .long LaBYTE-Latable
    .long LaXLEN-Latable
    .long LaDIVMOD10-Latable
    .long LaSWAP-Latable
    .long LaACCEPT-Latable
    .long LaREJECT-Latable
    .long LaALU-Latable
    .long LaALU-Latable
    .long LaCMP-Latable
    .long LaCMP-Latable
    .long LaINC-Latable
#ifdef __APPLE__
.section __TEXT,__cstring,cstring_literals
#else
.section .rodata
#endif
Laempty: .asciz ""
Lafield: .asciz "field overflow"
Labadmsg: .asciz "bad action"
Laoommsg: .asciz "out of memory"
#ifndef __APPLE__
.section .note.GNU-stack,"",%progbits
#endif
