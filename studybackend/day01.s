	.text
	.file	"day01.c"
	.globl	main
	.p2align	4
	.type	main,@function
main:
	.cfi_startproc
	movl	$42, %eax
	retq
.Lfunc_end0:
	.size	main, .Lfunc_end0-main
	.cfi_endproc

	.section	".note.GNU-stack","",@progbits
