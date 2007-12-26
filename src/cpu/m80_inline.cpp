/*
 * m80.cpp
 *
 * Copyright (C) 2001 Matt Ownby
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

/* Matt's Z80 (M80)	*/
/* a Z80 emulator by Matt Ownby	*/

/* Intentionally designed with a similar API to mame's z80 core since that was what I was using	*/
/* before I wrote this, and I kind of want this to be a drop-in replacement.	*/

/* Conforms to:	*/
/* The goal of this emulator is to conform to the Z80 as exactly as possible while at the same	*/
/* time running as fast as possible.	*/
/* The documents used to verify this emulator's proper operation can be found at	*/
/* http://www.msxnet.org/tech/Z80/ , specifically the 	*/
/*	Z80 Undocumented Features (MOST HELPFUL!)	*/
/*	Complete Opcode List	*/
/*	PDF version of Word document with all instruction tables	*/

#include <stdio.h>
#include "m80.h"
#ifdef USE_MAME_Z80_DEBUGGER
#include "z80dasm.h"
#endif // MAME_Z80_DEBUGGER
#include "m80_internal.h"
#include "m80tables.h"
#include "m80daa.h"
#include "../io/conout.h"
#ifdef WIN32
#pragma warning (disable:4244)	// disable the warning about possible loss of data
#endif

struct m80_context g_context;	/* full context for the cpu */
Uint32	g_cycles_executed = 0;	/* how many cycles we've executed this time around */
Uint32	g_cycles_to_execute = 0;	/* how many cycles we're supposed to execute */
Sint32 (*g_irq_callback)(int nothing) = 0;	/* function that gets called when we activate our IRQ */
Sint32 (*g_nmi_callback)() = 0;	/* function that gets called when we activate our NMI */
char s2[81] = "";
Uint8 *opcode_base = 0;

// # of cycles used per opcode assuming conditions fail in conditional instructions
// CB, DD, ED, and FD all get 0 cycles in this table because we add that in other tables
const Uint8 op_cycles[256] =
{
	4,10, 7, 6, 4, 4, 7, 4, 4,11, 7, 6, 4, 4, 7, 4,
	8,10, 7, 6, 4, 4, 7, 4,12,11, 7, 6, 4, 4, 7, 4,
	7,10,16, 6, 4, 4, 7, 4, 7,11,16, 6, 4, 4, 7, 4,
	7,10,13, 6,11,11,10, 4, 7,11,13, 6, 4, 4, 7, 4,
	4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
	4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
	4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
	7, 7, 7, 7, 7, 7, 4, 7, 4, 4, 4, 4, 4, 4, 7, 4,
	4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
	4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
	4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
	4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
	5,10,10,10,10,11, 7,11, 5,10,10, 0,10,17, 7,11,
	5,10,10,11,10,11, 7,11, 5, 4,10,11,10, 0, 7,11,
	5,10,10,19,10,11, 7,11, 5, 4,10, 4,10, 0, 7,11,
	5,10,10, 4,10,11, 7,11, 5, 6,10, 4,10, 0, 7,11
};

// # of cycles used per CB opcode
const Uint8 cb_cycles[256] =
{
	8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
	8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
	8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
	8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
	8, 8, 8, 8, 8, 8,12, 8, 8, 8, 8, 8, 8, 8,12, 8,
	8, 8, 8, 8, 8, 8,12, 8, 8, 8, 8, 8, 8, 8,12, 8,
	8, 8, 8, 8, 8, 8,12, 8, 8, 8, 8, 8, 8, 8,12, 8,
	8, 8, 8, 8, 8, 8,12, 8, 8, 8, 8, 8, 8, 8,12, 8,
	8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
	8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
	8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
	8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
	8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
	8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
	8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
	8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8
};

// # of cycles used per ED instruction
const Uint8 ed_cycles[256] =
{
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	12,12,15,20, 8, 8, 8, 9,12,12,15,20, 8, 8, 8, 9,
	12,12,15,20, 8, 8, 8, 9,12,12,15,20, 8, 8, 8, 9,
	12,12,15,20, 8, 8, 8,18,12,12,15,20, 8, 8, 8,18,
	12,12,15,20, 8, 8, 8, 8,12,12,15,20, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	16,16,16,16, 8, 8, 8, 8,16,16,16,16, 8, 8, 8, 8,
	16,16,16,16, 8, 8, 8, 8,16,16,16,16, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
};

// # of cycles used in IX and IY instructions (DD and FD)
// If an instruction is undefined (which many of these are), it gets treated as a NOP (4 cycles)
// CB gets 0 cycles because it is defined in another table
const Uint8 ddfd_cycles[256] =
{
	4, 4, 4, 4, 4, 4, 4, 4, 4,15, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4,15, 4, 4, 4, 4, 4, 4,
	4,14,20,10, 9, 9, 9, 4, 4,15,20,10, 9, 9, 9, 4,
	4, 4, 4, 4,23,23,19, 4, 4,15, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 9, 9,19, 4, 4, 4, 4, 4, 9, 9,19, 4,
	4, 4, 4, 4, 9, 9,19, 4, 4, 4, 4, 4, 9, 9,19, 4,
	9, 9, 9, 9, 9, 9,19, 9, 9, 9, 9, 9, 9, 9,19, 9,
	19,19,19,19,19,19, 4,19, 4, 4, 4, 4, 9, 9,19, 4,
	4, 4, 4, 4, 9, 9,19, 4, 4, 4, 4, 4, 9, 9,19, 4,
	4, 4, 4, 4, 9, 9,19, 4, 4, 4, 4, 4, 9, 9,19, 4,
	4, 4, 4, 4, 9, 9,19, 4, 4, 4, 4, 4, 9, 9,19, 4,
	4, 4, 4, 4, 9, 9,19, 4, 4, 4, 4, 4, 9, 9,19, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4,14, 4,23, 4,15, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4,10, 4, 4, 4, 4, 4, 4
};

// # of cycles used if a DDCB or FDCB instruction is called
const Uint8 ddfd_cb_cycles[256] =
{
	23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
	23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
	23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
	23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
	20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
	20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
	20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
	20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
	23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
	23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
	23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
	23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
	23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
	23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
	23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
	23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23
};

/* indicates where in memory the z80's 64k of RAM begins */
/* This function MUST BE CALLED to initialize the z80 */
void m80_set_opcode_base(Uint8 *address)
{
	opcode_base = address;

#ifdef USE_MAME_Z80_DEBUGGER
	OP_ROM = OP_RAM = address;
#endif // MAME_Z80_DEBUGGER


}

/* resets the z80 as if it was just powered on */
void m80_reset()
{
	int i = 0;
	
	/* clear all registers */
	for (i = M80_PC; i < M80_REG_COUNT; i++)
	{
		g_context.m80_regs[i].w = 0;
	}

	/* after some testing using Dragon's Lair, we've observed that most registers tend to start near 0xFFFF */
	/* it's always random though */
	IX = 0xFFFF;
	IY = 0xFFFF;
	AF = 0xFFFF;
	HL = 0xFFFF;
	BC = 0xFFFF;
	DE = 0xFFFF;
	SP = 0xFFFF;

	g_context.IFF1 = 0;
	g_context.IFF2 = 0;
	g_context.interrupt_mode = 0;
	M80_CHANGE_PC(0);
}

/* executes ONE ED-prefixed instruction, incrementing PC and g_cycles_executed variable appropriately */
#define M80_EXEC_ED	\
{	\
	Uint8 opcode = M80_GET_ARG;	/* get opcode and increment PC */	\
	Uint16 temp_word;	/* a temp word for storing temp variables :) */ \
	g_cycles_executed += ed_cycles[opcode];	\
	M80_INC_R;	/* for each DE instruction, increase R again */ \
	switch(opcode)	\
	{	\
	case 0x40:	/* IN B, (C) */	\
		M80_IN16(BC, B);	\
		break;	\
	case 0x41:	/* OUT (C), B */	\
		M80_OUT16(BC, B);	\
		break;	\
	case 0x42:	/* SBC HL, BC */	\
		M80_SBC_REGS16(HL, BC);	\
		break;	\
	case 0x43:	/* LD (nn), BC */	\
		temp_word = M80_PEEK_WORD;	\
		PC += 2;	\
		M80_WRITE_WORD(temp_word, M80_BC);	\
		break;	\
	case 0x44:	/* NEG, take two's compliment of A */	\
	case 0x4C:	\
	case 0x54:	\
	case 0x5C:	\
	case 0x64:	\
	case 0x6C:	\
	case 0x74:	\
	case 0x7C:	\
		{	\
			Uint8 temp = A;	\
			A = 0;	\
			M80_SUB_FROM_A(temp);	\
		}	\
		break;	\
	case 0x45:	/* RETN (return from NMI) or RETI (return from IRQ) , they do the same thing */	\
	case 0x4D:	\
	case 0x55:	\
	case 0x5D:	\
	case 0x65:	\
	case 0x6D:	\
	case 0x75:	\
	case 0x7D:	\
		g_context.IFF1 = g_context.IFF2;	\
		M80_RET;	\
		break;	\
	case 0x46:	/* IM 0 , go into interrupt mode 0 */	\
	case 0x4E:	\
	case 0x66:	\
	case 0x6E:	\
		g_context.interrupt_mode = 0;	\
		break;	\
	case 0x47:	/* LD I, A */	\
		I = A;	\
		break;	\
	case 0x48:	/* IN C, (C) */	\
		M80_IN16(BC, C);	\
		break;	\
	case 0x49:	/* OUT (C), C */	\
		M80_OUT16(BC, C);	\
		break;	\
	case 0x4A:	/* ADC HL, BC */	\
		M80_ADC_REGS16(HL, BC);	\
		break;	\
	case 0x4B:	/* LD BC, (nn) */	\
		temp_word = M80_PEEK_WORD;	\
		PC += 2;	\
		M80_READ_WORD(temp_word, M80_BC);	\
		break;	\
	/* case 0x4C, see 0x44 */	\
	/* case 0x4D, see 0x45 */	\
	/* case 0x4E, see 0x46 */	\
	case 0x4F:	/* LD R, A */	\
		R = A; /* bit 7 of R can be changed with this command */	\
		break;	\
	case 0x50:	/* IN D, (C) */	\
		M80_IN16(BC, D);	\
		break;	\
	case 0x51:	/* OUT (C), D */	\
		M80_OUT16(BC, D);	\
		break;	\
	case 0x52:	/* SBC HL, DE */	\
		M80_SBC_REGS16(HL, DE);	\
		break;	\
	case 0x53:	/* LD (nn), DE */	\
		temp_word = M80_PEEK_WORD;	\
		PC += 2;	\
		M80_WRITE_WORD(temp_word, M80_DE);	\
		break;	\
	/* case 0x54, see 0x44 */	\
	/* case 0x55, see 0x45 */	\
	case 0x56:	/* IM 1 */	\
	case 0x76:	\
		g_context.interrupt_mode = 1;	\
		break;	\
	case 0x57:	/* LD A, I */	\
		A = I;	\
		FLAGS &= C_FLAG;	/* preserve C, clear H and N */	\
		FLAGS |= m80_sz53_flags[A];	/* set S, Z, 5, 3 flags */	\
		FLAGS |= (g_context.IFF2 << 2);	/* put IFF2 in the V/P flag slot */	\
		break;	\
	case 0x58:	/* IN E, (C) */	\
		M80_IN16(BC, E);	\
		break;	\
	case 0x59:	/* OUT (C), E */	\
		M80_OUT16(BC, E);	\
		break;	\
	case 0x5A:	/* ADC HL, DE */	\
		M80_ADC_REGS16(HL, DE);	\
		break;	\
	case 0x5B:	/* LD DE, (nn) */	\
		temp_word = M80_PEEK_WORD;	\
		PC += 2;	\
		M80_READ_WORD(temp_word, M80_DE);	\
		break;	\
	/* case 0x5C, see 0x44 */	\
	/* case 0x5D, see 0x45 */	\
	case 0x5E:	/* IM 2 */	\
	case 0x7E:	\
		g_context.interrupt_mode = 2;	\
		break;	\
	case 0x5F:	/* LD A, R */	\
		A = R;	\
		FLAGS &= C_FLAG;	/* preserve C, clear H and N */	\
		FLAGS |= m80_sz53_flags[A];	/* set S, Z, 5, 3 flags */	\
		FLAGS |= (g_context.IFF2 << 2);	/* put IFF2 in the V/P flag slot */	\
		break;	\
	case 0x60:	/* IN H, (C) */	\
		M80_IN16(BC, H);	\
		break;	\
	case 0x61:	/* OUT (C), H */	\
		M80_OUT16(BC, H);	\
		break;	\
	case 0x62:	/* SBC HL, HL */	\
		M80_SBC_REGS16(HL, HL);	\
		break;	\
	case 0x63:	/* LD (nn), HL */	\
		temp_word = M80_PEEK_WORD;	\
		PC += 2;	\
		M80_WRITE_WORD(temp_word, M80_HL);	\
		break;	\
	/* case 0x64, see 0x44 */	\
	/* case 0x65, see 0x45 */	\
	/* case 0x66, see 0x46 */	\
	case 0x67:	/* RRD */	\
		/* Rotates (HL) right 4 times.  The lower 4 bits of A replace the newly undefined bits in (HL) */	\
		/* and the lower 4 bits of (HL) that rotated out go into the lower 4 bits of A */	\
		{	\
			Uint8 val = M80_READ_BYTE(HL);	\
			M80_WRITE_BYTE(HL, (val >> 4) | (A << 4));	\
			A = (A & 0xF0) | (val & 0x0F);	\
			FLAGS &= C_FLAG;	\
			FLAGS |= m80_sz53p_flags[A];	\
		}	\
		break;	\
	case 0x68:	/* IN L, (C) */	\
		M80_IN16(BC, L);	\
		break;	\
	case 0x69:	/* OUT (C), L */	\
		M80_OUT16(BC, L);	\
		break;	\
	case 0x6A:	/* ADC HL, HL */	\
		M80_ADC_REGS16(HL, HL);	\
		break;	\
	case 0x6B:	/* LD HL, (nn) */	\
		temp_word = M80_PEEK_WORD;	\
		PC += 2;	\
		M80_READ_WORD(temp_word, M80_HL);	\
		break;	\
	/* case 0x6C, see 0x44 */	\
	/* case 0x6D, see 0x45 */	\
	/* case 0x6E, see 0x46 */	\
	case 0x6F:	/* RLD */	\
		/* Rotates (HL) left 4 times.  See rules for RRD */	\
		{	\
			Uint8 val = M80_READ_BYTE(HL);	\
			M80_WRITE_BYTE(HL, (val << 4) | (A & 0x0F));	\
			A = (A & 0xF0) | (val >> 4);	\
			FLAGS &= C_FLAG;	\
			FLAGS |= m80_sz53p_flags[A];	\
		}	\
		break;	\
	case 0x70:	/* IN (C) / IN F, (C) */	\
		/* this read a port but doesn't store it to a register, it just affects the flags */	\
		FLAGS = FLAGS & C_FLAG;	/* preserve only carry */	\
		FLAGS |= m80_sz53p_flags[cpu_readport16(BC)];	\
		break;	\
	case 0x71:	/* OUT (C), 0 */	\
		M80_OUT16(BC, 0);	\
		break;	\
	case 0x72:	/* SBC HL, SP */	\
		M80_SBC_REGS16(HL, SP);	\
		break;	\
	case 0x73:	/* LD (nn), SP */	\
		temp_word = M80_PEEK_WORD;	\
		PC += 2;	\
		M80_WRITE_WORD(temp_word, M80_SP);	\
		break;	\
	/* case 0x74, see 0x44 */	\
	/* case 0x75, see 0x45 */	\
	/* case 0x76, see 0x56 */	\
	/* case 0x77, NOP */	\
	case 0x78:	/* IN A, (C) */	\
		M80_IN16(BC, A);	\
		break;	\
	case 0x79:	/* OUT (C), A */	\
		M80_OUT16(BC, A);	\
		break;	\
	case 0x7A:	/* ADC HL, SP */	\
		M80_ADC_REGS16(HL, SP);	\
		break;	\
	case 0x7B:	/* LD SP, nnnn */	\
		temp_word = M80_PEEK_WORD;	\
		PC += 2;	\
		M80_READ_WORD(temp_word, M80_SP);	\
		break;	\
	/* case 0x7C, see 0x44 */	\
	/* case 0x7D, see 0x45 */	\
	/* case 0x7E, see 0x5E */	\
	/* case 0x7F, NOP */	\
	case 0xA0:	/* LDI */	\
		M80_LDI();	\
		break;	\
	case 0xA1:	/* CPI */	\
		M80_CPI();	\
		break;	\
	case 0xA2:	/* INI */	\
		X;	\
		sprintf(s2,"Unimplemented opcode INI at PC: %x",PC); \
		printline(s2); \
		break;	\
	case 0xA3:	/* OUTI */	\
		M80_OUTI(); \
		break;	\
	case 0xA8:	/* LDD */	\
		M80_LDD();	\
		break;	\
	case 0xA9:	/* CPD */	\
		M80_CPD();	\
		break;	\
	case 0xAA:	/* IND */	\
		X;	\
		sprintf(s2,"Unimplemented opcode IND at PC: %x",PC); \
		printline(s2); \
		break;	\
	case 0xAB:	/* OUTD */	\
		X;	\
		sprintf(s2,"Unimplemented opcode OUTD at PC: %x",PC); \
		printline(s2); \
		break;	\
	case 0xB0:	/* LDIR */	\
		M80_BLOCK_REPEAT(M80_LDI());	\
		break;	\
	case 0xB1:	/* CPIR */	\
/*		M80_CPIR;	*/ \
		M80_BLOCK2_REPEAT(M80_CPI());	\
		break;	\
	case 0xB2:	/* INIR */	\
		X;	\
		sprintf(s2,"Unimplemented opcode INIR at PC: %x",PC); \
		printline(s2); \
		break;	\
	case 0xB3:	/* OTIR */	\
		M80_BLOCK3_REPEAT(M80_OUTI());	\
		break;	\
	case 0xB8:	/* LDDR */	\
		M80_BLOCK_REPEAT(M80_LDD());	\
	/*	M80_LDDR;	*/ \
		break;	\
	case 0xB9:	/* CPDR */	\
		M80_BLOCK2_REPEAT(M80_CPD());	\
	/*	M80_CPDR;	*/ \
		break;	\
	case 0xBA:	/* INDR */	\
		X;	\
		sprintf(s2,"Unimplemented opcode INDR at PC: %x",PC); \
		printline(s2); \
		break;	\
	case 0xBB:	/* OTDR */	\
		X;	\
		sprintf(s2,"Unimplemented opcode OTDR at PC: %x",PC); \
		printline(s2); \
		break;	\
	default:	\
		fprintf(stderr, "ED NOP!\n");	\
		break;	\
	/* ANYTHING that wasn't caught by this switch statement is a NOP */	\
	} /* end switch */	\
} /* end macro */


















