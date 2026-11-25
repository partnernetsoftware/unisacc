/* T1.3 Linux x86_64 minimal 8-slot hoststub dispatcher (m1_core frozen slots 0,1,2,3,4,5,6,8).
 * Symbols exported:
 *   void hoststub_lnx_x86_64_bind(uint64_t membase, uint64_t memlimit);
 *   int64_t hoststub_lnx_x86_64(uint64_t slot_id, uint64_t r0, uint64_t r1, uint64_t r2,
 *                               uint64_t r3, uint64_t r4, uint64_t r5);
 *   uint64_t hoststub_lnx_x86_64_mmap_count(void);
 *
 * Security / T0.4 B2+B3 contract:
 *   - Non-frozen slots (anything outside {0,1,2,3,4,5,6,8}) return -38 (-ENOSYS) without OS syscall.
 *   - All guest pointers are 64-bit real host pointers validated via unsigned subtraction
 *     against [hs_membase, hs_membase+hs_memlimit) or active hs_mmap spans.
 *   - open(path) scans strictly inside [path, span_end) for NUL before invoking sys_openat, else -14 (-EFAULT).
 *   - munmap only permits tracked active hs_mmap spans (never host text/stack/arena).
 */
.text
.p2align 4
.globl hoststub_lnx_x86_64_bind
.type hoststub_lnx_x86_64_bind,@function
hoststub_lnx_x86_64_bind:
    movq %rdi, hs_membase(%rip)
    movq %rsi, hs_memlimit(%rip)
    movq $0, hs_mmap_cnt(%rip)
    leaq hs_mmap_tab(%rip), %rdi
    xorl %eax, %eax
    movl $64, %ecx
    rep stosq
    ret
.size hoststub_lnx_x86_64_bind,.-hoststub_lnx_x86_64_bind

.p2align 4
.globl hoststub_lnx_x86_64_mmap_count
.type hoststub_lnx_x86_64_mmap_count,@function
hoststub_lnx_x86_64_mmap_count:
    movq hs_mmap_cnt(%rip), %rax
    ret
.size hoststub_lnx_x86_64_mmap_count,.-hoststub_lnx_x86_64_mmap_count

/* Internal helper: validate [rdi, rdi+rsi) inside hs_membase..+hs_memlimit or hs_mmap_tab[0..31].
 * Returns rax = 0 if valid, -14 (-EFAULT) if out of bounds. Clobbers r10, r11, rax. */
.Lchk_span:
    movq hs_membase(%rip), %r10
    movq hs_memlimit(%rip), %r11
    cmpq %r11, %rsi
    ja .Lchk_mmap_loop_prep
    movq %rdi, %rax
    subq %r10, %rax
    jb .Lchk_mmap_loop_prep
    subq %rsi, %r11
    cmpq %r11, %rax
    jbe .Lchk_ok
.Lchk_mmap_loop_prep:
    leaq hs_mmap_tab(%rip), %r10
    xorl %eax, %eax
.Lchk_mmap_loop:
    movq 8(%r10), %r11
    testq %r11, %r11
    jz .Lchk_mmap_next
    cmpq %r11, %rsi
    ja .Lchk_mmap_next
    movq %rdi, %rax
    subq 0(%r10), %rax
    jb .Lchk_mmap_next
    subq %rsi, %r11
    cmpq %r11, %rax
    jbe .Lchk_ok
.Lchk_mmap_next:
    addq $16, %r10
    leaq hs_mmap_tab+512(%rip), %rax
    cmpq %rax, %r10
    jb .Lchk_mmap_loop
    movq $-14, %rax
    ret
.Lchk_ok:
    xorl %eax, %eax
    ret

/* Internal helper: validate C string ptr in rdi is inside linear arena or hs_mmap_tab
 * AND has a NUL byte before the end of that containing span.
 * Returns rax = 0 if valid, -14 (-EFAULT) otherwise. Clobbers r10, r11, rax, rcx. */
