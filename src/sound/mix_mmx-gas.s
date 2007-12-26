# MMX 16-bit signed audio MIX function
# by Matt Ownby - May 7th, 2007
# For GNU Assembler

.data

.globl _asm_pSampleDst
_asm_pSampleDst:
	.space 8

.globl _asm_pMixBufs
_asm_pMixBufs:
	.space 4

.globl _asm_uBytesToMix
_asm_uBytesToMix:
	.space 4

####################################

.text

.globl _mix_mmx
_mix_mmx:

	push %ebp	# needs to be preserved
	push %esi	# points to sample2
	push %ebx	# # of iterations we are to do
	push %ecx	# # of iterations we have done
	push %edx	# points to sample1
	# eax points to the destination buffer, but eax doesn't need to be preserved because
	# it is assumed to hold the return value

	####################

	movl _asm_uBytesToMix, %ebx
	xorl %ecx, %ecx	# ecx = 0
	movl _asm_pSampleDst, %eax
	
MainLoop:
	# NOTE : for optimization purposes, we assume that asm_pMixBufs is never NULL
	movl _asm_pMixBufs, %edx

	pxor %mm0, %mm0	; mm0 = 0

MoreStreams:
	# get pMixBuf
	mov (%edx), %esi

	# edx = edx->next pointer
	mov 4(%edx), %edx

	# add bytes together with each other, 16-bit signed with 'saturation' (clipping)
	paddsw (%esi,%ecx), %mm0

	# is edx 0?
	test $-1, %edx
	jnz MoreStreams

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
