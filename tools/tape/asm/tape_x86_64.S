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
    /* Locals followed by 4096 pairs of return PC and guest stack pointer. */
    subq $65744,%rsp
    movq %rdi,%r12
    movq %rdx,%r13
    xorq %rax,%rax
    movq %r13,%rdi
    movl $5,%ecx
    rep stosq
    cmpq $4,0(%r12)
    je .Lrun_mode_ok
    cmpq $5,0(%r12)
    jne .Lrun_container
.Lrun_mode_ok:
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
    cmpq $5,0(%r12)
    jne .Lrun_entry_ok
    movq 32(%r12),%rbx
.Lrun_entry_ok:
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
    movq $0,176(%rsp)
    movq $-1,65728(%rsp)
    movq 8(%r12),%rdx
    movq 100(%rdx),%rax
    addq $7,%rax
    jc .Lrun_container
    andq $-8,%rax
    cmpq $8,124(%rdx)
    jne .Lrun_container
    movq 116(%rdx),%rcx
    movq 56(%r12),%r8
    subq $8,%r8
    cmpq %r8,%rcx
    ja .Lrun_container
    addq (%rdx,%rcx),%rax
    jc .Lrun_container
    movq 72(%r12),%rdx
    movq 16(%r12),%rcx
    cmpq %rax,%rcx
    jb .Lrun_container
    addq %rdx,%rax
    jc .Lrun_container
    movq %rax,184(%rsp)
    addq %rdx,%rcx
    jc .Lrun_container
    cmpq $5,0(%r12)
    je .Lrun_call_args
    movq 32(%r12),%r8
    cmpq $4095,%r8
    ja .Lrun_container
    movq 40(%r12),%r9
    testq %r9,%r9
    jnz .Lrun_argv_check
    testq %r8,%r8
    jnz .Lrun_container
    jmp .Lrun_stack_top
.Lrun_call_args:
    movq 40(%r12),%r9
    testq %r9,%r9
    jz .Lrun_stack_top
    movq 0(%r9),%r8
    movq %r8,48(%rsp)
    movq 8(%r9),%r8
    movq %r8,56(%rsp)
    movq 16(%r9),%r8
    movq %r8,64(%rsp)
    movq 24(%r9),%r8
    movq %r8,72(%rsp)
    movq 32(%r9),%r8
    movq %r8,80(%rsp)
    movq 40(%r9),%r8
    movq %r8,88(%rsp)
    jmp .Lrun_stack_top
.Lrun_argv_check:
    testq $7,%r9
    jnz .Lrun_container
    cmpq %rax,%r9
    jb .Lrun_container
    subq %r9,%rcx
    jc .Lrun_container
    leaq 8(,%r8,8),%r8
    cmpq %r8,%rcx
    jb .Lrun_container
    movq %r9,%rcx
.Lrun_stack_top:
    movq %rcx,104(%rsp)
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
    cmpl $10,%eax
    jbe .Lrun_shift
    cmpl $15,%eax
    jb .Lrun_divide
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
    cmpl $29,%eax
    je .Lrun_call
    cmpl $30,%eax
    je .Lrun_callr
    cmpl $31,%eax
    je .Lrun_callm
    cmpl $37,%eax
    je .Lrun_ret
    cmpl $38,%eax
    je .Lrun_frame
    cmpl $42,%eax
    je .Lrun_exit
    cmpl $43,%eax
    je .Lrun_sys
    cmpl $44,%eax
    je .Lrun_sys
    cmpl $45,%eax
    je .Lrun_argc
    cmpl $46,%eax
    je .Lrun_argv
    cmpl $47,%eax
    je .Lrun_next
    jmp .Lrun_opcode
.Lrun_argc:
    movq 32(%r12),%rax
    jmp .Lrun_store
.Lrun_argv:
    movq 48(%rsp,%rdx,8),%r8
    xorl %eax,%eax
    cmpq 32(%r12),%r8
    jae .Lrun_store
    movq 40(%r12),%rax
    movq (%rax,%r8,8),%rax
    jmp .Lrun_store
.Lrun_callm:
    movl %eax,%r10d
    movl $8,%r9d
    jmp .Lrun_mem_base
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
    jb .Lrun_mem_mapped
    movq 16(%r12),%r8
    subq %r9,%r8
    jb .Lrun_mem_mapped
    cmpq %r8,%rsi
    jbe .Lrun_mem_access
.Lrun_mem_mapped:
    movl $1,%r8d
    cmpl $21,%r10d
    je .Lrun_mem_check_map
    cmpl $23,%r10d
    je .Lrun_mem_check_map
    cmpl $31,%r10d
    je .Lrun_mem_check_map
    movl $2,%r8d
.Lrun_mem_check_map:
    call hoststub_lnx_x86_64_map_access
    testq %rax,%rax
    jnz .Lrun_memory
.Lrun_mem_access:
    cmpl $31,%r10d
    je .Lrun_callm_target
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
    xorl %r11d,%r11d
    jmp .Lrun_branch_target
.Lrun_call:
    movl $1,%r11d
