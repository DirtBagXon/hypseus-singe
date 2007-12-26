/*
 * interstellar.cpp
 *
 * Copyright (C) 2002-2005 Mark Broadhead
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


// interstellar.cpp
// by Mark Broadhead
//
// Interstellar ((C) 1983 Funai/ESP)
//
// TODO:
//	- what does memory address c000 on the ldp communications z80 do?
//	- find out the real clock speed for the z80s
//
//
// Memory Map
//
// Z80 0 (master)
// 0000-9fff - ROM (rom2-rom6)
// a000-a7ff - WORK RAM 
// a800-abff - VIDEO RAM 
// ac00-afff - VIDEO CONTROL RAM 
// b000-b1ff - SPRITE RAM
//
//					Input									Output
// Port 00 -	cp inputs							sound (to other sound z80)
// Port 01 -	unknown								unknown
// Port 02 -	dipswitch							unknown
// Port 03 -	dipswitch, more inputs			reenable NMI
// Port 04 -	unknown								8 bit color value for background
// Port 05 -	latch from z80 2, port 1		latch to z80 2, port 1
//
// IRQ - loads stuff into b000, probably from vblank
// NMI - only checks coins so its probably only caused by a coin input
//
//
//
// Z80 1 (sound)
// 0000-1fff - ROM (rom1)
// 4000-47ff - WORK RAM
//
//					Input								Output
// Port 00 -	latch from z80 0, port 0	unknown
// Port 01 -	reenable NMI					output to sn79489 #1 (front?)
// Port 02 -	unknown							output to sn79489 #2 (front?)
//
// IRQ - unused (jumps to 0x0000)
// NMI - caused when master z80 writes to sound latch
//
//
//
// Z80 2 (ldp communications)
// 0000 - 17ff - ROM (rom11, the rom is actually 0x2000 in size, but the top half isn't addressed)
// 1800 - 1fff - WORK RAM
// C000        - unknown, I believe it is connected to a timer on the top board though
//
//					Input									Output
// Port 00 -	LD-V1000 status					LD-V1000 command
// Port 01 -	latch from z80 0, port 5		latch to z80 0, port 5
// Port 02 -	reenable	NMI						unknown	
// Port 03 -	unknown								turn on/off LD background	
//
// IRQ - this is probably from ldv1000 status strobe
// NMI - caused when master z80 writes to ldp latch
//
//

#include <string.h>
#include "interstellar.h"
#include "../cpu/cpu.h"
#include "../io/conout.h"
#include "../ldp-in/ldv1000.h"
#include "../ldp-out/ldp.h"
#include "../video/palette.h"
#include "../cpu/generic_z80.h"

interstellar::interstellar()
{
	struct cpudef cpu;
	struct sounddef soundchip;
	
	m_shortgamename = "interstellar";
	m_disc_fps = 29.97; 
	m_game_type = GAME_INTERSTELLAR;

	memset(&cpu, 0, sizeof(struct cpudef));
	cpu.type = CPU_Z80;
	cpu.hz = INTERSTELLAR_CPU_SPEED; // unverified
	cpu.irq_period[0] = (1000.0 / 59.94); // this appears to be from vblank
	cpu.nmi_period = 0; // these come from coin inserts
	cpu.mem = m_cpumem;
	cpu.initial_pc = 0;
	cpu.must_copy_context = true;
	add_cpu(&cpu);	// add Z80 cpu

	memset(&cpu, 0, sizeof(struct cpudef));
	cpu.type = CPU_Z80;
	cpu.hz = INTERSTELLAR_CPU_SPEED; // unverified
	cpu.irq_period[0] = 0; 
	cpu.nmi_period = 0; // these come from the main cpu writing to the sound latch
	cpu.mem = m_cpumem2;
	cpu.initial_pc = 0;
	cpu.must_copy_context = true;
	add_cpu(&cpu);	// add Z80 cpu

	memset(&cpu, 0, sizeof(struct cpudef));
	cpu.type = CPU_Z80;
	cpu.hz = INTERSTELLAR_CPU_SPEED; // unverified
	cpu.irq_period[0] = (1000.0 / 59.94); // from ld-v1000 status strobe
	cpu.nmi_period = 0.0; // caused by writes from main cpu
	cpu.mem = m_cpumem3;
	cpu.initial_pc = 0;
	cpu.must_copy_context = true;
	add_cpu(&cpu);	// add Z80 cpu

	cpu_change_interleave(5);

	memset(&soundchip, 0, sizeof(struct sounddef));
	soundchip.type = SOUNDCHIP_SN76496;  // Interstellar uses 2 SN76496 sound chips
   soundchip.hz = INTERSTELLAR_CPU_SPEED;   // unverified
   m_soundchip1_id = add_soundchip(&soundchip); // add both chips
   m_soundchip2_id = add_soundchip(&soundchip);

	banks[0] = 0x00; // DON'T CHANGE, MUST BE 0x00!
	banks[1] = 0x00; // DON'T CHANGE, MUST BE 0x00!
	banks[2] = 0x30; // DON'T CHANGE, MUST BE 0x30!

	m_video_overlay_width = INTERSTELLAR_OVERLAY_W;
	m_video_overlay_height = INTERSTELLAR_OVERLAY_H;
	m_palette_color_count = INTERSTELLAR_COLOR_COUNT;
	m_video_row_offset = -16;	// 32 pixels, 16 rows

	// 0 is the transparent value by default, so we need set up nothing

	m_cpu0_nmi_enable = false;
	m_cpu1_nmi_enable = false;
	m_cpu2_nmi_enable = false;
//	bool m_unsticker = false;


	// NOTE : this must be static
	static struct rom_def roms[] =
	{
		// Z80 #0 program
		{ "rom2.top", NULL, &m_cpumem[0x0000], 0x2000, 0x5d643381 },
		{ "rom3.top", NULL, &m_cpumem[0x2000], 0x2000, 0xce5a2b09 },
		{ "rom4.top", NULL, &m_cpumem[0x4000], 0x2000, 0x7c2cb1f1 },
		{ "rom5.top", NULL, &m_cpumem[0x6000], 0x2000, 0x354377f6 },
		{ "rom6.top", NULL, &m_cpumem[0x8000], 0x2000, 0x0319bf40 },

		// Z80 #1 program
		{ "rom1.top", NULL, &m_cpumem2[0x0000], 0x2000, 0x4f34fb1d },
		
		// Z80 #2 program
		{ "rom11.bot", NULL, &m_cpumem3[0x0000], 0x2000, 0x165cbc57 },
		
		// Tiles
		{ "rom7.bot", NULL, &character[0x0000], 0x2000, 0x1447ce3a },
		{ "rom8.bot", NULL, &character[0x2000], 0x2000, 0xe9c9e490 },
		{ "rom9.bot", NULL, &character[0x4000], 0x2000, 0x9d79acb6 },

		// Color proms
		{ "red6b.bot",   NULL, &color_prom[0x000], 0x100, 0x5c52f844 },
		{ "green6c.bot", NULL, &color_prom[0x100], 0x100, 0x7d8c845c },
		{ "blue6d.bot",  NULL, &color_prom[0x200], 0x100, 0x5ebb81f9 },
		{ NULL }
	};

	m_rom_list = roms;
}

// does anything special needed to send an IRQ
void interstellar::do_irq(unsigned int which_irq)
{
	if (cpu_getactivecpu() == 0)
	{
			// only do a display_update on the vblank irq
			video_blit();
	}
	Z80_ASSERT_IRQ;
}

// does anything special needed to send an NMI
void interstellar::do_nmi()
{
	Z80_ASSERT_NMI;
}

// reads a byte from the cpu's memory
Uint8 interstellar::cpu_mem_read(Uint16 addr)
{
	Uint8 result = 0x00;
	char s[81] = { 0 };

	switch (cpu_getactivecpu())
	{
	case 0:
		if (addr <= 0x9fff)	// rom
		{
			result = m_cpumem[addr];
		}
		else if (addr >= 0xa000 && addr <= 0xb1ff ) // work + video + video control + sprite ram
		{
			result = m_cpumem[addr];
		}
		else
		{
			sprintf(s, "INTERSTELLAR: CPU 0: Unsupported Memory Read-> %x (PC is %x)", addr, Z80_GET_PC);
			printline(s);
			result = m_cpumem[addr];
		}
		break;
	case 1:
		if (addr <= 0x1fff)	// rom
		{
			result = m_cpumem2[addr];
		}
		else if (addr >= 0x4000 && addr <= 0x47ff ) // ram
		{
			result = m_cpumem2[addr];
		}
		else
		{
			sprintf(s, "INTERSTELLAR: CPU 1: Unsupported Memory Read-> %x (PC is %x)", addr, Z80_GET_PC);
			printline(s);
			result = m_cpumem2[addr];
		}
		break;
	case 2:
		if (addr <= 0x17ff)	// rom
		{
			result = m_cpumem3[addr];
		}
		else if (addr >= 0x1800 && addr <= 0x1fff ) // ram
		{
			result = m_cpumem3[addr];
		}
		else if (addr == 0xc000) // ?
		{
			result = m_cpumem3[addr];
		}
		else
		{
			sprintf(s, "INTERSTELLAR: CPU 2: Unsupported Memory Read-> %x (PC is %x)", addr, Z80_GET_PC);
			printline(s);
			result = m_cpumem3[addr];
		}
		break;
	default:
		{
			printline("cpu_mem_read from invalid CPU!");
		}
		break;
	}
	return result;
}

// writes a byte to the cpu's memory
void interstellar::cpu_mem_write(Uint16 addr, Uint8 value)
{
	char s[81] = { 0 };	
	
	switch (cpu_getactivecpu())
	{
	case 0:
		if (addr <= 0x9fff)	// rom	
		{
//			sprintf(s, "INTERSTELLAR: CPU 0: Attemped write to ROM!-> %x with %x (PC is %x)", addr, value, Z80_GET_PC);
//			printline(s);
		}
		else if (addr >= 0xa000 && addr <= 0xa7ff ) // ram
		{
			m_cpumem[addr] = value;
		}
		else if (addr >= 0xa800 && addr <= 0xb1ff ) // video + video control + sprite ram
		{
			m_cpumem[addr] = value;
			m_video_overlay_needs_update = true;
		}
		else
		{
			m_cpumem[addr] = value;	
			
			sprintf(s, "INTERSTELLAR: CPU 0: Unsupported Memory Write-> %x with %x (PC is %x)", addr, value, Z80_GET_PC);
			printline(s);
		}	
		break;
	case 1:
		if (addr <= 0x1fff)	// rom	
		{
			sprintf(s, "INTERSTELLAR: CPU 1: Attemped write to ROM!-> %x with %x (PC is %x)", addr, value, Z80_GET_PC);
			printline(s);
		}
		else if (addr >= 0x4000 && addr <= 0x47ff ) // ram
		{
			m_cpumem2[addr] = value;
		}
		else
		{
			m_cpumem2[addr] = value;
			sprintf(s, "INTERSTELLAR: CPU 1: Unsupported Memory Write-> %x with %x (PC is %x)", addr, value, Z80_GET_PC);
			printline(s);
		}	
		break;
	case 2:
		if (addr <= 0x17ff)	// rom	
		{
			sprintf(s, "INTERSTELLAR: CPU 2: Attemped write to ROM!-> %x with %x (PC is %x)", addr, value, Z80_GET_PC);
			printline(s);
		}
		else if (addr >= 0x1800 && addr <= 0x1fff ) // ram
		{
			m_cpumem3[addr] = value;
		}
		else
		{
			m_cpumem3[addr] = value;
			sprintf(s, "INTERSTELLAR: CPU 2: Unsupported Memory Write-> %x with %x (PC is %x)", addr, value, Z80_GET_PC);
			printline(s);
		}	
		break;
	default:
		{
			printline("cpu_write_read from invalid CPU!");
		}
		break;
	}
}

// reads a byte from the cpu's port
Uint8 interstellar::port_read(Uint16 port)
{
	char s[81] = { 0 };
	Uint8 result = 0x00;
	static Uint8 old1, old2, oldldp;

	port &= 0xFF;	// strip off high byte

	switch (cpu_getactivecpu())
	{
	case 0:		
	    switch(port)
	    {
		case 0x00:  // inputs
			result = banks[0];
			break;
		case 0x02:  // dipswitch
			result = banks[1];
			break;
		case 0x03:  // dispwitch, coin, start, test
			result = banks[2];
			break;
		case 0x05:  
			result = cpu_latch1;				
			if (result != old1)
			{
#ifdef DEBUG
				sprintf(s, "Main Z80 Read %x from LDP Z80 (PC is %x)", result, Z80_GET_PC);
				printline(s);
#endif
			}
			old1 = result;
			break;
	    default:
			sprintf(s, "INTERSTELLAR: CPU 0: Unsupported Port Input-> %x (PC is %x)", port, Z80_GET_PC);
	        printline(s);
	        break;
	    }
		break;
	case 1:		
	    switch(port)
	    {
		case 0x00:  // sound data from master z80
			result = sound_latch;
			break;
		case 0x01:
			m_cpu1_nmi_enable = true;
			break;
	    default:
			sprintf(s, "INTERSTELLAR: CPU 1: Unsupported Port Input-> %x (PC is %x)", port, Z80_GET_PC);
	        printline(s);
	        break;
	    }
		break;
	case 2:
	    switch(port)
	    {
		case 0x00:  
			result = read_ldv1000();
			if (result != oldldp)
			{
#ifdef DEBUG
				sprintf(s, "LDP Z80 Read %x from LD-V1000 (PC is %x)", result, Z80_GET_PC);
				printline(s);
#endif
			}
			oldldp = result;
			break;
		case 0x01:  
			result = cpu_latch2;				
			if (result != old2)
			{
				sprintf(s, "LDP Z80 Read %x from Main Z80 (PC is %x)", result, Z80_GET_PC);
				printline(s);
			}
			old2 = result;
			break;
		case 0x02: // reset NMI
			m_cpu2_nmi_enable = true;
			break;
	    default:
			sprintf(s, "INTERSTELLAR: CPU 2: Unsupported Port Input-> %x (PC is %x)", port, Z80_GET_PC);
	        printline(s);
	        break;
	    }
		break;
	default:
		printline("port_read on invalid cpu!");
		break;
	}

    return(result);
}

// writes a byte to the cpu's port
void interstellar::port_write(Uint16 port, Uint8 value)
{
	char s[81] = { 0 };

	port &= 0xFF;
	static Uint8 old1, old2, oldldp;
	switch (cpu_getactivecpu())
	{
	case 0:		
	    switch(port)
		{
		case 0x00: // output to sound latch
			sound_latch = value;
			if (m_cpu1_nmi_enable)
			{
            cpu_generate_nmi(1); // cause an NMI on the sound z80
				m_cpu1_nmi_enable = false; // disable NMI
			}
			break;
		case 0x02: // unknown		
			if (m_cpu2_nmi_enable)
			{
            //cpu_generate_nmi(2); // cause an NMI on the sound z80
				//m_cpu2_nmi_enable = false; // disable NMI
			}
			break;
		case 0x03: // reenable NMI after a coin insert
			m_cpu0_nmi_enable = true;
			break;
		case 0x04: 
			//Convert palette rom into a useable palette
			{
				SDL_Color temp_color = { 0 };
				Uint8 bit0,bit1,bit2;	

				/* red component */
				bit0 = static_cast<Uint8>((value >> 0) & 0x01);
				bit1 = static_cast<Uint8>((value >> 1) & 0x01);
				bit2 = static_cast<Uint8>((value >> 2) & 0x01);
				temp_color.r = static_cast<Uint8>((0x21 * bit0) + (0x47 * bit1) + (0x97 * bit2));
			
				/* green component */
				bit0 = static_cast<Uint8>((value >> 3) & 0x01);
				bit1 = static_cast<Uint8>((value >> 4) & 0x01);
				bit2 = static_cast<Uint8>((value >> 5) & 0x01);
				temp_color.g = static_cast<Uint8>((0x21 * bit0) + (0x47 * bit1) + (0x97 * bit2));
				
				/* blue component */
				bit0 = static_cast<Uint8>(0);
				bit1 = static_cast<Uint8>((value >> 6) & 0x01);
				bit2 = static_cast<Uint8>((value >> 7) & 0x01);
				temp_color.b = static_cast<Uint8>((0x21 * bit0) + (0x47 * bit1) + (0x97 * bit2));
			
				//apply gamma correction to make colors brighter overall
				//  Corrected value = 255 * (uncorrected value / 255) ^ (1.0 / gamma)
