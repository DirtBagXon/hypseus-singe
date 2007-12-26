; MMX BLEND function
; by Matt Ownby
; For MASM (Microschlop) only

.486
.MMX

.model flat, c

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.DATA

ALIGN 8

PUBLIC asm_line1
asm_line1	dq	0

PUBLIC asm_line2
asm_line2	dq	0

PUBLIC asm_dest
asm_dest	dq	0

PUBLIC asm_iterations
asm_iterations	dd	0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.CODE

PUBLIC blend_mmx
blend_mmx PROC NEAR

	push ebp	; needs to be preserved
	push esi	; points to asm_line2
	push ebx	; # of iterations we are to do
	push ecx	; # of iterations we have done
	push edx	; points to asm_line1
	; eax points to asm_dest, but eax doesn't need to be preserved because
	; it is assumed to hold the return value

	;;;;;;;;;;;;;;;;;;;;

	mov ebx, asm_iterations
	xor ecx, ecx
	pxor mm7, mm7
	pxor mm6, mm6
	mov edx, dword ptr[asm_line1]
	mov esi, dword ptr[asm_line2]
	mov eax, dword ptr[asm_dest] 

MainLoop:
	movq mm0, [edx+ecx]	; load 8 bytes from asm_line1
	movq mm2, mm0
	
	punpcklbw mm0, mm6	; convert to 16-bit, preserve bottom
	punpckhbw mm2, mm7	; convert to 16-bit, preserve top

	movq mm1, [esi+ecx]	; load 8 bytes from asm_line2
	movq mm3, mm1

	punpcklbw mm1, mm6	; convert to 16-bit, preserve bottom
	punpckhbw mm3, mm7	; convert to 16-bit, preserve top

	; add bytes together with each other
	paddw mm0, mm1
	paddw mm2, mm3

	; divide results by 2 (average together)
	psrlw mm0, 1
	psrlw mm2, 1

	packuswb mm0, mm2	; merge unpacked 16-bit words into 8 packed bytes
	movq [eax+ecx], mm0	; store final result back to system memory

	add ecx, 8	; advance index over the 8 bytes we've just handled
	cmp ecx, ebx	; is our current index less than the total iterations ?
	
	jl MainLoop	; if we have more iterations to do, then loop

	;;;;;;;;;;;;;;;;;;;;;

	emms

	pop edx
	pop ecx
	pop ebx
	pop esi
	pop ebp

	ret
blend_mmx ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

END