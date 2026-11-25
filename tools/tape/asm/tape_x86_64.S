/* T1.2 standalone ELF/SysV tape instruction decoder.
   rdi = cursor, rsi = exclusive CODE end, rdx = 48-byte decoded record.
   Return next cursor in rax, or null on malformed/truncated input.
   Record: opcode byte 0, register count byte 1, immediate count byte 2,
   ULEB tag bitset byte 3, packed register nibbles u32 at 4, immediates at 8.
   The caller consumes the record only on success. */
.text
.p2align 4
.globl tape_decode_x86_64
.type tape_decode_x86_64,@function
tape_decode_x86_64:
    cmpq %rsi,%rdi
    jae .Lbad_leaf
    movzbl (%rdi),%eax
    cmpb $72,%al
    jae .Lbad_leaf
    movb %al,0(%rdx)
    leaq .Lshape(%rip),%r8
    movzbl (%r8,%rax),%eax
    movl %eax,%r8d
    andl $15,%r8d
    shrl $4,%eax
    movb %r8b,1(%rdx)
    movb %al,2(%rdx)
    movb $0,3(%rdx)
    movl $0,4(%rdx)
    incq %rdi
    leal 1(%r8),%r9d
    shrl $1,%r9d
    movq %rsi,%r10
    subq %rdi,%r10
    cmpq %r9,%r10
    jb .Lbad_leaf
    testb $1,%r8b
    jz .Lreg_copy
    movzbl -1(%rdi,%r9),%r10d
    andl $0xf0,%r10d
    cmpl $0xf0,%r10d
    jne .Lbad_leaf
.Lreg_copy:
    xorl %r10d,%r10d
.Lreg_next:
    cmpl %r9d,%r10d
    jae .Lreg_done
    movb (%rdi,%r10),%r11b
    movb %r11b,4(%rdx,%r10)
    incl %r10d
    jmp .Lreg_next
.Lreg_done:
    addq %r9,%rdi
    testl %eax,%eax
    jz .Lsuccess_leaf
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    movl %eax,%r12d
    xorl %r13d,%r13d
.Loperand:
    cmpq %rsi,%rdi
    jae .Lbad
    movzbl (%rdi),%r14d
    incq %rdi
    cmpl $1,%r14d
    ja .Lbad
    testl %r14d,%r14d
    jz .Ltag_done
    movl %r13d,%ecx
    movb $1,%al
    shlb %cl,%al
    orb %al,3(%rdx)
.Ltag_done:
    xorl %ecx,%ecx
    xorl %r8d,%r8d
    xorl %r9d,%r9d
.Lleb:
    cmpq %rsi,%rdi
    jae .Lbad
    movzbl (%rdi),%r10d
    incq %rdi
    movl %r10d,%ebx
    andl $127,%ebx
    cmpl $63,%ecx
    jne .Lleb_acc
    testl %r14d,%r14d
    jz .Lsleb_last
    cmpl $1,%r10d
    ja .Lbad
    jmp .Lleb_acc
.Lsleb_last:
    testl %r10d,%r10d
    jz .Lleb_acc
    cmpl $127,%r10d
    jne .Lbad
.Lleb_acc:
    shlq %cl,%rbx
    orq %rbx,%r8
    testb $128,%r10b
    jz .Lleb_done
    movl %r10d,%r9d
    addl $7,%ecx
    jmp .Lleb
.Lleb_done:
    testl %r14d,%r14d
    jz .Lsleb_done
    testl %ecx,%ecx
    jz .Lstore
    testl %r10d,%r10d
    jz .Lbad
    jmp .Lstore
.Lsleb_done:
    testl %ecx,%ecx
    jz .Lsign
    testl %r10d,%r10d
    jnz .Lcheck_minus
    testb $64,%r9b
    jz .Lbad
.Lcheck_minus:
    cmpl $127,%r10d
    jne .Lsign
    testb $64,%r9b
    jnz .Lbad
.Lsign:
    cmpl $57,%ecx
    jae .Lstore
    testb $64,%r10b
    jz .Lstore
    addl $7,%ecx
    movq $-1,%rbx
    shlq %cl,%rbx
    orq %rbx,%r8