//				temp_color.r = (Uint8) (255 * pow((static_cast<double>(temp_color.r)) / 255, 1/ESH_GAMMA));
//				temp_color.g = (Uint8) (255 * pow((static_cast<double>(temp_color.g)) / 255, 1/ESH_GAMMA));
//				temp_color.b = (Uint8) (255 * pow((static_cast<double>(temp_color.b)) / 255, 1/ESH_GAMMA));

				palette_set_color(0, temp_color);
				m_background_color = temp_color;
			}
			palette_finalize();
			break;
		case 0x05:
			cpu_latch2 = value;
			if (old2 != value)
			{
				//cpu_generate_nmi(2);
#ifdef DEBUG
				sprintf(s, "Main Z80 Write %x to LDP Z80 (PC is %x)", value, Z80_GET_PC);
				printline(s);
#endif
			}
			if (m_cpu2_nmi_enable)
			{
				cpu_generate_nmi(2);
				m_cpu2_nmi_enable = false;
			}
			old2 = value;
			break;
		default:
			sprintf(s, "INTERSTELLAR: CPU 0: Unsupported Port Output-> %x:%x (PC is %x)", port, value, Z80_GET_PC);
			printline(s);
			break;
		}
		break;
	case 1:
	    switch(port)
		{
		case 0x01: // write data to sn79489 #1 (front?)
         value = ((value & 0x1) << 7) | ((value & 0x2) << 5) | ((value & 0x4) << 3) | ((value & 0x8) << 1) | ((value & 0x10) >> 1) | ((value & 0x20) >> 3) | ((value & 0x40) >> 5) | ((value & 0x80) >> 7);
         audio_writedata(m_soundchip1_id, value);
         break;
		case 0x02: // write data to sn79489 #2 (rear?)
         value = ((value & 0x1) << 7) | ((value & 0x2) << 5) | ((value & 0x4) << 3) | ((value & 0x8) << 1) | ((value & 0x10) >> 1) | ((value & 0x20) >> 3) | ((value & 0x40) >> 5) | ((value & 0x80) >> 7);
         audio_writedata(m_soundchip2_id, value);
			break;
		default:
			sprintf(s, "INTERSTELLAR: CPU 1: Unsupported Port Output-> %x:%x (PC is %x)", port, value, Z80_GET_PC);
			printline(s);
			break;
		}
		break;
	case 2:
	    switch(port)
		{
		case 0x00:
			write_ldv1000(value);
			if (oldldp != value)
			{
#ifdef DEBUG
				sprintf(s, "LDP Z80 Write %x to LD-V1000 (PC is %x)", value, Z80_GET_PC);
				printline(s);
#endif
			}
			oldldp = value;
			break;
		case 0x01:
			cpu_latch1 = value;
			if (old1 != value)
			{
#ifdef DEBUG
				sprintf(s, "LDP Z80 Write %x to Main Z80 (PC is %x)", value, Z80_GET_PC);
				printline(s);
#endif
			}
			old1 = value;
			break;
		// Port 3 - turns on and off the LD background
		case 0x03:
			if (!value) palette_set_transparency(0, true);
			else palette_set_transparency(0, false);
			break;
		default:
			sprintf(s, "INTERSTELLAR: CPU 2: Unsupported Port Output-> %x:%x (PC is %x)", port, value, Z80_GET_PC);
			printline(s);
			break;
		}
		break;
	default:
		printline("port_write on invalid cpu!");
		break;
	}
}