/* executes all of the 0xCB instructions that are part of the DD/FD instruction set */
#define M80_EXEC_DDFD_CB(IXIY)	\
{	\
	Uint8 offset = M80_GET_ARG;	/* on all of these instructions, the offset comes first */	\
	Uint8 opcode = M80_GET_ARG;	/* get opcode and increment PC */	\
	g_cycles_executed += ddfd_cb_cycles[opcode];	\
	\
	/* the R count does NOT increase for these instructions */	\
	\
	switch(opcode)	\
	{	\
	case 0:	/* RLC	B */	\
		M80_RLC_COMBO(B, IXIY + offset);	\
		break;	\
	case 1:	/* RLC	C */	\
		M80_RLC_COMBO(C, IXIY + offset);	\
		break;	\
	case 2:	/* RLC	D */	\
		M80_RLC_COMBO(D, IXIY + offset);	\
		break;	\
	case 3:	/* RLC	E */	\
		M80_RLC_COMBO(E, IXIY + offset);	\
		break;	\
	case 4:	/* RLC	H */	\
		M80_RLC_COMBO(H, IXIY + offset);	\
		break;	\
	case 5:	/* RLC	L */	\
		M80_RLC_COMBO(L, IXIY + offset);	\
		break;	\
	case 6:	/* RLC (IXIY + d) */	\
		{	\
			Uint8 val = M80_READ_BYTE(IXIY + offset);	\
			M80_RLC(val);	\
			M80_WRITE_BYTE(IXIY + offset, val);	\
		}	\
		break;	\
	case 7:	/* RLC	A */	\
		M80_RLC_COMBO(A, IXIY + offset);	\
		break;	\
	case 8:	/* RRC	B */	\
		M80_RRC_COMBO(B, IXIY + offset);	\
		break;	\
	case 9:	/* RRC	C */	\
		M80_RRC_COMBO(C, IXIY + offset);	\
		break;	\
	case 0x0A:	/* RRC	D */	\
		M80_RRC_COMBO(D, IXIY + offset);	\
		break;	\
	case 0x0B:	/* RRC	E */	\
		M80_RRC_COMBO(E, IXIY + offset);	\
		break;	\
	case 0x0C:	/* RRC	H */	\
		M80_RRC_COMBO(H, IXIY + offset);	\
		break;	\
	case 0x0D:	/* RRC	L */	\
		M80_RRC_COMBO(L, IXIY + offset);	\
		break;	\
	case 0x0E:	/* RRC	(IXIY + offset) */	\
		{	\
			Uint8 val = M80_READ_BYTE(IXIY + offset);	\
			M80_RRC(val);	\
			M80_WRITE_BYTE(IXIY + offset, val);	\
		}	\
		break;	\
	case 0x0F:	/* RRC	A */	\
		M80_RRC_COMBO(A, IXIY + offset);	\
		break;	\
	case 0x10:	/* RL B */	\
		M80_RL_COMBO(B, IXIY + offset);	\
		break;	\
	case 0x11:	/* RL C */	\
		M80_RL_COMBO(C, IXIY + offset);	\
		break;	\
	case 0x12:	/* RL D */	\
		M80_RL_COMBO(D, IXIY + offset);	\
		break;	\
	case 0x13:	/* RL E */	\
		M80_RL_COMBO(E, IXIY + offset);	\
		break;	\
	case 0x14:	/* RL H */	\
		M80_RL_COMBO(H, IXIY + offset);	\
		break;	\
	case 0x15:	/* RL L */	\
		M80_RL_COMBO(L, IXIY + offset);	\
		break;	\
	case 0x16:	/* RL (IXIY + offset, IXIY) */	\
		{	\
			Uint8 val = M80_READ_BYTE(IXIY + offset);	\
			M80_RL(val);	\
			M80_WRITE_BYTE(IXIY + offset, val);	\
		}	\
		break;	\
	case 0x17:	/* RL A */	\
		M80_RL_COMBO(A, IXIY + offset);	\
		break;	\
	case 0x18:	/* RR B */	\
		M80_RR_COMBO(B, IXIY + offset);	\
		break;	\
	case 0x19:	/* RR C */	\
		M80_RR_COMBO(C, IXIY + offset);	\
		break;	\
	case 0x1A:	/* RR D */	\
		M80_RR_COMBO(D, IXIY + offset);	\
		break;	\
	case 0x1B:	/* RR E */	\
		M80_RR_COMBO(E, IXIY + offset);	\
		break;	\
	case 0x1C:	/* RR H */	\
		M80_RR_COMBO(H, IXIY + offset);	\
		break;	\
	case 0x1D:	/* RR L */	\
		M80_RR_COMBO(L, IXIY + offset);	\
		break;	\
	case 0x1E:	/* RR (IXIY + offset) */	\
		{	\
			Uint8 val = M80_READ_BYTE(IXIY + offset);	\
			M80_RR(val);	\
			M80_WRITE_BYTE(IXIY + offset, val);	\
		}	\
		break;	\
	case 0x1F:	/* RR A */	\
		M80_RR_COMBO(A, IXIY + offset);	\
		break;	\
	case 0x20:	/* SLA B */	\
		M80_SLA_COMBO(B, IXIY + offset);	\
		break;	\
	case 0x21:	/* SLA C */	\
		M80_SLA_COMBO(C, IXIY + offset);	\
		break;	\
	case 0x22:	/* SLA D */	\
		M80_SLA_COMBO(D, IXIY + offset);	\
		break;	\
	case 0x23:	/* SLA E */	\
		M80_SLA_COMBO(E, IXIY + offset);	\
		break;	\
	case 0x24:	/* SLA H */	\
		M80_SLA_COMBO(H, IXIY + offset);	\
		break;	\
	case 0x25:	/* SLA L */	\
		M80_SLA_COMBO(L, IXIY + offset);	\
		break;	\
	case 0x26:	/* SLA (IXIY + offset, IXIY) */	\
		{	\
			Uint8 val = M80_READ_BYTE(IXIY + offset);	\
			M80_SLA(val);	\
			M80_WRITE_BYTE(IXIY + offset, val);	\
		}	\
		break;	\
	case 0x27:	/* SLA A */	\
		M80_SLA_COMBO(A, IXIY + offset);	\
		break;	\
	case 0x28:	/* SRA B */	\
		M80_SRA_COMBO(B, IXIY + offset);	\
		break;	\
	case 0x29:	/* SRA C */	\
		M80_SRA_COMBO(C, IXIY + offset);	\
		break;	\
	case 0x2A:	/* SRA D */	\
		M80_SRA_COMBO(D, IXIY + offset);	\
		break;	\
	case 0x2B:	/* SRA E */	\
		M80_SRA_COMBO(E, IXIY + offset);	\
		break;	\
	case 0x2C:	/* SRA H */	\
		M80_SRA_COMBO(H, IXIY + offset);	\
		break;	\
	case 0x2D:	/* SRA L */	\
		M80_SRA_COMBO(L, IXIY + offset);	\
		break;	\
	case 0x2E:	\
		{	\
			Uint8 val = M80_READ_BYTE(IXIY + offset);	\
			M80_SRA(val);	\
			M80_WRITE_BYTE(IXIY + offset, val);	\
		}	\
		break;	\
	case 0x2F:	/* SRA A */	\
		M80_SRA_COMBO(A, IXIY + offset);	\
		break;	\
	case 0x30:	/* SLIA B */	\
		M80_SLIA_COMBO(B, IXIY + offset);	\
		break;	\
	case 0x31:	/* SLIA C */	\
		M80_SLIA_COMBO(C, IXIY + offset);	\
		break;	\
	case 0x32:	/* SLIA D */	\
		M80_SLIA_COMBO(D, IXIY + offset);	\
		break;	\
	case 0x33:	/* SLIA E */	\
		M80_SLIA_COMBO(E, IXIY + offset);	\
		break;	\
	case 0x34:	/* SLIA H */	\
		M80_SLIA_COMBO(H, IXIY + offset);	\
		break;	\
	case 0x35:	/* SLIA L */	\
		M80_SLIA_COMBO(L, IXIY + offset);	\
		break;	\
	case 0x36:	\
		{	\
			Uint8 val = M80_READ_BYTE(IXIY + offset);	\
			M80_SLIA(val);	\
			M80_WRITE_BYTE(IXIY + offset, val);	\
		}	\
		break;	\
	case 0x37:	/* SLIA A */	\
		M80_SLIA_COMBO(A, IXIY + offset);	\
		break;	\
	case 0x38:	/* SRL B */	\
		M80_SRL_COMBO(B, IXIY + offset);	\
		break;	\
	case 0x39:	/* SRL C */	\
		M80_SRL_COMBO(C, IXIY + offset);	\
		break;	\
	case 0x3A:	/* SRL D */	\
		M80_SRL_COMBO(D, IXIY + offset);	\
		break;	\
	case 0x3B:	/* SRL E */	\
		M80_SRL_COMBO(E, IXIY + offset);	\
		break;	\
	case 0x3C:	/* SRL H */	\
		M80_SRL_COMBO(H, IXIY + offset);	\
		break;	\
	case 0x3D:	/* SRL L */	\
		M80_SRL_COMBO(L, IXIY + offset);	\
		break;	\
	case 0x3E:	\
		{	\
			Uint8 val = M80_READ_BYTE(IXIY + offset);	\
			M80_SRL(val);	\
			M80_WRITE_BYTE(IXIY + offset, val);	\
		}	\
		break;	\
	case 0x3F:	/* SRL A */	\
		M80_SRL_COMBO(A, IXIY + offset);	\
		break;	\
	case 0x40:	/* BIT 0, B */	\
	case 0x41:	/* BIT 0, C */	\
	case 0x42:	/* BIT 0, D */	\
	case 0x43:	/* BIT 0, E */	\
	case 0x44:	/* BIT 0, H */	\
	case 0x45:	/* BIT 0, L */	\
	case 0x47:	/* BIT 0, A */	\
		M80_BIT(0, M80_READ_BYTE(IXIY + offset));	\
		break;	\
	case 0x46:	\
		M80_BIT_NO35(0, M80_READ_BYTE(IXIY + offset));	\
		break;	\
	case 0x48:	/* BIT 1, B */	\
	case 0x49:	/* BIT 1, C */	\
	case 0x4A:	/* BIT 1, D */	\
	case 0x4B:	/* BIT 1, E */	\
	case 0x4C:	/* BIT 1, H */	\
	case 0x4D:	/* BIT 1, L */	\
	case 0x4F:	/* BIT 1, A */	\
		M80_BIT(1, M80_READ_BYTE(IXIY + offset));	\
		break;	\
	case 0x4E:	\
		M80_BIT_NO35(1, M80_READ_BYTE(IXIY + offset));	\
		break;	\
	case 0x50:	/* BIT 2, B */	\
	case 0x51:	/* BIT 2, C */	\
	case 0x52:	/* BIT 2, D */	\
	case 0x53:	/* BIT 2, E */	\
	case 0x54:	/* BIT 2, H */	\
	case 0x55:	/* BIT 2, L */	\
	case 0x57:	/* BIT 2, A */	\
		M80_BIT(2, M80_READ_BYTE(IXIY + offset));	\
		break;	\
	case 0x56:	\
		M80_BIT_NO35(2, M80_READ_BYTE(IXIY + offset));	\
		break;	\
	case 0x58:	/* BIT 3, B */	\
	case 0x59:	/* BIT 3, C */	\
	case 0x5A:	/* BIT 3, D */	\
	case 0x5B:	/* BIT 3, E */	\
	case 0x5C:	/* BIT 3, H */	\
	case 0x5D:	/* BIT 3, L */	\
	case 0x5F:	/* BIT 3, A */	\
		M80_BIT(3, M80_READ_BYTE(IXIY + offset));	\
		break;	\
	case 0x5E:	\
		M80_BIT_NO35(3, M80_READ_BYTE(IXIY + offset));	\
		break;	\
	case 0x60:	/* BIT 4, B */	\
	case 0x61:	/* BIT 4, C */	\
	case 0x62:	/* BIT 4, D */	\
	case 0x63:	/* BIT 4, E */	\
	case 0x64:	/* BIT 4, H */	\
	case 0x65:	/* BIT 4, L */	\
	case 0x67:	/* BIT 4, A */	\
		M80_BIT(4, M80_READ_BYTE(IXIY + offset));	\
		break;	\
	case 0x66:	\
		M80_BIT_NO35(4, M80_READ_BYTE(IXIY + offset));	\
		break;	\
	case 0x68:	/* BIT 5, B */	\
	case 0x69:	/* BIT 5, C */	\
	case 0x6A:	/* BIT 5, D */	\
	case 0x6B:	/* BIT 5, E */	\
	case 0x6C:	/* BIT 5, H */	\
	case 0x6D:	/* BIT 5, L */	\
	case 0x6F:	/* BIT 5, A */	\
		M80_BIT(5, M80_READ_BYTE(IXIY + offset));	\
		break;	\
	case 0x6E:	\
		M80_BIT_NO35(5, M80_READ_BYTE(IXIY + offset));	\
		break;	\
	case 0x70:	/* BIT 6, B */	\
	case 0x71:	/* BIT 6, C */	\
	case 0x72:	/* BIT 6, D */	\
	case 0x73:	/* BIT 6, E */	\
	case 0x74:	/* BIT 6, H */	\
	case 0x75:	/* BIT 6, L */	\
	case 0x77:	/* BIT 6, A */	\
		M80_BIT(6, M80_READ_BYTE(IXIY + offset));	\
		break;	\
	case 0x76:	\
		M80_BIT_NO35(6, M80_READ_BYTE(IXIY + offset));	\
		break;	\
	case 0x78:	/* BIT 7, B */	\
	case 0x79:	/* BIT 7, C */	\
	case 0x7A:	/* BIT 7, D */	\
	case 0x7B:	/* BIT 7, E */	\
	case 0x7C:	/* BIT 7, H */	\
	case 0x7D:	/* BIT 7, L */	\
	case 0x7F:	/* BIT 7, A */	\
		M80_BIT(7, M80_READ_BYTE(IXIY + offset));	\
		break;	\
	case 0x7E:	\
		M80_BIT_NO35(7, M80_READ_BYTE(IXIY + offset));	\
		break;	\
	case 0x80:	/* RES	0, B */	\
		M80_RES_COMBO(0, B, IXIY + offset);	\
		break;	\
	case 0x81:	/* RES	0, C */	\
		M80_RES_COMBO(0, C, IXIY + offset);	\
		break;	\
	case 0x82:	/* RES	0, D */	\
		M80_RES_COMBO(0, D, IXIY + offset);	\
		break;	\
	case 0x83:	/* RES	0, E */	\
		M80_RES_COMBO(0, E, IXIY + offset);	\
		break;	\
	case 0x84:	/* RES	0, H */	\
		M80_RES_COMBO(0, H, IXIY + offset);	\
		break;	\
	case 0x85:	/* RES	0, L */	\
		M80_RES_COMBO(0, L, IXIY + offset);	\
		break;	\
	case 0x86:	\
		{	\
			Uint8 val = M80_READ_BYTE(IXIY + offset);	\
			M80_RES(0, val);	\
			M80_WRITE_BYTE(IXIY + offset, val);	\
		}	\
		break;	\
	case 0x87:	/* RES	0, A */	\
		M80_RES_COMBO(0, A, IXIY + offset);	\
		break;	\
	case 0x88:	/* RES	1, B */	\
		M80_RES_COMBO(1, B, IXIY + offset);	\
		break;	\
	case 0x89:	/* RES	1, C */	\
		M80_RES_COMBO(1, C, IXIY + offset);	\
		break;	\
	case 0x8A:	/* RES	1, D */	\
		M80_RES_COMBO(1, D, IXIY + offset);	\
		break;	\
	case 0x8B:	/* RES	1, E */	\
		M80_RES_COMBO(1, E, IXIY + offset);	\
		break;	\
	case 0x8C:	/* RES	1, H */	\
		M80_RES_COMBO(1, H, IXIY + offset);	\
		break;	\
	case 0x8D:	/* RES	1, L */	\
		M80_RES_COMBO(1, L, IXIY + offset);	\
		break;	\
	case 0x8E:	\
		{	\
			Uint8 val = M80_READ_BYTE(IXIY + offset);	\
			M80_RES(1, val);	\
			M80_WRITE_BYTE(IXIY + offset, val);	\
		}	\
		break;	\
	case 0x8F:	/* RES	1, A */	\
		M80_RES_COMBO(1, A, IXIY + offset);	\
		break;	\
	case 0x90:	/* RES	2, B */	\
		M80_RES_COMBO(2, B, IXIY + offset);	\
		break;	\
	case 0x91:	/* RES	2, C */	\
		M80_RES_COMBO(2, C, IXIY + offset);	\
		break;	\
	case 0x92:	/* RES	2, D */	\
		M80_RES_COMBO(2, D, IXIY + offset);	\
		break;	\
	case 0x93:	/* RES	2, E */	\
		M80_RES_COMBO(2, E, IXIY + offset);	\
		break;	\
	case 0x94:	/* RES	2, H */	\
		M80_RES_COMBO(2, H, IXIY + offset);	\
		break;	\
	case 0x95:	/* RES	2, L */	\
		M80_RES_COMBO(2, L, IXIY + offset);	\
		break;	\
	case 0x96:	\
		{	\
			Uint8 val = M80_READ_BYTE(IXIY + offset);	\
			M80_RES(2, val);	\
			M80_WRITE_BYTE(IXIY + offset, val);	\
		}	\
		break;	\
	case 0x97:	/* RES	2, A */	\
		M80_RES_COMBO(2, A, IXIY + offset);	\
		break;	\
	case 0x98:	/* RES	3, B */	\
		M80_RES_COMBO(3, B, IXIY + offset);	\
		break;	\
	case 0x99:	/* RES	3, C */	\
		M80_RES_COMBO(3, C, IXIY + offset);	\
		break;	\
	case 0x9A:	/* RES	3, D */	\
		M80_RES_COMBO(3, D, IXIY + offset);	\
		break;	\
	case 0x9B:	/* RES	3, E */	\
		M80_RES_COMBO(3, E, IXIY + offset);	\
		break;	\
	case 0x9C:	/* RES	3, H */	\
		M80_RES_COMBO(3, H, IXIY + offset);	\
		break;	\
	case 0x9D:	/* RES	3, L */	\
		M80_RES_COMBO(3, L, IXIY + offset);	\
		break;	\
	case 0x9E:	\
		{	\
			Uint8 val = M80_READ_BYTE(IXIY + offset);	\
			M80_RES(3, val);	\
			M80_WRITE_BYTE(IXIY + offset, val);	\
		}	\
		break;	\
	case 0x9F:	/* RES	3, A */	\
		M80_RES_COMBO(3, A, IXIY + offset);	\
		break;	\
	case 0xA0:	/* RES	4, B */	\
		M80_RES_COMBO(4, B, IXIY + offset);	\
		break;	\
	case 0xA1:	/* RES	4, C */	\
		M80_RES_COMBO(4, C, IXIY + offset);	\
		break;	\
	case 0xA2:	/* RES	4, D */	\
		M80_RES_COMBO(4, D, IXIY + offset);	\
		break;	\
	case 0xA3:	/* RES	4, E */	\
		M80_RES_COMBO(4, E, IXIY + offset);	\
		break;	\
	case 0xA4:	/* RES	4, H */	\
		M80_RES_COMBO(4, H, IXIY + offset);	\
		break;	\
	case 0xA5:	/* RES	4, L */	\
		M80_RES_COMBO(4, L, IXIY + offset);	\
		break;	\
	case 0xA6:	\
		{	\
			Uint8 val = M80_READ_BYTE(IXIY + offset);	\
			M80_RES(4, val);	\
			M80_WRITE_BYTE(IXIY + offset, val);	\
		}	\
		break;	\
	case 0xA7:	/* RES	4, A */	\
		M80_RES_COMBO(4, A, IXIY + offset);	\
		break;	\
	case 0xA8:	/* RES	5, B */	\
		M80_RES_COMBO(5, B, IXIY + offset);	\
		break;	\
	case 0xA9:	/* RES	5, C */	\
		M80_RES_COMBO(5, C, IXIY + offset);	\
		break;	\
	case 0xAA:	/* RES	5, D */	\
		M80_RES_COMBO(5, D, IXIY + offset);	\
		break;	\
	case 0xAB:	/* RES	5, E */	\
		M80_RES_COMBO(5, E, IXIY + offset);	\
		break;	\
	case 0xAC:	/* RES	5, H */	\
		M80_RES_COMBO(5, H, IXIY + offset);	\
		break;	\
	case 0xAD:	/* RES	5, L */	\
		M80_RES_COMBO(5, L, IXIY + offset);	\
		break;	\
	case 0xAE:	\
		{	\
			Uint8 val = M80_READ_BYTE(IXIY + offset);	\
			M80_RES(5, val);	\
			M80_WRITE_BYTE(IXIY + offset, val);	\
		}	\
		break;	\
	case 0xAF:	/* RES	5, A */	\
		M80_RES_COMBO(5, A, IXIY + offset);	\
		break;	\
	case 0xB0:	/* RES	6, B */	\
		M80_RES_COMBO(6, B, IXIY + offset);	\
		break;	\
	case 0xB1:	/* RES	6, C */	\
		M80_RES_COMBO(6, C, IXIY + offset);	\
		break;	\
	case 0xB2:	/* RES	6, D */	\
		M80_RES_COMBO(6, D, IXIY + offset);	\
		break;	\
	case 0xB3:	/* RES	6, E */	\
		M80_RES_COMBO(6, E, IXIY + offset);	\
		break;	\
	case 0xB4:	/* RES	6, H */	\
		M80_RES_COMBO(6, H, IXIY + offset);	\
		break;	\
	case 0xB5:	/* RES	6, L */	\
		M80_RES_COMBO(6, L, IXIY + offset);	\
		break;	\
	case 0xB6:	\
		{	\
			Uint8 val = M80_READ_BYTE(IXIY + offset);	\
			M80_RES(6, val);	\
			M80_WRITE_BYTE(IXIY + offset, val);	\
		}	\
		break;	\
	case 0xB7:	/* RES	6, A */	\
		M80_RES_COMBO(6, A, IXIY + offset);	\
		break;	\
	case 0xB8:	/* RES	7, B */	\
		M80_RES_COMBO(7, B, IXIY + offset);	\
		break;	\
	case 0xB9:	/* RES	7, C */	\
		M80_RES_COMBO(7, C, IXIY + offset);	\
		break;	\
	case 0xBA:	/* RES	7, D */	\
		M80_RES_COMBO(7, D, IXIY + offset);	\
		break;	\
	case 0xBB:	/* RES	7, E */	\
		M80_RES_COMBO(7, E, IXIY + offset);	\
		break;	\
	case 0xBC:	/* RES	7, H */	\
		M80_RES_COMBO(7, H, IXIY + offset);	\
		break;	\
	case 0xBD:	/* RES	7, L */	\
		M80_RES_COMBO(7, L, IXIY + offset);	\
		break;	\
	case 0xBE:	\
		{	\
			Uint8 val = M80_READ_BYTE(IXIY + offset);	\
			M80_RES(7, val);	\
			M80_WRITE_BYTE(IXIY + offset, val);	\
		}	\
		break;	\
	case 0xBF:	/* RES	7, A */	\
		M80_RES_COMBO(7, A, IXIY + offset);	\
		break;	\
	case 0xC0:	/* SET	0, B */	\
		M80_SET_COMBO(0, B, IXIY + offset);	\
		break;	\
	case 0xC1:	/* SET	0, C */	\
		M80_SET_COMBO(0, C, IXIY + offset);	\
		break;	\
	case 0xC2:	/* SET	0, D */	\
		M80_SET_COMBO(0, D, IXIY + offset);	\
		break;	\
	case 0xC3:	/* SET	0, E */	\
		M80_SET_COMBO(0, E, IXIY + offset);	\
		break;	\
	case 0xC4:	/* SET	0, H */	\
		M80_SET_COMBO(0, H, IXIY + offset);	\
		break;	\
	case 0xC5:	/* SET	0, L */	\
		M80_SET_COMBO(0, L, IXIY + offset);	\
		break;	\
	case 0xC6:	\
		{	\
			Uint8 val = M80_READ_BYTE(IXIY + offset);	\
			M80_SET(0, val);	\
			M80_WRITE_BYTE(IXIY + offset, val);	\
		}	\
		break;	\
	case 0xC7:	/* SET	0, A */	\
		M80_SET_COMBO(0, A, IXIY + offset);	\
		break;	\
	case 0xC8:	/* SET	1, B */	\
		M80_SET_COMBO(1, B, IXIY + offset);	\
		break;	\
	case 0xC9:	/* SET	1, C */	\
		M80_SET_COMBO(1, C, IXIY + offset);	\
		break;	\
	case 0xCA:	/* SET	1, D */	\
		M80_SET_COMBO(1, D, IXIY + offset);	\
		break;	\
	case 0xCB:	/* SET	1, E */	\
		M80_SET_COMBO(1, E, IXIY + offset);	\
		break;	\
	case 0xCC:	/* SET	1, H */	\
		M80_SET_COMBO(1, H, IXIY + offset);	\
		break;	\
	case 0xCD:	/* SET	1, L */	\
		M80_SET_COMBO(1, L, IXIY + offset);	\
		break;	\
	case 0xCE:	\
		{	\
			Uint8 val = M80_READ_BYTE(IXIY + offset);	\
			M80_SET(1, val);	\
			M80_WRITE_BYTE(IXIY + offset, val);	\
		}	\
		break;	\
	case 0xCF:	/* SET	1, A */	\
		M80_SET_COMBO(1, A, IXIY + offset);	\
		break;	\
	case 0xD0:	/* SET	2, B */	\
		M80_SET_COMBO(2, B, IXIY + offset);	\
		break;	\
	case 0xD1:	/* SET	2, C */	\
		M80_SET_COMBO(2, C, IXIY + offset);	\
		break;	\
	case 0xD2:	/* SET	2, D */	\
		M80_SET_COMBO(2, D, IXIY + offset);	\
		break;	\
	case 0xD3:	/* SET	2, E */	\
		M80_SET_COMBO(2, E, IXIY + offset);	\
		break;	\
	case 0xD4:	/* SET	2, H */	\
		M80_SET_COMBO(2, H, IXIY + offset);	\
		break;	\
	case 0xD5:	/* SET	2, L */	\
		M80_SET_COMBO(2, L, IXIY + offset);	\
		break;	\
	case 0xD6:	\
		{	\
			Uint8 val = M80_READ_BYTE(IXIY + offset);	\
			M80_SET(2, val);	\
			M80_WRITE_BYTE(IXIY + offset, val);	\
		}	\
		break;	\
	case 0xD7:	/* SET	2, A */	\
		M80_SET_COMBO(2, A, IXIY + offset);	\
		break;	\
	case 0xD8:	/* SET	3, B */	\
		M80_SET_COMBO(3, B, IXIY + offset);	\
		break;	\
	case 0xD9:	/* SET	3, C */	\
		M80_SET_COMBO(3, C, IXIY + offset);	\
		break;	\
	case 0xDA:	/* SET	3, D */	\
		M80_SET_COMBO(3, D, IXIY + offset);	\
		break;	\
	case 0xDB:	/* SET	3, E */	\
		M80_SET_COMBO(3, E, IXIY + offset);	\
		break;	\
	case 0xDC:	/* SET	3, H */	\
		M80_SET_COMBO(3, H, IXIY + offset);	\
		break;	\
	case 0xDD:	/* SET	3, L */	\
		M80_SET_COMBO(3, L, IXIY + offset);	\
		break;	\
	case 0xDE:	\
		{	\
			Uint8 val = M80_READ_BYTE(IXIY + offset);	\
			M80_SET(3, val);	\
			M80_WRITE_BYTE(IXIY + offset, val);	\
		}	\
		break;	\
	case 0xDF:	/* SET	3, A */	\
		M80_SET_COMBO(3, A, IXIY + offset);	\
		break;	\
	case 0xE0:	/* SET	4, B */	\
		M80_SET_COMBO(4, B, IXIY + offset);	\
		break;	\
	case 0xE1:	/* SET	4, C */	\
		M80_SET_COMBO(4, C, IXIY + offset);	\
		break;	\
	case 0xE2:	/* SET	4, D */	\
		M80_SET_COMBO(4, D, IXIY + offset);	\
		break;	\
	case 0xE3:	/* SET	4, E */	\
		M80_SET_COMBO(4, E, IXIY + offset);	\
		break;	\
	case 0xE4:	/* SET	4, H */	\
		M80_SET_COMBO(4, H, IXIY + offset);	\
		break;	\
	case 0xE5:	/* SET	4, L */	\
		M80_SET_COMBO(4, L, IXIY + offset);	\
		break;	\
	case 0xE6:	\
		{	\
			Uint8 val = M80_READ_BYTE(IXIY + offset);	\
			M80_SET(4, val);	\
			M80_WRITE_BYTE(IXIY + offset, val);	\
		}	\
		break;	\
	case 0xE7:	/* SET	4, A */	\
		M80_SET_COMBO(4, A, IXIY + offset);	\
		break;	\
	case 0xE8:	/* SET	5, B */	\
		M80_SET_COMBO(5, B, IXIY + offset);	\
		break;	\
	case 0xE9:	/* SET	5, C */	\
		M80_SET_COMBO(5, C, IXIY + offset);	\
		break;	\
	case 0xEA:	/* SET	5, D */	\
		M80_SET_COMBO(5, D, IXIY + offset);	\
		break;	\
	case 0xEB:	/* SET	5, E */	\
		M80_SET_COMBO(5, E, IXIY + offset);	\
		break;	\
	case 0xEC:	/* SET	5, H */	\
		M80_SET_COMBO(5, H, IXIY + offset);	\
		break;	\
	case 0xED:	/* SET	5, L */	\
		M80_SET_COMBO(5, L, IXIY + offset);	\
		break;	\
	case 0xEE:	\
		{	\
			Uint8 val = M80_READ_BYTE(IXIY + offset);	\
			M80_SET(5, val);	\
			M80_WRITE_BYTE(IXIY + offset, val);	\
		}	\
		break;	\
	case 0xEF:	/* SET	5, A */	\
		M80_SET_COMBO(5, A, IXIY + offset);	\
		break;	\
	case 0xF0:	/* SET	6, B */	\
		M80_SET_COMBO(6, B, IXIY + offset);	\
		break;	\
	case 0xF1:	/* SET	6, C */	\
		M80_SET_COMBO(6, C, IXIY + offset);	\
		break;	\
	case 0xF2:	/* SET	6, D */	\
		M80_SET_COMBO(6, D, IXIY + offset);	\
		break;	\
	case 0xF3:	/* SET	6, E */	\
		M80_SET_COMBO(6, E, IXIY + offset);	\
		break;	\
	case 0xF4:	/* SET	6, H */	\
		M80_SET_COMBO(6, H, IXIY + offset);	\
		break;	\
	case 0xF5:	/* SET	6, L */	\
		M80_SET_COMBO(6, L, IXIY + offset);	\
		break;	\
	case 0xF6:	\
		{	\
			Uint8 val = M80_READ_BYTE(IXIY + offset);	\
			M80_SET(6, val);	\
			M80_WRITE_BYTE(IXIY + offset, val);	\
		}	\
		break;	\
	case 0xF7:	/* SET	6, A */	\
		M80_SET_COMBO(6, A, IXIY + offset);	\
		break;	\
	case 0xF8:	/* SET	7, B */	\
		M80_SET_COMBO(7, B, IXIY + offset);	\
		break;	\
	case 0xF9:	/* SET	7, C */	\
		M80_SET_COMBO(7, C, IXIY + offset);	\
		break;	\
	case 0xFA:	/* SET	7, D */	\
		M80_SET_COMBO(7, D, IXIY + offset);	\
		break;	\
	case 0xFB:	/* SET	7, E */	\
		M80_SET_COMBO(7, E, IXIY + offset);	\
		break;	\
	case 0xFC:	/* SET	7, H */	\
		M80_SET_COMBO(7, H, IXIY + offset);	\
		break;	\
	case 0xFD:	/* SET	7, L */	\
		M80_SET_COMBO(7, L, IXIY + offset);	\
		break;	\
	case 0xFE:	\
		{	\
			Uint8 val = M80_READ_BYTE(IXIY + offset);	\
			M80_SET(7, val);	\
			M80_WRITE_BYTE(IXIY + offset, val);	\
		}	\
		break;	\
	case 0xFF:	/* SET	7, A */	\
		M80_SET_COMBO(7, A, IXIY + offset);	\
		break;	\
	}	\
}

