.Lstore:
    movq %r8,8(%rdx,%r13,8)
    incl %r13d
    cmpl %r12d,%r13d
    jb .Loperand
    movq %rdi,%rax
    jmp .Lrestore
.Lbad:
    xorl %eax,%eax
.Lrestore:
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    ret
.Lsuccess_leaf:
    movq %rdi,%rax
    ret
.Lbad_leaf:
    xorl %eax,%eax
    ret
.size tape_decode_x86_64,.-tape_decode_x86_64

/* rdi=CODE, rsi=length, rdx=bitmap, rcx=bitmap capacity.
   Return 0 for a completely decoded stream, 2 otherwise. */
.p2align 4
.globl tape_bitmap_x86_64
.type tape_bitmap_x86_64,@function
tape_bitmap_x86_64:
    testq %rdi,%rdi
    jz .Lbitmap_bad_leaf
    testq %rsi,%rsi
    jz .Lbitmap_bad_leaf
    testq %rdx,%rdx
    jz .Lbitmap_bad_leaf
    movq %rsi,%r8
    shrq $3,%r8
    testb $7,%sil
    jz .Lbitmap_cap
    incq %r8
.Lbitmap_cap:
    cmpq %r8,%rcx
    jb .Lbitmap_bad_leaf
    movq %rdi,%r9
    addq %rsi,%r9
    jc .Lbitmap_bad_leaf
    movq %rdx,%r10
    addq %r8,%r10
    jc .Lbitmap_bad_leaf
    cmpq %r10,%rdi
    jae .Lbitmap_clear
    cmpq %r9,%rdx
    jb .Lbitmap_bad_leaf
.Lbitmap_clear:
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    subq $48,%rsp
    movq %rdi,%r12
    movq %rdi,%rbx
    movq %r9,%r13
    movq %rdx,%r14
    movq %rdx,%rdi
    movq %r8,%rcx
    xorl %eax,%eax
    rep stosb
.Lbitmap_next:
    movq %rbx,%rdi
    movq %r13,%rsi
    movq %rsp,%rdx
    call tape_decode_x86_64
    testq %rax,%rax
    jz .Lbitmap_bad
    movq %rbx,%rcx
    subq %r12,%rcx
    movq %rcx,%rdx
    shrq $3,%rdx
    andl $7,%ecx
    movb $1,%r8b
    shlb %cl,%r8b
    orb %r8b,(%r14,%rdx)
    movq %rax,%rbx
    cmpq %r13,%rbx
    jb .Lbitmap_next
    xorl %eax,%eax
    jmp .Lbitmap_done
.Lbitmap_bad:
    movl $2,%eax
.Lbitmap_done:
    addq $48,%rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    ret
.Lbitmap_bad_leaf:
    movl $2,%eax
    ret
.size tape_bitmap_x86_64,.-tape_bitmap_x86_64

/* Minimal M1 entry: imm, mov, sys(write), nop and exit. The loader owns DATA and RELO
   validation. Unsupported instructions fail with their current CODE offset. */
.p2align 4
.globl tape_run_x86_64
.type tape_run_x86_64,@function
tape_run_x86_64:
    testq %rdi,%rdi
    jz .Lrun_noresult
    movq 48(%rdi),%rdx
    testq %rdx,%rdx
    jz .Lrun_noresult
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    subq $192,%rsp
    movq %rdi,%r12
    movq %rdx,%r13
    xorq %rax,%rax
    movq %r13,%rdi
    movl $5,%ecx
    rep stosq
    cmpq $4,0(%r12)
    jne .Lrun_container
    movq 8(%r12),%r14
    testq %r14,%r14
    jz .Lrun_container
    movq 56(%r12),%rdx
    cmpq $208,%rdx
    jb .Lrun_container
    movq %r14,%rax
    addq %rdx,%rax
    jc .Lrun_container
    movabsq $0x4e49424550415455,%rax
    cmpq %rax,0(%r14)
    jne .Lrun_container
    cmpl $0x00030001,8(%r14)
    jne .Lrun_container
    cmpl $1,12(%r14)
    jne .Lrun_container
    cmpl $6,20(%r14)
    jne .Lrun_container
    cmpl $64,24(%r14)
    jne .Lrun_container
    cmpl $0x45444f43,64(%r14)
    jne .Lrun_container
    movq 68(%r14),%rax
    cmpq $208,%rax
    jb .Lrun_container
    testb $7,%al
    jnz .Lrun_container
    subq %rax,%rdx
    jb .Lrun_container
    movq 76(%r14),%rsi
    cmpq %rsi,%rdx
    jb .Lrun_container
    movl 16(%r14),%ebx
    cmpq %rsi,%rbx
    jae .Lrun_container
    addq %rax,%r14
    leaq (%r14,%rsi),%r15
    movq %r14,%rdi
    movq 80(%r12),%rdx
    movq 88(%r12),%rcx
    call tape_bitmap_x86_64
    testl %eax,%eax
    jnz .Lrun_container
    movq 80(%r12),%rax
    movq %rbx,%rdx
    shrq $3,%rdx
    movzbl (%rax,%rdx),%eax
    movl %ebx,%ecx
    andl $7,%ecx
    btl %ecx,%eax
    jnc .Lrun_container
    addq %r14,%rbx
    leaq 48(%rsp),%rdi
    xorl %eax,%eax
    movl $16,%ecx
    rep stosq
