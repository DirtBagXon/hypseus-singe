/*
 * cop.cpp
 *
 * Copyright (C) 2001 Mark Broadhead
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

/****************************************************************************/
/*                                                                          */
/*         Portable National Semiconductor COP420/421 MCU emulator          */
/*                                                                          */
/*	   Copyright 2001 Mark Broadhead, released under the GPL 2 License		*/
/*																			*/
/* The COP MCU has an internal clock divider that is mask programmed into	*/
/* the device. Make sure it's set correctly in the frequency to call		*/
/* cop421_execute(unsigned int cycles). Thayer's Quest uses a 32 divider	*/
/* and Looping uses a 4 divider.											*/
/*																			*/
/* Some of the functions of the 420	aren't implemented, including the inil	*/
/* and inin commands. Also some	functions of the EN register aren't			*/
/* implemented.																*/
/*                                                                          */
/****************************************************************************/

// MATT : Visual Studio .NET links to some new C and C++ libraries which require bundling new DLL's
// in with the code.  Therefore, in order to get rid of the C++ .DLL, I am getting rid of all
// 'iostream' code found in daphne, which is basically only in this file, and only error messages.
// Since Thayer's Quest seems to be working just fine, I figure we don't need error messages to be displayed.
// If in the future error messages become necessary, you can uncomment the cout's in here for your
// own purposes.

#include "cop.h"
//#include <iostream.h>
//#include <iomanip.h>

#ifdef WIN32
#pragma warning (disable:4244)	// disable the warning about possible loss of data
#endif

// Internal functions
void preinst(void); // Function for pre intruction execution
void pc_inc(void); // Function for PC incrementing
void counter_inc(void); // Function for internal counter incrementing
void execute_one_byte(); // Execute one byte instruction
void execute_two_byte(); // Execute one two instruction
void push_stack(); // Push Stack
void pop_stack(); // Pop Stack

// Instruction functions
void skmbz(unsigned char);
void xis(unsigned char);
void ld(unsigned char);
void x(unsigned char);
void xds(unsigned char);
void lbi(unsigned char, unsigned char);
void rmb(unsigned char);
void smb(unsigned char);
void skgbz(unsigned char);

// Global Variables for Internal Registers
unsigned char A = 0; //	4-bit Accumulator
unsigned char Br = 0; // 6-bit RAM Address Register(top 2 bits)
unsigned char Bd = 0; // 6-bit RAM Address Register(bottom 4 bits)
unsigned char C = 0; // 1-bit Carry Register
unsigned char EN = 0; // 4-bit Enable Register
unsigned char G = 0; // 4-bit Register to latch data for G I/O Port
unsigned char L = 0; // 8-bit TRI-STATE I/O Port
unsigned int PC = 0; // 10-bit ROM Address Register (program counter)
unsigned char Q = 0; // 8-bit Register to latch data for L I/O Port
unsigned int SA = 0; // 10-bit Subroutine Save Register A
unsigned int SB = 0; // 10-bit Subroutine Save Register B
unsigned int SC = 0; // 10-bit Subroutine Save Register C
unsigned char SIO = 0; // 4-bit Shift Register and Counter
unsigned int COUNTER = 0; // 10-bit Internal Counter
unsigned char COUNT_CARRY = 0; // Counter Latch
unsigned char cur_inst;
unsigned char cur_operand;
unsigned int inst_pc;
unsigned char g_skip = 0; // g_skip flag
unsigned char lbi_skip = 0; // LBI skip flag TODO: implement this

// Global array for internal RAM
unsigned char copram[0x4][0x10];

// Global pointer to internal ROM
unsigned char *coprom;

void cop421_reset(void)
{
	A = Br = Bd = C = EN = G = PC = SA = SB = SC = 0;
}

void cop421_setmemory(unsigned char *rom)
{
	coprom = rom;
}