void interstellar::palette_calculate()
{
	SDL_Color temp_color;
	//Convert palette rom into a useable palette
	for (int i = 0; i < INTERSTELLAR_COLOR_COUNT; i++)
	{
		Uint8 bit0,bit1,bit2,bit3;	

		// TODO: get the real interstellar resistor values
		/* red component */
		bit0 = static_cast<Uint8>((color_prom[i+0x000] >> 3) & 0x01);
		bit1 = static_cast<Uint8>((color_prom[i+0x000] >> 2) & 0x01);
		bit2 = static_cast<Uint8>((color_prom[i+0x000] >> 1) & 0x01);
		bit3 = static_cast<Uint8>((color_prom[i+0x000] >> 0) & 0x01);
		temp_color.r = static_cast<Uint8>((0x8f * bit0) + (0x43 * bit1) + (0x1f * bit2) + (0x0e * bit3));
			
		/* green component */
		bit0 = static_cast<Uint8>((color_prom[i+0x100] >> 3) & 0x01);
		bit1 = static_cast<Uint8>((color_prom[i+0x100] >> 2) & 0x01);
		bit2 = static_cast<Uint8>((color_prom[i+0x100] >> 1) & 0x01);
		bit3 = static_cast<Uint8>((color_prom[i+0x100] >> 0) & 0x01);
		temp_color.g = static_cast<Uint8>((0x8f * bit0) + (0x43 * bit1) + (0x1f * bit2) + (0x0e * bit3));
				
		/* blue component */
		bit0 = static_cast<Uint8>((color_prom[i+0x200] >> 3) & 0x01);
		bit1 = static_cast<Uint8>((color_prom[i+0x200] >> 2) & 0x01);
		bit2 = static_cast<Uint8>((color_prom[i+0x200] >> 1) & 0x01);
		bit3 = static_cast<Uint8>((color_prom[i+0x200] >> 0) & 0x01);
		temp_color.b = static_cast<Uint8>((0x8f * bit0) + (0x43 * bit1) + (0x1f * bit2) + (0x0e * bit3));

		palette_set_color(i, temp_color);
	}
}