.Lchk_cstr:
    testq %rdi, %rdi
    jz .Lcstr_bad
    movq hs_membase(%rip), %r10
    movq hs_memlimit(%rip), %r11
    movq %rdi, %rax
    subq %r10, %rax
    jb .Lcstr_mmap_prep
    cmpq %r11, %rax
    jae .Lcstr_mmap_prep
    subq %rax, %r11
    jmp .Lcstr_scan
.Lcstr_mmap_prep:
    leaq hs_mmap_tab(%rip), %r10
.Lcstr_mmap_loop:
    movq 8(%r10), %r11
    testq %r11, %r11
    jz .Lcstr_mmap_next
    movq %rdi, %rax
    subq 0(%r10), %rax
    jb .Lcstr_mmap_next
    cmpq %r11, %rax
    jae .Lcstr_mmap_next
    subq %rax, %r11
    jmp .Lcstr_scan
.Lcstr_mmap_next:
    addq $16, %r10
    leaq hs_mmap_tab+512(%rip), %rax
    cmpq %rax, %r10
    jb .Lcstr_mmap_loop
.Lcstr_bad:
    movq $-14, %rax
    ret
.Lcstr_scan:
    /* Scan up to r11 bytes starting at rdi for NUL byte */
    xorl %ecx, %ecx
.Lcstr_byte:
    cmpq %r11, %rcx
    jae .Lcstr_bad
    cmpb $0, (%rdi,%rcx)
    je .Lchk_ok
    incq %rcx
    jmp .Lcstr_byte

.p2align 4
.globl hoststub_lnx_x86_64
.type hoststub_lnx_x86_64,@function
hoststub_lnx_x86_64:
    /* rdi=slot_id, rsi=r0, rdx=r1, rcx=r2, r8=r3, r9=r4, 8(%rsp)=r5 */
    cmpl $0, %edi
    je .Lslot_exit
    cmpl $1, %edi
    je .Lslot_read
    cmpl $2, %edi
    je .Lslot_write
    cmpl $3, %edi
    je .Lslot_mmap
    cmpl $4, %edi
    je .Lslot_munmap
    cmpl $5, %edi
    je .Lslot_open
    cmpl $6, %edi
    je .Lslot_close
    cmpl $8, %edi
    je .Lslot_fstat
    cmpl $9, %edi
    je .Lslot_stat
    cmpl $10, %edi
    je .Lslot_lseek
    cmpl $12, %edi
    je .Lslot_spawn
    cmpl $19, %edi
    je .Lslot_sleep
    cmpl $36, %edi
    je .Lslot_isatty
    jmp .Lnosys

.Lnosys:
    movq $-38, %rax
    ret
.Leinval:
    movq $-22, %rax
    ret

.Lslot_exit:
    movl %esi, %edi
    andl $255, %edi
    movl $60, %eax
    syscall
    ret

.Lslot_read:
    pushq %rsi
    pushq %rdx
    pushq %rcx
    movq %rdx, %rdi
    movq %rcx, %rsi
    call .Lchk_span
    popq %rdx
    popq %rsi
    popq %rdi
    testq %rax, %rax
    jnz .Lret_done
    xorl %eax, %eax
    syscall
.Lret_done:
    ret

.Lslot_write:
    pushq %rsi
    pushq %rdx
    pushq %rcx
    movq %rdx, %rdi
    movq %rcx, %rsi
    call .Lchk_span
    popq %rdx
    popq %rsi
    popq %rdi
    testq %rax, %rax
    jnz .Lret_done
    movl $1, %eax
    syscall
    ret

.Lslot_close:
    movl %esi, %edi
    movl $3, %eax
    syscall
    ret