unsigned int cop421_execute(unsigned int cycles)
{
	unsigned int completed_cycles;
	
	for (completed_cycles = 0; completed_cycles < cycles; completed_cycles++)
	{
		
		// Store current instruction
		cur_inst = coprom[PC];
		inst_pc = PC;
		// Do preinstruction stuff
		preinst();
		pc_inc();		

		if (!g_skip)
		{
			if (cur_inst == 0x23 || cur_inst == 0x33 || (cur_inst >= 0x60 && cur_inst <= 0x63) || (cur_inst >= 0x68 && cur_inst <= 0x6b))
			{
				cur_operand = coprom[PC];
				pc_inc();
				execute_two_byte();
				
				// TODO: two byte instructions take two clocks?
				completed_cycles++; 
				counter_inc();
				counter_inc();
			}
	
			else // If its one byte
			{
				counter_inc();
				execute_one_byte();
			}
		}
		else
		{
			g_skip = 0;
			if (cur_inst == 0x23 || cur_inst == 0x33 || (cur_inst >= 0x60 && cur_inst <= 0x63) || (cur_inst >= 0x68 && cur_inst <= 0x6b))
			{
				pc_inc();
			}
			completed_cycles--; // TODO: a skipped instruction doesn't take a clock cycle?
		}
	}

	return completed_cycles;
}

