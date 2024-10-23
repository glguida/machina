	.text
	.globl _start
_start:
	mov $stacktop, %esp
	call ___start
	ud2



	.data
	.align 4096
stackbtm:
	.skip 4096
stacktop:	
