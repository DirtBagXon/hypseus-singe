/*
 * m80_internal.h
 *
 * Copyright (C) 2001,2003 Matt Ownby
 *
 * This file is part of DAPHNE, a laserdisc arcade game emulator
 *
 * DAPHNE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * DAPHNE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#ifndef M80_INTERNAL_H
#define M80_INTERNAL_H

// m80_internal.h
// NOTE : This should not be #included by any other file than m80.cpp

#include <SDL.h>
// SDL.h is used to determine endianness and define some variable types
// No actual SDL functions are used, so you can redefine your own variables
// if you choose.

/* if we are integrating with daphne, define this */
#define INTEGRATE 1

#ifdef INTEGRATE
#include "mamewrap.h"
#include "cpu-debug.h"

#else

#define CLEAR_LINE 0
#define ASSERT_LINE 1

#endif

///////////////////

typedef union
{

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	struct
	{
		Uint8 l,h;
	} b;
#else
	struct
	{
		Uint8 h, l;
	} b;
#endif
	Uint16 w;
} pair;

/////////////////////////

struct m80_context
{
	pair m80_regs[M80_REG_COUNT];	/* for quick access by m80_get_reg */
	Uint8 halted;		/* whether the CPU is halted, waiting for an interrupt */
	Uint8 interrupt_mode;	/* which IM we're in */
	Uint8 IFF1;	/* interrupt flip-flop 1 */
	Uint8 IFF2;	/* interrupt flip-flop 2 */
	Uint8 irq_state;	/* whether IRQ line is asserted or cleared */
	Uint8 nmi_state;	/* whether NMI line is asserted or cleared */
	Uint8 got_EI;
	/* if this flag is true, no interrupts will be issued until after the next instruction */
	/* EI masks all interrupts for the proceeding instruction */
	/* (see Sean Young's undocumented z80 document for explanation of this behavior) */
};

#define C_FLAG	1
#define N_FLAG	2
#define P_FLAG	4
#define V_FLAG	4
#define U3_FLAG	8
#define H_FLAG	16
#define U5_FLAG	32
#define Z_FLAG	64
#define S_FLAG	128

// I _hate_ macros but these just plain make the code look better =)
#define FLAGS	g_context.m80_regs[M80_AF].b.l
#define A		g_context.m80_regs[M80_AF].b.h
#define AF		g_context.m80_regs[M80_AF].w
#define AFPRIME	g_context.m80_regs[M80_AFPRIME].w
#define B		g_context.m80_regs[M80_BC].b.h
#define C		g_context.m80_regs[M80_BC].b.l
#define BC		g_context.m80_regs[M80_BC].w
#define BCPRIME	g_context.m80_regs[M80_BCPRIME].w
#define D		g_context.m80_regs[M80_DE].b.h
#define E		g_context.m80_regs[M80_DE].b.l
#define DE		g_context.m80_regs[M80_DE].w
#define DEPRIME	g_context.m80_regs[M80_DEPRIME].w
#define H		g_context.m80_regs[M80_HL].b.h
#define L		g_context.m80_regs[M80_HL].b.l
#define HL		g_context.m80_regs[M80_HL].w
#define HLPRIME	g_context.m80_regs[M80_HLPRIME].w
#define PC		g_context.m80_regs[M80_PC].w
#define SP		g_context.m80_regs[M80_SP].w
#define R		g_context.m80_regs[M80_RI].b.h
#define I		g_context.m80_regs[M80_RI].b.l
#define IXh	g_context.m80_regs[M80_IX].b.h
#define IXl	g_context.m80_regs[M80_IX].b.l
#define IX	g_context.m80_regs[M80_IX].w
#define IYh	g_context.m80_regs[M80_IY].b.h
#define IYl	g_context.m80_regs[M80_IY].b.l
#define IY	g_context.m80_regs[M80_IY].w

#ifdef CPU_DEBUG
#define M80_ERROR(MESSAGE)	printf(MESSAGE); printf("Opcode %x at PC %x\n", opcode_base[PC-1], PC-1)
#else
#define M80_ERROR(MESSAGE)
#endif

/////////////////////////////

extern struct m80_context g_context;
extern Uint8 *opcode_base;
extern Uint32	g_cycles_executed;

// None of the games need this function call, but cputest does, so we will compile it in in debug mode for that reason
#ifdef CPUTEST
// keeps our emulated hardware advised of our current PC (only updated on branches or calls)
#define M80_CHANGE_PC(newpc) change_pc16(newpc)
#else
// turns out none of the games need this, so we can save a lot of cycles by not using it
#define M80_CHANGE_PC(newpc)
#endif

// returns a byte from memory
#define M80_READ_BYTE(addr)	\
	cpu_readmem16(addr)	\
/*	opcode_base[addr] */

// read Z80 memory into 16-bit z80 register
__inline__ void M80_READ_WORD(unsigned int addr, unsigned int reg_index)
{
	g_context.m80_regs[reg_index].b.l = M80_READ_BYTE(addr);
	g_context.m80_regs[reg_index].b.h = M80_READ_BYTE(addr+1);
}

// writes an 8-bit byte into z80 memory
// addr is where to write, val is which value to write
#define M80_WRITE_BYTE(addr, val)	\
	cpu_writemem16(addr, val)	\
/*	opcode_base[addr] = val */

// write 16-bit z80 reg into Z80 memory
__inline__ void M80_WRITE_WORD(unsigned int addr, unsigned int reg_index)
{
	M80_WRITE_BYTE(addr, g_context.m80_regs[reg_index].b.l);
	M80_WRITE_BYTE(addr+1, g_context.m80_regs[reg_index].b.h);
}

#define M80_GET_ARG opcode_base[PC++]
#define M80_PEEK_ARG opcode_base[PC]

// get a word and advance PC
// Little Endian has a distinct direct advantage to this problem by being
// able to use a single memory read, while
// Big Endian has to use two 8-bit memory reads
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define M80_GET_WORD 	*( (Uint16 *) ( (Uint8 *) (opcode_base + PC))); PC += 2
#else
#define M80_GET_WORD	(opcode_base[PC++] | (opcode_base[PC++] << 8))
#endif

// peek a word but don't advance PC
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define M80_PEEK_WORD	*( (Uint16 *) ( (Uint8 *) (opcode_base + PC)))
#else
#define M80_PEEK_WORD	(opcode_base[PC] | (opcode_base[PC+1] << 8))
#endif

// push macro, NOTE, you have to use the register index, not the value
#define M80_PUSH16(reg_index) SP -= 2; M80_WRITE_WORD(SP, reg_index)