// updates interstellar's video
void interstellar::video_repaint()
{
	// clear the video before drawing
	SDL_FillRect(m_video_overlay[m_active_video_overlay], NULL, 0);

	// draw the sprites
	for (int sprite = 0x200 - 4; sprite >= 0x000; sprite-=4)
	{
		// Sprites (256 16x16 sprites, uses the same data as the 8x8 characters)
		// each 4 bytes from 0xb000 - 0xb0c0(?)
		//
		// byte 0 - 240 - y coordinate 
		// byte 1 - sprite number in table
		// byte 2 - yx??pppp where x = xflip, y = yflip, p = palette
		// byte 3 - x coordinate
			
		int sprite_data = sprite + 0xb000;
						
		if ((m_cpumem[sprite_data + 1] != 0xff) && (m_cpumem[sprite_data + 3] != 0xff) && (((~m_cpumem[sprite_data + 0]) & 0xff) != 0xff))
		{				
			draw_16x16(m_cpumem[sprite_data + 1], m_cpumem[sprite_data + 3], 240 - m_cpumem[sprite_data + 0], m_cpumem[sprite_data + 2] & 0x40, m_cpumem[sprite_data + 2] & 0x80, m_cpumem[sprite_data + 2] & 0x0f);
		}
	}

	// draw the characters
	for (int charx = 0; charx < 32; charx++)
	{
		for (int chary = 0; chary < 32; chary++)
		{
			// Memory region AC00-AFFF controls the video
			//
			//	????pppp where p = palette
			
			int palette = (m_cpumem[(chary << 5) + charx + 0xac00] & 0x0f);
			int current_char = (m_cpumem[(chary << 5) + charx + 0xa800]);
				
			draw_8x8(current_char, charx*8, chary*8, 0, 0, palette);
		}
	}
}

