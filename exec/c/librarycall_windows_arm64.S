/* Windows ARM64 normal-return host bridge. uint64_t(entry,args6,softtop).
   ABI: https://learn.microsoft.com/en-us/cpp/build/arm64-windows-abi-conventions
   Unwind: https://learn.microsoft.com/en-us/cpp/build/arm64-exception-handling
   x18 is reserved and never touched; low d8..d15 and x19..x30 saved.
   This host frame has unwind metadata; guest code needs its own unwind
   protocol before exception propagation across guest frames is supported. */
.text
.p2align 2
.globl us_library_bridge_raw
.def us_library_bridge_raw; .scl 2; .type 32; .endef
us_library_bridge_raw:
.seh_proc us_library_bridge_raw
    sub sp, sp, #160
    .seh_stackalloc 160
    stp x29, x30, [sp, #0]
    .seh_save_fplr 0
    mov x29, sp
    .seh_set_fp
    stp x19, x20, [sp, #16]
    .seh_save_regp x19, 16
    stp x21, x22, [sp, #32]
    .seh_save_regp x21, 32
    stp x23, x24, [sp, #48]
    .seh_save_regp x23, 48
    stp x25, x26, [sp, #64]
    .seh_save_regp x25, 64
    stp x27, x28, [sp, #80]
    .seh_save_regp x27, 80
    stp d8, d9, [sp, #96]
    .seh_save_fregp d8, 96
    stp d10, d11, [sp, #112]
    .seh_save_fregp d10, 112
    stp d12, d13, [sp, #128]
    .seh_save_fregp d12, 128
    stp d14, d15, [sp, #144]
    .seh_save_fregp d14, 144
    .seh_endprologue
    mov x16, x0
    mov x17, x1
    sub x7, x2, #8
    adr x9, Lwindows_library_return
    str x9, [x7]
    ldp x0, x1, [x17, #0]
    ldp x2, x3, [x17, #16]
    ldp x4, x5, [x17, #32]
    mov x6, #0
    br x16
Lwindows_library_return:
    .seh_startepilogue
    ldp d8, d9, [sp, #96]
    .seh_save_fregp d8, 96
    ldp d10, d11, [sp, #112]
    .seh_save_fregp d10, 112
    ldp d12, d13, [sp, #128]
    .seh_save_fregp d12, 128
    ldp d14, d15, [sp, #144]
    .seh_save_fregp d14, 144
    ldp x19, x20, [sp, #16]
    .seh_save_regp x19, 16
    ldp x21, x22, [sp, #32]
    .seh_save_regp x21, 32
    ldp x23, x24, [sp, #48]
    .seh_save_regp x23, 48
    ldp x25, x26, [sp, #64]
    .seh_save_regp x25, 64
    ldp x27, x28, [sp, #80]
    .seh_save_regp x27, 80
    ldp x29, x30, [sp, #0]
    .seh_save_fplr 0
    add sp, sp, #160
    .seh_stackalloc 160
    .seh_endepilogue
    ret
    .seh_endfunclet
    .seh_endproc

/* Windows ARM64 ALL_STACK normal-return bridge. uint64_t(entry,slots,softtop,count).
   ABI: https://learn.microsoft.com/en-us/cpp/build/arm64-windows-abi-conventions
   Unwind: https://learn.microsoft.com/en-us/cpp/build/arm64-exception-handling
   x18 is reserved and never touched; low d8..d15 and x19..x30 saved.
   This host frame has unwind metadata; guest code needs its own unwind
   protocol before exception propagation across guest frames is supported. */
.text
.p2align 2
.globl us_library_bridge_stack_raw
.def us_library_bridge_stack_raw; .scl 2; .type 32; .endef
us_library_bridge_stack_raw:
.seh_proc us_library_bridge_stack_raw
    sub sp, sp, #160
    .seh_stackalloc 160
    stp x29, x30, [sp, #0]
    .seh_save_fplr 0
    mov x29, sp
    .seh_set_fp
    stp x19, x20, [sp, #16]
    .seh_save_regp x19, 16
    stp x21, x22, [sp, #32]
    .seh_save_regp x21, 32
    stp x23, x24, [sp, #48]
    .seh_save_regp x23, 48
    stp x25, x26, [sp, #64]
    .seh_save_regp x25, 64
    stp x27, x28, [sp, #80]
    .seh_save_regp x27, 80
    stp d8, d9, [sp, #96]
    .seh_save_fregp d8, 96
    stp d10, d11, [sp, #112]
    .seh_save_fregp d10, 112
    stp d12, d13, [sp, #128]
    .seh_save_fregp d12, 128
    stp d14, d15, [sp, #144]
    .seh_save_fregp d14, 144
    .seh_endprologue
    mov x16, x0
    mov x17, x1
    /* Reserve N raw words above the software-stack continuation. The
       callee's saved frame word makes argument k appear at FP+16+8*k. */
    sub x7, x2, x3, lsl #3
    sub x7, x7, #8
    adr x9, Lwindows_library_stack_return
    str x9, [x7]
    add x9, x7, #8
    cbz x3, Lwindows_library_stack_return_enter
Lwindows_library_stack_return_copy:
    ldr x10, [x17], #8
    str x10, [x9], #8
    subs x3, x3, #1
    b.ne Lwindows_library_stack_return_copy
Lwindows_library_stack_return_enter:
    br x16
Lwindows_library_stack_return:
    .seh_startepilogue
    ldp d8, d9, [sp, #96]
    .seh_save_fregp d8, 96
    ldp d10, d11, [sp, #112]
    .seh_save_fregp d10, 112
    ldp d12, d13, [sp, #128]
    .seh_save_fregp d12, 128
    ldp d14, d15, [sp, #144]
    .seh_save_fregp d14, 144
    ldp x19, x20, [sp, #16]
    .seh_save_regp x19, 16
    ldp x21, x22, [sp, #32]
    .seh_save_regp x21, 32
    ldp x23, x24, [sp, #48]
    .seh_save_regp x23, 48
    ldp x25, x26, [sp, #64]
    .seh_save_regp x25, 64
    ldp x27, x28, [sp, #80]
    .seh_save_regp x27, 80
    ldp x29, x30, [sp, #0]
    .seh_save_fplr 0
    add sp, sp, #160
    .seh_stackalloc 160
    .seh_endepilogue
    ret
    .seh_endfunclet
    .seh_endproc