.Lrun_next:
    movq %rbx,%rax
    subq %r14,%rax
    movq %rax,24(%r13)
    movq 16(%r13),%rax
    cmpq 24(%r12),%rax
    jae .Lrun_budget
    cmpq %r15,%rbx
    jae .Lrun_code
    movq %rbx,%rdi
    movq %r15,%rsi
    movq %rsp,%rdx
    call tape_decode_x86_64
    testq %rax,%rax
    jz .Lrun_opcode
    movq %rax,%rbx
    incq 16(%r13)
    movzbl 0(%rsp),%eax
    movzbl 4(%rsp),%ecx
    movl %ecx,%edx
    andl $15,%ecx
    shrl $4,%edx
    cmpl $0,%eax
    je .Lrun_imm
    cmpl $1,%eax
    je .Lrun_mov
    cmpl $7,%eax
    jbe .Lrun_bit_arith
    cmpl $15,%eax
    jb .Lrun_noncmp
    cmpl $20,%eax
    jbe .Lrun_compare
.Lrun_noncmp:
    cmpl $24,%eax
    jbe .Lrun_mem
    cmpl $25,%eax
    je .Lrun_lea
    cmpl $26,%eax
    je .Lrun_mem
    cmpl $27,%eax
    je .Lrun_jump
    cmpl $28,%eax
    je .Lrun_jumpz
    cmpl $42,%eax
    je .Lrun_exit
    cmpl $43,%eax
    je .Lrun_sys
    cmpl $44,%eax
    je .Lrun_sys
    cmpl $47,%eax
    je .Lrun_next
    jmp .Lrun_opcode
.Lrun_mem:
    movl %eax,%r10d
    movq $8,%r9
    cmpl $23,%eax
    jb .Lrun_mem_width
    movq 16(%rsp),%r9
.Lrun_mem_width:
    cmpl $26,%eax
    je .Lrun_mem_base
    cmpq $1,%r9
    je .Lrun_mem_base
    cmpq $2,%r9
    je .Lrun_mem_base
    cmpq $4,%r9
    je .Lrun_mem_base
    cmpq $8,%r9
    jne .Lrun_opcode
.Lrun_mem_base:
    movq 48(%rsp,%rcx,8),%rdi
    cmpl $21,%eax
    je .Lrun_mem_load_base
    cmpl $23,%eax
    jne .Lrun_mem_offset
.Lrun_mem_load_base:
    movq 48(%rsp,%rdx,8),%rdi
.Lrun_mem_offset:
    movq 8(%rsp),%r8
    testq %r8,%r8
    js .Lrun_mem_negative
    addq %r8,%rdi
    jc .Lrun_memory
    jmp .Lrun_mem_bounds
.Lrun_mem_negative:
    addq %r8,%rdi
    jnc .Lrun_memory
