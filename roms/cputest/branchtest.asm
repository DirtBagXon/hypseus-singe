; branchtest
; by Matt Ownby (20 Mar 2006)
; To test whether a bug in GCC4 has been fixed or not.

org 100h
	; write a JP 0000 at $0 to prevent endless spammage of text
	ld a, $c3
	ld ($0), a
	ld a, 0
	ld ($1), a
	ld ($2), a

	ld c, 9
	jr lsuccess
	ld de, strFail
	call 5
	jp $0
lsuccess:
	ld de, strSuccess
	call 5
	jp $0	; we're done ...

strFail:
	db "Branch failed, bug in compiler!$"

strSuccess:
	db "Branch succeeded, compiler is working!$"