.Lslot_mmap:
    /* r0=addr(%rsi), r1=len(%rdx), r2=prot(%rcx), r3=hs_flags(%r8), r4=fd(%r9), r5=off(8(%rsp)) */
    testq $-8, %rcx
    jnz .Leinval
    testq $-16, %r8
    jnz .Leinval
    movl %r8d, %eax
    andl $3, %eax
    cmpl $3, %eax
    je .Leinval
    /* Translate hs_flags -> Linux MAP_*: bits 0..1 stay, bit 2 (4 ANON) -> 0x20, bit 3 (8 FIXED) -> 0x10 */
    movl %eax, %r10d
    testl $4, %r8d
    jz .Lmmap_nofix_prep
    orl $0x20, %r10d
.Lmmap_nofix_prep:
    testl $8, %r8d
    jz .Lmmap_do
    orl $0x10, %r10d
.Lmmap_do:
    movq 8(%rsp), %r11          /* save r5=off before stack adjustments */
    pushq %rdx                  /* save len for mmap_tab tracking */
    movq %rsi, %rdi             /* addr */
    movq %rdx, %rsi             /* len */
    movq %rcx, %rdx             /* prot */
    /* r10 already os_flags */
    movq %r9, %r8               /* fd */
    movq %r11, %r9              /* off */
    movl $9, %eax
    syscall
    popq %rdx                   /* restore len */
    cmpq $-4096, %rax
    ja .Lret_done
    testq %rdx, %rdx
    jz .Lret_done
    /* Track [rax, rax+rdx) in first free slot of hs_mmap_tab */
    leaq hs_mmap_tab(%rip), %r10
.Lmmap_ins:
    cmpq $0, 8(%r10)
    je .Lmmap_store
    addq $16, %r10
    leaq hs_mmap_tab+512(%rip), %r11
    cmpq %r11, %r10
    jb .Lmmap_ins
    /* Table full: undo mapping and return -ENOMEM (-12) so no untracked escape persists */
    movq %rax, %rdi
    movq %rdx, %rsi
    movl $11, %eax
    syscall
    movq $-12, %rax
    ret
.Lmmap_store:
    movq %rax, 0(%r10)
    movq %rdx, 8(%r10)
    incq hs_mmap_cnt(%rip)
    ret

.Lslot_munmap:
    /* r0=addr(%rsi), r1=len(%rdx). Only allow exact tracked mmap entry. */
    leaq hs_mmap_tab(%rip), %r10
.Lmunmap_find:
    cmpq %rsi, 0(%r10)
    jne .Lmunmap_next
    cmpq %rdx, 8(%r10)
    jne .Lmunmap_next
    testq %rdx, %rdx
    jz .Lmunmap_next
    pushq %r10
    movq %rsi, %rdi
    movq %rdx, %rsi
    movl $11, %eax
    syscall
    popq %r10
    testq %rax, %rax
    jnz .Lret_done
    movq $0, 0(%r10)
    movq $0, 8(%r10)
    decq hs_mmap_cnt(%rip)
    ret
.Lmunmap_next:
    addq $16, %r10
    leaq hs_mmap_tab+512(%rip), %r11
    cmpq %r11, %r10
    jb .Lmunmap_find
    jmp .Leinval

.Lslot_open:
    /* r0=path(%rsi), r1=hs_flags(%rdx), r2=mode(%rcx) */
    testq $-0x400, %rdx
    jnz .Leinval
    movl %edx, %eax
    andl $3, %eax
    cmpl $3, %eax
    je .Leinval
    pushq %rsi
    pushq %rdx
    pushq %rcx
    movq %rsi, %rdi
    call .Lchk_cstr
    popq %r10                   /* mode */
    popq %rdx                   /* hs_flags */
    popq %rsi                   /* path */
    testq %rax, %rax
    jnz .Lret_done
    /* Translate HS_O_*: low 2 bits stay */
    movl %edx, %eax
    andl $3, %eax
    testl $0x10, %edx
    jz .Lopen_f2
    orl $0x40, %eax             /* O_CREAT */
.Lopen_f2:
    testl $0x20, %edx
    jz .Lopen_f3
    orl $0x80, %eax             /* O_EXCL */
.Lopen_f3:
    testl $0x40, %edx
    jz .Lopen_f4
    orl $0x200, %eax            /* O_TRUNC */
