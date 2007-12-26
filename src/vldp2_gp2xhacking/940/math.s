
	.align 4
	.globl Divide
	.globl Mod

UnsignedDivide:
	mov r3, #0
	rsbs r2, r0, r1, LSR#3
	bcc div_3bits
	rsbs r2, r0, r1, LSR#8
	bcc div_8bits
	mov r0, r0, LSL#8
	orr r3, r3, #0xFF000000
	rsbs r2, r0, r1, LSR#4
	bcc div_4bits
	rsbs r2, r0, r1, LSR#8
	bcc div_8bits
	mov r0, r0, LSL#8
	orr r3, r3, #0x00FF0000
	rsbs r2, r0, r1, LSR#8
	movcs r0, r0, LSL#8
	orrcs r3, r3, #0x0000FF00
	rsbs r2, r0, r1, LSR#4
	bcc div_4bits
	rsbs r2, r0, #0
	bcs div_by_0
div_loop:
	movcs r0, r0, LSR#8
div_8bits:
	rsbs r2, r0, r1, LSR#7
	subcs r1, r1, r0, LSL#7
	adc r3, r3, r3
	rsbs r2, r0, r1, LSR#6
	subcs r1, r1, r0, LSL#6
	adc r3, r3, r3
	rsbs r2, r0, r1, LSR#5
	subcs r1, r1, r0, LSL#5
	adc r3, r3, r3
	rsbs r2, r0, r1, LSR#4
	subcs r1, r1, r0, LSL#4
	adc r3, r3, r3
div_4bits:
	rsbs r2, r0, r1, LSR#3
	subcs r1, r1, r0, LSL#3
	adc r3, r3, r3
div_3bits:
	rsbs r2, r0, r1, LSR#2
	subcs r1, r1, r0, LSL#2
	adc r3, r3, r3
	rsbs r2, r0, r1, LSR#1
	subcs r1, r1, r0, LSL#1
	adc r3, r3, r3
	rsbs r2, r0, r1
	subcs r1, r1, r0
	adcs r3, r3, r3
div_next:
	bcs div_loop
	mov r0, r3
	mov pc, lr
div_by_0:
	mov r0, #-1
	mov r1, #-1
	mov pc, lr

SignedDivide:
	stmfd sp!, {lr}
	ands r12, r0, #1<<31
	rsbmi r0, r0, #0
	eors r12, r12, r1, ASR#32
	rsbcs r1, r1, #0
	bl UnsignedDivide
	movs r12, r12, LSL#1
	rsbcs r0, r0, #0
	rsbmi r1, r1, #0
	ldmfd sp!, {pc}

Divide:
	stmfd sp!, {lr}
	mov r2, r0
	mov r0, r1
	mov r1, r2
	bl SignedDivide
	ldmfd sp!, {pc}

Mod:
	stmfd sp!, {lr}
	bl Divide
	mov r0, r1
	ldmfd sp!, {pc}
