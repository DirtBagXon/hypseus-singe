; MMX RGB2YUV conversion function
; by Matt Ownby
; For MASM (Microschlop) only

; This function is equivalent to this C code from Intel:
;
;	Y = (unsigned char) (((9798*R)+(19235*G)+(3736*B))/32768);
;	U = (unsigned char) ((((-5537*R)+(-10878*G)+(16384*B))/32768) + 128);
;	V = (unsigned char) ((((16384*R)+(-13729*G)+(-2664*B))/32768) + 128);

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; C function prototype is:

;	extern "C" void asm_rgb2yuv();

; This function is controlled through global variables (because I found that to be the fastest).
; To set the RGB values that you want converted, you write three 8-bit values (R, G, and B) to
; an array of 16-bit shorts (I found this to be the fastest).
; As far as you're concerned, the array you write to looks like this:
;
;	extern "C" unsigned short asm_rgb2yuv_input[3];
;
; (it actually is 64-bits but the last element is ignored)
;
; Here's some example C code to set your input:
;
;	unsigned char rgb[3] = { 255, 0, 0 };	// a red color for example (notice it's 8-bit!!)
;	asm_rgb2yuv_input[0] = rgb[0];
;	asm_rgb2yuv_input[1] = rgb[1];
;	asm_rgb2yuv_input[2] = rgb[2];
;	asm_rgb2yuv();	// do the calculation here
;
; OK after you've done your calculation, the 8-bit YUV results will be written to three global
; variables:
;
;	extern "C" unsigned char asm_rgb2yuv_result_y;
;	extern "C" unsigned char asm_rgb2yuv_result_u;
;	extern "C" unsigned char asm_rgb2yuv_result_v;
;
;	NOTE : these are actually 64-bits in size, but only the first byte matters.
;	Also, you might want to change these to unsigned int for better cpu efficiency.
;	I made them unsigned char to illustrate that they were 8-bit results.
;
;	So to continue my C program example, you might do something like this
;
;	printf("The results of that conversion are Y: %u, U: %u, V: %u\n",
;		asm_rgb2yuv_result_y, asm_rgb2yuv_result_u, asm_rgb2yuv_result_v);
;
;	I hope this all makes sense. Good luck!
;	--Matt
;
;	PS : This is my first attempt at MMX programming so if it is lousy, sorry :)

.486
.MMX

.model flat, c

.DATA

ALIGN 8

the_offset	dq	0000008000000080h
ystuff_q	dq	00000E984B232646h	; 9798, 19235, and 3736 from the equation
ustuff_q	dq	00004000D582EA5Fh	; -5537, -10878, and 16384 from the equation
vstuff_q	dq	0000F598CA5F4000h	; 16384, -13729, and -2664 from the equation

PUBLIC asm_rgb2yuv_input
asm_rgb2yuv_input		dq	0

PUBLIC asm_rgb2yuv_result_y
asm_rgb2yuv_result_y	dq	0

PUBLIC asm_rgb2yuv_result_u
asm_rgb2yuv_result_u	dq	0

PUBLIC asm_rgb2yuv_result_v
asm_rgb2yuv_result_v	dq	0

.CODE

PUBLIC asm_rgb2yuv
asm_rgb2yuv PROC NEAR

	push ebp		; required to preserve ebp, I'm not sure where exactly it gets changed

	;;;;;;;;;;;;;;;;;;;;

	movq mm1, asm_rgb2yuv_input
	movq mm7, the_offset

	; Calculate Y
	movq mm0, ystuff_q
	pmaddwd mm0, mm1
	movq mm2, mm0
	psrlq mm2, 32
	paddd mm0, mm2
	psrad mm0, 15	; divide by 32768
	movq asm_rgb2yuv_result_y, mm0

	; Calculate U
	movq mm0, ustuff_q
	pmaddwd mm0, mm1
	movq mm2, mm0
	psrlq mm2, 32
	paddd mm0, mm2
	psrad mm0, 15	; divide by 32768
	paddd mm0, mm7	; add 128
	movq asm_rgb2yuv_result_u, mm0

	; Calculate V
	movq mm0, vstuff_q
	pmaddwd mm0, mm1
	movq mm2, mm0
	psrlq mm2, 32
	paddd mm0, mm2
	psrad mm0, 15	; divide by 32768
	paddd mm0, mm7	; add 128
	movq asm_rgb2yuv_result_v, mm0

	;;;;;;;;;;;;;;;;;;;;;

	emms

	pop ebp

	ret
asm_rgb2yuv ENDP
END