/* handles DD/FD prefixed instructions */
#define M80_EXEC_DDFD(IXIY)	\
{	\
	Uint8 opcode = M80_GET_ARG;	/* get opcode and increment PC */	\
	Uint16 temp_word;	/* temporary word to move data around with */	\
	g_cycles_executed += ddfd_cycles[opcode];	\
	M80_INC_R;	/* for each DD/FD instruction, increase R at least once */ \
	switch(opcode)	\
	{	\
	case 0: /* NOP */       \
		break;  \
    case 1: /* LD BC, NN */ \
        BC = M80_GET_WORD;      \
        break;  \
    case 2: /* LD (BC), A */        \
        M80_WRITE_BYTE(BC, A);  \
        break;  \
    case 3: /* INC BC */    \
        BC++;   \
        break;  \
    case 4: /* INC B */     \
        M80_INC_REG8(B);        \
        break;  \
    case 5: /* DEC B */     \
        M80_DEC_REG8(B);        \
        break;  \
    case 6: /* LD B, N */   \
        B = M80_GET_ARG;        \
        break;  \
    case 7: /* RLCA */      \
		M80_RLCA();	\
        break;  \
	case 8: /* EX AF, AF' */        \
		M80_EX_AFS();	\
		break;  \
	case 9:	/* ADD IX/IY, BC */	\
		M80_ADD_REGS16(IXIY, BC);	\
		break;	\
	case 0xA:	/* LD A, (BC) */	\
		A = M80_READ_BYTE(BC);	\
		break;	\
	case 0xB:	/* DEC BC */	\
		BC--;	\
		break;	\
	case 0xC:	/* INC C */	\
		M80_INC_REG8(C);	\
		break;	\
	case 0xD:	/* DEC C */	\
		M80_DEC_REG8(C);	\
		break;	\
	case 0xE:	/* LD C, N */	\
		C = M80_GET_ARG;	\
		break;	\
	case 0xF:	/* RRCA */	\
		M80_RRCA();	\
		break;	\
	case 0x10:	/* DJNZ $+2 */	\
		B--;	\
		M80_BRANCH_COND (B != 0);	\
		break;	\
	case 0x11:	/* LD DE,NN */	\
		DE = M80_GET_WORD;	\
		break;	\
	case 0x12:	/* LD (DE), A */	\
		M80_WRITE_BYTE(DE, A);	\
		break;	\
	case 0x13:	/* INC DE */	\
		DE++;	\
		break;	\
	case 0x14:	/* INC D */	\
		M80_INC_REG8(D);	\
		break;	\
	case 0x15:	/* DEC D */	\
		M80_DEC_REG8(D);	\
		break;	\
	case 0x16:	/* LD D, N */	\
		D = M80_GET_ARG;	\
		break;	\
	case 0x17:	/* RLA */	\
		M80_RLA();	\
		break;	\
	case 0x18:	/* JR $N+2 */	\
		M80_BRANCH();	\
		break;	\
	case 0x19:	/* ADD IX/IY, DE */	\
		M80_ADD_REGS16(IXIY, DE);	\
		break;	\
	case 0x1A:	/* LD A, (DE) */	\
		A = M80_READ_BYTE(DE);	\
		break;	\
	case 0x1B:	/* DEC DE */	\
		DE--;	\
		break;	\
	case 0x1C:	/* INC E */	\
		M80_INC_REG8(E);	\
		break;	\
	case 0x1D:	/* DEC E */	\
		M80_DEC_REG8(E);	\
		break;	\
	case 0x1E:	/* LD E, N */	\
		E = M80_GET_ARG;	\
		break;	\
	case 0x1F:	/* RRA */	\
		M80_RRA();	\
		break;	\
	case 0x20:	/* JR NZ,$+2 */	\
		M80_BRANCH_COND ((FLAGS & Z_FLAG) == 0);	\
		break;	\
	case 0x21:	/* LD IX/IY, NN */	\
		IXIY = M80_GET_WORD;	\
		break;	\
	case 0x22:	/* LD (NN), IX/IY */	\
		temp_word = M80_PEEK_WORD;	\
		PC += 2;	\
		M80_WRITE_WORD(temp_word, M80_ ## IXIY);	\
		break;	\
	case 0x23:	/* INC IX/IY */	\
		IXIY++;	\
		break;	\
	case 0x24:	/* INC IXh/IYh */	\
		M80_INC_REG8(IXIY ## h);	\
		break;	\
	case 0x25:	/* DEC IXh/IYh */	\
		M80_DEC_REG8(IXIY ## h);	\
		break;	\
	case 0x26:	/* LD IXh/IYh, N */	\
		IXIY ## h = M80_GET_ARG;	\
		break;	\
	case 0x27:	/* DAA */	\
		M80_DAA();	\
		break;	\
	case 0x28:	/* JR Z, $+2 */	\
		M80_BRANCH_COND (FLAGS & Z_FLAG);	\
		break;	\
	case 0x29:	/* ADD IXIY, IXIY */	\
		M80_ADD_REGS16(IXIY, IXIY);	\
		break;	\
	case 0x2A:	/* LD IXIY, (NN) */	\
		temp_word = M80_PEEK_WORD;	\
		PC += 2;	\
		M80_READ_WORD(temp_word, M80_ ## IXIY);	\
		break;	\
	case 0x2B:	/* DEC IXIY */	\
		IXIY--;	\
		break;	\
	case 0x2C:	/* INC IXl/IYl */	\
		M80_INC_REG8(IXIY ## l);	\
		break;	\
	case 0x2D:	/* DEC IXl/IYl */	\
		M80_DEC_REG8(IXIY ## l);	\
		break;	\
	case 0x2E:	/* LD IXl/IYl, N */	\
		IXIY ## l = M80_GET_ARG;	\
		break;	\
	case 0x2F:	/* CPL, XOR's accumulator by 0xFF */	\
		M80_CPL();	\
		break;	\
	case 0x30:	/* JR NC, $+2 */	\
		M80_BRANCH_COND (!(FLAGS & C_FLAG));	/* if Carry flag is clear, branch */	\
		break;	\
	case 0x31:	/* LD SP, NN */	\
		SP = M80_GET_WORD;	\
		break;	\
	case 0x32:	/* LD (NN), A */	\
		temp_word = M80_PEEK_WORD;	\
		PC += 2;	\
		M80_WRITE_BYTE(temp_word, A);	\
		break;	\
	case 0x33:	/* INC SP */	\
		SP++;	\
		break;	\
	case 0x34:	/* INC (IXIY + d) */	\
		{	\
			Uint8 temp = M80_READ_BYTE(IXIY_OFFSET_PEEK(IXIY));	\
			M80_INC_REG8(temp);	\
			M80_WRITE_BYTE(IXIY_OFFSET(IXIY), temp);	\
		}	\
		break;	\
	case 0x35:	/* DEC (IXIY + d) */	\
		{	\
			Uint8 temp = M80_READ_BYTE(IXIY_OFFSET_PEEK(IXIY));	\
			M80_DEC_REG8(temp);	\
			M80_WRITE_BYTE(IXIY_OFFSET(IXIY), temp);	\
		}	\
		break;	\
	case 0x36:	/* LD (IXIY + d), N */	\
		{	\
			Uint16 addr = IXIY_OFFSET(IXIY);	\
			M80_WRITE_BYTE(addr, M80_GET_ARG);	\
		}	\
		break;	\
	case 0x37:	/* SCF (Set Carry Flag) */	\
		M80_SCF();	\
		break;	\
	case 0x38:	/* JR C, $+2 */	\
		M80_BRANCH_COND (FLAGS & C_FLAG);	\
		break;	\
	case 0x39:	/* ADD IXIY, SP */	\
		M80_ADD_REGS16(IXIY, SP);	\
		break;	\
	case 0x3A:	/* LD A, (NN) */	\
		temp_word = M80_PEEK_WORD;	\
		PC += 2;	\
		A = M80_READ_BYTE(temp_word);	\
		break;	\
	case 0x3B:	/* DEC SP */	\
		SP--;	\
		break;	\
	case 0x3C:	/* INC A */	\
		M80_INC_REG8(A);	\
		break;	\
	case 0x3D:	/* DEC A */	\
		M80_DEC_REG8(A);	\
		break;	\
	case 0x3E:	/* LD A, N */	\
		A = M80_GET_ARG;	\
		break;	\
	case 0x3F:	/* CCF	complement carry flag */	\
		M80_CCF();	\
		break;	\
	case 0x40:	/*LD B,B	(nop) */	\
		break;	\
	case 0x41:	/* LD B,C */	\
		B = C;	\
		break;	\
	case 0x42:	/* LD B,D */	\
		B = D;	\
		break;	\
	case 0x43:	/* LD B,E */	\
		B = E;	\
		break;	\
	case 0x44:	/* LD B,IXh/IYh */	\
		B = IXIY ## h;	\
		break;	\
	case 0x45:	/* LD B,IXl/IYl */	\
		B = IXIY ## l;	\
		break;	\
	case 0x46:	/* LD B,(IXIY + d) */	\
		B = M80_READ_BYTE(IXIY_OFFSET(IXIY));	\
		break;	\
	case 0x47:	/* LD B,A */	\
		B = A;	\
		break;	\
	case 0x48:	/* LD C,B */	\
		C = B;	\
		break;	\
	case 0x49:	/* LD C,C (NOP) */	\
		break;	\
	case 0x4A:	/* LD C,D */	\
		C = D;	\
		break;	\
	case 0x4B:	/* LD C,E */	\
		C = E;	\
		break;	\
	case 0x4C:	/* LD C,IXh/IYh */	\
		C = IXIY ## h;	\
		break;	\
	case 0x4D:	/* LD C,IXl/IYl */	\
		C = IXIY ## l;	\
		break;	\
	case 0x4E:	/* LD C,(IXIY + d) */	\
		C = M80_READ_BYTE(IXIY_OFFSET(IXIY));	\
		break;	\
	case 0x4F:	/* LD C,A */	\
		C = A;	\
		break;	\
	case 0x50:	/* LD D,B */	\
		D = B;	\
		break;	\
	case 0x51:	/* LD D,C */	\
		D = C;	\
		break;	\
	case 0x52:	/* LD D,D */	\
		break;	\
	case 0x53:	/* LD D,E */	\
		D = E;	\
		break;	\
	case 0x54:	/* LD D,IXh/IYh */	\
		D = IXIY ## h;	\
		break;	\
	case 0x55:	/* LD D,IXl/IYl */	\
		D = IXIY ## l;	\
		break;	\
	case 0x56:	/* LD D,(IXIY + d) */	\
		D = M80_READ_BYTE(IXIY_OFFSET(IXIY));	\
		break;	\
	case 0x57:	/* LD D,A */	\
		D = A;	\
		break;	\
	case 0x58:	/* LD E,B */	\
		E = B;	\
		break;	\
	case 0x59:	/* LD E,C */	\
		E = C;	\
		break;	\
	case 0x5A:	/* LD E,D */	\
		E = D;	\
		break;	\
	case 0x5B:	/* LD E,E */	\
		/* nop */	\
		break;	\
	case 0x5C:	/* LD E, IXh/IYh */	\
		E = IXIY ## h;	\
		break;	\
	case 0x5D:	/* LD E, IXl/IYl */	\
		E = IXIY ## l;	\
		break;	\
	case 0x5E:	/* LD E, (IXIY + d) */	\
		E = M80_READ_BYTE(IXIY_OFFSET(IXIY));	\
		break;	\
	case 0x5F:	/* LD E,A */	\
		E = A;	\
		break;	\
	case 0x60:	/* LD IXh/IYh,B */	\
		IXIY ## h = B;	\
		break;	\
	case 0x61:	/* LD IXh/IYh,C */	\
		IXIY ## h = C;	\
		break;	\
	case 0x62:	/* LD IXh/IYh,D */	\
		IXIY ## h = D;	\
		break;	\
	case 0x63:	/* LD IXh/IYh,E */	\
		IXIY ## h = E;	\
		break;	\
	case 0x64:	/* LD IXh/IYh */	\
		/* nop */	\
		break;	\
	case 0x65:	/* LD IXh/IYh, IXl/IYl */	\
		IXIY ## h = IXIY ## l;	\
		break;	\
	case 0x66:	/* LD H, (IXIY + d) */	\
		H = M80_READ_BYTE(IXIY_OFFSET(IXIY));	\
		break;	\
	case 0x67:	/* LD IXh/IYh, A */	\
		IXIY ## h = A;	\
		break;	\
	case 0x68:	/* LD IXl/IYl, B */	\
		IXIY ## l = B;	\
		break;	\
	case 0x69:	/* LD IXl/IYl, C */	\
		IXIY ## l = C;	\
		break;	\
	case 0x6A:	/* LD IXl/IYl, D */	\
		IXIY ## l = D;	\
		break;	\
	case 0x6B:	/* LD IXIY ## l, E */	\
		IXIY ## l = E;	\
		break;	\
	case 0x6C:	/* LD IXl/IYl, IXIY ## h */	\
		IXIY ## l = IXIY ## h;	\
		break;	\
	case 0x6D:	/* LD IXl, IXl */	\
		/* nop */	\
		break;	\
	case 0x6E:	/* LD L, (IXIY + d) */	\
		L = M80_READ_BYTE(IXIY_OFFSET(IXIY));	\
		break;	\
	case 0x6F:	/* LD IXl/IYl, A */	\
		IXIY ## l = A;	\
		break;	\
	case 0x70:	/* LD (IXIY + d), B */	\
		M80_WRITE_BYTE(IXIY_OFFSET(IXIY), B);	\
		break;	\
	case 0x71:	/* LD (IXIY + d), C */	\
		M80_WRITE_BYTE(IXIY_OFFSET(IXIY), C);	\
		break;	\
	case 0x72:	/* LD (IXIY + d), D */	\
		M80_WRITE_BYTE(IXIY_OFFSET(IXIY), D);	\
		break;	\
	case 0x73:	/* LD (IXIY + d), E */	\
		M80_WRITE_BYTE(IXIY_OFFSET(IXIY), E);	\
		break;	\
	case 0x74:	/* LD (IXIY + d), H */	\
		M80_WRITE_BYTE(IXIY_OFFSET(IXIY), H);	\
		break;	\
	case 0x75:	/* LD (IXIY + d), L */	\
		M80_WRITE_BYTE(IXIY_OFFSET(IXIY), L);	\
		break;	\
	case 0x76:	/* HALT (waits for an interrupt) */	\
		M80_START_HALT;	\
		break;	\
	case 0x77:	/* LD (IXIY + d), A */	\
		M80_WRITE_BYTE(IXIY_OFFSET(IXIY), A);	\
		break;	\
	case 0x78:	/* LD A,B */	\
		A = B;	\
		break;	\
	case 0x79:	/* LD A,C */	\
		A = C;	\
		break;	\
	case 0x7A:	/* LD A,D */	\
		A = D;	\
		break;	\
	case 0x7B:	/* LD A,E */	\
		A = E;	\
		break;	\
	case 0x7C:	/* LD A,IXIY ## h */	\
		A = IXIY ## h;	\
		break;	\
	case 0x7D:	/* LD A,IXIY ## l */	\
		A = IXIY ## l;	\
		break;	\
	case 0x7E:	/* LD A, (IXIY + d) */	\
		A = M80_READ_BYTE(IXIY_OFFSET(IXIY));	\
		break;	\
	case 0x84:	/* ADD A,IXIY ## h */	\
		M80_ADD_TO_A(IXIY ## h);	\
		break;	\
	case 0x85:	/* ADD A,IXIY ## l */	\
		M80_ADD_TO_A(IXIY ## l);	\
		break;	\
	case 0x86:	/* ADD A,(IXIY + d) */	\
		M80_ADD_TO_A(M80_READ_BYTE(IXIY_OFFSET_PEEK(IXIY)));	\
		PC++;	\
		break;	\
	case 0x87:	/* ADD A,A */	\
		M80_ADD_TO_A(A);	\
		break;	\
	case 0x88:	/* ADC A,B */	\
		M80_ADC_TO_A(B);	\
		break;	\
	case 0x89:	/* ADC A,C */	\
		M80_ADC_TO_A(C);	\
		break;	\
	case 0x8A:	/* ADC A,D */	\
		M80_ADC_TO_A(D);	\
		break;	\
	case 0x8B:	/* ADC A,E */	\
		M80_ADC_TO_A(E);	\
		break;	\
	case 0x8C:	/* ADC A,IXIY ## h */	\
		M80_ADC_TO_A(IXIY ## h);	\
		break;	\
	case 0x8D:	/* ADC A,IXIY ## l */	\
		M80_ADC_TO_A(IXIY ## l);	\
		break;	\
	case 0x8E:	/* ADC A,(IXIY + d) */	\
		M80_ADC_TO_A(M80_READ_BYTE(IXIY_OFFSET_PEEK(IXIY)));	\
		PC++;	\
		break;	\
	case 0x8F:	/* ADC A,A */	\
		M80_ADC_TO_A(A);	\
		break;	\
	case 0x90:	/* SUB B */	\
		M80_SUB_FROM_A(B);	\
		break;	\
	case 0x91:	/* SUB C */	\
		M80_SUB_FROM_A(C);	\
		break;	\
	case 0x92:	/* SUB D */	\
		M80_SUB_FROM_A(D);	\
		break;	\
	case 0x93:	/* SUB E */	\
		M80_SUB_FROM_A(E);	\
		break;	\
	case 0x94:	/* SUB IXIY ## h */	\
		M80_SUB_FROM_A(IXIY ## h);	\
		break;	\
	case 0x95:	/* SUB IXIY ## l */	\
		M80_SUB_FROM_A(IXIY ## l);	\
		break;	\
	case 0x96:	/* SUB (IXIY + d) */	\
		M80_SUB_FROM_A(M80_READ_BYTE(IXIY_OFFSET_PEEK(IXIY)));	\
		PC++;	\
		break;	\
	case 0x97:	/* SUB A */	\
		M80_SUB_FROM_A(A);	\
		break;	\
	case 0x98:	/* SBC A,B */	\
		M80_SBC_FROM_A(B);	\
		break;	\
	case 0x99:	/* SBC A,C */	\
		M80_SBC_FROM_A(C);	\
		break;	\
	case 0x9A:	/* SBC A,D */	\
		M80_SBC_FROM_A(D);	\
		break;	\
	case 0x9B:	/* SBC A,E */	\
		M80_SBC_FROM_A(E);	\
		break;	\
	case 0x9C:	/* SBC A,IXIY ## h */	\
		M80_SBC_FROM_A(IXIY ## h);	\
		break;	\
	case 0x9D:	/* SBC A,IXIY ## l */	\
		M80_SBC_FROM_A(IXIY ## l);	\
		break;	\
	case 0x9E:	/* SBC A, (IXIY + d) */	\
		M80_SBC_FROM_A(M80_READ_BYTE(IXIY_OFFSET_PEEK(IXIY)));	\
		PC++;	\
		break;	\
	case 0x9F:	/* SBC A,A */	\
		M80_SBC_FROM_A(A);	\
		break;	\
	case 0xA0:	/* AND B */	\
		M80_AND_WITH_A(B);	\
		break;	\
	case 0xA1:	/* AND C */	\
		M80_AND_WITH_A(C);	\
		break;	\
	case 0xA2:	/* AND D */	\
		M80_AND_WITH_A(D);	\
		break;	\
	case 0xA3:	/* AND E */	\
		M80_AND_WITH_A(E);	\
		break;	\
	case 0xA4:	/* AND IXIY ## h */	\
		M80_AND_WITH_A(IXIY ## h);	\
		break;	\
	case 0xA5:	/* AND IXIY ## l */	\
		M80_AND_WITH_A(IXIY ## l);	\
		break;	\
	case 0xA6:	/* AND (IXIY + d) */	\
		M80_AND_WITH_A(M80_READ_BYTE(IXIY_OFFSET_PEEK(IXIY)));	\
		PC++;	\
		break;	\
	case 0xA7:	/* AND A */	\
		M80_AND_WITH_A(A);	\
		break;	\
	case 0xA8:	/* XOR B */	\
		M80_XOR_WITH_A(B);	\
		break;	\
	case 0xA9:	/* XOR C */	\
		M80_XOR_WITH_A(C);	\
		break;	\
	case 0xAA:	/* XOR D */	\
		M80_XOR_WITH_A(D);	\
		break;	\
	case 0xAB:	/* XOR E */	\
		M80_XOR_WITH_A(E);	\
		break;	\
	case 0xAC:	/* XOR IXIY ## h */	\
		M80_XOR_WITH_A(IXIY ## h);	\
		break;	\
	case 0xAD:	/* XOR IXIY ## l */	\
		M80_XOR_WITH_A(IXIY ## l);	\
		break;	\
	case 0xAE:	/* XOR (IXIY + d) */	\
		M80_XOR_WITH_A(M80_READ_BYTE(IXIY_OFFSET_PEEK(IXIY)));	\
		PC++;	\
		break;	\
	case 0xAF:	/* XOR A */	\
		A = 0;	/* XOR'ing a register with itself produces 0 */	\
		FLAGS = Z_FLAG | P_FLAG; /* signed=clear, Zero=set, HC=clear, parity is even, N=clear, C=clear */	\
		break;	\
	case 0xB0:	/* OR B */	\
		M80_OR_WITH_A(B);	\
		break;	\
	case 0xB1:	/* OR C */	\
		M80_OR_WITH_A(C);	\
		break;	\
	case 0xB2:	/* OR D */	\
		M80_OR_WITH_A(D);	\
		break;	\
	case 0xB3:	/* OR E */	\
		M80_OR_WITH_A(E);	\
		break;	\
	case 0xB4:	/* OR IXIY ## h */	\
		M80_OR_WITH_A(IXIY ## h);	\
		break;	\
	case 0xB5:	/* OR IXIY ## l */	\
		M80_OR_WITH_A(IXIY ## l);	\
		break;	\
	case 0xB6:	/* OR (IXIY + d) */	\
		M80_OR_WITH_A(M80_READ_BYTE(IXIY_OFFSET_PEEK(IXIY)));	\
		PC++;	\
		break;	\
	case 0xB7:	/* OR A */	\
		M80_OR_WITH_A(A);	\
		break;	\
	case 0xB8:	/* Compare B */	\
		M80_COMPARE_WITH_A(B);	\
		break;	\
	case 0xB9:	/* CP C */	\
		M80_COMPARE_WITH_A(C);	\
		break;	\
	case 0xBA:	/* CP D */	\
		M80_COMPARE_WITH_A(D);	\
		break;	\
	case 0xBB:	/* CP E */	\
		M80_COMPARE_WITH_A(E);	\
		break;	\
	case 0xBC:	/* CP IXIY ## h */	\
		M80_COMPARE_WITH_A(IXIY ## h);	\
		break;	\
	case 0xBD:	/* CP IXIY ## l */	\
		M80_COMPARE_WITH_A(IXIY ## l);	\
		break;	\
	case 0xBE:	/* CP (IXIY + d) */	\
		M80_COMPARE_WITH_A(M80_READ_BYTE(IXIY_OFFSET_PEEK(IXIY)));	\
		PC++;	\
		break;	\
	case 0xBF:	/* CP A */	\
		M80_COMPARE_WITH_A(A);	\
		break;	\
	case 0xC0:	/* Return if Z_FLAG is not set */	\
		M80_RET_COND((FLAGS & Z_FLAG) == 0);	\
		break;	\
	case 0xC1:	/* POP top of stack into BC  */	\
		M80_POP16(M80_BC);	\
		break;	\
	case 0xC2:	/* Jump if Not Z_FLAG to nnnn */	\
		M80_JUMP_COND((FLAGS & Z_FLAG) == 0);	\
		break;	\
	case 0xC3:	/* unconditional Jump to nnnn */	\
		M80_JUMP();	\
		break;	\
	case 0xC4:	/* Call nnnn if not Z_FLAG */	\
		M80_CALL_COND((FLAGS & Z_FLAG) == 0);	\
		break;	\
	case 0xC5:	/* PUSH BC */	\
		M80_PUSH16(M80_BC);	\
		break;	\
	case 0xC6:	/* ADD A, nn */	\
		M80_ADD_TO_A(M80_PEEK_ARG);	\
		PC++;	\
		break;	\
	case 0xC7:	/* RST 0  (Reset 0) */	\
		M80_RST(0);	\
		break;	\
	case 0xC8:	/* RET Z (Return if Z_Flag is set) */	\
		M80_RET_COND (FLAGS & Z_FLAG);	\
		break;	\
	case 0xC9:	/* unconditional RET */	\
		M80_RET;	\
		break;	\
	case 0xCA:	/* JP Z, nnnn  (Jump if Z_FLAG is set) */	\
		M80_JUMP_COND (FLAGS & Z_FLAG);	\
		break;	\
	case 0xCB:	/* there are a ton of "DD/FD CB" instructions */	\
		M80_EXEC_DDFD_CB(IXIY);	\
		break;	\
	case 0xCC:	/* CALL Z, nnnn (Call function if Z_FLAG is set) */	\
		M80_CALL_COND (FLAGS & Z_FLAG);	\
		break;	\
	case 0xCD:	/*  unconditional CALL */	\
		M80_CALL();	\
		break;	\
	case 0xCE:	/* ADC A, nn */	\
		M80_ADC_TO_A(M80_PEEK_ARG);	\
		PC++;	\
		break;	\
	case 0xCF:	/* RST 8 */	\
		M80_RST(8);	\
		break;	\
	case 0xD0:	/* RET NC (return if carry flag is clear) */	\
		M80_RET_COND ((FLAGS & C_FLAG) == 0);	\
		break;	\
	case 0xD1:	/* POP DE */	\
		M80_POP16(M80_DE);	\
		break;	\
	case 0xD2:	/* JP NC, nnnn (absolute jump if C_FLAG is clear) */	\
		M80_JUMP_COND ((FLAGS & C_FLAG) == 0);	\
		break;	\
	case 0xD3:	/* OUT (nn), A	Send A to the specified port */	\
		M80_OUT_A(M80_GET_ARG);	\
		break;	\
	case 0xD4:	/* CALL NC, nnnn	(call if C_FLAG is clear) */	\
		M80_CALL_COND ((FLAGS & C_FLAG) == 0);	\
		break;	\
	case 0xD5:	/* PUSH DE */	\
		M80_PUSH16(M80_DE);	\
		break;	\
	case 0xD6:	/* SUB nn */	\
		M80_SUB_FROM_A(M80_PEEK_ARG);	\
		PC++;	\
		break;	\
	case 0xD7:	/* RST 0x10 */	\
		M80_RST(0x10);	\
		break;	\
	case 0xD8:	/* RET C (return if C_FLAG is set) */	\
		M80_RET_COND (FLAGS & C_FLAG);	\
		break;	\
	case 0xD9:	/* EXX (Exchange all registers with their counterparts, except AF) */	\
		M80_EXX();	\
		break;	\
	case 0xDA:	/* JP C, nnnn	Absolute jump if Carry is set */	\
		M80_JUMP_COND (FLAGS & C_FLAG);	\
		break;	\
	case 0xDB:	/* IN A, (nn) */	\
		M80_IN_A(M80_GET_ARG);	\
		break;	\
	case 0xDC:	/* CALL C, nnnn	Call if carry is set */	\
		M80_CALL_COND (FLAGS & C_FLAG);	\
		break;	\
	case 0xDD:	/* We should NEVER get this far.  If we do, it's a bug in our emu */	\
		M80_ERROR("Arrived at a second 0xDD inside the 0xFD/0xDD loop!  WTF?");	\
		break;	\
	case 0xDE:	/*	SBC A, nn */	\
		M80_SBC_FROM_A(M80_PEEK_ARG);	\
		PC++;	\
		break;	\
	case 0xDF:	/* RST 0x18 */	\
		M80_RST(0x18);	\
		break;	\
	case 0xE0:	/* RET PO	Return of Parity is Odd (parity flag cleared) */	\
		M80_RET_COND ((FLAGS & P_FLAG) == 0);	\
		break;	\
	case 0xE1:	/* POP IXIY */	\
		M80_POP16(M80_ ## IXIY);	\
		break;	\
	case 0xE2:	/* JP PO, nnnn	(absolute jump of parity is odd, P_FLAG cleared) */	\
		M80_JUMP_COND ((FLAGS & P_FLAG) == 0);	\
		break;	\
	case 0xE3:	/* EX (SP), IXIY	Exchange IXIY with what's stored in memory at SP */	\
		{	\
			pair temp;	\
			temp.w = IXIY;	\
			M80_READ_WORD(SP, M80_ ## IXIY);	\
			M80_WRITE_BYTE(SP, temp.b.l);	\
			M80_WRITE_BYTE(SP+1, temp.b.h);	\
		}	\
		break;	\
	case 0xE4:	/* CALL PO, nnnn	Call if P_FLAG is cleared */	\
		M80_CALL_COND ((FLAGS & P_FLAG) == 0);	\
		break;	\
	case 0xE5:	/* PUSH IXIY */	\
		M80_PUSH16(M80_ ## IXIY);	\
		break;	\
	case 0xE6:	/* AND nn */	\
		M80_AND_WITH_A(M80_PEEK_ARG);	\
		PC++;	\
		break;	\
	case 0xE7:	/* RST 0x20 */	\
		M80_RST(0x20);	\
		break;	\
	case 0xE8:	/* RET PE (return if parity is even/P_FLAG is set) */	\
		M80_RET_COND (FLAGS & P_FLAG);	\
		break;	\
	case 0xE9:	/* JP IXIY	jump to the address contained in IXIY */	\
		PC = IXIY;	\
		M80_CHANGE_PC(PC);	\
		break;	\
	case 0xEA:	/*	JP PE, nnnn	(absolute jump if parity is even) */	\
		M80_JUMP_COND (FLAGS & P_FLAG);	\
		break;	\
	case 0xEB:	/* EX DE, HL	(swap DE and HL) */	\
		M80_EX_DEHL();	\
		break;	\
	case 0xEC:	/* CALL PE, nnnn	(call if parity is even) */	\
		M80_CALL_COND (FLAGS & P_FLAG);	\
		break;	\
	case 0xED:	/* a whole new block of ED instructions */	\
		M80_EXEC_ED;	\
		break;	\
	case 0xEE:	/* XOR nn */	\
		M80_XOR_WITH_A(M80_PEEK_ARG);	\
		PC++;	\
		break;	\
	case 0xEF:	/* RST 0x28 */	\
		M80_RST(0x28);	\
		break;	\
	case 0xF0:	/* RET P	return if positive (S_FLAG is cleared) */	\
		M80_RET_COND ((FLAGS & S_FLAG) == 0);	\
		break;	\
	case 0xF1:	/* POP AF */	\
		M80_POP16(M80_AF);	\
		break;	\
	case 0xF2:	/* JP P, nnnn	Jump if positive */	\
		M80_JUMP_COND ((FLAGS & S_FLAG) == 0);	\
		break;	\
	case 0xF3:	/* DI (Disable Interrupts) */	\
		M80_DI;	\
		break;	\
	case 0xF4:	/* CALL P, nnnn	Call if positive (S_FLAG cleared) */	\
		M80_CALL_COND ((FLAGS & S_FLAG) == 0);	\
		break;	\
	case 0xF5:	/* PUSH AF */	\
		M80_PUSH16(M80_AF);	\
		break;	\
	case 0xF6:	/* OR nn */	\
		M80_OR_WITH_A(M80_PEEK_ARG);	\
		PC++;	\
		break;	\
	case 0xF7:	/* RST 0x30 */	\
		M80_RST(0x30);	\
		break;	\
	case 0xF8:	/* RET M	Return if negative (S_FLAG set) */	\
		M80_RET_COND(FLAGS & S_FLAG);	\
		break;	\
	case 0xF9:	/* LD SP, IXIY	transfer IXIY to SP */	\
		SP = IXIY;	\
		break;	\
	case 0xFA:	/* JP M, nnnn	Jump if Minus sign */	\
		M80_JUMP_COND (FLAGS & S_FLAG);	\
		break;	\
	case 0xFB:	/* EI (enable interrupts) */	\
		M80_EI;	\
		break;	\
	case 0xFC:	/* CALL M, nnnn */	\
		M80_CALL_COND (FLAGS & S_FLAG);	\
		break;	\
	case 0xFD:	/* We should NEVER get here. If we do, it's a bug in our code */	\
		M80_ERROR("Got to second 0xFD inside 0xDD/0xFD loop!  BUG!");	\
		break;	\
	case 0xFE:	/* CP n */	\
		M80_COMPARE_WITH_A(M80_PEEK_ARG);	\
		PC++;	\
		break;	\
	case 0xFF:	/* RST 0x38 */	\
		M80_RST(0x38);	\
		break;	\
	} /* end switch */	\
} /* end macro */























/* executes ONE instruction, incrementing PC and g_cycles_executed variable appropriately */
#define M80_EXEC_CUR_INSTR	\
{	\
	Uint8 opcode = M80_GET_ARG;	/* get opcode and increment PC */	\
	Uint16 temp_word;	\
	g_cycles_executed += op_cycles[opcode];	\
	M80_INC_R;	/* for each instruction, increase R at least once */ \
	switch(opcode)	\
	{	\
	case 0:	/* NOP */	\
		break;	\
	case 1:	/* LD BC, NN */	\
		BC = M80_GET_WORD;	\
		break;	\
	case 2:	/* LD (BC), A */	\
		M80_WRITE_BYTE(BC, A);	\
		break;	\
	case 3:	/* INC BC */	\
		BC++;	\
		break;	\
	case 4: /* INC B */	\
		M80_INC_REG8(B);	\
		break;	\
	case 5:	/* DEC B */	\
		M80_DEC_REG8(B);	\
		break;	\
	case 6:	/* LD B, N */	\
		B = M80_GET_ARG;	\
		break;	\
	case 7:	/* RLCA */	\
		M80_RLCA();	\
		break;	\
	case 8:	/* EX AF, AF' */	\
		M80_EX_AFS();	\
		break;	\
	case 9:	/* ADD HL, BC */	\
		M80_ADD_REGS16(HL, BC);	\
		break;	\
	case 0xA:	/* LD A, (BC) */	\
		A = M80_READ_BYTE(BC);	\
		break;	\
	case 0xB:	/* DEC BC */	\
		BC--;	\
		break;	\
	case 0xC:	/* INC C */	\
		M80_INC_REG8(C);	\
		break;	\
	case 0xD:	/* DEC C */	\
		M80_DEC_REG8(C);	\
		break;	\
	case 0xE:	/* LD C, N */	\
		C = M80_GET_ARG;	\
		break;	\
	case 0xF:	/* RRCA */	\
		M80_RRCA();	\
		break;	\
	case 0x10:	/* DJNZ $+2 */	\
		B--;	\
		M80_BRANCH_COND (B != 0);	\
		break;	\
	case 0x11:	/* LD DE,NN */	\
		DE = M80_GET_WORD;	\
		break;	\
	case 0x12:	/* LD (DE), A */	\
		M80_WRITE_BYTE(DE, A);	\
		break;	\
	case 0x13:	/* INC DE */	\
		DE++;	\
		break;	\
	case 0x14:	/* INC D */	\
		M80_INC_REG8(D);	\
		break;	\
	case 0x15:	/* DEC D */	\
		M80_DEC_REG8(D);	\
		break;	\
	case 0x16:	/* LD D, N */	\
		D = M80_GET_ARG;	\
		break;	\
	case 0x17:	/* RLA */	\
		M80_RLA();	\
		break;	\
	case 0x18:	/* JR $N+2 */	\
		M80_BRANCH();	\
		break;	\
	case 0x19:	/* ADD HL, DE */	\
		M80_ADD_REGS16(HL, DE);	\
		break;	\
	case 0x1A:	/* LD A, (DE) */	\
		A = M80_READ_BYTE(DE);	\
		break;	\
	case 0x1B:	/* DEC DE */	\
		DE--;	\
		break;	\
	case 0x1C:	/* INC E */	\
		M80_INC_REG8(E);	\
		break;	\
	case 0x1D:	/* DEC E */	\
		M80_DEC_REG8(E);	\
		break;	\
	case 0x1E:	/* LD E, N */	\
		E = M80_GET_ARG;	\
		break;	\
	case 0x1F:	/* RRA */	\
		M80_RRA();	\
		break;	\
	case 0x20:	/* JR NZ,$+2 */	\
		M80_BRANCH_COND ((FLAGS & Z_FLAG) == 0);	\
		break;	\
	case 0x21:	/* LD HL, NN */	\
		HL = M80_GET_WORD;	\
		break;	\
	case 0x22:	/* LD (NN), HL */	\
		temp_word = M80_PEEK_WORD;	\
		PC += 2;	\
		M80_WRITE_WORD(temp_word, M80_HL);	\
		break;	\
	case 0x23:	/* INC HL */	\
		HL++;	\
		break;	\
	case 0x24:	/* INC H */	\
		M80_INC_REG8(H);	\
		break;	\
	case 0x25:	/* DEC H */	\
		M80_DEC_REG8(H);	\
		break;	\
	case 0x26:	/* LD H, N */	\
		H = M80_GET_ARG;	\
		break;	\
	case 0x27:	/* DAA */	\
		M80_DAA();	\
		break;	\
	case 0x28:	/* JR Z, $+2 */	\
		M80_BRANCH_COND (FLAGS & Z_FLAG);	\
		break;	\
	case 0x29:	/* ADD HL, HL */	\
		M80_ADD_REGS16(HL, HL);	\
		break;	\
	case 0x2A:	/* LD HL, (NN) */	\
		temp_word = M80_PEEK_WORD;	\
		PC += 2;	\
		M80_READ_WORD(temp_word, M80_HL);	\
		break;	\
	case 0x2B:	/* DEC HL */	\
		HL--;	\
		break;	\
	case 0x2C:	/* INC L */	\
		M80_INC_REG8(L);	\
		break;	\
	case 0x2D:	/* DEC L */	\
		M80_DEC_REG8(L);	\
		break;	\
	case 0x2E:	/* LD L, N */	\
		L = M80_GET_ARG;	\
		break;	\
	case 0x2F:	/* CPL, XOR's accumulator by 0xFF */	\
		M80_CPL();	\
		break;	\
	case 0x30:	/* JR NC, $+2 */	\
		M80_BRANCH_COND (!(FLAGS & C_FLAG));	/* if Carry flag is clear, branch */	\
		break;	\
	case 0x31:	/* LD SP, NN */	\
		SP = M80_GET_WORD;	\
		break;	\
	case 0x32:	/* LD (NN), A */	\
		temp_word = M80_PEEK_WORD;	\
		PC += 2;	\
		M80_WRITE_BYTE(temp_word, A);	\
		break;	\
	case 0x33:	/* INC SP */	\
		SP++;	\
		break;	\
	case 0x34:	/* INC (HL) */	\
		{	\
			Uint8 temp = M80_READ_BYTE(HL);	\
			M80_INC_REG8(temp);	\
			M80_WRITE_BYTE(HL, temp);	\
		}	\
		break;	\
	case 0x35:	/* DEC (HL) */	\
		{	\
			Uint8 temp = M80_READ_BYTE(HL);	\
			M80_DEC_REG8(temp);	\
			M80_WRITE_BYTE(HL, temp);	\
		}	\
		break;	\
	case 0x36:	/* LD (HL), N */	\
		M80_WRITE_BYTE(HL, M80_GET_ARG);	\
		break;	\
	case 0x37:	/* SCF (Set Carry Flag) */	\
		M80_SCF();	\
		break;	\
	case 0x38:	/* JR C, $+2 */	\
		M80_BRANCH_COND (FLAGS & C_FLAG);	\
		break;	\
	case 0x39:	/* ADD HL, SP */	\
		M80_ADD_REGS16(HL, SP);	\
		break;	\
	case 0x3A:	/* LD A, (NN) */	\
		temp_word = M80_PEEK_WORD;	\
		PC += 2;	\
		A = M80_READ_BYTE(temp_word);	\
		break;	\
	case 0x3B:	/* DEC SP */	\
		SP--;	\
		break;	\
	case 0x3C:	/* INC A */	\
		M80_INC_REG8(A);	\
		break;	\
	case 0x3D:	/* DEC A */	\
		M80_DEC_REG8(A);	\
		break;	\
	case 0x3E:	/* LD A, N */	\
		A = M80_GET_ARG;	\
		break;	\
	case 0x3F:	/* CCF	complement carry flag */	\
		M80_CCF();	\
		break;	\
	case 0x40:	/*LD B,B	(nop) */	\
		break;	\
	case 0x41:	/* LD B,C */	\
		B = C;	\
		break;	\
	case 0x42:	/* LD B,D */	\
		B = D;	\
		break;	\
	case 0x43:	/* LD B,E */	\
		B = E;	\
		break;	\
	case 0x44:	/* LD B,H */	\
		B = H;	\
		break;	\
	case 0x45:	/* LD B,L */	\
		B = L;	\
		break;	\
	case 0x46:	/* LD B,(HL) */	\
		B = M80_READ_BYTE(HL);	\
		break;	\
	case 0x47:	/* LD B,A */	\
		B = A;	\
		break;	\
	case 0x48:	/* LD C,B */	\
		C = B;	\
		break;	\
	case 0x49:	/* LD C,C (NOP) */	\
		break;	\
	case 0x4A:	/* LD C,D */	\
		C = D;	\
		break;	\
	case 0x4B:	/* LD C,E */	\
		C = E;	\
		break;	\
	case 0x4C:	/* LD C,H */	\
		C = H;	\
		break;	\
	case 0x4D:	/* LD C,L */	\
		C = L;	\
		break;	\
	case 0x4E:	/* LD C,(HL) */	\
		C = M80_READ_BYTE(HL);	\
		break;	\
	case 0x4F:	/* LD C,A */	\
		C = A;	\
		break;	\
	case 0x50:	/* LD D,B */	\
		D = B;	\
		break;	\
	case 0x51:	/* LD D,C */	\
		D = C;	\
		break;	\
	case 0x52:	/* LD D,D */	\
		break;	\
	case 0x53:	/* LD D,E */	\
		D = E;	\
		break;	\
	case 0x54:	/* LD D,H */	\
		D = H;	\
		break;	\
	case 0x55:	/* LD D,L */	\
		D = L;	\
		break;	\
	case 0x56:	/* LD D,(HL) */	\
		D = M80_READ_BYTE(HL);	\
		break;	\
	case 0x57:	/* LD D,A */	\
		D = A;	\
		break;	\
	case 0x58:	/* LD E,B */	\
		E = B;	\
		break;	\
	case 0x59:	/* LD E,C */	\
		E = C;	\
		break;	\
	case 0x5A:	/* LD E,D */	\
		E = D;	\
		break;	\
	case 0x5B:	/* LD E,E */	\
		/* nop */	\
		break;	\
	case 0x5C:	/* LD E, H */	\
		E = H;	\
		break;	\
	case 0x5D:	/* LD E, L */	\
		E = L;	\
		break;	\
	case 0x5E:	/* LD E, (HL) */	\
		E = M80_READ_BYTE(HL);	\
		break;	\
	case 0x5F:	/* LD E,A */	\
		E = A;	\
		break;	\
	case 0x60:	/* LD H,B */	\
		H = B;	\
		break;	\
	case 0x61:	/* LD H,C */	\
		H = C;	\
		break;	\
	case 0x62:	/* LD H,D */	\
		H = D;	\
		break;	\
	case 0x63:	/* LD H,E */	\
		H = E;	\
		break;	\
	case 0x64:	/* LD H, H */	\
		/* nop */	\
		break;	\
	case 0x65:	/* LD H, L */	\
		H = L;	\
		break;	\
	case 0x66:	/* LD H, (HL) */	\
		H = M80_READ_BYTE(HL);	\
		break;	\
	case 0x67:	/* LD H, A */	\
		H = A;	\
		break;	\
	case 0x68:	/* LD L, B */	\
		L = B;	\
		break;	\
	case 0x69:	/* LD L, C */	\
		L = C;	\
		break;	\
	case 0x6A:	/* LD L, D */	\
		L = D;	\
		break;	\
	case 0x6B:	/* LD L, E */	\
		L = E;	\
		break;	\
	case 0x6C:	/* LD L, H */	\
		L = H;	\
		break;	\
	case 0x6D:	/* LD L, L */	\
		/* nop */	\
		break;	\
	case 0x6E:	/* LD L, (HL) */	\
		L = M80_READ_BYTE(HL);	\
		break;	\
	case 0x6F:	/* LD L, A */	\
		L = A;	\
		break;	\
	case 0x70:	/* LD (HL), B */	\
		M80_WRITE_BYTE(HL, B);	\
		break;	\
	case 0x71:	/* LD (HL), C */	\
		M80_WRITE_BYTE(HL, C);	\
		break;	\
	case 0x72:	/* LD (HL), D */	\
		M80_WRITE_BYTE(HL, D);	\
		break;	\
	case 0x73:	/* LD (HL), E */	\
		M80_WRITE_BYTE(HL, E);	\
		break;	\
	case 0x74:	/* LD (HL), H */	\
		M80_WRITE_BYTE(HL, H);	\
		break;	\
	case 0x75:	/* LD (HL), L */	\
		M80_WRITE_BYTE(HL, L);	\
		break;	\
	case 0x76:	/* HALT (waits for an interrupt) */	\
		M80_START_HALT;	\
		break;	\
	case 0x77:	/* LD (HL), A */	\
		M80_WRITE_BYTE(HL, A);	\
		break;	\
	case 0x78:	/* LD A,B */	\
		A = B;	\
		break;	\
	case 0x79:	/* LD A,C */	\
		A = C;	\
		break;	\
	case 0x7A:	/* LD A,D */	\
		A = D;	\
		break;	\
	case 0x7B:	/* LD A,E */	\
		A = E;	\
		break;	\
	case 0x7C:	/* LD A,H */	\
		A = H;	\
		break;	\
	case 0x7D:	/* LD A,L */	\
		A = L;	\
		break;	\
	case 0x7E:	/* LD A, (HL) */	\
		A = M80_READ_BYTE(HL);	\
		break;	\
	case 0x7F:	/* LD A,A */	\
		/* nop */	\
		break;	\
	case 0x80:	/* ADD A,B */	\
		M80_ADD_TO_A(B);	\
		break;	\
	case 0x81:	/* ADD A,C */	\
		M80_ADD_TO_A(C);	\
		break;	\
	case 0x82:	/* ADD A,D */	\
		M80_ADD_TO_A(D);	\
		break;	\
	case 0x83:	/* ADD A,E */	\
		M80_ADD_TO_A(E);	\
		break;	\
	case 0x84:	/* ADD A,H */	\
		M80_ADD_TO_A(H);	\
		break;	\
	case 0x85:	/* ADD A,L */	\
		M80_ADD_TO_A(L);	\
		break;	\
	case 0x86:	/* ADD A,(HL) */	\
		M80_ADD_TO_A(M80_READ_BYTE(HL));	\
		break;	\
	case 0x87:	/* ADD A,A */	\
		M80_ADD_TO_A(A);	\
		break;	\
	case 0x88:	/* ADC A,B */	\
		M80_ADC_TO_A(B);	\
		break;	\
	case 0x89:	/* ADC A,C */	\
		M80_ADC_TO_A(C);	\
		break;	\
	case 0x8A:	/* ADC A,D */	\
		M80_ADC_TO_A(D);	\
		break;	\
	case 0x8B:	/* ADC A,E */	\
		M80_ADC_TO_A(E);	\
		break;	\
	case 0x8C:	/* ADC A,H */	\
		M80_ADC_TO_A(H);	\
		break;	\
	case 0x8D:	/* ADC A,L */	\
		M80_ADC_TO_A(L);	\
		break;	\
	case 0x8E:	/* ADC A,(HL) */	\
		M80_ADC_TO_A(M80_READ_BYTE(HL));	\
		break;	\
	case 0x8F:	/* ADC A,A */	\
		M80_ADC_TO_A(A);	\
		break;	\
	case 0x90:	/* SUB B */	\
		M80_SUB_FROM_A(B);	\
		break;	\
	case 0x91:	/* SUB C */	\
		M80_SUB_FROM_A(C);	\
		break;	\
	case 0x92:	/* SUB D */	\
		M80_SUB_FROM_A(D);	\
		break;	\
	case 0x93:	/* SUB E */	\
		M80_SUB_FROM_A(E);	\
		break;	\
	case 0x94:	/* SUB H */	\
		M80_SUB_FROM_A(H);	\
		break;	\
	case 0x95:	/* SUB L */	\
		M80_SUB_FROM_A(L);	\
		break;	\
	case 0x96:	/* SUB (HL) */	\
		M80_SUB_FROM_A(M80_READ_BYTE(HL));	\
		break;	\
	case 0x97:	/* SUB A */	\
		M80_SUB_FROM_A(A);	\
		break;	\
	case 0x98:	/* SBC A,B */	\
		M80_SBC_FROM_A(B);	\
		break;	\
	case 0x99:	/* SBC A,C */	\
		M80_SBC_FROM_A(C);	\
		break;	\
	case 0x9A:	/* SBC A,D */	\
		M80_SBC_FROM_A(D);	\
		break;	\
	case 0x9B:	/* SBC A,E */	\
		M80_SBC_FROM_A(E);	\
		break;	\
	case 0x9C:	/* SBC A,H */	\
		M80_SBC_FROM_A(H);	\
		break;	\
	case 0x9D:	/* SBC A,L */	\
		M80_SBC_FROM_A(L);	\
		break;	\
	case 0x9E:	/* SBC A, (HL) */	\
		M80_SBC_FROM_A(M80_READ_BYTE(HL));	\
		break;	\
	case 0x9F:	/* SBC A,A */	\
		M80_SBC_FROM_A(A);	\
		break;	\
	case 0xA0:	/* AND B */	\
		M80_AND_WITH_A(B);	\
		break;	\
	case 0xA1:	/* AND C */	\
		M80_AND_WITH_A(C);	\
		break;	\
	case 0xA2:	/* AND D */	\
		M80_AND_WITH_A(D);	\
		break;	\
	case 0xA3:	/* AND E */	\
		M80_AND_WITH_A(E);	\
		break;	\
	case 0xA4:	/* AND H */	\
		M80_AND_WITH_A(H);	\
		break;	\
	case 0xA5:	/* AND L */	\
		M80_AND_WITH_A(L);	\
		break;	\
	case 0xA6:	/* AND (HL) */	\
		M80_AND_WITH_A(M80_READ_BYTE(HL));	\
		break;	\
	case 0xA7:	/* AND A */	\
		M80_AND_WITH_A(A);	\
		break;	\
	case 0xA8:	/* XOR B */	\
		M80_XOR_WITH_A(B);	\
		break;	\
	case 0xA9:	/* XOR C */	\
		M80_XOR_WITH_A(C);	\
		break;	\
	case 0xAA:	/* XOR D */	\
		M80_XOR_WITH_A(D);	\
		break;	\
	case 0xAB:	/* XOR E */	\
		M80_XOR_WITH_A(E);	\
		break;	\
	case 0xAC:	/* XOR H */	\
		M80_XOR_WITH_A(H);	\
		break;	\
	case 0xAD:	/* XOR L */	\
		M80_XOR_WITH_A(L);	\
		break;	\
	case 0xAE:	/* XOR (HL) */	\
		M80_XOR_WITH_A(M80_READ_BYTE(HL));	\
		break;	\
	case 0xAF:	/* XOR A */	\
		A = 0;	/* XOR'ing a register with itself produces 0 */	\
		FLAGS = Z_FLAG | P_FLAG; /* signed=clear, Zero=set, HC=clear, parity is even, N=clear, C=clear */	\
		break;	\
	case 0xB0:	/* OR B */	\
		M80_OR_WITH_A(B);	\
		break;	\
	case 0xB1:	/* OR C */	\
		M80_OR_WITH_A(C);	\
		break;	\
	case 0xB2:	/* OR D */	\
		M80_OR_WITH_A(D);	\
		break;	\
	case 0xB3:	/* OR E */	\
		M80_OR_WITH_A(E);	\
		break;	\
	case 0xB4:	/* OR H */	\
		M80_OR_WITH_A(H);	\
		break;	\
	case 0xB5:	/* OR L */	\
		M80_OR_WITH_A(L);	\
		break;	\
	case 0xB6:	/* OR (HL) */	\
		M80_OR_WITH_A(M80_READ_BYTE(HL));	\
		break;	\
	case 0xB7:	/* OR A */	\
		M80_OR_WITH_A(A);	\
		break;	\
	case 0xB8:	/* Compare B */	\
		M80_COMPARE_WITH_A(B);	\
		break;	\
	case 0xB9:	/* CP C */	\
		M80_COMPARE_WITH_A(C);	\
		break;	\
	case 0xBA:	/* CP D */	\
		M80_COMPARE_WITH_A(D);	\
		break;	\
	case 0xBB:	/* CP E */	\
		M80_COMPARE_WITH_A(E);	\
		break;	\
	case 0xBC:	/* CP H */	\
		M80_COMPARE_WITH_A(H);	\
		break;	\
	case 0xBD:	/* CP L */	\
		M80_COMPARE_WITH_A(L);	\
		break;	\
	case 0xBE:	/* CP (HL) */	\
		M80_COMPARE_WITH_A(M80_READ_BYTE(HL));	\
		break;	\
	case 0xBF:	/* CP A */	\
		M80_COMPARE_WITH_A(A);	\
		break;	\
	case 0xC0:	/* Return if Z_FLAG is not set */	\
		M80_RET_COND((FLAGS & Z_FLAG) == 0);	\
		break;	\
	case 0xC1:	/* POP top of stack into BC  */	\
		M80_POP16(M80_BC);	\
		break;	\
	case 0xC2:	/* Jump if Not Z_FLAG to nnnn */	\
		M80_JUMP_COND((FLAGS & Z_FLAG) == 0);	\
		break;	\
	case 0xC3:	/* unconditional Jump to nnnn */	\
		M80_JUMP();	\
		break;	\
	case 0xC4:	/* Call nnnn if not Z_FLAG */	\
		M80_CALL_COND((FLAGS & Z_FLAG) == 0);	\
		break;	\
	case 0xC5:	/* PUSH BC */	\
		M80_PUSH16(M80_BC);	\
		break;	\
	case 0xC6:	/* ADD A, nn */	\
		M80_ADD_TO_A(M80_PEEK_ARG);	\
		PC++;	\
		break;	\
	case 0xC7:	/* RST 0  (Reset 0) */	\
		M80_RST(0);	\
		break;	\
	case 0xC8:	/* RET Z (Return if Z_Flag is set) */	\
		M80_RET_COND (FLAGS & Z_FLAG);	\
		break;	\
	case 0xC9:	/* unconditional RET */	\
		M80_RET;	\
		break;	\
	case 0xCA:	/* JP Z, nnnn  (Jump if Z_FLAG is set) */	\
		M80_JUMP_COND (FLAGS & Z_FLAG);	\
		break;	\
	case 0xCB:	/* there are a ton of "CB" instructions */	\
		m80_exec_cb();	\
		break;	\
	case 0xCC:	/* CALL Z, nnnn (Call function if Z_FLAG is set) */	\
		M80_CALL_COND (FLAGS & Z_FLAG);	\
		break;	\
	case 0xCD:	/*  unconditional CALL */	\
		M80_CALL();	\
		break;	\
	case 0xCE:	/* ADC A, nn */	\
		M80_ADC_TO_A(M80_PEEK_ARG);	\
		PC++;	\
		break;	\
	case 0xCF:	/* RST 8 */	\
		M80_RST(8);	\
		break;	\
	case 0xD0:	/* RET NC (return if carry flag is clear) */	\
		M80_RET_COND ((FLAGS & C_FLAG) == 0);	\
		break;	\
	case 0xD1:	/* POP DE */	\
		M80_POP16(M80_DE);	\
		break;	\
	case 0xD2:	/* JP NC, nnnn (absolute jump if C_FLAG is clear) */	\
		M80_JUMP_COND ((FLAGS & C_FLAG) == 0);	\
		break;	\
	case 0xD3:	/* OUT (nn), A	Send A to the specified port */	\
		M80_OUT_A(M80_GET_ARG);	\
		break;	\
	case 0xD4:	/* CALL NC, nnnn	(call if C_FLAG is clear) */	\
		M80_CALL_COND ((FLAGS & C_FLAG) == 0);	\
		break;	\
	case 0xD5:	/* PUSH DE */	\
		M80_PUSH16(M80_DE);	\
		break;	\
	case 0xD6:	/* SUB nn */	\
		M80_SUB_FROM_A(M80_PEEK_ARG);	\
		PC++;	\
		break;	\
	case 0xD7:	/* RST 0x10 */	\
		M80_RST(0x10);	\
		break;	\
	case 0xD8:	/* RET C (return if C_FLAG is set) */	\
		M80_RET_COND (FLAGS & C_FLAG);	\
		break;	\
	case 0xD9:	/* EXX (Exchange all registers with their counterparts, except AF) */	\
		M80_EXX();	\
		break;	\
	case 0xDA:	/* JP C, nnnn	Absolute jump if Carry is set */	\
		M80_JUMP_COND (FLAGS & C_FLAG);	\
		break;	\
	case 0xDB:	/* IN A, (nn) */	\
		M80_IN_A(M80_GET_ARG);	\
		break;	\
	case 0xDC:	/* CALL C, nnnn	Call if carry is set */	\
		M80_CALL_COND (FLAGS & C_FLAG);	\
		break;	\
	case 0xDD:	/* extended instructions */	\
		M80_EAT_EXTRA_DD_FD;	\
		M80_EXEC_DDFD(IX);	\
		break;	\
	case 0xDE:	/*	SBC A, nn */	\
		M80_SBC_FROM_A(M80_PEEK_ARG);	\
		PC++;	\
		break;	\
	case 0xDF:	/* RST 0x18 */	\
		M80_RST(0x18);	\
		break;	\
	case 0xE0:	/* RET PO	Return of Parity is Odd (parity flag cleared) */	\
		M80_RET_COND ((FLAGS & P_FLAG) == 0);	\
		break;	\
	case 0xE1:	/* POP HL */	\
		M80_POP16(M80_HL);	\
		break;	\
	case 0xE2:	/* JP PO, nnnn	(absolute jump of parity is odd, P_FLAG cleared) */	\
		M80_JUMP_COND ((FLAGS & P_FLAG) == 0);	\
		break;	\
	case 0xE3:	/* EX (SP), HL	Exchange HL with what's stored in memory at SP */	\
		{	\
			pair temp;	\
			temp.w = HL;	\
			g_context.m80_regs[M80_HL].b.l = M80_READ_BYTE(SP);	\
			g_context.m80_regs[M80_HL].b.h = M80_READ_BYTE(SP+1);	\
			M80_WRITE_BYTE(SP, temp.b.l);	\
			M80_WRITE_BYTE(SP+1, temp.b.h);	\
		}	\
		break;	\
	case 0xE4:	/* CALL PO, nnnn	Call if P_FLAG is cleared */	\
		M80_CALL_COND ((FLAGS & P_FLAG) == 0);	\
		break;	\
	case 0xE5:	/* PUSH HL */	\
		M80_PUSH16(M80_HL);	\
		break;	\
	case 0xE6:	/* AND nn */	\
		M80_AND_WITH_A(M80_PEEK_ARG);	\
		PC++;	\
		break;	\
	case 0xE7:	/* RST 0x20 */	\
		M80_RST(0x20);	\
		break;	\
	case 0xE8:	/* RET PE (return if parity is even/P_FLAG is set) */	\
		M80_RET_COND (FLAGS & P_FLAG);	\
		break;	\
	case 0xE9:	/* JP HL	jump to the address contained in HL */	\
		PC = HL;	\
		M80_CHANGE_PC(PC);	\
		break;	\
	case 0xEA:	/*	JP PE, nnnn	(absolute jump if parity is even) */	\
		M80_JUMP_COND (FLAGS & P_FLAG);	\
		break;	\
	case 0xEB:	/* EX DE, HL	(swap DE and HL) */	\
		M80_EX_DEHL();	\
		break;	\
	case 0xEC:	/* CALL PE, nnnn	(call if parity is even) */	\
		M80_CALL_COND (FLAGS & P_FLAG);	\
		break;	\
	case 0xED:	/* a whole new block of ED instructions */	\
		M80_EXEC_ED;	\
		break;	\
	case 0xEE:	/* XOR nn */	\
		M80_XOR_WITH_A(M80_PEEK_ARG);	\
		PC++;	\
		break;	\
	case 0xEF:	/* RST 0x28 */	\
		M80_RST(0x28);	\
		break;	\
	case 0xF0:	/* RET P	return if positive (S_FLAG is cleared) */	\
		M80_RET_COND ((FLAGS & S_FLAG) == 0);	\
		break;	\
	case 0xF1:	/* POP AF */	\
		M80_POP16(M80_AF);	\
		break;	\
	case 0xF2:	/* JP P, nnnn	Jump if positive */	\
		M80_JUMP_COND ((FLAGS & S_FLAG) == 0);	\
		break;	\
	case 0xF3:	/* DI (Disable Interrupts) */	\
		M80_DI;	\
		break;	\
	case 0xF4:	/* CALL P, nnnn	Call if positive (S_FLAG cleared) */	\
		M80_CALL_COND ((FLAGS & S_FLAG) == 0);	\
		break;	\
	case 0xF5:	/* PUSH AF */	\
		M80_PUSH16(M80_AF);	\
		break;	\
	case 0xF6:	/* OR nn */	\
		M80_OR_WITH_A(M80_PEEK_ARG);	\
		PC++;	\
		break;	\
	case 0xF7:	/* RST 0x30 */	\
		M80_RST(0x30);	\
		break;	\
	case 0xF8:	/* RET M	Return if negative (S_FLAG set) */	\
		M80_RET_COND(FLAGS & S_FLAG);	\
		break;	\
	case 0xF9:	/* LD SP, HL	transfer HL to SP */	\
		SP = HL;	\
		break;	\
	case 0xFA:	/* JP M, nnnn	Jump if Minus sign */	\
		M80_JUMP_COND (FLAGS & S_FLAG);	\
		break;	\
	case 0xFB:	/* EI (enable interrupts) */	\
		M80_EI;	\
		break;	\
	case 0xFC:	/* CALL M, nnnn */	\
		M80_CALL_COND (FLAGS & S_FLAG);	\
		break;	\
	case 0xFD:	/* extended instructions */	\
		M80_EAT_EXTRA_DD_FD;	\
		M80_EXEC_DDFD(IY);	\
		break;	\
	case 0xFE:	/* CP n */	\
		M80_COMPARE_WITH_A(M80_PEEK_ARG);	\
		PC++;	\
		break;	\
	case 0xFF:	/* RST 0x38 */	\
		M80_RST(0x38);	\
		break;	\
	} /* end switch */	\
} /* end macro */





/* attempts to the number of cycles specified.  Returns the number of cycles actually executed. */
Uint32 m80_execute(Uint32 cycles_to_execute)
{
	g_cycles_executed = 0;	/* we haven't executed any yet this time around */
	g_cycles_to_execute = cycles_to_execute;

	/* keep executing instructions until we've exceeded our quota */
	while (g_cycles_executed < cycles_to_execute)
	{
		CHECK_INTERRUPT;	/* it's ok to check the interrupt at this stage */

		/* HERE IS WHERE THE FAST LOOP IS.  WE SHOULD STAY IN THIS LOOP MOST OF THE TIME */
		/* NOTE: interrupts can't occur within this loop at all */
		while ((g_cycles_executed < cycles_to_execute) && !g_context.got_EI)
		{
#ifdef INTEGRATE
#ifdef CPU_DEBUG
			MAME_Debug();
#endif
#endif
			M80_EXEC_CUR_INSTR;
		}

		/* after we get an EI, we have to execute the next instruction before checking */
		/* for interrupts.  In case we have a string of EI's, we use a while loop here. */
		while (g_context.got_EI)
		{
#ifdef INTEGRATE
#ifdef CPU_DEBUG
			MAME_Debug();
#endif
#endif
			g_context.got_EI = 0;	/* clear this flag (it can be set in the next instruction) */
			M80_EXEC_CUR_INSTR;
		}

	} /* end while */

	return (g_cycles_executed);
  
}

/* executes all of the 0xCB instructions */
__inline__ void m80_exec_cb()
{

	Uint8 opcode = M80_GET_ARG;	/* get opcode and increment PC */
	g_cycles_executed += cb_cycles[opcode];

	M80_INC_R;	/* R needs to increase again with CB instructions */

	switch(opcode)
	{
	case 0:	/* RLC	B	(Rotate Left Circular) */
		M80_RLC(B);
		break;
	case 1:	/* RLC	C */
		M80_RLC(C);
		break;
	case 2:	/* RLC	D */
		M80_RLC(D);
		break;
	case 3:	/* RLC	E */
		M80_RLC(E);
		break;
	case 4:	/* RLC	H */
		M80_RLC(H);
		break;
	case 5:	/* RLC	L */
		M80_RLC(L);
		break;
	case 6:	/* RLC	(HL) */
		{
			Uint8 val = M80_READ_BYTE(HL);
			M80_RLC(val);
			M80_WRITE_BYTE(HL, val);
		}
		break;
	case 7:	/* RLC	A */
		M80_RLC(A);
		break;
	case 8:	/* RRC	B */
		M80_RRC(B);
		break;
	case 9:	/* RRC	C */
		M80_RRC(C);
		break;
	case 0x0A:	/* RRC	D */
		M80_RRC(D);
		break;
	case 0x0B:	/* RRC	E */
		M80_RRC(E);
		break;
	case 0x0C:	/* RRC	H */
		M80_RRC(H);
		break;
	case 0x0D:	/* RRC	L */
		M80_RRC(L);
		break;
	case 0x0E:	/* RRC	(HL) */
		{
			Uint8 val = M80_READ_BYTE(HL);
			M80_RRC(val);
			M80_WRITE_BYTE(HL, val);
		}
		break;
	case 0x0F:	/* RRC	A */
		M80_RRC(A);
		break;
	case 0x10:	/* RL B */
		M80_RL(B);
		break;
	case 0x11:	/* RL C */
		M80_RL(C);
		break;
	case 0x12:	/* RL D */
		M80_RL(D);
		break;
	case 0x13:	/* RL E */
		M80_RL(E);
		break;
	case 0x14:	/* RL H */
		M80_RL(H);
		break;
	case 0x15:	/* RL L */
		M80_RL(L);
		break;
	case 0x16:	/* RL (HL) */
		{
			Uint8 val = M80_READ_BYTE(HL);
			M80_RL(val);
			M80_WRITE_BYTE(HL, val);
		}
		break;
	case 0x17:	/* RL A */
		M80_RL(A);
		break;
	case 0x18:	/* RR B */
		M80_RR(B);
		break;
	case 0x19:	/* RR C */
		M80_RR(C);
		break;
	case 0x1A:	/* RR D */
		M80_RR(D);
		break;
	case 0x1B:	/* RR E */
		M80_RR(E);
		break;
	case 0x1C:	/* RR H */
		M80_RR(H);
		break;
	case 0x1D:	/* RR L */
		M80_RR(L);
		break;
	case 0x1E:	/* RR (HL) */
		{
			Uint8 val = M80_READ_BYTE(HL);
			M80_RR(val);
			M80_WRITE_BYTE(HL, val);
		}
		break;
	case 0x1F:	/* RR A */
		M80_RR(A);
		break;
	case 0x20:	/* SLA B */
		M80_SLA(B);
		break;
	case 0x21:	/* SLA C */
		M80_SLA(C);
		break;
	case 0x22:	/* SLA D */
		M80_SLA(D);
		break;
	case 0x23:	/* SLA E */
		M80_SLA(E);
		break;
	case 0x24:	/* SLA H */
		M80_SLA(H);
		break;
	case 0x25:	/* SLA L */
		M80_SLA(L);
		break;
	case 0x26:	/* SLA (HL) */
		{
			Uint8 val = M80_READ_BYTE(HL);
			M80_SLA(val);
			M80_WRITE_BYTE(HL, val);
		}
		break;
	case 0x27:	/* SLA A */
		M80_SLA(A);
		break;
	case 0x28:	/* SRA B */
		M80_SRA(B);
		break;
	case 0x29:	/* SRA C */
		M80_SRA(C);
		break;
	case 0x2A:	/* SRA D */
		M80_SRA(D);
		break;
	case 0x2B:	/* SRA E */
		M80_SRA(E);
		break;
	case 0x2C:	/* SRA H */
		M80_SRA(H);
		break;
	case 0x2D:	/* SRA L */
		M80_SRA(L);
		break;
	case 0x2E:	/* SRA (HL) */
		{
			Uint8 val = M80_READ_BYTE(HL);
			M80_SRA(val);
			M80_WRITE_BYTE(HL, val);
		}
		break;
	case 0x2F:	/* SRA A */
		M80_SRA(A);
		break;
	case 0x30:	/* SLIA B */
		M80_SLIA(B);
		break;
	case 0x31:	/* SLIA C */
		M80_SLIA(C);
		break;
	case 0x32:	/* SLIA D */
		M80_SLIA(D);
		break;
	case 0x33:	/* SLIA E */
		M80_SLIA(E);
		break;
	case 0x34:	/* SLIA H */
		M80_SLIA(H);
		break;
	case 0x35:	/* SLIA L */
		M80_SLIA(L);
		break;
	case 0x36:	/* SLIA (HL) */
		{
			Uint8 val = M80_READ_BYTE(HL);
			M80_SLIA(val);
			M80_WRITE_BYTE(HL, val);
		}
		break;
	case 0x37:	/* SLIA A */
		M80_SLIA(A);
		break;
	case 0x38:	/* SRL B */
		M80_SRL(B);
		break;
	case 0x39:	/* SRL C */
		M80_SRL(C);
		break;
	case 0x3A:	/* SRL D */
		M80_SRL(D);
		break;
	case 0x3B:	/* SRL E */
		M80_SRL(E);
		break;
	case 0x3C:	/* SRL H */
		M80_SRL(H);
		break;
	case 0x3D:	/* SRL L */
		M80_SRL(L);
		break;
	case 0x3E:	/* SRL (HL) */
		{
			Uint8 val = M80_READ_BYTE(HL);
			M80_SRL(val);
			M80_WRITE_BYTE(HL, val);
		}
		break;
	case 0x3F:	/* SRL A */
		M80_SRL(A);
		break;
	case 0x40:	/* BIT 0, B */
		M80_BIT(0, B);
		break;
	case 0x41:	/* BIT 0, C */
		M80_BIT(0, C);
		break;
	case 0x42:	/* BIT 0, D */
		M80_BIT(0, D);
		break;
	case 0x43:	/* BIT 0, E */
		M80_BIT(0, E);
		break;
	case 0x44:	/* BIT 0, H */
		M80_BIT(0, H);
		break;
	case 0x45:	/* BIT 0, L */
		M80_BIT(0, L);
		break;
	case 0x46:	/* BIT 0, (HL) */
		M80_BIT_NO35(0, M80_READ_BYTE(HL));
		break;
	case 0x47:	/* BIT 0, A */
		M80_BIT(0, A);
		break;
	case 0x48:	/* BIT 1, B */
		M80_BIT(1, B);
		break;
	case 0x49:	/* BIT 1, C */
		M80_BIT(1, C);
		break;
	case 0x4A:	/* BIT 1, D */
		M80_BIT(1, D);
		break;
	case 0x4B:	/* BIT 1, E */
		M80_BIT(1, E);
		break;
	case 0x4C:	/* BIT 1, H */
		M80_BIT(1, H);
		break;
	case 0x4D:	/* BIT 1, L */
		M80_BIT(1, L);
		break;
	case 0x4E:	/* BIT 1, (HL) */
		M80_BIT_NO35(1, M80_READ_BYTE(HL));
		break;
	case 0x4F:	/* BIT 1, A */
		M80_BIT(1, A);
		break;
	case 0x50:	/* BIT 2, B */
		M80_BIT(2, B);
		break;
	case 0x51:	/* BIT 2, C */
		M80_BIT(2, C);
		break;
	case 0x52:	/* BIT 2, D */
		M80_BIT(2, D);
		break;
	case 0x53:	/* BIT 2, E */
		M80_BIT(2, E);
		break;
	case 0x54:	/* BIT 2, H */
		M80_BIT(2, H);
		break;
	case 0x55:	/* BIT 2, L */
		M80_BIT(2, L);
		break;
	case 0x56:	/* BIT 2, (HL) */
		M80_BIT_NO35(2, M80_READ_BYTE(HL));
		break;
	case 0x57:	/* BIT 2, A */
		M80_BIT(2, A);
		break;
	case 0x58:	/* BIT 3, B */
		M80_BIT(3, B);
		break;
	case 0x59:	/* BIT 3, C */
		M80_BIT(3, C);
		break;
	case 0x5A:	/* BIT 3, D */
		M80_BIT(3, D);
		break;
	case 0x5B:	/* BIT 3, E */
		M80_BIT(3, E);
		break;
	case 0x5C:	/* BIT 3, H */
		M80_BIT(3, H);
		break;
	case 0x5D:	/* BIT 3, L */
		M80_BIT(3, L);
		break;
	case 0x5E:	/* BIT 3, (HL) */
		M80_BIT_NO35(3, M80_READ_BYTE(HL));
		break;
	case 0x5F:	/* BIT 3, A */
		M80_BIT(3, A);
		break;
	case 0x60:	/* BIT 4, B */
		M80_BIT(4, B);
		break;
	case 0x61:	/* BIT 4, C */
		M80_BIT(4, C);
		break;
	case 0x62:	/* BIT 4, D */
		M80_BIT(4, D);
		break;
	case 0x63:	/* BIT 4, E */
		M80_BIT(4, E);
		break;
	case 0x64:	/* BIT 4, H */
		M80_BIT(4, H);
		break;
	case 0x65:	/* BIT 4, L */
		M80_BIT(4, L);
		break;
	case 0x66:	/* BIT 4, (HL) */
		M80_BIT_NO35(4, M80_READ_BYTE(HL));
		break;
	case 0x67:	/* BIT 4, A */
		M80_BIT(4, A);
		break;
	case 0x68:	/* BIT 5, B */
		M80_BIT(5, B);
		break;
	case 0x69:	/* BIT 5, C */
		M80_BIT(5, C);
		break;
	case 0x6A:	/* BIT 5, D */
		M80_BIT(5, D);
		break;
	case 0x6B:	/* BIT 5, E */
		M80_BIT(5, E);
		break;
	case 0x6C:	/* BIT 5, H */
		M80_BIT(5, H);
		break;
	case 0x6D:	/* BIT 5, L */
		M80_BIT(5, L);
		break;
	case 0x6E:	/* BIT 5, (HL) */
		M80_BIT_NO35(5, M80_READ_BYTE(HL));
		break;
	case 0x6F:	/* BIT 5, A */
		M80_BIT(5, A);
		break;
	case 0x70:	/* BIT 6, B */
		M80_BIT(6, B);
		break;
	case 0x71:	/* BIT 6, C */
		M80_BIT(6, C);
		break;
	case 0x72:	/* BIT 6, D */
		M80_BIT(6, D);
		break;
	case 0x73:	/* BIT 6, E */
		M80_BIT(6, E);
		break;
	case 0x74:	/* BIT 6, H */
		M80_BIT(6, H);
		break;
	case 0x75:	/* BIT 6, L */
		M80_BIT(6, L);
		break;
	case 0x76:	/* BIT 6, (HL) */
		M80_BIT_NO35(6, M80_READ_BYTE(HL));
		break;
	case 0x77:	/* BIT 6, A */
		M80_BIT(6, A);
		break;
	case 0x78:	/* BIT 7, B */
		M80_BIT(7, B);
		break;
	case 0x79:	/* BIT 7, C */
		M80_BIT(7, C);
		break;
	case 0x7A:	/* BIT 7, D */
		M80_BIT(7, D);
		break;
	case 0x7B:	/* BIT 7, E */
		M80_BIT(7, E);
		break;
	case 0x7C:	/* BIT 7, H */
		M80_BIT(7, H);
		break;
	case 0x7D:	/* BIT 7, L */
		M80_BIT(7, L);
		break;
	case 0x7E:	/* BIT 7, (HL) */
		M80_BIT_NO35(7, M80_READ_BYTE(HL));
		break;
	case 0x7F:	/* BIT 7, A */
		M80_BIT(7, A);
		break;
	case 0x80:	/* RES	0, B */
		M80_RES(0, B);
		break;
	case 0x81:	/* RES	0, C */
		M80_RES(0, C);
		break;
	case 0x82:	/* RES	0, D */
		M80_RES(0, D);
		break;
	case 0x83:	/* RES	0, E */
		M80_RES(0, E);
		break;
	case 0x84:	/* RES	0, H */
		M80_RES(0, H);
		break;
	case 0x85:	/* RES	0, L */
		M80_RES(0, L);
		break;
	case 0x86:	/* RES	0, (HL) */
		{
			Uint8 val = M80_READ_BYTE(HL);
			M80_RES(0, val);
			M80_WRITE_BYTE(HL, val);
		}
		break;
	case 0x87:	/* RES	0, A */
		M80_RES(0, A);
		break;
	case 0x88:	/* RES	1, B */
		M80_RES(1, B);
		break;
	case 0x89:	/* RES	1, C */
		M80_RES(1, C);
		break;
	case 0x8A:	/* RES	1, D */
		M80_RES(1, D);
		break;
	case 0x8B:	/* RES	1, E */
		M80_RES(1, E);
		break;
	case 0x8C:	/* RES	1, H */
		M80_RES(1, H);
		break;
	case 0x8D:	/* RES	1, L */
		M80_RES(1, L);
		break;
	case 0x8E:	/* RES	1, (HL) */
		{
			Uint8 val = M80_READ_BYTE(HL);
			M80_RES(1, val);
			M80_WRITE_BYTE(HL, val);
		}
		break;
	case 0x8F:	/* RES	1, A */
		M80_RES(1, A);
		break;
	case 0x90:	/* RES	2, B */
		M80_RES(2, B);
		break;
	case 0x91:	/* RES	2, C */
		M80_RES(2, C);
		break;
	case 0x92:	/* RES	2, D */
		M80_RES(2, D);
		break;
	case 0x93:	/* RES	2, E */
		M80_RES(2, E);
		break;
	case 0x94:	/* RES	2, H */
		M80_RES(2, H);
		break;
	case 0x95:	/* RES	2, L */
		M80_RES(2, L);
		break;
	case 0x96:	/* RES	2, (HL) */
		{
			Uint8 val = M80_READ_BYTE(HL);
			M80_RES(2, val);
			M80_WRITE_BYTE(HL, val);
		}
		break;
	case 0x97:	/* RES	2, A */
		M80_RES(2, A);
		break;
	case 0x98:	/* RES	3, B */
		M80_RES(3, B);
		break;
	case 0x99:	/* RES	3, C */
		M80_RES(3, C);
		break;
	case 0x9A:	/* RES	3, D */
		M80_RES(3, D);
		break;
	case 0x9B:	/* RES	3, E */
		M80_RES(3, E);
		break;
	case 0x9C:	/* RES	3, H */
		M80_RES(3, H);
		break;
	case 0x9D:	/* RES	3, L */
		M80_RES(3, L);
		break;
	case 0x9E:	/* RES	3, (HL) */
		{
			Uint8 val = M80_READ_BYTE(HL);
			M80_RES(3, val);
			M80_WRITE_BYTE(HL, val);
		}
		break;
	case 0x9F:	/* RES	3, A */
		M80_RES(3, A);
		break;
	case 0xA0:	/* RES	4, B */
		M80_RES(4, B);
		break;
	case 0xA1:	/* RES	4, C */
		M80_RES(4, C);
		break;
	case 0xA2:	/* RES	4, D */
		M80_RES(4, D);
		break;
	case 0xA3:	/* RES	4, E */
		M80_RES(4, E);
		break;
	case 0xA4:	/* RES	4, H */
		M80_RES(4, H);
		break;
	case 0xA5:	/* RES	4, L */
		M80_RES(4, L);
		break;
	case 0xA6:	/* RES	4, (HL) */
		{
			Uint8 val = M80_READ_BYTE(HL);
			M80_RES(4, val);
			M80_WRITE_BYTE(HL, val);
		}
		break;
	case 0xA7:	/* RES	4, A */
		M80_RES(4, A);
		break;
	case 0xA8:	/* RES	5, B */
		M80_RES(5, B);
		break;
	case 0xA9:	/* RES	5, C */
		M80_RES(5, C);
		break;
	case 0xAA:	/* RES	5, D */
		M80_RES(5, D);
		break;
	case 0xAB:	/* RES	5, E */
		M80_RES(5, E);
		break;
	case 0xAC:	/* RES	5, H */
		M80_RES(5, H);
		break;
	case 0xAD:	/* RES	5, L */
		M80_RES(5, L);
		break;
	case 0xAE:	/* RES	5, (HL) */
		{
			Uint8 val = M80_READ_BYTE(HL);
			M80_RES(5, val);
			M80_WRITE_BYTE(HL, val);
		}
		break;
	case 0xAF:	/* RES	5, A */
		M80_RES(5, A);
		break;
	case 0xB0:	/* RES	6, B */
		M80_RES(6, B);
		break;
	case 0xB1:	/* RES	6, C */
		M80_RES(6, C);
		break;
	case 0xB2:	/* RES	6, D */
		M80_RES(6, D);
		break;
	case 0xB3:	/* RES	6, E */
		M80_RES(6, E);
		break;
	case 0xB4:	/* RES	6, H */
		M80_RES(6, H);
		break;
	case 0xB5:	/* RES	6, L */
		M80_RES(6, L);
		break;
	case 0xB6:	/* RES	6, (HL) */
		{
			Uint8 val = M80_READ_BYTE(HL);
			M80_RES(6, val);
			M80_WRITE_BYTE(HL, val);
		}
		break;
	case 0xB7:	/* RES	6, A */
		M80_RES(6, A);
		break;
	case 0xB8:	/* RES	7, B */
		M80_RES(7, B);
		break;
	case 0xB9:	/* RES	7, C */
		M80_RES(7, C);
		break;
	case 0xBA:	/* RES	7, D */
		M80_RES(7, D);
		break;
	case 0xBB:	/* RES	7, E */
		M80_RES(7, E);
		break;
	case 0xBC:	/* RES	7, H */
		M80_RES(7, H);
		break;
	case 0xBD:	/* RES	7, L */
		M80_RES(7, L);
		break;
	case 0xBE:	/* RES	7, (HL) */
		{
			Uint8 val = M80_READ_BYTE(HL);
			M80_RES(7, val);
			M80_WRITE_BYTE(HL, val);
		}
		break;
	case 0xBF:	/* RES	7, A */
		M80_RES(7, A);
		break;

	case 0xC0:	/* SET	0, B */
		M80_SET(0, B);
		break;
	case 0xC1:	/* SET	0, C */
		M80_SET(0, C);
		break;
	case 0xC2:	/* SET	0, D */
		M80_SET(0, D);
		break;
	case 0xC3:	/* SET	0, E */
		M80_SET(0, E);
		break;
	case 0xC4:	/* SET	0, H */
		M80_SET(0, H);
		break;
	case 0xC5:	/* SET	0, L */
		M80_SET(0, L);
		break;
	case 0xC6:	/* SET	0, (HL) */
		{
			Uint8 val = M80_READ_BYTE(HL);
			M80_SET(0, val);
			M80_WRITE_BYTE(HL, val);
		}
		break;
	case 0xC7:	/* SET	0, A */
		M80_SET(0, A);
		break;
	case 0xC8:	/* SET	1, B */
		M80_SET(1, B);
		break;
	case 0xC9:	/* SET	1, C */
		M80_SET(1, C);
		break;
	case 0xCA:	/* SET	1, D */
		M80_SET(1, D);
		break;
	case 0xCB:	/* SET	1, E */
		M80_SET(1, E);
		break;
	case 0xCC:	/* SET	1, H */
		M80_SET(1, H);
		break;
	case 0xCD:	/* SET	1, L */
		M80_SET(1, L);
		break;
	case 0xCE:	/* SET	1, (HL) */
		{
			Uint8 val = M80_READ_BYTE(HL);
			M80_SET(1, val);
			M80_WRITE_BYTE(HL, val);
		}
		break;
	case 0xCF:	/* SET	1, A */
		M80_SET(1, A);
		break;
	case 0xD0:	/* SET	2, B */
		M80_SET(2, B);
		break;
	case 0xD1:	/* SET	2, C */
		M80_SET(2, C);
		break;
	case 0xD2:	/* SET	2, D */
		M80_SET(2, D);
		break;
	case 0xD3:	/* SET	2, E */
		M80_SET(2, E);
		break;
	case 0xD4:	/* SET	2, H */
		M80_SET(2, H);
		break;
	case 0xD5:	/* SET	2, L */
		M80_SET(2, L);
		break;
	case 0xD6:	/* SET	2, (HL) */
		{
			Uint8 val = M80_READ_BYTE(HL);
			M80_SET(2, val);
			M80_WRITE_BYTE(HL, val);
		}
		break;
	case 0xD7:	/* SET	2, A */
		M80_SET(2, A);
		break;
	case 0xD8:	/* SET	3, B */
		M80_SET(3, B);
		break;
	case 0xD9:	/* SET	3, C */
		M80_SET(3, C);
		break;
	case 0xDA:	/* SET	3, D */
		M80_SET(3, D);
		break;
	case 0xDB:	/* SET	3, E */
		M80_SET(3, E);
		break;
	case 0xDC:	/* SET	3, H */
		M80_SET(3, H);
		break;
	case 0xDD:	/* SET	3, L */
		M80_SET(3, L);
		break;
	case 0xDE:	/* SET	3, (HL) */
		{
			Uint8 val = M80_READ_BYTE(HL);
			M80_SET(3, val);
			M80_WRITE_BYTE(HL, val);
		}
		break;
	case 0xDF:	/* SET	3, A */
		M80_SET(3, A);
		break;
	case 0xE0:	/* SET	4, B */
		M80_SET(4, B);
		break;
	case 0xE1:	/* SET	4, C */
		M80_SET(4, C);
		break;
	case 0xE2:	/* SET	4, D */
		M80_SET(4, D);
		break;
	case 0xE3:	/* SET	4, E */
		M80_SET(4, E);
		break;
	case 0xE4:	/* SET	4, H */
		M80_SET(4, H);
		break;
	case 0xE5:	/* SET	4, L */
		M80_SET(4, L);
		break;
	case 0xE6:	/* SET	4, (HL) */
		{
			Uint8 val = M80_READ_BYTE(HL);
			M80_SET(4, val);
			M80_WRITE_BYTE(HL, val);
		}
		break;
	case 0xE7:	/* SET	4, A */
		M80_SET(4, A);
		break;
	case 0xE8:	/* SET	5, B */
		M80_SET(5, B);
		break;
	case 0xE9:	/* SET	5, C */
		M80_SET(5, C);
		break;
	case 0xEA:	/* SET	5, D */
		M80_SET(5, D);
		break;
	case 0xEB:	/* SET	5, E */
		M80_SET(5, E);
		break;
	case 0xEC:	/* SET	5, H */
		M80_SET(5, H);
		break;
	case 0xED:	/* SET	5, L */
		M80_SET(5, L);
		break;
	case 0xEE:	/* SET	5, (HL) */
		{
			Uint8 val = M80_READ_BYTE(HL);
			M80_SET(5, val);
			M80_WRITE_BYTE(HL, val);
		}
		break;
	case 0xEF:	/* SET	5, A */
		M80_SET(5, A);
		break;
	case 0xF0:	/* SET	6, B */
		M80_SET(6, B);
		break;
	case 0xF1:	/* SET	6, C */
		M80_SET(6, C);
		break;
	case 0xF2:	/* SET	6, D */
		M80_SET(6, D);
		break;
	case 0xF3:	/* SET	6, E */
		M80_SET(6, E);
		break;
	case 0xF4:	/* SET	6, H */
		M80_SET(6, H);
		break;
	case 0xF5:	/* SET	6, L */
		M80_SET(6, L);
		break;
	case 0xF6:	/* SET	6, (HL) */
		{
			Uint8 val = M80_READ_BYTE(HL);
			M80_SET(6, val);
			M80_WRITE_BYTE(HL, val);
		}
		break;
	case 0xF7:	/* SET	6, A */
		M80_SET(6, A);
		break;
	case 0xF8:	/* SET	7, B */
		M80_SET(7, B);
		break;
	case 0xF9:	/* SET	7, C */
		M80_SET(7, C);
		break;
	case 0xFA:	/* SET	7, D */
		M80_SET(7, D);
		break;
	case 0xFB:	/* SET	7, E */
		M80_SET(7, E);
		break;
	case 0xFC:	/* SET	7, H */
		M80_SET(7, H);
		break;
	case 0xFD:	/* SET	7, L */
		M80_SET(7, L);
		break;
	case 0xFE:	/* SET	7, (HL) */
		{
			Uint8 val = M80_READ_BYTE(HL);
			M80_SET(7, val);
			M80_WRITE_BYTE(HL, val);
		}
		break;
	case 0xFF:	/* SET	7, A */
		M80_SET(7, A);
		break;
	}
}

/* call this when you want to change the state of the NMI line */
/* If you want to assert the NMI, use ASSERT_LINE */
/* If you want to clear the NMI, use CLEAR_LINE */
void m80_set_nmi_line(Uint8 new_nmi_state)
{
	g_context.nmi_state = new_nmi_state;
}

/* call this when you want to change the state of the IRQ line */
/* To assert the IRQ, use ASSERT_LINE */
/* To clear the IRQ, use CLEAR_LINE */
/* You need to have an IRQ callback defined before asserting the IRQ line */
void m80_set_irq_line(Uint8 irq_state)
{
	g_context.irq_state = irq_state;
}

/* calls the nmi service routine at 0x66 */
void m80_activate_nmi()
{
	(*g_nmi_callback)();
		
	M80_INC_R;	/* increment R when activating nmi */
	M80_STOP_HALT;	/* get out of halt mode if we were in it */

	g_context.IFF1 = 0;	/* as soon as the NMI is asserted, it clears flipflop1 */
	M80_PUSH16(M80_PC);	/* now Call 0x66, which is where the nmi service routine always is */
	PC = 0x66;
	M80_CHANGE_PC(PC);
	g_cycles_executed += 11;	/* Sean Young says this is how many cycles it takes to do an NMI */
	g_context.nmi_state = CLEAR_LINE;	// so that our code can assert the NMI without having to call m80_execute (so it can leave the NMI asserted)
}

/* calls the interrupt service routine */
void m80_activate_irq()
{
	/* IRQ ENABLED HERE */

	Uint32 bus_value = (*g_irq_callback)(0);

	M80_INC_R;	/* increase R register by 1 */
	M80_STOP_HALT;	/* get out of halt mode, if we were in it */

	g_context.IFF1 = 0;
	g_context.IFF2 = 0;	/* disable interrupts once the IRQ is activated */

	/* the interrupt mode determines how we handle the IRQ */
	switch (g_context.interrupt_mode)
	{
	case 0:	/* mode 0 (the instruction on the bus is executed) */
#ifdef CPU_DEBUG
		{
			static Uint8 warned = 0;
			if (!warned)
			{
				fprintf(stderr, "Z80 warning: interrupt mode 0 is being used which isn't well supported!\n");
				fprintf(stderr, "You might want to code in better support for it.\n");
				warned = 1;
			}
		}
#endif
		g_cycles_executed += 2;	/* an RST takes 11 cycles, and since this would take */
									/* 13 cycles if it were an RST, I am adding two now */
									/* and adding the 11 (or whatever) next */
		if (bus_value == 0xFF)
		{
			g_cycles_executed += op_cycles[bus_value];
			M80_RST(0x38);
		}
		else
		{
			fprintf(stderr, "Z80 error: Using interrupt mode 0, a value other than 0xFF was returned on the bus.\n");
			fprintf(stderr, "This is unsupported.  You'll have to code in support for this if you need it.\n");
		}
		break;
	case 1:	/* mode 1 (RST 38h is executed no matter what) */
		g_cycles_executed += 13;	/* according to Sean Young */
		M80_RST(0x38);
		break;
	default:	/* mode 2 */
		g_cycles_executed += 19;	/* according to Sean Young */

		/* Call the address stored at (I*256)+bus_value */
		M80_PUSH16(M80_PC);
		M80_READ_WORD((I << 8) + bus_value, M80_PC);
		M80_CHANGE_PC(PC);
		break;
	} /* end switch */
}

/* returns the current program counter */
Uint32 m80_get_pc()
{
	return (Uint32) PC;
}

/* returns the current stack pointer */
Uint16 m80_get_sp()
{
	return SP;
}

Uint16 m80_get_reg(int index)
{
	return g_context.m80_regs[index].w;
}

void m80_set_reg(int index, Uint16 value)
{
	g_context.m80_regs[index].w = value;
}

void m80_set_pc(Uint32 value)
{
	PC = (Uint16) value;
}

void m80_set_sp(Uint16 value)
{
	SP = value;
}

void m80_set_irq_callback(Sint32 (*callback)(int nothing))
{
	g_irq_callback = callback;	
}

void m80_set_nmi_callback(Sint32 (*callback)())
{
	g_nmi_callback = callback;
}

// returns how many cycles we've executed since the last loop (NOT the total # of cycles executed!!!!!)
Uint32 m80_get_cycles_executed()
{
	return g_cycles_executed;
}

// copies m80's context into 'context' and returns size (in bytes) of the context
Uint32 m80_get_context(void *context)
{
	memcpy(context, &g_context, sizeof(struct m80_context));
	return sizeof(g_context);
}

// replaces m80's context with 'context'
void m80_set_context(void *context)
{
	memcpy(&g_context, context, sizeof(struct m80_context));
}

// what gets called by the cpu debugger to disassemble a section of code ...
unsigned int m80_dasm( char *buffer, unsigned pc )
{
#ifdef CPU_DEBUG
	#ifdef USE_MAME_Z80_DEBUGGER
		return DasmZ80( buffer, m80_get_pc() );
	#else
		sprintf(buffer, "<NOT AVAILABLE>");
		return 1;
	#endif
#else
	sprintf( buffer, "$%02X", cpu_readop(m80_get_pc()) );
	return 1;
#endif
}


// returns an ASCII string giving info about registers and other stuff ...
const char *m80_info(void *context, int regnum)
{
	static char buffer[1][81];
	static int which = 0;

	// WE IGNORE THE CONTEXT :)

	buffer[which][0] = 0;	// make sure the current string is empty

	// find which register they are requesting info about
	switch( regnum )
	{
		case CPU_INFO_REG+M80_PC: sprintf(buffer[which], "PC:%04X", PC); break;
		case CPU_INFO_REG+M80_SP: sprintf(buffer[which], "SP:%04X", SP); break;
		case CPU_INFO_REG+M80_AF: sprintf(buffer[which], "AF:%04X", AF); break;
		case CPU_INFO_REG+M80_BC: sprintf(buffer[which], "BC:%04X", BC); break;
		case CPU_INFO_REG+M80_DE: sprintf(buffer[which], "DE:%04X", DE); break;
		case CPU_INFO_REG+M80_HL: sprintf(buffer[which], "HL:%04X", HL); break;
		case CPU_INFO_REG+M80_IX: sprintf(buffer[which], "IX:%04X", IX); break;
		case CPU_INFO_REG+M80_IY: sprintf(buffer[which], "IY:%04X", IY); break;
		case CPU_INFO_REG+M80_RI: sprintf(buffer[which], "RI:%02X %02X", R, I); break;
		case CPU_INFO_REG+M80_AFPRIME: sprintf(buffer[which], "AF'%04X", AFPRIME); break;
		case CPU_INFO_REG+M80_BCPRIME: sprintf(buffer[which], "BC'%04X", BCPRIME); break;
		case CPU_INFO_REG+M80_DEPRIME: sprintf(buffer[which], "DE'%04X", DEPRIME); break;
		case CPU_INFO_REG+M80_HLPRIME: sprintf(buffer[which], "HL'%04X", HLPRIME); break;
		case CPU_INFO_REG+M80_RI+1: sprintf(buffer[which], "IFF1: %02X IFF2: %02X",
										g_context.IFF1, g_context.IFF2); break;

		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c",
				FLAGS & 0x80 ? 'S':'.',
				FLAGS & 0x40 ? 'Z':'.',
				FLAGS & 0x20 ? '5':'.',
				FLAGS & 0x10 ? 'H':'.',
				FLAGS & 0x08 ? '3':'.',
				FLAGS & 0x04 ? 'P':'.',
				FLAGS & 0x02 ? 'N':'.',
				FLAGS & 0x01 ? 'C':'.');
			break;
	}

	return buffer[which];
}