// this gets called when the user presses a key or moves the joystick
void interstellar::input_enable(Uint8 move)
{
	switch (move)
	{
	case SWITCH_RIGHT: // fr left
		banks[0] |= 0x01;
		break;
	case SWITCH_LEFT: // fr left
		banks[0] |= 0x02;
		break;
	case SWITCH_UP: // flap right
		banks[0] |= 0x04;
		break;
	case SWITCH_DOWN: // flap left
		banks[0] |= 0x08;
		break;
	case SWITCH_BUTTON1: // fazer
		banks[0] |= 0x10;
		break;
	case SWITCH_BUTTON2: // burn
		banks[0] |= 0x20;
		break;
	case SWITCH_START1:
		banks[2] |= 0x80;
		break;
	case SWITCH_START2:
		banks[2] |= 0x40;
		break;
	case SWITCH_COIN1:
		banks[2] &= ~0x20;
		if (m_cpu0_nmi_enable)
		{
			cpu_generate_nmi(0); // cause an NMI on the master z80 when coin is inserted
			m_cpu0_nmi_enable = false;
		}			
		break;
	case SWITCH_COIN2:
		banks[2] &= ~0x10;
		if (m_cpu0_nmi_enable)
		{
			cpu_generate_nmi(0); // cause an NMI on the master z80 when coin is inserted
			m_cpu0_nmi_enable = false;
		}			
		break;
	case SWITCH_TEST: 
		banks[2] |= 0x08;
		break;
	}
}  

