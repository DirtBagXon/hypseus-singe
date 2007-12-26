; Windows 95/98/ME parallel port output routine
; by Matt Ownby
; For MASM (Microschlop) only
; !!! Not designed to work on Windows NT/2000/XP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; C function prototype is:

;	extern "C" void asm_par_write();

; This function is controlled through global variables
; asm_par_base is the port you want written to
; asm_par_data is the byte you want sent to this port
;
;	extern "C" unsigned short asm_par_base;
;	extern "C" unsigned char asm_par_data;
;

.486

.model flat, c

.DATA

ALIGN 8

PUBLIC asm_par_base
asm_par_base		dw	0

PUBLIC asm_par_data
asm_par_data	db	0

.CODE

PUBLIC asm_par_write
asm_par_write PROC NEAR

	push ebp		; required to preserve ebp, I'm not sure where exactly it gets changed

	;;;;;;;;;;;;;;;;;;;;

	push ax
	push dx
	mov dx,asm_par_base
	mov al,asm_par_data;
	out dx,al
	pop dx
	pop ax

	;;;;;;;;;;;;;;;;;;;;;

	pop ebp

	ret
asm_par_write ENDP
END