// pop macro, NOTE, you have to use the register index, not the value
#define M80_POP16(reg_index) M80_READ_WORD(SP, reg_index); SP += 2;

// Interrupt check macro
#define CHECK_INTERRUPT	\
	if (g_context.nmi_state == ASSERT_LINE) /* NMI takes priority over IRQ so check it first */	\
	{	\
		m80_activate_nmi();	\
	}	\
	else if (g_context.irq_state == ASSERT_LINE)	\
	{	\
		/* we can only do an IRQ if IFF1 flipflop is set */	\
		if (g_context.IFF1)	\
		{	\
			m80_activate_irq();	\
		}	\
	}

#define M80_EAT_EXTRA_DD_FD	\
/* any extra DD's or FD's in front of a DD/FD instruction is ignored */	\
/* But in order to avoid a recursive loop, we eat up any extras right now */	\
	while ((opcode_base[PC] == 0xDD) || (opcode_base[PC] == 0xFD))	\
	{	\
		PC++;	\
		M80_INC_R;	/* each extra DD or FD costs an R */	\
		g_cycles_executed += op_cycles[0]; /* each extra DD or FD costs a NOP cycle count */	\
	}


// Enable Maskable Interrupt macro
#define M80_EI	\
	/* IFF1 and IFF2 are simultaneously set by EI */	\
	g_context.IFF1 = 1;	\
	g_context.IFF2 = 1;	\
	g_context.got_EI = 1;	/* the next instruction must not be interrupted */

// Disable Maskable Interrupt macro
#define M80_DI	\
	g_context.IFF1 = 0;	\
	g_context.IFF2 = 0;	\
/*	g_context.got_EI = 1; */	/* it's DI, but we still need the same action */

// FIXME: I removed the above g_context.got_EI in order to compare exactly with mame's core
// but I believe mame has a bug in this regard and that the g_context.got_EI should be uncommencted
// once we're done debugging

// Macro to go into HALT mode
#define M80_START_HALT	\
	g_context.halted = 1;	\
	PC--;	\
	/* move PC so it's pointing back to this HALT statement, so next time instruction executes, */	\
	/* it comes to this HALT again */	\
	/* This way we don't have to waste time doing any checking at the beginning */	\
	/* If we didn't just barely get an EI */	\
	/* instruction, then interrupts won't be set this time around and we */	\
	/* can safely use up the rest of the cycles quickly. */	\
	if (!g_context.got_EI)	\
	{	\
		int remaining = g_cycles_to_execute - g_cycles_executed;	\
		/* if we still have cycles that need to be used up */	\
		if (remaining > 0)	\
		{	\
			g_cycles_executed += (((remaining >> 2) + 1) << 2);	\
			/* use up remaining cycles we're committed to, in multiples of 4 */	\
			/* so that we exceed cycles_to_execute */	\
		}	\
	}

// macro to check to see if we're halted and if so, fix the PC and unhalt us
#define M80_STOP_HALT	\
	if (g_context.halted)	\
	{	\
		g_context.halted = 0;	\
		PC++;	\
	}

// workaround gcc4 bug (get the offset in a separate instruction before branching)
__inline__ void M80_BRANCH()
{
	int iOffset = (int) ((Sint8) M80_GET_ARG);
	PC = PC + iOffset;
	M80_CHANGE_PC(PC);
}

__inline__ void M80_BRANCH_COND(unsigned int condition)
{
	if (condition)
	{
		g_cycles_executed += 5; /* 5 extra cycles if branch is taken */
		M80_BRANCH();
	}
	/* else if the branch is not taken, we advance 1 to get to next instruction */
	else { PC++; }
}

// Jump macro
__inline__ void M80_JUMP()
{
	PC = M80_PEEK_WORD;
	M80_CHANGE_PC(PC);
}

// Conditional jump macro that adds the extra cycles
__inline__ void M80_JUMP_COND(unsigned int condition)
{
	if (condition)
	{
		M80_JUMP();
	}
	/* if condition isn't true, we have to advance the PC by 2 to skip over the address */	\
	else
	{
		PC += 2;
	}
}

// Call macro
__inline__ void M80_CALL()
{
		unsigned int destination = M80_GET_WORD;
		M80_PUSH16(M80_PC);
		PC = destination;
		M80_CHANGE_PC(PC);
}

// Conditional call macro that adds the extra cycles
__inline__ void M80_CALL_COND(unsigned int condition)
{
	if (condition)
	{
		g_cycles_executed += 7;	/* always 7 extra cycles for conditional calls */
		M80_CALL();
	}
	/* if condition isn't true, we have to advance the PC by 2 to skip over address */
	else
	{
		PC += 2;
	}
}

// return macro
#define M80_RET	M80_POP16(M80_PC)

// conditional return macro that adds the extra cycles
__inline__ void M80_RET_COND(unsigned int condition)
{
	if (condition)
	{
		g_cycles_executed += 6;	/* always 6 extra cycles for conditional returns */
		M80_RET;
	}
	/* else don't advance PC, it's already in the right spot */
}

// Reset macro
__inline__ void M80_RST(unsigned int addr)
{
	M80_PUSH16(M80_PC);
	PC = addr;
	M80_CHANGE_PC(PC);
}

extern Uint8 m80_inc_flags[];
extern Uint8 m80_szhp_bit_flags[];
extern Uint8 m80_sz53_flags[];

// when incrementing a z80 register, here are the rules
// Carry flag is preserved
// N flag is always cleared
// if old_value & 0xF == 0xF then H flag will be set (half carry)
// if new_value == 0x80 then V flag will be set (2's compliment overflow)
// if new_value & 0x80 then S flag will be set (negative number)
// if new_value == 0 then Z flag will be set (zero flag)
__inline__ void M80_INC_REG8(unsigned char &which_reg)
{
	++which_reg;
	FLAGS = (FLAGS & C_FLAG); /* preserve carry */
	FLAGS |= m80_inc_flags[which_reg];
}

extern Uint8 m80_dec_flags[];

// when decrementing a z80 register, here are the rules
// Carry flag is preserved
// N flag is always set
// if old_value & 0xF == 0x00 then H flag will be set (half carry)
// if new_value == 0x7F then V flag will be set (2's compliment overflow)
// if new_value & 0x80 then S flag will be set (negative number)
// if new_value == 0 then Z flag will be set (zero flag)	
__inline__ void M80_DEC_REG8(unsigned char &which_reg)
{
	--which_reg;
	FLAGS = (FLAGS & C_FLAG);
	FLAGS |= m80_dec_flags[which_reg];
}

