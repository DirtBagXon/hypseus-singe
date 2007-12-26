# MMX BLEND function
# by Matt Ownby - Feb 10th, 2004
# For GNU Assembler

.data

.globl _asm_line1
_asm_line1:
	.space 8

.globl _asm_line2
_asm_line2:
	.space 8

.globl _asm_dest
_asm_dest:
	.space 8

.globl _asm_iterations
_asm_iterations:
	.space 4

####################################

.text

.globl _blend_mmx
_blend_mmx:

	push %ebp	# needs to be preserved
	push %esi	# points to asm_line2
	push %ebx	# # of iterations we are to do
	push %ecx	# # of iterations we have done
	push %edx	# points to asm_line1
	# eax points to asm_dest, but eax doesn't need to be preserved because
	# it is assumed to hold the return value

	####################

	movl _asm_iterations, %ebx
	xorl %ecx, %ecx
	pxor %mm7, %mm7
	pxor %mm6, %mm6
	movl _asm_line1, %edx
	movl _asm_line2, %esi
	movl _asm_dest, %eax
	
MainLoop:
	movq (%edx,%ecx), %mm0	# load 8 bytes from asm_line1
	movq %mm0, %mm2
	
	punpcklbw %mm6, %mm0	# convert to 16-bit, preserve bottom
	punpckhbw %mm7, %mm2 	# convert to 16-bit, preserve top

	movq (%esi,%ecx), %mm1 	# load 8 bytes from asm_line2
	movq %mm1, %mm3

	punpcklbw %mm6, %mm1	# convert to 16-bit, preserve bottom
	punpckhbw %mm7, %mm3	# convert to 16-bit, preserve top

	# add bytes together with each other
	paddw %mm1, %mm0
	paddw %mm3, %mm2

	# divide results by 2 (average together)
	psrlw $1, %mm0
	psrlw $1, %mm2

	packuswb %mm2, %mm0	# merge unpacked 16-bit words into 8 packed bytes
	movq %mm0, (%eax, %ecx)	# store final result back to system memory

	add $8, %ecx	# advance index over the 8 bytes we've just handled
	cmp %ebx, %ecx	# is our current index less than the total iterations ?
	
	jl MainLoop	# if we have more iterations to do, then loop

	#####################

	emms

	pop %edx
	pop %ecx
	pop %ebx
	pop %esi
	pop %ebp

	ret

####################################################