.Lrun_branch_target:
    testb $1,3(%rsp)
    jz .Lrun_code
    movq 8(%rsp),%rax
    jmp .Lrun_checked_target
.Lrun_callr:
    movl $3,%r11d
    movq 48(%rsp,%rcx,8),%rax
    jmp .Lrun_checked_target
.Lrun_callm_target:
    movl $3,%r11d
    movq (%rdi),%rax
.Lrun_checked_target:
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
    cmpl $2,%r11d
    je .Lrun_lea_code_done
    cmpl $3,%r11d
    jne .Lrun_target_ready
    cmpq 65728(%rsp),%rax
    je .Lrun_target_ready
    /* An indirect target also needs a declared CODE relocation. */
    movq 8(%r12),%r8
    movq 164(%r8),%rdx
    movq 172(%r8),%rcx
    testb $15,%cl
    jnz .Lrun_code
    movq 56(%r12),%rsi
    subq %rdx,%rsi
    jb .Lrun_code
    cmpq %rsi,%rcx
    ja .Lrun_code
    addq %r8,%rdx
.Lrun_callr_relo:
    testq %rcx,%rcx
    jz .Lrun_code
    cmpb $0,8(%rdx)
    jne .Lrun_callr_next
    movslq 12(%rdx),%rsi
    cmpq %rax,%rsi
    jne .Lrun_callr_next
    movq %rax,65728(%rsp)
    jmp .Lrun_target_ready
.Lrun_callr_next:
    addq $16,%rdx
    subq $16,%rcx
    jmp .Lrun_callr_relo
.Lrun_target_ready:
    testl %r11d,%r11d
    jz .Lrun_branch_take
    movq 176(%rsp),%rcx
    cmpq $4096,%rcx
    jae .Lrun_stack
    movq 104(%rsp),%r8
    subq $8,%r8
    jc .Lrun_stack
    cmpq 184(%rsp),%r8
    jb .Lrun_stack
    movq %r8,104(%rsp)
    shlq $4,%rcx
    movq %rbx,192(%rsp,%rcx)
    movq %r8,200(%rsp,%rcx)
    incq 176(%rsp)
.Lrun_branch_take:
    leaq (%r14,%rax),%rbx
    jmp .Lrun_next
.Lrun_ret:
    movq 176(%rsp),%rcx
    testq %rcx,%rcx
    jnz .Lrun_ret_pop
    cmpq $5,0(%r12)
    jne .Lrun_stack
    movq 48(%rsp),%rax
    movq %rax,8(%r13)
    movq $0,24(%r13)
    jmp .Lrun_return
.Lrun_ret_pop:
    decq %rcx
    movq %rcx,176(%rsp)
    shlq $4,%rcx
    movq 104(%rsp),%r8
    cmpq 200(%rsp,%rcx),%r8
    jne .Lrun_stack
    addq $8,104(%rsp)
    movq 192(%rsp,%rcx),%rbx
    jmp .Lrun_next
.Lrun_frame:
    movq 8(%rsp),%rax
    movq 104(%rsp),%rcx
    testq %rax,%rax
    js .Lrun_frame_release
    subq %rax,%rcx
    jb .Lrun_stack
    cmpq 184(%rsp),%rcx
    jb .Lrun_stack
    jmp .Lrun_frame_set
.Lrun_frame_release:
    subq %rax,%rcx
    jnc .Lrun_stack
    movq 72(%r12),%rax
    addq 16(%r12),%rax
    cmpq $4,0(%r12)
    jne .Lrun_frame_bound
    cmpq $0,40(%r12)
    cmovneq 40(%r12),%rax
.Lrun_frame_bound:
    cmpq %rax,%rcx
    ja .Lrun_stack
.Lrun_frame_set:
    movq %rcx,104(%rsp)
    jmp .Lrun_next
.Lrun_stack:
    movl $5,%edx
    jmp .Lrun_fault
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
.Lrun_shift:
    movq 48(%rsp,%rdx,8),%r8
    movzbl 5(%rsp),%edx
    andl $15,%edx
    movl %ecx,%r10d
    movq 48(%rsp,%rdx,8),%rcx
    cmpl $8,%eax
    je .Lrun_shl
    cmpl $9,%eax
    je .Lrun_sar
    shrq %cl,%r8
    jmp .Lrun_shift_done
.Lrun_shl:
    shlq %cl,%r8
    jmp .Lrun_shift_done
.Lrun_sar:
    sarq %cl,%r8
.Lrun_shift_done:
    movl %r10d,%ecx
    jmp .Lrun_arith_result
.Lrun_divide:
    movl %eax,%r10d
    movq 48(%rsp,%rdx,8),%rax
    movzbl 5(%rsp),%edx
    andl $15,%edx
    movq 48(%rsp,%rdx,8),%r9
    testq %r9,%r9
    jz .Lrun_opcode
    cmpl $13,%r10d
    jae .Lrun_divide_unsigned
    cmpq $-1,%r9
    jne .Lrun_divide_signed
    movabsq $0x8000000000000000,%r8
    cmpq %r8,%rax
    je .Lrun_opcode