// macro to increment R
// this is the same as a regular increment except bit 7 never changes
#define M80_INC_R	\
	R = (R + 1) & 0x7F | (R & 0x80)

// Rotate Left Accumulator - flags are different from RL A
__inline__ void M80_RLA()
{
	/* use a temp variable greater than 8 bits so we can find out if there was a carry */
	unsigned int result = ((A << 1) | (FLAGS & C_FLAG));
	FLAGS &= (S_FLAG | Z_FLAG | P_FLAG);
	FLAGS |= (result & (U5_FLAG | U3_FLAG));
	FLAGS |= (result >> 8);	/* set carry flag if bit 8 of result is set */
	A = (Uint8) result;
}

extern Uint8 m80_sz53p_flags[];

// Rotate Left - Hi bit goes into carry, carry goes into bit 0
__inline__ void M80_RL(unsigned char &which_reg)
	{
		Uint32 temp = (which_reg << 1) | (FLAGS & C_FLAG);
		which_reg = (Uint8) temp;
		FLAGS = m80_sz53p_flags[which_reg];	/* set carry to 0 and get S, Z and P flags */
		FLAGS |= (temp >> 8); /* set carry to the value of the hi bit */
	}

/* does an RL on (IXIY + d) and on a register (undocumented instruction) */
__inline__ void M80_RL_COMBO(unsigned char &reg, unsigned int addr)	\
{	\
	Uint8 val = M80_READ_BYTE(addr);	\
	M80_RL(val);	\
	M80_WRITE_BYTE(addr, val);	\
	reg = val;	\
}

// Rotate Right Accumulator - flags are different from RR A
__inline__ void M80_RRA()
{
	Uint8 result = (A >> 1) | (FLAGS << 7);
	FLAGS &= (S_FLAG | Z_FLAG | P_FLAG);
	FLAGS |= (A & 1);
	FLAGS |= (result & (U5_FLAG | U3_FLAG));
	A = (Uint8) result;
}

// Rotate Right - Bit 0 goes into carry, carry goes into Hi bit
__inline__ void M80_RR(Uint8 &which_reg)
	{
		Uint8 new_carry = which_reg & C_FLAG;
		which_reg = (which_reg >> 1) | (FLAGS << 7);
		FLAGS = m80_sz53p_flags[which_reg];	/* set carry to 0 and get S, Z and P flags */
		FLAGS |= new_carry;
	}

/* does an RR on (IXIY + d) and on a register (undocumented instruction) */
__inline__ void M80_RR_COMBO(Uint8 &reg, unsigned int addr)
{
	Uint8 val = M80_READ_BYTE(addr);
	M80_RR(val);
	M80_WRITE_BYTE(addr, val);
	reg = val;
}

// Rotate Left Accumulator - Flags are different from the RLC A instruction
__inline__ void M80_RLCA()
{
	A = (A >> 7) | (A << 1);
	FLAGS &= (S_FLAG | Z_FLAG | P_FLAG);	/* preserve S, Z and P flags, clear H and N flags */
	FLAGS |= (A & (U5_FLAG | U3_FLAG | C_FLAG));	/* carry contains the the previous hi-bit which is now bit 0 */
}

// Rotate Left Circular - Carry gets hi-bit but is otherwise irrelevant to this operation
__inline__ void M80_RLC(Uint8 &src_reg)
{
	FLAGS = src_reg >> 7; /* put hi-bit into carry flag */
	src_reg = ((src_reg << 1) | FLAGS); /* and also into bit 0 of new value */
	FLAGS |= m80_sz53p_flags[src_reg];
}

/* does an RLC on (IXIY + d) and on a register (undocumented instruction) */
__inline__ void M80_RLC_COMBO(Uint8 &reg, unsigned int addr)
{
	Uint8 val = M80_READ_BYTE(addr);
	M80_RLC(val);
	M80_WRITE_BYTE(addr, val);
	reg = val;
}

// macro for RRCA - flags are different from RRC A
__inline__ void M80_RRCA()
{
	FLAGS &= (S_FLAG | Z_FLAG | P_FLAG);
	FLAGS |= (A & C_FLAG);
	A = (A << 7) | (A >> 1);
	FLAGS |= (Uint8) (A & (U5_FLAG | U3_FLAG));
}

// Rotate Right Circular - Carry gets low-bit but is otherwise irrelevant to this operation
__inline__ void M80_RRC(Uint8 &src_reg)
{
	FLAGS = src_reg & C_FLAG;	/* put low-bit into carry flag */
	src_reg = ((src_reg >> 1) | (src_reg << 7)); /* low-bit becomes hi-bit */
	FLAGS |= m80_sz53p_flags[src_reg];
}

/* does an RRC on (IXIY + d) and on a register (undocumented instruction) */
__inline__ void M80_RRC_COMBO(Uint8 &reg, unsigned int addr)	\
{	\
	Uint8 val = M80_READ_BYTE(addr);	\
	M80_RRC(val);	\
	M80_WRITE_BYTE(addr, val);	\
	reg = val;	\
}

// Shift Left Arithmetic/Logical - Carry gets hi-bit, a 0 gets put in at bit 0
__inline__ void M80_SLA(Uint8 &reg)
{
	FLAGS = reg >> 7;	/* get value for new carry flag */
	reg = reg << 1;
	FLAGS |= m80_sz53p_flags[reg];
}

/* does an SLA on (IXIY + d) and on a register (undocumented instruction) */
__inline__ void M80_SLA_COMBO(Uint8 &reg, unsigned int addr)
{
	Uint8 val = M80_READ_BYTE(addr);
	M80_SLA(val);
	M80_WRITE_BYTE(addr, val);
	reg = val;
}

// Shift left inverted arithmetic - Carry gets hi-bit, a 1 gets put in at bit 0
__inline__ void M80_SLIA(Uint8 &reg)
{
	FLAGS = reg >> 7;	/* get value for new carry */
	reg = (reg << 1) | 1;
	FLAGS |= m80_sz53p_flags[reg];
}

/* does an SLIA on (IXIY + d) and on a register (undocumented instruction) */
__inline__ void M80_SLIA_COMBO(Uint8 &reg, unsigned int addr)
{
	Uint8 val = M80_READ_BYTE(addr);
	M80_SLIA(val);
	M80_WRITE_BYTE(addr, val);
	reg = val;
}