.Lopen_f4:
    testl $0x80, %edx
    jz .Lopen_f5
    orl $0x400, %eax            /* O_APPEND */
.Lopen_f5:
    testl $0x100, %edx
    jz .Lopen_f6
    orl $0x80000, %eax          /* O_CLOEXEC */
.Lopen_f6:
    testl $0x200, %edx
    jz .Lopen_call
    orl $0x800, %eax            /* O_NONBLOCK */
.Lopen_call:
    movq $-100, %rdi            /* AT_FDCWD */
    movl %eax, %edx             /* os_flags */
    movl $257, %eax             /* sys_openat */
    syscall
    ret

.Lslot_fstat:
    /* r0=fd(%rsi), r1=out(%rdx) -> 64B LE hs_stat64 */
    pushq %rsi
    pushq %rdx
    movq %rdx, %rdi
    movl $64, %esi
    call .Lchk_span
    popq %r10                   /* out ptr */
    popq %rdi                   /* fd */
    testq %rax, %rax
    jnz .Lret_done
    pushq %r10
    subq $152, %rsp             /* aligned kernel struct stat buffer (144B) */
    movq %rsp, %rsi
    movl $5, %eax               /* sys_fstat */
    syscall
    testq %rax, %rax
    js .Lfstat_done
.Lfstat_fill:
    movq 152(%rsp), %r10        /* out ptr */
    /* 0: size (st_size @48) */
    movq 48(%rsp), %rax
    movq %rax, 0(%r10)
    /* 8: ino (st_ino @8) */
    movq 8(%rsp), %rax
    movq %rax, 8(%r10)
    /* 16: dev (st_dev @0) */
    movq 0(%rsp), %rax
    movq %rax, 16(%r10)
    /* 24: mtime_ns = st_mtime(@88)*1e9 + st_mtime_nsec(@96) */
    movq 88(%rsp), %rax
    imulq $1000000000, %rax, %rax
    addq 96(%rsp), %rax
    movq %rax, 24(%r10)
    /* 32: ctime_ns = st_ctime(@104)*1e9 + st_ctime_nsec(@112) */
    movq 104(%rsp), %rax
    imulq $1000000000, %rax, %rax
    addq 112(%rsp), %rax
    movq %rax, 32(%r10)
    /* 40: mode = st_mode(@24) & 07777 */
    movl 24(%rsp), %ecx
    movl %ecx, %eax
    andl $0xfff, %eax
    movl %eax, 40(%r10)
    /* 44: kind char from .Lkindtab[(st_mode >> 12) & 15] */
    shrl $12, %ecx
    andl $15, %ecx
    leaq .Lkindtab(%rip), %r11
    movzbl (%r11,%rcx), %eax
    movl %eax, 44(%r10)
    /* 48: uid (@28), 52: gid (@32) */
    movl 28(%rsp), %eax
    movl %eax, 48(%r10)
    movl 32(%rsp), %eax
    movl %eax, 52(%r10)
    /* 56: rdev (st_rdev @40 if kind == 'c'(2) or 'b'(6), else 0) */
    xorl %eax, %eax
    cmpl $2, %ecx
    je .Lfstat_rdev
    cmpl $6, %ecx
    jne .Lfstat_wrdev
.Lfstat_rdev:
    movq 40(%rsp), %rax
.Lfstat_wrdev:
    movq %rax, 56(%r10)
    xorl %eax, %eax
.Lfstat_done:
    addq $160, %rsp
    ret

.Lslot_stat:
    /* r0=path(%rsi), r1=out(%rdx), r2=flags(%rcx) */
    testq $-2, %rcx
    jnz .Leinval
    pushq %rsi
    pushq %rdx
    pushq %rcx
    movq %rsi, %rdi
    call .Lchk_cstr
    testq %rax, %rax
    jnz .Lstat_bad3
    movq 8(%rsp), %rdi
    movl $64, %esi
    call .Lchk_span
    testq %rax, %rax
    jnz .Lstat_bad3
    popq %rcx                   /* flags */
    popq %r10                   /* out ptr */
    popq %rsi                   /* path */
    pushq %r10
    subq $152, %rsp
    movq $-100, %rdi            /* AT_FDCWD */
    movq %rsp, %rdx
    xorl %r10d, %r10d
    testl $1, %ecx
    jz .Lstat_call
    movl $0x100, %r10d          /* AT_SYMLINK_NOFOLLOW */
