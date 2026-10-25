// Hand-written AArch64 (GNU syntax), inside the `unisacc as` subset (R18-1).
// tests/asmtext.sh assembles it with unisacc and with the system assembler;
// the text, data and relocations must agree.
	.text
	.globl	sum
	.type	sum,%function
sum:                            // long sum(long *p, long n)
	mov	x2, #0
.Lloop:
	cbz	x1, .Ldone
	ldr	x3, [x0]
	add	x2, x2, x3
	add	x0, x0, #8
	sub	x1, x1, #1
	b	.Lloop
.Ldone:
	mov	x0, x2
	ret
	.globl	greet
greet:
	sub	sp, sp, #16
	str	x30, [sp, #8]
	adrp	x0, msg
	add	x0, x0, :lo12:msg
	bl	puts
	adrp	x1, counter
	add	x1, x1, :lo12:counter
	ldr	x2, [x1]
	add	x2, x2, #1
	str	x2, [x1]
	mov	x0, #0
	ldr	x30, [sp, #8]
	add	sp, sp, #16
	ret
far:
	cmp	x0, #1000
	cset	x1, gt
	b.le	.Lsmall
	mul	x0, x0, x0
	lsl	x0, x0, #3
	asr	x0, x0, x1
.Lsmall:
	movk	x3, #0x1234, lsl #16
	mov	x4, #-1
	ldrsb	x5, [x0, #3]
	ldur	x6, [x7, #-8]
	strb	w5, [x1, #1]
	sxtw	x5, w5
	sdiv	x0, x0, x5
	msub	x0, x5, x0, x1
	scvtf	d16, x0
	fmul	d16, d16, d17
	fcvtzs	x0, d16
	eor	x0, x0, #1
	blr	x9
	ret
	.data
msg:
	.ascii	"hello from as\0"
	.byte	1, 2, 0x7f, 255
	.bss
counter:
	.zero	8