// Shift Right Arithmetic (preserves sign) - Carry gets bit 0, the old bit 7 goes to the new bit 7
__inline__ void M80_SRA(Uint8 &reg)
{
	FLAGS = reg & C_FLAG;	/* put low bit into carry */
	reg = (reg >> 1) | (reg & 0x80);	/* shift and preserve hi bit */
	FLAGS |= m80_sz53p_flags[reg];
}

__inline__ void M80_SRA_COMBO(Uint8 &reg, unsigned int addr)
{
	Uint8 val = M80_READ_BYTE(addr);
	M80_SRA(val);
	M80_WRITE_BYTE(addr, val);
	reg = val;
}

// shift right logical - Carry gets bit 0, bit 7 gets 0
__inline__ void M80_SRL(Uint8 &reg)
{
	FLAGS = reg & C_FLAG;
	reg = reg >> 1;
	FLAGS |= m80_sz53p_flags[reg];
}

/* does an SRL on (IXIY + d) and on a register (undocumented instruction) */
__inline__ void M80_SRL_COMBO(Uint8 &reg, unsigned int addr)
{
	Uint8 val = M80_READ_BYTE(addr);
	M80_SRL(val);
	M80_WRITE_BYTE(addr, val);
	reg = val;
}

// tests the status of a bit in the register
__inline__ void M80_BIT(unsigned int bit, Uint8 reg)
{
	FLAGS = (FLAGS & C_FLAG); /* preserve C flag, clear N flag */
	FLAGS |= m80_szhp_bit_flags[(1 << bit) & reg];	/* update the SZHP flags based on result */
	FLAGS |= (reg & 0x28);	/* set 3/5 flags based on value of register */
	/* (using YAZE as authoritative since there is some dispute about how BIT works */
}

// tests the status of a bit in the register
// clears bits 3 and 5 (for the BIT x, (HL) instructions)
__inline__ void M80_BIT_NO35(unsigned int bit, Uint8 reg)
{
//	FLAGS = (FLAGS & C_FLAG); /* preserve C flag, clear N, 5, 3 flags */
//	FLAGS |= m80_szhp_bit_flags[(1 << bit) & reg];	/* update the SZHP flags based on result */
 
	FLAGS = (FLAGS & C_FLAG) /* preserve C flag, clear N, 5, 3 flags */
		| m80_szhp_bit_flags[(1 << bit) & reg];	/* update the SZHP flags based on result */
}

// NOTE : this function is apparently not used
#define M80_BIT_OP(opcode, bit, reg)	\
	if (reg & (1 << bit)) AF = (AF & ~0xfe) | H_FLAG | (((opcode & 0x38) == 0x38) << 7);	\
	else AF = (AF & ~0xfe) | 0x54;	/* Z, H, and P flag */	\
	if ((opcode & 7) != 6) AF |= (reg & 0x28); /* 5 and 3 flags */

// clear a bit in the register
#define M80_RES(bit, reg)	reg &= ~(1 << bit)

// a combo version of the reset instruction, for IXIY + d
__inline__ void M80_RES_COMBO(unsigned int bit, Uint8 &reg, unsigned int addr)
{
	Uint8 val = M80_READ_BYTE(addr);
	M80_RES(bit, val);
	M80_WRITE_BYTE(addr, val);
	reg = val;
}

// set a bit in the register
#define M80_SET(bit, reg)	reg |= (1 << bit)

/* an IXIY combo version of the set instruction */
__inline__ void M80_SET_COMBO(unsigned int bit, Uint8 &reg, unsigned int addr)
{
	Uint8 val = M80_READ_BYTE(addr);
	M80_SET(bit, val);
	M80_WRITE_BYTE(addr, val);
	reg = val;
}

// macros for 16-bit register addition

#ifdef GCC_X86_ASM
#define M80_ADD_REGS16(dest_reg, source_reg)	\
	__asm__ __volatile__(	\
	"addb	%%bl, %%cl\n\t"	/* the actual addition is here */ \
	/* pipeline stall, can anyone fix? */	\
	"adcb	%%bh, %%ch\n\t"	/* 16-bit add does NOT work because Aux Carry (HC) flag is wrong */ \
	"lahf	\n\t"	/* put flags into ah register */	\
	"movb	%%ch, %%bh\n\t"	/* prepare to get flags 3/5 (we can't modify %ch) */	\
	"andb	$0x11, %%ah\n\t"	/* isolate carry and auxiliary carry flags */	\
	"andb	$0x28, %%bh\n\t"	/* get bits 3 and 5 from upper byte ot result */	\
	"andb	$196, %%al\n\t"	/* preserve S, Z, and V flags */	\
	"orb	%%ah, %%al\n\t"	/* put carry and auxiliary carry into Z80 flags */	\
	"orb	%%bh, %%al\n\t"	/* adds bits 3 and 5 to the FLAGS register */	\
	:"=c" (dest_reg), "=a" (FLAGS)	\
	:"c" (dest_reg), "a" (FLAGS), "b" (source_reg)	\
	);
#else
__inline__ void M80_ADD_REGS16(Uint16 &dest_reg, Uint16 source_reg)
{
	unsigned int result = dest_reg + source_reg;
	unsigned int cbits = (dest_reg ^ source_reg ^ result) >> 8;
	dest_reg = (Uint16) result;
	AF = (AF & (0xFF00 | S_FLAG | Z_FLAG | V_FLAG)) |
		((result >> 8) & (U3_FLAG | U5_FLAG)) |
		(cbits & H_FLAG) |
		((cbits >> 8) & C_FLAG);
}
#endif