// this gets called when the user releases a key or moves the joystick back to center position
void interstellar::input_disable(Uint8 move)
{
	switch (move)
	{
	case SWITCH_RIGHT: 
		banks[0] &= ~0x01;
		break;
	case SWITCH_LEFT: 
		banks[0] &= ~0x02;
		break;
	case SWITCH_UP: 
		banks[0] &= ~0x04;
		break;
	case SWITCH_DOWN: 
		banks[0] &= ~0x08;
		break;
	case SWITCH_BUTTON1: 
		banks[0] &= ~0x10;
		break;
	case SWITCH_BUTTON2: 
		banks[0] &= ~0x20;
		break;
	case SWITCH_START1: 
		banks[2] &= ~0x80;
		break;
	case SWITCH_START2: 
		banks[2] &= ~0x40;
		break;
	case SWITCH_COIN1: 
		banks[2] |= 0x20;
		break;
	case SWITCH_COIN2: 
		banks[2] |= 0x10;
		break;
	case SWITCH_TEST: 
		banks[2] &= ~0x08;
		break;
	}
}

// used to set dip switch values
bool interstellar::set_bank(Uint8 which_bank, Uint8 value)
{
	bool result = true;
	
	switch (which_bank)
	{
	case 0:	// bank A
		banks[1] |= (Uint8) (value & 0x3f);	
		break;
	case 1:	// bank B
		banks[1] |= (Uint8) ((value << 6) & 0xc0);
		banks[2] |= (Uint8) ((value >> 2) & 0x07);
		break;
	default:
		printline("ERROR: Bank specified is out of range!");
		result = false;
		break;
	}
	
	return result;
}