void execute_one_byte()
{
	unsigned char temp;
	switch (cur_inst)
	{		
	case 0x00: // CLRA
		A = 0;
		break;
	case 0x01: // SKMBZ 0
		skmbz(0x0);
		break;
	case 0x02: // XOR
		A = (A ^ copram[Br][Bd]) & 0xf;
		break;
	case 0x03: // SKMBZ 2
		skmbz(0x2);
		break;
	case 0x04: // XIS 0
		xis(0x0);
		break;
	case 0x05: // LD 0
		ld(0x0);
		break;
	case 0x06: // X 0
		x(0x0);
		break;
	case 0x07: // XDS 0
		xds(0x0);
		break;
	case 0x08: // LBI 0,9
		lbi(0, 9);
		break;
	case 0x09: // LBI 0,10
		lbi(0, 10);
		break;
	case 0x0a: // LBI 0,11
		lbi(0, 11);
		break;
	case 0x0b: // LBI 0,12
		lbi(0, 12);
		break;
	case 0x0c: // LBI 0,13
		lbi(0, 13);
		break;
	case 0x0d: // LBI 0,14
		lbi(0, 14);
		break;
	case 0x0e: // LBI 0,15
		lbi(0, 15);
		break;
	case 0x0f: // LBI 0,0
		lbi(0, 0);
		break;
	case 0x10: // CASC
		A = ((~A & 0x0f) + copram[Br][Bd] + C);
		C = (A & 0xF0)?1:0;
		A &= 0x0F;
		if (C)
		{
			g_skip = 1;
		}
		break;
	case 0x11: // SKMBZ 1
		skmbz(0x1);
		break;
	case 0x12: // XABR
		temp = A;
		A = Br;
		Br = temp & 0x3;
		break;
	case 0x13: // SKMBZ 3
		skmbz(0x3);
		break;
	case 0x14: // XIS 1
		xis(0x1);
		break;
	case 0x15: // LD 1
		ld(0x1);
		break;
	case 0x16: // X 1
		x(0x1);
		break;
	case 0x17: // XDS 1
		xds(0x1);
		break;
	case 0x18: // LBI 1,9
		lbi(1, 9);
		break;
	case 0x19: // LBI 1,10
		lbi(1, 10);
		break;
	case 0x1a: // LBI 1,11
		lbi(1, 11);
		break;
	case 0x1b: // LBI 1,12
		lbi(1, 12);
		break;
	case 0x1c: // LBI 1,13
		lbi(1, 13);
		break;
	case 0x1d: // LBI 1,14
		lbi(1, 14);
		break;
	case 0x1e: // LBI 1,15
		lbi(1, 15);
		break;
	case 0x1f: // LBI 1,0
		lbi(1, 0);
		break;
	case 0x20: // SKC
		if (C)
		{
			g_skip = 1;
		}
		break;
	case 0x21: // SKE
		if (A == copram[Br][Bd])
		{
			g_skip = 1;
		}
		break;
	case 0x22: // SC
		C = 1;
		break;
	case 0x24: // XIS 2
		xis(0x2);
		break;
	case 0x25: // LD 2
		ld(0x2);
		break;
	case 0x26: // X 2
		x(0x2);
		break;
	case 0x27: // XDS 2
		xds(0x2);
		break;
	case 0x28: // LBI 2,9
		lbi(2, 9);
		break;
	case 0x29: // LBI 2,10
		lbi(2, 10);
		break;
	case 0x2a: // LBI 2,11
		lbi(2, 11);
		break;
	case 0x2b: // LBI 2,12
		lbi(2, 12);
		break;
	case 0x2c: // LBI 2,13
		lbi(2, 13);
		break;
	case 0x2d: // LBI 2,14
		lbi(2, 14);
		break;
	case 0x2e: // LBI 2,15
		lbi(2, 15);
		break;
	case 0x2f: // LBI 2,0
		lbi(2, 0);
		break;
	case 0x30: // ASC
		A = (A + copram[Br][Bd] + C);
		C = (A & 0xF0)?1:0;
		A = A & 0x0F;
		if (C)
		{
			g_skip = 1;
		}
		break;
	case 0x31: // ADD
		A = (A + copram[Br][Bd]) & 0x0F; 
		break;
	case 0x32: // RC
		C = 0;
		break;
	case 0x34: // XIS 3
		xis(0x3);
		break;
	case 0x35: // LD 3
		ld(0x3);
		break;
	case 0x36: // X 3
		x(0x3);
		break;
	case 0x37: // XDS 3
		xds(0x3);
		break;
	case 0x38: // LBI 3,9
		lbi(3, 9);
		break;
	case 0x39: // LBI 3,10
		lbi(3, 10);
		break;
	case 0x3a: // LBI 3,11
		lbi(3, 11);
		break;
	case 0x3b: // LBI 3,12
		lbi(3, 12);
		break;
	case 0x3c: // LBI 3,13
		lbi(3, 13);
		break;
	case 0x3d: // LBI 3,14
		lbi(3, 14);
		break;
	case 0x3e: // LBI 3,15
		lbi(3, 15);
		break;
	case 0x3f: // LBI 3,0
		lbi(3, 0);
		break;
	case 0x40: // COMP
		A = (~A) & 0xf;
		break;
	case 0x41: // SKT
		if (COUNT_CARRY)
		{
			g_skip = 1;
			COUNT_CARRY = 0;
		}
		break;
	case 0x42: // RMB 2
		rmb(2);
		break;
	case 0x43: // RMB 3
		rmb(3);
		break;
	case 0x44: // NOP
		break;
	case 0x45: // RMB 1
		rmb(1);
		break;
	case 0x46: // SMB 2
		smb(2);
		break;
	case 0x47: // SMB 1
		smb(1);
		break;
	case 0x48: // RET
		pop_stack();
		break;
	case 0x49: // RETSK
		pop_stack();
		g_skip = 1;
		break;
	case 0x4a: // ADT
		A = (A + 10) & 0xf;
		break;
	case 0x4b: // SMB 3
		smb(3);
		break;
	case 0x4c: // RMB 0
		rmb(0);
		break;
	case 0x4d: // SMB 0
		smb(0);
		break;
	case 0x4e: // CBA
		A = Bd;
		break;
	case 0x4f: // XAS
		temp = A;
		A = SIO;
		SIO = temp;
		break;
	case 0x50: // CAB
		Bd = A;
		break;
	case 0x51: // AISC
	case 0x52:
	case 0x53:
	case 0x54:
	case 0x55:
	case 0x56:
	case 0x57:
	case 0x58:
	case 0x59:
	case 0x5a:
	case 0x5b:
	case 0x5c:
	case 0x5d:
	case 0x5e:
	case 0x5f:
		A = A + (cur_inst & 0xf);
		if (A & 0xf0)
		{
			g_skip = 1;
		}
		A &= 0xf;
		break;	
	case 0x70: // STII
	case 0x71:
	case 0x72:
	case 0x73:
	case 0x74:
	case 0x75:
	case 0x76:
	case 0x77:
	case 0x78:
	case 0x79:
	case 0x7a:
	case 0x7b:
	case 0x7c:
	case 0x7d:
	case 0x7e:
	case 0x7f:
		copram[Br][Bd] = cur_inst & 0x0f;
		Bd++;
		if (Bd > 0xf)
		{
			Bd = 0;
		}
		break;
	case 0xbf: // LQID
		Q = coprom[(PC & 0x300) | ((A << 4) & 0xf0) | (copram[Br][Bd] & 0xf)];
		SC = SB;
		break;
	case 0xff: // JID
		PC = coprom[(PC & 0x300) | ((A << 4) & 0xf0) | (copram[Br][Bd] & 0xf)];
		break;
	default:
		if (cur_inst >= 0x80 && cur_inst < 0xbf) 
		{
			if (PC >= 0x080 && PC < 0x100) // JP within SRP
			{				
				PC = (PC & 0x380) | (cur_inst & 0x7f);
			}
			else // JSRP
			{				
				push_stack();
				PC = 0x080 | (cur_inst & 0x3f);
			}
		}
		else if (cur_inst >= 0xc0 && cur_inst < 0xff)
		{
			PC = (PC & 0x3c0) | (cur_inst & 0x3f);
		}
		else
		{
			// MATT : commented this stuff out for reasons explained at top of file
//			cout.setf(ios::hex);
//			cout.setf(ios::right, ios::adjustfield);
//			cout << "Invalid Instruction: " << setw(2) << setfill( '0' ) << static_cast<int>(cur_inst) << " at PC: " << setw(3) << setfill( '0' ) << static_cast<int>(inst_pc) << endl;
			break;
		}
	}
}