#ifdef GCC_X86_ASM
// We unfortunately could not use a 16-bit subtraction because the x86 Aux Carry flag would
// be different from the z80's.  However, with 8-bit, it is the same.  This subtraction is
// further complicated because we have to change the Z flag, which means we have to AND
// the Z flag result from both 8-bit addtions.
// This assembly is optimized for pipe-lining.  See if you can do better (the last 3 lines will stall)
#define M80_SBC_REGS16(dest_reg, source_reg)	\
	__asm__ __volatile__(	\
	"shrb	$1, %%al\n\t"	/* put z80 carry flag into x86 carry flag, we don't need to preserve flags */	\
	"sbbb	%%bl, %%cl\n\t"	/* subtract lower bytes */ \
	"lahf	\n\t"	/* we need to capture the Z flag from the lower result */	\
	"sbbb	%%bh, %%ch\n\t"	/* subtract upper bytes */ \
	"movb	%%ah, %%bl\n\t"	/* store lower flags into bl (which we can safely clobber) */	\
	"lahf	\n\t"	/* put upper x86 flags into ah register */	\
	"movb	%%ch, %%bh\n\t"	/* prepare to get 3/5 flags, we can't modify %ch */	\
	"setob	%%al\n\t"	/* if overflow is true, set it (we're starting new flags here) */	\
	"andb	$0x40, %%bl\n\t"	/* isolate Z flag from lower operation */	\
	"shlb	$2, %%al\n\t"	/* shift overflow flag to its proper spot */	\
	"orb	$0xBF, %%bl\n\t"	/* set all the other lower bits to prepare for an AND merge with the upper */	\
	"andb	$0xD1, %%ah\n\t"	/* isolate S/Z/H and C x86 flags from upper operation */	\
	"andb	$0x28, %%bh\n\t"	/* get bits 3 and 5 from upper byte ot result */	\
	"orb	$2, %%al\n\t"	/* set N flag, which is always set in subtraction */	\
	"andb	%%ah, %%bl\n\t"	/* merge lower bits and upper, taking the AND of the Z flag */	\
	"orb	%%bh, %%al\n\t"	/* adds bits 3 and 5 to the FLAGS register */	\
	"orb	%%bl, %%al\n\t"	/* merge bits and V_FLAG */	\
	:"=c" (dest_reg), "=a" (FLAGS)	\
	:"c" (dest_reg), "a" (FLAGS), "b" (source_reg)	\
	);
#else
// Subtract with carry for two 16-bit registers (dest_reg = dest_reg - source-reg - Carry)
__inline__ void M80_SBC_REGS16(Uint16 &dest_reg, Uint16 source_reg)
{
	Uint32 result = dest_reg - source_reg - (AF & C_FLAG);
	Uint32 cbits = (dest_reg ^ source_reg ^ result) >> 8;
	dest_reg = result;
	AF = (AF & ~0xff) | ((result >> 8) & 0xa8) |
		(((result & 0xffff) == 0) << 6) |
		(((cbits >> 6) ^ (cbits >> 5)) & 4) |
		(cbits & 0x10) | 2 | ((cbits >> 8) & 1);
}
#endif

// Add with carry for two 16-bit registers (dest_reg = dest_reg + source_reg + Carry)
#ifdef GCC_X86_ASM
// We unfortunately could not use a 16-bit addition because the x86 Aux Carry flag would
// be different from the z80's.  However, with 8-bit, it is the same.  This addition is
// further complicated because we have to change the Z flag, which means we have to AND
// the Z flag result from both 8-bit addtions.
#define M80_ADC_REGS16(dest_reg, source_reg)	\
	__asm__ __volatile__(	\
	"shrb	$1, %%al\n\t"	/* put z80 carry flag into x86 carry flag, we don't need to preserve flags */	\
	"adcb	%%bl, %%cl\n\t"	/* add lower bytes */ \
	"lahf	\n\t"	/* we need to capture the Z flag from the lower result */	\
	"adcb	%%bh, %%ch\n\t"	/* add upper bytes */ \
	"movb	%%ah, %%bl\n\t"	/* store lower flags into bl (which we can safely clobber) */	\
	"lahf	\n\t"	/* put upper x86 flags into ah register */	\
	"movb	%%ch, %%bh\n\t"	/* prepare to get 3/5 flags, we can't modify %ch */	\
	"setob	%%al\n\t"	/* if overflow is true, set it (we're starting new flags here) */	\
	"andb	$0x40, %%bl\n\t"	/* isolate Z flag from lower operation */	\
	"shlb	$2, %%al\n\t"	/* shift overflow flag to its proper spot */	\
	"orb	$0xBF, %%bl\n\t"	/* set all the other lower bits to prepare for an AND merge with the upper */	\
	"andb	$0xD1, %%ah\n\t"	/* isolate S/Z/H and C x86 flags from upper operation */	\
	"andb	$0x28, %%bh\n\t"	/* get bits 3 and 5 from upper byte ot result */	\
	"andb	%%ah, %%bl\n\t"	/* merge lower bits and upper, taking the AND of the Z flag */	\
	"orb	%%bh, %%al\n\t"	/* adds bits 3 and 5 to the FLAGS register */	\
	"orb	%%bl, %%al\n\t"	/* merge bits and V_FLAG */	\
	:"=c" (dest_reg), "=a" (FLAGS)	\
	:"c" (dest_reg), "a" (FLAGS), "b" (source_reg)	\
	);
#else
__inline__ void M80_ADC_REGS16(Uint16 &dest_reg, Uint16 source_reg)
{
	Uint32 result = dest_reg + source_reg + (AF & C_FLAG);
	Uint32 cbits = (dest_reg ^ source_reg ^ result) >> 8;
	dest_reg = result;
	AF = (AF & ~0xff) | ((result >> 8) & 0xa8) |
		(((result & 0xffff) == 0) << 6) |	/* Z_FLAG */
		(((cbits >> 6) ^ (cbits >> 5)) & 4) |
		(cbits & 0x10) | ((cbits >> 8) & 1);
}
#endif

#define X	printf("Instruction executed that is not implemented yet!\n")

#ifdef GCC_X86_ASM
// NOTE: I optimized this assembly for pipe-lining.  see if you can improve it!
#define M80_ADD_TO_A(which_reg)	\
	__asm__ __volatile__ (	\
	"addb %2,%0	\n\t" /* do the addition */	\
	"lahf	\n\t" /* put intel flags (which are almost the same as z80) into ah */	\
	"setob %1	\n\t" /* set flags reg to 1 if intel overflow flag is set */	\
	"andb $0xd1,%%ah	\n\t" /* strip out non-common intel flags, preserve S,Z,HC, and C */ \
	"shlb $2,%1 	\n\t"	/* shift left twice so overflow flag is in correct position (1 cycle on Pentium) */	\
	"movb %0,%%al	\n\t" /* move addition result into %%al so we can get X/Y z80 flags */	\
	/* stall */	\
	"orb %%ah,%1	\n\t" /* merge overflow bit with the rest of the intel flags */	\
	/* stall */ \
	"andb $0x28,%%al	\n\t" /* isolate X/Y flags */	\
	/* stall */ \
	"orb %%al,%1	\n\t" /* merge them in with the other flags */	\
	:"=r" (A), "=r" (FLAGS)	\
	:"q" (which_reg), "0" (A), "1" (FLAGS)	\
	: "cc", "ah", "al"	\
	);
