
		# count for 10 billion instructions
		#   total is 1 + [4+(x*3)]y + 2
		# 10 billion if x=3 y=769230769

# Sycscalls
.equ SYSCALL_EXIT,      1

	.globl _start
_start:

	ldr	r3,outer_count			@ load outer counter

outer_loop:


	ldr	r2,count			@ set inner count

inner_loop:
	add	r2,r2,#-1
	cmp	r2,#0
	bne	inner_loop			@ repeat till zero

	add	r3,r3,#-1
	cmp	r3,#0
	bne	outer_loop

	@================================
	@ Exit
	@================================
exit:
	mov     r0,#0				@ result is zero
	mov	r7,#SYSCALL_EXIT
	swi     0x0            			@ and exit

count:		.word 3
outer_count:	.word 769230769