void execute_two_byte()
{
	unsigned char temp;
	switch (cur_inst)
	{		
	case 0x23:
		if (cur_operand < 0x40) // LDD
		{
			A = copram[cur_operand >> 4][cur_operand & 0xf];
		}
		else if (cur_operand >= 0x80 && cur_operand < 0xc0) // XAD
		{
			temp = A;
			A = copram[(cur_operand >> 4) & 0x3][cur_operand & 0xf];
			copram[(cur_operand >> 4) & 0x3][cur_operand & 0xf] = temp;
		}
		else 
		{
			// MATT : commented this stuff out (see top of file)
//			cout.setf(ios::hex);
//			cout.setf(ios::right, ios::adjustfield);
//			cout << "Invalid Operand for " << setw(2) << setfill( '0' ) << static_cast<int>(cur_inst) << ": " << static_cast<int>(cur_operand) << " at PC: " << setw(3) << setfill( '0' ) << static_cast<int>(inst_pc) << endl;
		}			
		break;
	case 0x33:
		switch(cur_operand)
		{
		case 0x3c: // CAMQ
			Q = (A << 4) | copram[Br][Bd];
			break;
		case 0x2c: // CQMA
			A = Q & 0x0f;
			copram[Br][Bd] = (Q >> 4);
			break;
		case 0x2a: // ING
			A = read_g_port();
			break;
		case 0x29: // INIL
			// MATT commented out (See top)
//			cout << "Unimplemented Opcode - INIL!" << endl;
			break;
		case 0x28: // ININ
			// MATT commented out (see top)
//			cout << "Unimplemented Opcode - ININ!" << endl;
			break;
		case 0x2e: // INL
			A = (L & 0x0f);
			copram[Br][Bd] = (L >> 4) & 0x0f;
			break;
		case 0x81: // LBI
		case 0x82:
		case 0x83:
		case 0x84:
		case 0x85:
		case 0x86:
		case 0x87:
		case 0x88:
		case 0x91:
		case 0x92:
		case 0x93:
		case 0x94:
		case 0x95:
		case 0x96:
		case 0x97:
		case 0x98:
		case 0xa1:
		case 0xa2:
		case 0xa3:
		case 0xa4:
		case 0xa5:
		case 0xa6:
		case 0xa7:
		case 0xa8:
		case 0xb1:
		case 0xb2:
		case 0xb3:
		case 0xb4:
		case 0xb5:
		case 0xb6:
		case 0xb7:
		case 0xb8:
			lbi((((cur_operand & 0xf0) >> 4) & 0x03), (cur_operand & 0x0f));
			break;
		case 0x60: // LEI
		case 0x61:
		case 0x62:
		case 0x63:
		case 0x64:
		case 0x65:
		case 0x66:
		case 0x67:
		case 0x68:
		case 0x69:
		case 0x6a:
		case 0x6b:
		case 0x6c:
		case 0x6d:
		case 0x6e:
		case 0x6f:
			EN = (cur_operand & 0x0f);
			break;
		case 0x3e: // OBD
			write_d_port(Bd);	
			break;
		case 0x3a: // OMG
			G = copram[Br][Bd];
			break;
		case 0x50: // OGI
		case 0x51:
		case 0x52:
		case 0x53:
		case 0x54:
		case 0x55:
		case 0x56:
		case 0x57:
		case 0x58:
		case 0x59:
		case 0x5a:
		case 0x5b:
		case 0x5c:
		case 0x5d:
		case 0x5e:
		case 0x5f:
			G = cur_operand & 0x0f;
			break;
		case 0x01: // SKGBZ 0
			skgbz(0);
			break;
		case 0x11: // SKGBZ 1
			skgbz(1);
			break;
		case 0x03: // SKGBZ 2
			skgbz(2);
			break;
		case 0x13: // SKGBZ 3
			skgbz(3);
			break;
		case 0x21: // SKGZ
			if (!(read_g_port()))
			{
				g_skip = 1;
			}
			break;
		default:
			// MATT : commented this out (see top of file)
			//cout.setf(ios::hex);
			//cout.setf(ios::right, ios::adjustfield);
			//cout << "Invalid Operand for " << setw(2) << setfill( '0' ) << static_cast<int>(cur_inst) << ": " << static_cast<int>(cur_operand) << " at PC: " << setw(3) << setfill( '0' ) << static_cast<int>(inst_pc) << endl;
			break;
		}
		break;
	
	case 0x60: // JMP
	case 0x61:
	case 0x62:
	case 0x63:
		PC = ((cur_inst & 0x03) << 8) | cur_operand;
		break;

	case 0x68: // JSR
	case 0x69:
	case 0x6a:
	case 0x6b:
		push_stack();
		PC = ((cur_inst & 0x03) << 0x8) | cur_operand;
		break;

	default:
		// MATT : commented this out (See top of file)
		//cout.setf(ios::hex);
		//cout.setf(ios::right, ios::adjustfield);
		//cout << "Invalid Operand for " << setw(2) << setfill( '0' ) << static_cast<int>(cur_inst) << ": " << static_cast<int>(cur_operand) << " at PC: " << setw(3) << setfill( '0' ) << static_cast<int>(inst_pc) << endl;
		break;
	}
}