#else
__inline__ void M80_ADD_TO_A(Uint8 which_reg)
{
	Uint32 sum = which_reg + A;
		/* Overflow can only occur in addition if the signs of the operands are the same */
		/* If new A's sign is different from old A's, and if the signs of the operands are the same, set V */
	FLAGS = ((((sum ^ A) & (which_reg ^ A ^ 0x80)) >> 5) & V_FLAG);	/* clear N flag */
	FLAGS |= ((sum ^ A ^ which_reg) & H_FLAG);	/* H_FLAG is set if 1 or all of the bit 4's are set */
	FLAGS |= ((sum >> 8) & C_FLAG); /* if bit 8 of sum is set, set carry flag */
	A = (Uint8) sum;
	FLAGS |= m80_sz53_flags[A];	/* set S, Z, 5 and 3 flags */
}
#endif


#ifdef GCC_X86_ASM
// optimized for pipelining, see if you can improve it!
#define M80_ADC_TO_A(which_reg)	\
	__asm__ __volatile__ (	\
	"shrb  $1,%1	\n\t"	/* copy Z80 carry flag into Intel carry flag */	\
	"adcb %2,%0	\n\t" /* do the addition, including the carry */	\
	"lahf	\n\t" /* put intel flags (which are almost the same as z80) into ah */	\
	"setob %1	\n\t" /* set flags reg to 1 if intel overflow flag is set */	\
	"andb $0xd1,%%ah	\n\t" /* strip out non-common intel flags, preserve S,Z,HC, and C */ \
	"shlb $2,%1 	\n\t"	/* shift left twice so overflow flag is in correct position (1 cycle on Pentium) */	\
	"movb %0,%%al	\n\t" /* move addition result into %%al so we can get X/Y z80 flags */	\
	/* stall */	\
	"orb %%ah,%1	\n\t" /* merge overflow bit with the rest of the intel flags */	\
	/* stall */	\
	"andb $0x28,%%al	\n\t" /* isolate X/Y flags */	\
	/* stall */	\
	"orb %%al,%1	\n\t" /* merge them in with the other flags */	\
	:"=r" (A), "=r" (FLAGS)	\
	:"q" (which_reg), "0" (A), "1" (FLAGS)	\
	: "cc", "ah", "al" \
	);
#else
__inline__ void M80_ADC_TO_A(Uint8 which_reg)
{
	Uint32 sum = which_reg + A + (FLAGS & C_FLAG);
		/* Overflow can only occur in addition if the signs of the operands are the same */
		/* If new A's sign is different from old A's, and if the signs of the operands are the same, set V */
	FLAGS = ((((sum ^ A) & (which_reg ^ A ^ 0x80)) >> 5) & V_FLAG);	/* clear N flag */
	FLAGS |= ((sum ^ A ^ which_reg) & H_FLAG);	/* H_FLAG is set if 1 or all of the bit 4's are set */
	FLAGS |= ((sum >> 8) & C_FLAG); /* if bit 8 of sum is set, set carry flag */
	A = (Uint8) sum;
	FLAGS |= m80_sz53_flags[A];	/* set S, Z, Y and X flags */
}
#endif


#ifdef GCC_X86_ASM
#define M80_SUB_FROM_A(which_reg)	\
	__asm__ __volatile__ (	\
	"subb %2,%1	\n\t" /* do the subtraction (1 cycle) */	\
	"lahf	\n\t" /* put intel flags (which are almost the same as z80) into ah (2 cycles) */	\
	"setob %0	\n\t" /* set flags reg to 1 if intel overflow flag is set (1 or 2 cycles) */	\
	"andb $0xd1,%%ah	\n\t" /* strip out non-common intel flags (1 cycle) */ \
	"shlb $2,%0 	\n\t"	/* shift left twice so overflow flag is in correct position (1 cycle on Pentium) */	\
	"movb %1,%%al	\n\t" /* move subtraction result into available reg for X/Y z80 flags (1 cycle) */	\
	"orb $2, %0	\n\t" /* set N_FLAG in the flags register (1 cycle) */	\
	"andb $0x28,%%al	\n\t" /* isolate X/Y flags (1 cycle) */	\
	/* stall */	\
	"orb %%ah,%0	\n\t" /* merge overflow bit with the rest of the intel flags (1 cycle) */	\
	/* stall */	\
	"orb %%al,%0	\n\t" /* merge them in with the other flags (1 cycle) */	\
	:"=r" (FLAGS), "=r" (A)	\
	:"q" (which_reg), "0" (FLAGS), "1" (A)	\
	: "cc", "ah", "al" \
	);
#else
__inline__ void M80_SUB_FROM_A(Uint8 which_reg)
{
	Uint32 diff = A - which_reg;
	FLAGS = N_FLAG;	/* N_FLAG always gets set when subtracting */
		/* Overflow can only occur in subtraction if the signs of the operands are different */
		/* If new A's sign is different from old A's, and if the signs of the operands are different, set V */
	FLAGS |= ((((diff ^ A) & (which_reg ^ A)) >> 5) & V_FLAG);
	FLAGS |= ((diff ^ A ^ which_reg) & H_FLAG);	/* H_FLAG is set if 1 or all of the bit 4's are set */
	FLAGS |= ((diff >> 8) & C_FLAG); /* if bit 8 of diff is set, set carry flag */
	A = (Uint8) diff;
	FLAGS |= m80_sz53_flags[A];	/* set S, Z, Y and X flags */
}
#endif


#ifdef GCC_X86_ASM
#define M80_SBC_FROM_A(which_reg)	\
	__asm__ __volatile__ (	\
	"shrb $1,%0	\n\t" /* copy z80 carry flag into intel carry flag */	\
	"sbbb %2,%1	\n\t" /* do the subtraction w/ carry (1 cycle) */	\
	"lahf	\n\t" /* put intel flags (which are almost the same as z80) into ah (2 cycles) */	\
	"setob %0	\n\t" /* set flags reg to 1 if intel overflow flag is set (1 or 2 cycles) */	\
	"andb $0xd1,%%ah	\n\t" /* strip out non-common intel flags (1 cycle) */ \
	"shlb $2,%0 	\n\t"	/* shift left twice so overflow flag is in correct position (1 cycle on Pentium) */	\
	"movb %1,%%al	\n\t" /* move subtraction result into available reg for X/Y z80 flags (1 cycle) */	\
	"orb $2, %0	\n\t" /* set N_FLAG in the flags register (1 cycle) */	\
	"andb $0x28,%%al	\n\t" /* isolate X/Y flags (1 cycle) */	\
	/* stall */	\
	"orb %%ah,%0	\n\t" /* merge overflow bit with the rest of the intel flags (1 cycle) */	\
	/* stall */	\
	"orb %%al,%0	\n\t" /* merge them in with the other flags (1 cycle) */	\
	:"=r" (FLAGS), "=r" (A)	\
	:"q" (which_reg), "0" (FLAGS), "1" (A)	\
	: "cc", "ah", "al"	\
	);
