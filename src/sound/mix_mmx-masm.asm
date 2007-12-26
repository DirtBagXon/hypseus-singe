; MMX MIX function
; by Matt Ownby
; For MASM (Microschlop) only

.486
.MMX

.model flat, c

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.DATA

ALIGN 8

PUBLIC asm_pSampleDst
asm_pSampleDst	dq	0

; This is a pointer to a struct that _must_ start like this:
; struct mix_s {
;   void *ptrStream;
;   struct mix_s *ptrNext;
;   ...
; };
; So it's sort of a linked list struct that this ASM code will iterate through to do mixing.
; The last stream should have ptrNext set to NULL.
PUBLIC asm_pMixBufs
asm_pMixBufs	dd	0

PUBLIC asm_uBytesToMix
asm_uBytesToMix	dd	0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.CODE

PUBLIC mix_mmx
mix_mmx PROC NEAR

	push ebp	; needs to be preserved
	push esi	; points to asm_pMixBufs->pMixBuf
	push ebx	; # of iterations we are to do
	push ecx	; # of iterations we have done
	push edx	; points to asm_pMixBufs
	; eax points to asm_pSampleDst, but eax doesn't need to be preserved because
	; it is assumed to hold the return value

	;;;;;;;;;;;;;;;;;;;;

	mov ebx, asm_uBytesToMix
	xor ecx, ecx
	mov eax, dword ptr[asm_pSampleDst] 

MainLoop:
	; NOTE : for optimization purposes, we assume that asm_pMixBufs is never NULL	
	mov edx, dword ptr[asm_pMixBufs]

	pxor mm0, mm0	; mm0 = 0

MoreStreams:
	; get pMixBuf	
	mov esi, [edx]

	; edx = edx->next pointer
	mov edx, dword ptr[edx+4]

	; is this faster?
	paddsw mm0, [esi+ecx]
		
	test edx, -1
	; if we have another stream to mix, then loop
	jnz MoreStreams

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
mix_mmx ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

END