void preinst()
{
	if ((EN & 0x1) && (EN & 0x8))
	{
		write_so_bit(1);
	}
	
	else if ((!(EN & 0x1)) && (!(EN & 0x8)))
	{
		write_so_bit(0);
		SIO = (((SIO << 1) & 0xf) | (read_si_bit()));
	}
	
	if (EN & 0x4)
	{
		write_l_port(Q);
	}
	else
	{
		L = read_l_port();
	}
}

void pc_inc()
{
	// Increment program counter
	if (PC < 0x400)
	{
		PC++;
	}
	else 
	{
		PC = 0x000;
	}
}

void counter_inc()
{
	// Increment the internal counter
	if (COUNTER < 0x400)
	{
		COUNTER++;
	}
	else
	{
		COUNT_CARRY = 1;
		COUNTER = 0x000;
	}
}

void pop_stack()
{
	PC = SA;
	SA = SB;
	SB = SC;
	SC = 0;
}

void push_stack()
{
	SC = SB;
	SB = SA;
	SA = PC;
}

void skmbz(unsigned char o)
{
	if (!((copram[Br][Bd] >> o) & 0x01))
	{
		g_skip = 1;
	}
}

void xis(unsigned char o)
{
	unsigned char temp;
	temp = A;
	A = copram[Br][Bd];
	copram[Br][Bd] = temp;
	if (Bd < 15)
	{
		Bd++;
	}
	else
	{
		Bd = 0;
		g_skip = 1;
	}
	Br = (Br ^ o) & 0x3;
}

void ld(unsigned char o)
{
	A = copram[Br][Bd];
	Br = (Br ^ o) & 0x03;
}

void x(unsigned char o)
{
	unsigned char temp;
	temp = A;
	A = copram[Br][Bd];
	copram[Br][Bd] = temp;
	Br = (Br ^ o) & 0x03;
}

void xds(unsigned char o)
{
	unsigned char temp;
	temp = A;
	A = copram[Br][Bd];
	copram[Br][Bd] = temp;
	if (Bd > 0)
	{
		Bd--;
	}
	else 
	{
		Bd = 0xf;
		g_skip = 1;
	}
	Br = (Br ^ o) & 0x3;
}

void lbi(unsigned char r, unsigned char d)
{
	Br = r;
	Bd = d;
}

void rmb(unsigned char o)
{
	copram[Br][Bd] = ((copram[Br][Bd] & ~(0x01 << o)) & 0x0f);
}

void smb(unsigned char o)
{
	copram[Br][Bd] = ((copram[Br][Bd] | (0x01 << o)) & 0x0f);
}

void skgbz(unsigned char o)
{
	if (!((read_g_port() >> o) & 0x1))
	{
		g_skip = 1;
	}
}