.Lrun_mem_bounds:
    movq 72(%r12),%r8
    testq %r8,%r8
    jz .Lrun_memory
    movq %rdi,%rsi
    subq %r8,%rsi
    jb .Lrun_memory
    movq 16(%r12),%r8
    subq %r9,%r8
    jb .Lrun_memory
    cmpq %r8,%rsi
    ja .Lrun_memory
    cmpl $26,%r10d
    je .Lrun_mem_zero
    cmpl $21,%r10d
    je .Lrun_mem_load
    cmpl $23,%r10d
    je .Lrun_mem_load
    movq 48(%rsp,%rdx,8),%rax
    cmpq $8,%r9
    je .Lrun_mem_st8
    cmpq $4,%r9
    je .Lrun_mem_st4
    cmpq $2,%r9
    je .Lrun_mem_st2
    movb %al,(%rdi)
    jmp .Lrun_next
.Lrun_mem_st2:
    movw %ax,(%rdi)
    jmp .Lrun_next
.Lrun_mem_st4:
    movl %eax,(%rdi)
    jmp .Lrun_next
.Lrun_mem_st8:
    movq %rax,(%rdi)
    jmp .Lrun_next
.Lrun_mem_load:
    cmpq $8,%r9
    je .Lrun_mem_ld8
    cmpq $4,%r9
    je .Lrun_mem_ld4
    cmpq $2,%r9
    je .Lrun_mem_ld2
    movsbq (%rdi),%rax
    jmp .Lrun_store
.Lrun_mem_ld2:
    movswq (%rdi),%rax
    jmp .Lrun_store
.Lrun_mem_ld4:
    movslq (%rdi),%rax
    jmp .Lrun_store
.Lrun_mem_ld8:
    movq (%rdi),%rax
    jmp .Lrun_store
.Lrun_mem_zero:
    movq %r9,%rcx
    xorl %eax,%eax
    rep stosb
    jmp .Lrun_next
.Lrun_jumpz:
    cmpq $0,48(%rsp,%rcx,8)
    jne .Lrun_next
.Lrun_jump:
    testb $1,3(%rsp)
    jz .Lrun_code
    movq 8(%rsp),%rax
    movq %r15,%rdx
    subq %r14,%rdx
    cmpq %rdx,%rax
    jae .Lrun_code
    movq %rax,%rdx
    shrq $3,%rdx
    movq 80(%r12),%r8
    movzbl (%r8,%rdx),%r8d
    movl %eax,%ecx
    andl $7,%ecx
    btl %ecx,%r8d
    jnc .Lrun_code
    leaq (%r14,%rax),%rbx
    jmp .Lrun_next
.Lrun_imm:
    movq 8(%rsp),%rax
    jmp .Lrun_store
.Lrun_mov:
    movq 48(%rsp,%rdx,8),%rax
.Lrun_store:
    movq %rax,48(%rsp,%rcx,8)
    jmp .Lrun_next
.Lrun_bit_arith:
    movq 48(%rsp,%rdx,8),%r8
    movzbl 5(%rsp),%edx
    andl $15,%edx
    movq 48(%rsp,%rdx,8),%r9
    cmpl $3,%eax
    jb .Lrun_add
    je .Lrun_sub
    cmpl $5,%eax
    jb .Lrun_mul
    je .Lrun_xor
    cmpl $6,%eax
    je .Lrun_and
    orq %r9,%r8
    jmp .Lrun_arith_result
.Lrun_add:
    addq %r9,%r8
    jmp .Lrun_arith_result
.Lrun_sub:
    subq %r9,%r8
    jmp .Lrun_arith_result
.Lrun_mul:
    imulq %r9,%r8
    jmp .Lrun_arith_result
.Lrun_xor:
    xorq %r9,%r8
    jmp .Lrun_arith_result
.Lrun_and:
    andq %r9,%r8
.Lrun_arith_result:
    movq %r8,%rax
    jmp .Lrun_store
.Lrun_compare:
    movq 48(%rsp,%rdx,8),%r8
    movzbl 5(%rsp),%edx
    andl $15,%edx
    cmpq 48(%rsp,%rdx,8),%r8
    setl %r8b
    setle %r9b
    seta %r10b
    setb %r11b
    sete %dl
    cmpl $16,%eax
    jb .Lrun_cmp_slt
    je .Lrun_cmp_sle
    cmpl $18,%eax
    jb .Lrun_cmp_ult
    je .Lrun_cmp_ule
    cmpl $19,%eax
    je .Lrun_cmp_eq
    xorb $1,%dl