void interstellar::draw_8x8(int character_number, int xcoord, int ycoord, int xflip, int yflip, int palette)
{
	Uint8 pixel[8] = {0};
	
	for (int y = 0; y < 8; y++)
	{
		if ((y + ycoord) < INTERSTELLAR_OVERLAY_H)
		{
			Uint8 byte3 = character[character_number*8+y];
			Uint8 byte2 = character[character_number*8+y+0x2000];
			Uint8 byte1 = character[character_number*8+y+0x4000];

			pixel[0] = static_cast<Uint8>(((byte1 & 0x80) >> 5) | ((byte2 & 0x80) >> 6) | ((byte3 & 0x80) >> 7));
			pixel[1] = static_cast<Uint8>(((byte1 & 0x40) >> 4) | ((byte2 & 0x40) >> 5) | ((byte3 & 0x40) >> 6));
			pixel[2] = static_cast<Uint8>(((byte1 & 0x20) >> 3) | ((byte2 & 0x20) >> 4) | ((byte3 & 0x20) >> 5));
			pixel[3] = static_cast<Uint8>(((byte1 & 0x10) >> 2) | ((byte2 & 0x10) >> 3) | ((byte3 & 0x10) >> 4));
			pixel[4] = static_cast<Uint8>(((byte1 & 0x08) >> 1) | ((byte2 & 0x08) >> 2) | ((byte3 & 0x08) >> 3));
			pixel[5] = static_cast<Uint8>(((byte1 & 0x04) << 0) | ((byte2 & 0x04) >> 1) | ((byte3 & 0x04) >> 2));
			pixel[6] = static_cast<Uint8>(((byte1 & 0x02) << 1) | ((byte2 & 0x02) << 0) | ((byte3 & 0x02) >> 1));
			pixel[7] = static_cast<Uint8>(((byte1 & 0x01) << 2) | ((byte2 & 0x01) << 1) | ((byte3 & 0x01) << 0));

			for (int x = 0; x < 8; x++)
			{
				if ((pixel[x]) && ((x + xcoord) < INTERSTELLAR_OVERLAY_W))
				{
					*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + ((ycoord + (yflip ? (7-y) : y)) * INTERSTELLAR_OVERLAY_W) + (xcoord + (xflip ? (7-x) : x))) = (Uint8) (pixel[x] | (palette << 3));
				}
			}
		}
	}
}

void interstellar::draw_16x16(int character_number, int xcoord, int ycoord, int xflip, int yflip, int palette)
{
	draw_8x8((character_number * 4) + 0, xcoord + (xflip?8:0), ycoord + (yflip?8:0), xflip, yflip, palette);
	draw_8x8((character_number * 4) + 1, xcoord + (xflip?0:8), ycoord + (yflip?8:0), xflip, yflip, palette);
	draw_8x8((character_number * 4) + 2, xcoord + (xflip?8:0), ycoord + (yflip?0:8), xflip, yflip, palette);
	draw_8x8((character_number * 4) + 3, xcoord + (xflip?0:8), ycoord + (yflip?0:8), xflip, yflip, palette);
}