#else
__inline__ void M80_SBC_FROM_A(Uint8 which_reg)
{
	unsigned int diff = A - which_reg - (FLAGS & C_FLAG);
	FLAGS = N_FLAG;	/* N_FLAG always gets set when subtracting */
		/* Overflow can only occur in subtraction if the signs of the operands are different */
		/* If new A's sign is different from old A's, and if the signs of the operands are different, set V */
	FLAGS |= ((((diff ^ A) & (which_reg ^ A)) >> 5) & V_FLAG);
	FLAGS |= ((diff ^ A ^ which_reg) & H_FLAG);	/* H_FLAG is set if 1 or all of the bit 4's are set */
	FLAGS |= ((diff >> 8) & C_FLAG); /* if bit 8 of diff is set, set carry flag */
	A = (Uint8) diff;
	FLAGS |= m80_sz53_flags[A];	/* set S, Z, 5 and 3 flags */
}
#endif

#ifdef GCC_X86_ASM
#define M80_COMPARE_WITH_A(which_reg)	\
	__asm__ __volatile__ (	\
	"cmpb %1,%2	\n\t" /* do the comparison */	\
	"lahf	\n\t" /* put intel flags (which are almost the same as z80) into ah (2 cycles) */	\
	"setob %0	\n\t" /* set flags reg to 1 if intel overflow flag is set (1 or 2 cycles) */	\
	"andb $0xd1,%%ah	\n\t" /* strip out non-common intel flags (1 cycle) */ \
	"shlb $2,%0 	\n\t"	/* shift left twice so overflow flag is in correct position (1 cycle on Pentium) */	\
	"andb $0x28,%1	\n\t" /* isolate X/Y flags from operand (1 cycle) */	\
	"orb $2, %0	\n\t" /* set N_FLAG in the flags register (1 cycle) */	\
	/* stall */	\
	"orb %%ah,%0	\n\t" /* merge overflow bit with the rest of the intel flags (1 cycle) */	\
	/* stall */	\
	"orb %1,%0	\n\t" /* merge them in with the other flags (1 cycle) */	\
	: "=r" (FLAGS)	\
	: "r" (which_reg), "r" (A), "0" (FLAGS)	\
	: "%ah"	\
	);
#else
__inline__ void M80_COMPARE_WITH_A(Uint8 which_reg)
{
	unsigned int sum = A - which_reg;
	unsigned int cbits = A ^ which_reg ^ sum;
		AF = (AF & ~0xff) | (sum & 0x80) |
			(((sum & 0xff) == 0) << 6) | (which_reg & 0x28) |
			(((cbits >> 6) ^ (cbits >> 5)) & 4) | 2 |
			(cbits & 0x10) | ((cbits >> 8) & 1);
}
#endif

// FIXME: I think the above COMPARE is broken.  According to Sean Young, flags 5 and 3 are
// from the operand, not the result.  Of course this is very minor, but it's still a bug that needs fixing.

__inline__ void M80_AND_WITH_A(Uint8 which_reg)
{
	A &= which_reg;
	/* clear N and C, set H, and fill in S, Z, and P appropriately */
	FLAGS = m80_sz53p_flags[A] | H_FLAG;
}

__inline__ void M80_XOR_WITH_A(Uint8 which_reg)
{
	A ^= which_reg;
	/* clear N,C, and H, and fill in S, Z, and P appropriately */
	FLAGS = m80_sz53p_flags[A];
}

__inline__ void M80_OR_WITH_A(Uint8 which_reg)
{
	A |= which_reg;
	/* clear N,C, and H, and fill in S, Z, and P appropriately */
	FLAGS = m80_sz53p_flags[A];
}

// macro to swap AF with AF'
__inline__ void M80_EX_AFS()
{
	Uint16 tmp = AFPRIME;
	AFPRIME = AF;
	AF = tmp;
}

// macro to swap HL and DE
__inline__ void M80_EX_DEHL()
{
	Uint16 temp;
	temp = HL;
	HL = DE;
	DE = temp;
}

// compliment A
__inline__ void M80_CPL()
{
	A = (Uint8) (A ^ 0xFF);
	FLAGS &= (Uint8) (S_FLAG | Z_FLAG | P_FLAG | C_FLAG );
	FLAGS |= (Uint8) (H_FLAG | N_FLAG);
	FLAGS |= (Uint8) (A & (U5_FLAG | U3_FLAG));
	/* see Sean Young's z80 doc for more info about how CPL behaves */
}

// set carry flag
__inline__ void M80_SCF()
{
	FLAGS &= (Uint8) (S_FLAG | Z_FLAG | P_FLAG);
	FLAGS |= C_FLAG;
	FLAGS |= (Uint8) (A & (U5_FLAG | U3_FLAG));
}

// compliment carry flag
__inline__ void M80_CCF()
{
	FLAGS &= (S_FLAG | Z_FLAG | P_FLAG | C_FLAG);
	FLAGS |= (FLAGS & C_FLAG) << 4;	/* H flag gets the old value of C */ 
	FLAGS ^= C_FLAG;
	FLAGS |= (A & (U5_FLAG | U3_FLAG));
}

// Exchange most of the registers (EXX)
__inline__ void M80_EXX()
{
	Uint16 temp = BC;
	BC = BCPRIME;
	BCPRIME = temp;
	temp = DE;
	DE = DEPRIME;
	DEPRIME = temp;
	temp = HL;
	HL = HLPRIME;
	HLPRIME = temp;
}

extern Uint16 daa_table[];

// performs fast DAA using a lookup table
// Thx to Sean young for the idea and Juergen Buchmueller for the implementation :)
__inline__ void M80_DAA()
{
	int index = A;
	if ( FLAGS & C_FLAG )
	{
		index |= 0x100;
	}
	if (FLAGS & H_FLAG)
	{
		index |= 0x200;
	}
	if (FLAGS & N_FLAG)
	{
		index |= 0x400;
	}
	AF = daa_table[index];
}


// writes A into a port.  We add A*256 to the port according to Sean Young's document
#define M80_OUT_A(port)	\
	cpu_writeport16(port | (A << 8), A)

// outputs a register to a port
#define M80_OUT16(port, value)	\
	cpu_writeport16(port, value)

