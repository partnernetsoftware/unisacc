# Hand-written x86-64 (AT&T), inside the `unisacc as` subset (R18-1).
# tests/asmtext.sh assembles it with unisacc and with the system assembler;
# the text, data and relocations must agree.
	.text
	.globl	sum
	.type	sum,@function
sum:                            # long sum(long *p, long n)
	xorq	%rax, %rax
.Lloop:
	testq	%rsi, %rsi
	je	.Ldone
	movq	(%rdi), %rcx
	addq	%rcx, %rax
	addq	$8, %rdi
	subq	$1, %rsi
	jmp	.Lloop
.Ldone:
	retq
	.globl	greet
greet:
	pushq	%rbx
	leaq	msg(%rip), %rdi
	callq	puts@PLT
	movq	counter(%rip), %rax
	addq	$1, %rax
	movq	%rax, counter(%rip)
	movl	$0x0, %eax
	popq	%rbx
	retq
far:
	cmpq	$1000, %rdi
	jle	.Lsmall
	imulq	%rdi, %rdi
	shrq	%cl, %rdi
	sarq	%rdi
.Lsmall:
	movabsq	$0x123456789a, %rdx
	movsbq	3(%rsp), %rax
	movslq	-8(%rbp), %rax
	movb	%al, 1(%rbx)
	movw	%ax, (%rbx,%rcx,2)
	setne	%al
	movzbq	%al, %rax
	cqto
	idivq	%rcx
	cvtsi2sdq	%rax, %xmm0
	mulsd	%xmm1, %xmm0
	cvttsd2si	%xmm0, %rax
	callq	*%r11
	retq
	.data
msg:
	.ascii	"hello from as\0"
	.byte	1, 2, 0x7f, 255
	.bss
counter:
	.zero	8
