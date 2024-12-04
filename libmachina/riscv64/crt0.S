	.text
	.globl _start
_start:
	.option push
	.option norelax
	la gp, __global_pointer$
	.option pop
	la sp, stacktop
	call ___start
	ebreak


	.data
	.align 4096
stackbtm:
	.skip 4096
stacktop:	