.Lrun_cmp_eq:
    movzbl %dl,%eax
    jmp .Lrun_store
.Lrun_cmp_slt:
    movzbl %r8b,%eax
    jmp .Lrun_store
.Lrun_cmp_sle:
    movzbl %r9b,%eax
    jmp .Lrun_store
.Lrun_cmp_ult:
    movzbl %r11b,%eax
    jmp .Lrun_store
.Lrun_cmp_ule:
    movzbl %r10b,%eax
    xorl $1,%eax
    jmp .Lrun_store
.Lrun_lea:
    testb $1,3(%rsp)
    jz .Lrun_opcode
    movq 8(%rsp),%rax
    cmpq 16(%r12),%rax
    ja .Lrun_memory
    movq 72(%r12),%rdx
    testq %rdx,%rdx
    jz .Lrun_memory
    addq %rdx,%rax
    jc .Lrun_memory
    jmp .Lrun_store
.Lrun_sys:
    testb $1,3(%rsp)
    jz .Lrun_host
    movq 8(%rsp),%rdi
    cmpq $8,%rdi
    ja .Lrun_host
    cmpq $7,%rdi
    je .Lrun_host
    testq %rdi,%rdi
    jz .Lrun_exit
    cmpq $3,%rdi
    jne .Lrun_sys_dispatch
    cmpl $44,%eax
    jne .Lrun_host
.Lrun_sys_dispatch:
    movq 64(%r12),%r11
    testq %r11,%r11
    jz .Lrun_host
    movq 48(%rsp,%rcx,8),%rsi
    movq 48(%rsp,%rdx,8),%rdx
    movzbl 5(%rsp),%ecx
    movl %ecx,%r10d
    andl $15,%ecx
    movq 48(%rsp,%rcx,8),%rcx
    xorl %r8d,%r8d
    xorl %r9d,%r9d
    cmpl $44,%eax
    jne .Lrun_sys_short
    shrl $4,%r10d
    movq 48(%rsp,%r10,8),%r8
    movzbl 6(%rsp),%r10d
    movl %r10d,%eax
    andl $15,%eax
    movq 48(%rsp,%rax,8),%r9
    shrl $4,%r10d
    movq 48(%rsp,%r10,8),%rax
    jmp .Lrun_sys_call
.Lrun_sys_short:
    xorl %eax,%eax
.Lrun_sys_call:
    subq $16,%rsp
    movq %rax,(%rsp)
    call *%r11
    addq $16,%rsp
    movq %rax,48(%rsp)
    jmp .Lrun_next
.Lrun_memory:
    movl $2,%edx
    jmp .Lrun_fault
.Lrun_host:
    movl $6,%edx
    jmp .Lrun_fault
.Lrun_exit:
    movq 48(%rsp,%rcx,8),%rax
    movq %rax,8(%r13)
    movq $0,24(%r13)
    jmp .Lrun_return
.Lrun_container:
    movl $7,%edx
    jmp .Lrun_fault
.Lrun_code:
    movl $3,%edx
    jmp .Lrun_fault
.Lrun_opcode:
    movl $1,%edx
.Lrun_fault:
    movl $2,%eax
    movq %rdx,32(%r13)
    jmp .Lrun_status
.Lrun_budget:
    movl $3,%eax
.Lrun_status:
    movq %rax,0(%r13)
.Lrun_return:
    addq $192,%rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    ret
.Lrun_noresult:
    movl $2,%eax
    ret
.size tape_run_x86_64,.-tape_run_x86_64

.section .rodata
/* High nibble: immediate operands. Low nibble: register operands.
   Order is the opcode order in exec/facts/tape-shape.tsv. */
.Lshape:
    .byte 0x11,0x02
    .fill 19,1,0x03
    .byte 0x12,0x12,0x22,0x22,0x11,0x21,0x10,0x11,0x10,0x01,0x11
    .byte 0x02,0x02,0x51,0x11,0x11,0x00,0x10,0x11,0x01,0x02,0x01
    .byte 0x13,0x16,0x01,0x02,0x00
    .fill 14,1,0x03
    .fill 10,1,0x02
.section .note.GNU-stack,"",@progbits