// inputs from a port (8-bits) into A (no flags are affected by this)
// adds A*256 to the 8-bit port according to Sean Young's document
#define M80_IN_A(port)	\
	A = cpu_readport16(port | (A << 8))

// inputs from a port (16-bits) into a register
__inline__ void M80_IN16(Uint8 port, Uint8 &reg)
{
	reg = cpu_readport16(port);
	FLAGS = FLAGS & C_FLAG;	/* preserve only carry */
	FLAGS |= m80_sz53p_flags[reg];
}

// block instructions here

// some of the the repeating block instructions have this code in common
#define M80_BLOCK_REPEAT(base)	\
	base;	\
	if (BC)	\
	{	\
		PC -= 2;	\
		g_cycles_executed += 5;	\
	}

// repeat until BC is 0 OR until A = (HL)  (which is true when Z flag is set)
#define M80_BLOCK2_REPEAT(base)	\
	base;	\
	if ((BC != 0) && !(FLAGS & Z_FLAG))	\
	{	\
		PC -= 2;	\
		g_cycles_executed += 5;	\
	}

// some of the the repeating block instructions have this code in common
#define M80_BLOCK3_REPEAT(base)	\
	base;	\
	if (B)	\
	{	\
		PC -= 2;	\
		g_cycles_executed += 5;	\
	}

__inline__ void M80_LDI()
{
		Uint8 temp = M80_READ_BYTE(HL);
		M80_WRITE_BYTE(DE, temp);
		DE++;
		HL++;
		BC--;
		FLAGS &= S_FLAG | Z_FLAG | C_FLAG; /* preserve s, z, c, clear h and c */
		/* if BC is not 0, set P flag */
		if (BC)
		{
			FLAGS |= P_FLAG;
		}
		FLAGS |= ((A + temp) & 2) << 4;	/* U5_FLAG is bit 1 of A+(HL) */
		FLAGS |= ((A + temp) & U3_FLAG);	/* U3_FLAG is bit 3 of A+(HL) */
}

// macro for the CPI instruction
__inline__ void M80_CPI()
	{
		Uint32 temp = M80_READ_BYTE(HL);
		Uint32 cbits, sum;
		++HL;
		sum = A - temp;
		cbits = A ^ temp ^ sum;
		AF = (AF & ~0xfe) | (sum & 0x80) | (!(sum & 0xff) << 6) |
			(((sum - ((cbits&16)>>4))&2) << 4) | (cbits & 16) |
			((sum - ((cbits >> 4) & 1)) & 8) |
			((--BC & 0xffff) != 0) << 2 | 2;
		if ((sum & 15) == 8 && (cbits & 16) != 0)
			AF &= ~8;
	}

/*
#define M80_CPIR	\
{	\
	Uint32 acu = A;	\
	Uint32 temp, op, sum, cbits;	\
	BC &= 0xffff;	\
	do {	\
		temp = M80_READ_BYTE(HL); ++HL;	\
		op = --BC != 0;	\
		sum = acu - temp;	\
	} while (op && sum != 0);	\
	cbits = acu ^ temp ^ sum;	\
	AF = (AF & ~0xfe) | (sum & 0x80) | (!(sum & 0xff) << 6) |	\
		(((sum - ((cbits&16)>>4))&2) << 4) |	\
		(cbits & 16) | ((sum - ((cbits >> 4) & 1)) & 8) |	\
		op << 2 | 2;	\
	if ((sum & 15) == 8 && (cbits & 16) != 0)	\
		AF &= ~8;	\
}
*/

__inline__ void M80_CPD()
{
	Uint8 val = M80_READ_BYTE(HL);
	Uint8 res = A - val;
	HL--; BC--;
	FLAGS = (FLAGS & C_FLAG) | (m80_sz53_flags[res] & ~(U5_FLAG|U3_FLAG))
		| ((A ^ val ^ res) & H_FLAG) | N_FLAG; 
	if( FLAGS & H_FLAG ) res -= 1;
	if( res & 0x02 ) FLAGS |= U5_FLAG; /* bit 1 -> flag 5 */
	if( res & 0x08 ) FLAGS |= U3_FLAG; /* bit 3 -> flag 3 */
	if( BC ) FLAGS |= V_FLAG;
}

/*
#define M80_CPDR	\
{	\
	Uint32 temp;	\
	Uint32 op;	\
	Uint32 sum;	\
	Uint32 cbits;	\
	do {	\
		temp = M80_READ_BYTE(HL);	\
		--HL;	\
		op = --BC != 0;	\
		sum = A - temp;	\
	} while (op && sum != 0);	\
	cbits = A ^ temp ^ sum;	\
	AF = (AF & ~0xfe) | (sum & 0x80) | (!(sum & 0xff) << 6) |	\
		(((sum - ((cbits&16)>>4))&2) << 4) |	\
		(cbits & 16) | ((sum - ((cbits >> 4) & 1)) & 8) |	\
		op << 2 | 2;	\
	if ((sum & 15) == 8 && (cbits & 16) != 0)	\
		AF &= ~8;	\
}
*/

// macro for LDD instruction
__inline__ void M80_LDD()
{
	Uint8 acu = M80_READ_BYTE(HL);
	--HL;
	M80_WRITE_BYTE(DE, acu);
	--DE;
	acu += A;
	AF = (AF & ~0x3e) | (acu & 8) | ((acu & 2) << 4) |
		(((--BC & 0xffff) != 0) << 2);
}

/*
// macro for LDDR instruction
#define M80_LDDR	\
{	\
	Uint8 acu;	\
	BC &= 0xffff;	\
	do	\
	{	\
		acu = M80_READ_BYTE(HL);	\
		--HL;	\
		M80_WRITE_BYTE(DE, acu);	\
		--DE;	\
	} while (--BC);	\
	acu += A;	\
	AF = (AF & ~0x3e) | (acu & 8) | ((acu & 2) << 4);	\
}
*/

__inline__ void M80_OUTI()
{
	M80_OUT16(BC, M80_READ_BYTE(HL));
	HL++;
	B--;
}

// puts together an IXIY + d and advances PC, useful for most instructions
#define IXIY_OFFSET(IXIY)	IXIY + ((Sint8) M80_GET_ARG)

// puts together an IXIY + d but does not advance PC.  Needed for math operations that use this
// value more than once
#define IXIY_OFFSET_PEEK(IXIY)	IXIY + ((Sint8) M80_PEEK_ARG)

#endif // end M80_INTERNAL_H