.Lstat_call:
    movl $262, %eax             /* sys_newfstatat */
    syscall
    testq %rax, %rax
    js .Lfstat_done
    jmp .Lfstat_fill
.Lstat_bad3:
    addq $24, %rsp
    ret

.Lslot_lseek:
    /* r0=fd(%rsi), r1=off(%rdx), r2=whence(%rcx) */
    cmpq $2, %rcx
    ja .Leinval
    movl %esi, %edi
    movq %rdx, %rsi
    movl %ecx, %edx
    movl $8, %eax               /* sys_lseek */
    syscall
    ret

.Lslot_spawn:
    /* r0=path(%rsi), r1=argv(%rdx), r2=envp(%rcx) */
    pushq %rsi
    pushq %rdx
    pushq %rcx
    movq %rsi, %rdi
    call .Lchk_cstr
    testq %rax, %rax
    jnz .Lstat_bad3
    movq 8(%rsp), %rdi
    testq %rdi, %rdi
    jz .Lspawn_chk_env
    movl $8, %esi
    call .Lchk_span
    testq %rax, %rax
    jnz .Lstat_bad3
.Lspawn_chk_env:
    movq 0(%rsp), %rdi
    testq %rdi, %rdi
    jz .Lspawn_fork
    movl $8, %esi
    call .Lchk_span
    testq %rax, %rax
    jnz .Lstat_bad3
.Lspawn_fork:
    movl $57, %eax              /* sys_fork */
    syscall
    testq %rax, %rax
    js .Lstat_bad3
    jnz .Lspawn_parent
    /* child */
    popq %rdx                   /* envp */
    popq %rsi                   /* argv */
    popq %rdi                   /* path */
    movl $59, %eax              /* sys_execve */
    syscall
    movl $127, %edi
    movl $231, %eax             /* sys_exit_group */
    syscall
.Lspawn_parent:
    addq $24, %rsp
    ret

.Lslot_sleep:
    /* r0=req(%rsi), r1=rem(%rdx) */
    pushq %rsi
    pushq %rdx
    movq %rsi, %rdi
    movl $16, %esi
    call .Lchk_span
    testq %rax, %rax
    jnz .Lsleep_bad2
    movq 0(%rsp), %rdi
    testq %rdi, %rdi
    jz .Lsleep_call
    movl $16, %esi
    call .Lchk_span
    testq %rax, %rax
    jnz .Lsleep_bad2
.Lsleep_call:
    popq %rsi                   /* rem */
    popq %rdi                   /* req */
    movl $35, %eax              /* sys_nanosleep */
    syscall
    ret
.Lsleep_bad2:
    addq $16, %rsp
    ret

.Lslot_isatty:
    /* r0=fd(%rsi) */
    subq $72, %rsp
    movl %esi, %edi
    movl $0x5401, %esi          /* TCGETS */
    movq %rsp, %rdx
    movl $16, %eax              /* sys_ioctl */
    syscall
    addq $72, %rsp
    testq %rax, %rax
    jnz .Lisatty_ret
    movq $1, %rax
.Lisatty_ret:
    ret
.size hoststub_lnx_x86_64,.-hoststub_lnx_x86_64

.section .rodata
.Lkindtab:
    .ascii "?pc?d?b?f?l?s???"

.bss
.p2align 4
hs_membase:
    .quad 0
hs_memlimit:
    .quad 0
hs_mmap_cnt:
    .quad 0
hs_mmap_tab:
    .zero 512

.section .note.GNU-stack,"",@progbits
