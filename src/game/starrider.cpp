/*
 * starrider.cpp
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

// starrider.cpp
// by Mark Broadhead
//

#include <string.h>
#include "starrider.h"
#include "../cpu/mc6809.h"
#include "../io/conout.h"
#include "../ldp-out/ldp.h"

////////////////

// MEMORY MAP (so far)

// MAIN PCB
// -------------------------------
// 0xE000-0xFFFF = ROM (2764 U52)
// 0xD800-0xDFFF = RAM (6116 U7, no chip there, seems like a special case on schematics)
// 0xD000-0xD7FF = RAM (6116 U14)
// 0xC800-0xCFFF = ?
// 0xC000-0xC777 = ?
// 0xB800-0xBFFF = RAM (6116 U25)
// 0xB000-0xB777 = RAM (6116 U36)
// 0xA800-0xAFFF = RAM (6116 U44)
// 0xA000-0xA7FF = RAM (6116 U51)

// 0x2xxx, 0x6xxx, 0xExxx - ROM @ 2764 seems active somehow

// 0x8xxx may enable 2764 also
// 0x4xxx may enable rom 3
// 0x0000 may enable rom 1



// PROCESSOR INTERFACE PCB (PIF)
// -----------------------------------------
// 0xF000-0xFFFF = ROM (U3)
// 0x8xxx = PIA control (U1)
//		A0 : RS0 -> 0 = peripheral register or direction reg, 1 = control register
//		A1 : RS1 -> 0 = peripheral A (Video Graphics Generator), 1 = periperal B (PR-8210A)
// 0x8002 = PR-8210A, perf or direction reg
// 0x8003 = PR-8210A, control reg
// 0x6xxx = ?
// 0x4xxx = "ENABLE"
// 0x2xxx = "CONTROL"
// 0x0000-0x1FFF = RAM (U2, only 2k so memory region repeats every 0-0x7FF)



starrider::starrider()
{
	struct cpudef cpu;
	
	m_shortgamename = "starrider";
	memset(&cpu, 0, sizeof(struct cpudef));
	memset(banks, 0xFF, 3);	// fill banks with 0xFF's
	m_game_type = GAME_STARRIDER;
	m_disc_fps = 29.97;

	cpu.type = CPU_M6809;
	cpu.hz = 14318180 / 4;	
	cpu.initial_pc = 0;
	cpu.must_copy_context = false;	// set to true for multi 6809's
	cpu.nmi_period = (1000.0 / 60.0); // nmi from ?? strobe
	cpu.irq_period[0] = (1000.0 / (10.20 * 60.0)); // firq 8 * hblank + irq + nmi
	cpu.mem = m_cpumem;
	add_cpu(&cpu);	// add 6809

//	m_transparent_color = 0;
	current_bank = 0;

	// ADDME : add ROM information here
	// Since Star Rider doesn't work anyway, I got lazy and didn't add the ROM loading info here
	// You can see the necessary info in the commented-out "load_roms" function below ...
}

// we cheat here... we call do_irq 10 times per hblank and do 8 firq's, 1 irq, and 1 nmi
void starrider::do_irq()
{
	static int int_count = 0;


	if (int_count < 8)
	{
		int_count++;
		
		if (firq_on)
		{
			mc6809_firq = 1;
		}
	}
	else if (int_count == 8)
	{
		int_count++;
		
		if (irq_on)
		{
			mc6809_irq = 1;
		}
	}
	else if (int_count == 9)
	{		
		int_count = 0;

		if (nmi_on)
		{
			mc6809_nmi = 1;
		}
	}
}

// we cheat here... we call this once per hblank to update the display
void starrider::do_nmi()
{
	video_blit();
}

// does anything special needed to send an FIRQ
void starrider::do_firq()
{
}

Uint8 starrider::cpu_mem_read(Uint16 addr)
{
	char s[81] = {0};

	Uint8 result = m_cpumem[addr];

	// this memory is selected through bank (at c800)
	if (addr <= 0x9fff)
	{
		switch (current_bank)
		{
		case 0x8:
			result = rombank1[addr];
			break;
		case 0xc:
			result = rombank2[addr];
			break;
		}			
	}

	// scratch ram
	else if ((addr >= 0xa000 && addr <= 0xbfff) || (addr >= 0xd000 && addr <= 0xdfff))
	{
	}

	// main program
	else if (addr >= 0xe000)
	{
	}

	else 
	{
		sprintf(s, "STARRIDER: Unmapped read from %x", addr);
		printline(s);
	}

	return result;
}

void starrider::cpu_mem_write(Uint16 addr, Uint8 value)
{
	char s[81] = {0};
	
	// this memory is selected through bank (at c800)
	if (addr <= 0x9fff)
	{
		switch (current_bank)
		{
		case 0x8:
			printline("STARRIDER: Attempted write to bank1 rom!");
			break;
		case 0xc:
			printline("STARRIDER: Attempted write to bank2 rom!");
			break;
		}			
	}

	// bank selector
	else if (addr == 0xc800)
	{
		sprintf(s, "STARRIDER: Switch to bank %x", value & 0xf);
		printline(s);
		current_bank = value & 0xf;
	}

	// Watchdog reset
	else if (addr == 0xc900)
	{
	}

	// scratch ram
	else if ((addr >= 0xa000 && addr <= 0xbfff) || (addr >= 0xd000 && addr <= 0xdfff))
	{
	}

	// main program
	else if (addr >= 0xe000)
	{
		printline("STARRIDER: Attempted write to main rom!");
	}

	else 
	{
		sprintf(s, "STARRIDER: Unmapped write to %x with %x", addr, value);
		printline(s);
	}

	m_cpumem[addr] = value;
}

void starrider::palette_calculate()
{
}

// updates starrider's video
void starrider::video_repaint()
{
	for (int charx = 6; charx < 47; charx++)
	{
		for (int chary = 2; chary < 32; chary++)
		{
			for (int x=0; x < 4; x++)
			{
				for (int y = 0; y < 8; y++)
				{
						Uint8 left_pixel = static_cast<Uint8>((character[m_cpumem[chary * 64 + charx + 0x4000]*32+x+4*y] & 0xf0) >> 4);
						Uint8 right_pixel = static_cast<Uint8>((character[m_cpumem[chary * 64 + charx + 0x4000]*32+x+4*y] & 0x0f));
						*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + (((chary - 2) * 8 + y) * STARRIDER_OVERLAY_W) + ((charx  - 6) * 8 + x * 2)) = left_pixel;
						*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + (((chary - 2) * 8 + y) * STARRIDER_OVERLAY_W) + ((charx  - 6) * 8 + x * 2 + 1)) = right_pixel;
				}
			}
		}
	}
}

// this gets called when the user presses a key or moves the joystick
void starrider::input_enable(Uint8 move)
{
	switch (move)
	{
	case SWITCH_UP:
		break;
	case SWITCH_LEFT:
		break;
	case SWITCH_RIGHT:
		break;
	case SWITCH_DOWN:
		break;
	case SWITCH_BUTTON1: // '1' on keyboard
		banks[0] &= ~0x08; 
		break;
	case SWITCH_BUTTON2: // '2' on keyboard
		banks[0] &= ~0x10; 
		break;
	case SWITCH_BUTTON3: // space on keyboard
		banks[0] &= ~0x20; 
		break;
	case SWITCH_COIN1: 
		banks[0] &= ~0x01; 
		break;
	case SWITCH_COIN2: 
		banks[0] &= ~0x02; 
		break;
	case SWITCH_SERVICE: 
		banks[0] &= ~0x04; 
		break;
	case SWITCH_TEST: 
		break;
	default:
		printline("Error, bug in move enable");
		break;
	}
}  

// this gets called when the user releases a key or moves the joystick back to center position
void starrider::input_disable(Uint8 move)
{
	switch (move)
	{
	case SWITCH_UP:
		break;
	case SWITCH_LEFT:
		break;
	case SWITCH_RIGHT:
		break;
	case SWITCH_DOWN:
		break;
	case SWITCH_BUTTON1: // '1' on keyboard
		banks[0] |= 0x08; 
		break;
	case SWITCH_BUTTON2: // '2' on keyboard
		banks[0] |= 0x10; 
		break;
	case SWITCH_BUTTON3: // space on keyboard
		banks[0] |= 0x20; 
		break;
	case SWITCH_COIN1: 
		banks[0] |= 0x01; 
		break;
	case SWITCH_COIN2: 
		banks[0] |= 0x02; 
		break;
	case SWITCH_SERVICE: 
		banks[0] |= 0x04; 
		break;
	case SWITCH_TEST: 
		break;
	default:
		printline("Error, bug in move enable");
		break;
	}
}

// used to set dip switch values
bool starrider::set_bank(unsigned char which_bank, unsigned char value)
{
	bool result = true;
	
	switch (which_bank)
	{
	case 0:	// dipswitch 1
		banks[1] = (unsigned char) (value ^ 0xFF);	// dip switches are active low
		break;
	case 1:	// dipswitch 2
		banks[2] = (unsigned char) (value ^ 0xFF);	// switches are active low
		break;
	default:
		printline("ERROR: Bank specified is out of range!");
		result = false;
		break;
	}
	
	return result;
}
