/*
 * cputest.cpp
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

// CPU TEST
// only tests the Z80
// by Matt Ownby

// just loads a generic rom
// For CPU core development and testing

#include <stdio.h>
#include <stdlib.h>
#include <string.h>	// for memset
#include "cputest.h"
#include "../cpu/cpu-debug.h"
#include "../cpu/cpu.h"
#include "../cpu/generic_z80.h"
#include "../io/conout.h"
#include "../io/input.h"
#include "../timer/timer.h"
#include "../io/numstr.h"

//////////////////////////////////////////////////////////////////////////

cputest::cputest() : m_uZeroCount(0), m_bStarted(false)
{
	struct cpudef cpu;	// structure we will define our cpu in
	
	m_shortgamename = "cputest";
	memset(&cpu, 0, sizeof(struct cpudef));
	cpu.type = CPU_Z80;
	cpu.hz = 2000000000;	// 2000 mhz (fast as possible)!
	cpu.initial_pc = 0x100;
	cpu.must_copy_context = false;
	cpu.mem = m_cpumem;
	add_cpu(&cpu);	// add this cpu to the list (it will be our only one)
	
	m_disc_fps = 29.97;	// for now this needs _some_ value for vblank purposes

	m_num_sounds = 0;
	m_game_uses_video_overlay = false;
}

bool cputest::init()
{
	bool bSuccess = false;

#ifdef CPU_DEBUG
	// this cpu test requires CPU_DEBUG to be enabled because that's the only way that the update_pc callback will get called.
	bSuccess = true;
#else
	printline("This build was not compiled with CPU_DEBUG defined.  Recompile with CPU_DEBUG defined in order to run the cpu tests.");
#endif // CPU_DEBUG

	return bSuccess;
}

void cputest::shutdown()
{
	Uint32 elapsed_ms = GET_TICKS() - m_speedtimer;
	
	string s = "Z80 cputest executed in ";
	s += numstr::ToStr(elapsed_ms);
	s += " ms";
	printline(s.c_str());
}

void cputest::start()
{
	m_speedtimer = GET_TICKS();
	m_bStarted = true;
}

Uint8 cputest::port_read(Uint16 port)
{
	return 0;
}

// writes a byte to the cpu's port
void cputest::port_write(Uint16 port, Uint8 value)
{
}

void cputest::update_pc(Uint32 new_pc)
{
	// this halts the tests
	if (new_pc == 0)
	{
		if (m_bStarted && !get_quitflag())
		{
			printline("PC went to 0 (test complete)");
			set_quitflag();
		}
	}

	// the MSX has a subroutine at 5 that can print messages to the screen (indicating pass or fail)
	else if (new_pc == 5)
	{
		Uint8 command = Z80_GET_BC & 0x0F;
		Uint8 *membase = m_cpumem;

		// if C is 9 when '5' is called, it means to print a $-terminated message to the screen
		if (command == 9)
		{
			char s[81] = { 0 };
			int i = 0;
			Uint16 srcmsg = Z80_GET_DE;	// the source message is stored at DE
			Uint16 sp = Z80_GET_SP;
			Uint16 new_pc = membase[sp] | (membase[sp+1] << 8);	// calculate return value
			
			// strings are terminated with a $ oddly enough
			while ((membase[srcmsg+i] != '$') && (i < 80))
			{
				s[i] = membase[srcmsg+i];
				i++;
			}
			s[i] = 0;	// terminate string
			printline(s);	// and print it to the console
			
			Z80_SET_PC(new_pc);	// return from call
			Z80_SET_SP(sp+2);	// pop return address off stack
		}
		// if C is 0, it means to terminate the program (I think!)
		else if (command == 0)
		{
			printline("Got quit command!\n");
			set_quitflag();
		}
		// print just one character to the console
		else if (command == 2)
		{
			Uint16 sp = Z80_GET_SP;
			Uint16 new_pc = membase[sp] | (membase[sp+1] << 8);	// calculate return value
			outchr(Z80_GET_DE & 0xFF);
			Z80_SET_PC(new_pc);	// return from call
			Z80_SET_SP(sp+2);	// pop return address off stack			
		}
		else
		{
			printf("Warning, unknown command received at 5!\n");
			set_quitflag();
		}
	}
}

void cputest::set_preset(int val)
{
	// NOTE : I've streamlined this down to the only two rom test programs that I use with the Z80 CPU anymore
	// The first one is useful for benchmarking, the second is useful for testing accuracy of emulation (and also benchmarking hehe)

	const static struct rom_def savage_rom[] =
	{
		{ "savage.com", NULL, &m_cpumem[0x100], 14080, 0xc8d0edf6 },
		{ NULL }
	};

	const static struct rom_def zexall_rom[] = 
	{
		{ "zexall.com", NULL, &m_cpumem[0x100], 8704, 0xecf70fd6 },
		{ NULL }
	};

	const static struct rom_def branchtest_rom[] = 
	{
		{ "branchtest.bin", NULL, &m_cpumem[0x100], 106, 0x7d0c9115 },
		{ NULL }
	};

	switch (val)
	{
	case 0:
		m_rom_list = savage_rom;
		break;
	case 1:	// this is the real test, right here
		m_rom_list = zexall_rom;
		break;
	case 2:	// tests for GCC4 compiler bug
		m_rom_list = branchtest_rom;
		break;
	default:
		printline("Bad preset!");
		set_quitflag();
		break;
	} // end switch	
}