.Lrun_divide_signed:
    cqto
    idivq %r9
    jmp .Lrun_divide_result
.Lrun_divide_unsigned:
    xorl %edx,%edx
    divq %r9
.Lrun_divide_result:
    testl $1,%r10d
    cmovzq %rdx,%rax
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
    movq 8(%rsp),%rax
    testb $1,3(%rsp)
    jz .Lrun_lea_code
    cmpq 16(%r12),%rax
    ja .Lrun_memory
    movq 72(%r12),%rdx
    testq %rdx,%rdx
    jz .Lrun_memory
    addq %rdx,%rax
    jc .Lrun_memory
    jmp .Lrun_store
.Lrun_lea_code:
    movl $2,%r11d
    jmp .Lrun_checked_target
.Lrun_lea_code_done:
    movzbl 4(%rsp),%ecx
    andl $15,%ecx
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
    cmpq $3,%rdi
    jne .Lrun_sys_call
    /* Linux tape uses MAP_SHARED/PRIVATE and MAP_ANONYMOUS. Do not
       accept fixed mappings or flags outside this tape dialect subset. */
    testq $-36,%r8
    jnz .Lrun_sys_einval
    movl %r8d,%r10d
    andl $3,%r8d
    shrl $3,%r10d
    orl %r10d,%r8d
    jmp .Lrun_sys_call
.Lrun_sys_short:
    cmpq $8,%rdi
    je .Lrun_fstat
    xorl %eax,%eax
.Lrun_sys_call:
    subq $16,%rsp
    movq %rax,(%rsp)
    call *%r11
    addq $16,%rsp
.Lrun_sys_result:
    movq %rax,48(%rsp)
    jmp .Lrun_next
.Lrun_sys_einval:
    movq $-22,%rax
    jmp .Lrun_sys_result
.pushsection .rodata
.Lstat_kind:
    .zero 98
    .byte 6,2,4,0,8
    .zero 5
    .byte 10
    .zero 3
    .byte 1,0,0,12
    .zero 140
.popsection
.Lrun_fstat:
    /* Project the Linux x86_64 fields consumed by sys.fs from hs_stat64.
       Fields not supplied by the frozen ABI are not part of this projection. */
    movq %rdx,%rdi
    movq %rdx,%rax
    subq 72(%r12),%rax
    jb .Lrun_fstat_mapped
    movq 16(%r12),%r9
    subq $144,%r9
    jb .Lrun_fstat_mapped
    cmpq %r9,%rax
    jbe .Lrun_fstat_call
.Lrun_fstat_mapped:
    subq $16,%rsp
    movq %rsi,(%rsp)
    movq %r11,8(%rsp)
    movl $144,%r9d
    movl $2,%r8d
    call hoststub_lnx_x86_64_map_access
    movq (%rsp),%rsi
    movq 8(%rsp),%r11
    addq $16,%rsp
    testq %rax,%rax
    jnz .Lrun_sys_result
.Lrun_fstat_call:
    subq $80,%rsp
    movq %rdi,8(%rsp)
    movq %rdi,%rdx
    movl $8,%edi
    movq $0,(%rsp)
    call *%r11
    testq %rax,%rax
    jnz .Lrun_fstat_done
    movq 8(%rsp),%rdi
    movq %rdi,%rsi
    leaq 16(%rsp),%rdi
    movl $8,%ecx
    rep movsq
    movq 8(%rsp),%rdi
    xorl %eax,%eax
    movl $18,%ecx
    rep stosq
    movq 8(%rsp),%rdi
    movq 16(%rsp),%rax
    movq %rax,48(%rdi)
    movq 24(%rsp),%rax
    movq %rax,8(%rdi)
    movq 32(%rsp),%rax
    movq %rax,0(%rdi)
    movl 56(%rsp),%eax
    movzbl 60(%rsp),%ecx
    leaq .Lstat_kind(%rip),%rdx
    movzbl (%rdx,%rcx),%ecx
    shll $12,%ecx
    orl %ecx,%eax
    movl %eax,24(%rdi)
    movq 64(%rsp),%rax
    movq %rax,28(%rdi)
    movq 72(%rsp),%rax
    movq %rax,40(%rdi)
    movl $1000000000,%ecx
    movq 40(%rsp),%rax
    cqto
    idivq %rcx
    testq %rdx,%rdx
    jns .Lrun_fstat_mtime
    decq %rax
    addq %rcx,%rdx
.Lrun_fstat_mtime:
    movq %rax,88(%rdi)
    movq %rdx,96(%rdi)
    movq 48(%rsp),%rax
    cqto
    idivq %rcx
    testq %rdx,%rdx
    jns .Lrun_fstat_ctime
    decq %rax
    addq %rcx,%rdx
.Lrun_fstat_ctime:
    movq %rax,104(%rdi)
    movq %rdx,112(%rdi)
    xorl %eax,%eax
.Lrun_fstat_done:
    addq $80,%rsp
    jmp .Lrun_sys_result
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
    addq $65744,%rsp
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
