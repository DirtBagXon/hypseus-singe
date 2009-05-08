/*
 * esh.cpp
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

// esh.cpp
// mostly by Mark Broadhead with some ROM disassembly work by Matt Ownby
// (plus a few tweaks by Warren Ondras)
//
// Memory map
// 0000 - 3fff Program roms
// e000 - e7ff Work ram - all of this appears to be backed up with a nicad battery
// f000 - f3ff video ram
// f400 - f7ff video control ram

#ifdef WIN32
// disable "unreferenced inline function has been removed" warning
#pragma warning( disable: 4514 )
#endif

#include <string.h>
#include <math.h>	// for pow
#include "esh.h"
#include "../cpu/cpu.h"
#include "../cpu/generic_z80.h"
#include "../io/conout.h"
#include "../ldp-in/ldv1000.h"
#include "../ldp-out/ldp.h"
#include "../video/palette.h"
#include "../video/video.h"

enum
{
	S_ESH_BEEP
};

////////////////

esh::esh() :
	m_needlineblink(false),
	m_needcharblink(false)
{
	struct cpudef cpu;

	m_shortgamename = "esh";	
	memset(&cpu, 0, sizeof(struct cpudef));
	memset(banks, 0xFF, 4);	// fill banks with 0xFF's

	// assuming we won't have any nvram saved, set these values.
	// If we do have nvram, these will be overwritten.
	m_cpumem[0xE463] = 5;	// default to 5 lives instead of 1
	m_cpumem[0xE465] = 1;	// default to audio on instead of off
	
	m_game_type = GAME_ESH;
	m_disc_fps = 29.97;

	m_video_overlay_width = ESH_OVERLAY_W;
	m_video_overlay_height = ESH_OVERLAY_H;
	m_palette_color_count = ESH_COLOR_COUNT;

	cpu.type = CPU_Z80;
	cpu.hz = 3072000;	// PCB has 18.432 MHz crystal, 
						// Pac-Man uses same xtal and divides by 6 for CPU clock,
						// so we'll use that here too
	cpu.initial_pc = 0;
	cpu.must_copy_context = false;
	cpu.nmi_period = (1000.0 / 60.0); // nmi from LD-V1000 command strobe (likely guess)
	cpu.irq_period[0] = (1000.0 / 60.0); // irq from vblank (guess)
	cpu.mem = m_cpumem;
	add_cpu(&cpu);	// add z80 cpu

	blank_count = 0;
	palette_high_bit = 0;

	m_num_sounds = 1;
	m_sound_name[S_ESH_BEEP] = "esh_beep.wav";
	
	m_game_issues = "Game can be completed, but driver is very immature.  Various video/sound problems.";

	m_nvram_begin = &m_cpumem[0xE000];
	m_nvram_size = 0x800;
	
	// NOTE : this must be static
	const static struct rom_def roms[] =
	{
		// Z80 program
		{ "h8_is1.bin", NULL, &m_cpumem[0x0000], 0x2000, 0x114C912B },
		{ "f8_is2.bin", NULL, &m_cpumem[0x2000], 0x2000, 0x0E3B6E62 },
		
		// character (tile) graphics
		{ "m3_a.bin", NULL, &character[0x0000], 0x1000, 0xA04736D8 },
		{ "l3_b.bin", NULL, &character[0x1000], 0x1000, 0x9366DDE7 },
		{ "k3_c.bin", NULL, &character[0x2000], 0x1000, 0xA936EF01 },
		
		// color lookup prom, video timing proms
		{ "j1_rgb.bin", NULL, &color_prom[0x0000], 0x0200, 0x1E9F795F },
		{ "c5_h.bin", NULL, &miscprom[0x0000], 0x0100, 0xABDE5E4B },
		{ "c6_v.bin", NULL, &miscprom[0x0100], 0x0100, 0x7157BA22 },
		{ NULL }	
	};

	m_rom_list = roms;	
}

void esh::set_version(int version)
{
	if (version == 1) 
	{
		// default version, do nothing
	}
	else if (version == 2)  // alternate set 1
	{
		m_shortgamename = "eshalt";
		
		// NOTE : this must be static
		static struct rom_def roms[] =
		{
			// Z80 program
			{ "h8_is1.bin", "esh", &m_cpumem[0x0000], 0x2000, 0x114C912B },
			{ "f8_alt1.bin", NULL, &m_cpumem[0x2000], 0x2000, 0x7A562F49 },
			// NOTE : f8 is the only rom that has changed from the default set
			
			// character (tile) graphics
			{ "m3_a.bin", "esh", &character[0x0000], 0x1000, 0xA04736D8 },
			{ "l3_b.bin", "esh", &character[0x1000], 0x1000, 0x9366DDE7 },
			{ "k3_c.bin", "esh", &character[0x2000], 0x1000, 0xA936EF01 },
			
			// color lookup prom, video timing proms
			{ "j1_rgb.bin", "esh", &color_prom[0x0000], 0x0200, 0x1E9F795F },
			{ "c5_h.bin", "esh", &miscprom[0x0000], 0x0100, 0xABDE5E4B },
			{ "c6_v.bin", "esh", &miscprom[0x0100], 0x0100, 0x7157BA22 },
			{ NULL }
		};
		m_rom_list = roms;
	}
	else if (version == 3)  //alternate set 2
	{
		m_shortgamename = "eshalt2";
		
		// NOTE : this must be static
		static struct rom_def roms[] =
		{
			// Z80 program
			{ "h8_alt2.bin", NULL, &m_cpumem[0x0000], 0x2000, 0x8D27D363 },
			// NOTE : h8 is the only rom that has changed from the default set
			{ "f8_is2.bin", "esh", &m_cpumem[0x2000], 0x2000, 0x0E3B6E62 },
			
			// character (tile) graphics
			{ "m3_a.bin", "esh", &character[0x0000], 0x1000, 0xA04736D8 },
			{ "l3_b.bin", "esh", &character[0x1000], 0x1000, 0x9366DDE7 },
			{ "k3_c.bin", "esh", &character[0x2000], 0x1000, 0xA936EF01 },
			
			// color lookup prom, video timing proms
			{ "j1_rgb.bin", "esh", &color_prom[0x0000], 0x0200, 0x1E9F795F },
			{ "c5_h.bin", "esh", &miscprom[0x0000], 0x0100, 0xABDE5E4B },
			{ "c6_v.bin", "esh", &miscprom[0x0100], 0x0100, 0x7157BA22 },
			{ NULL }
		};
		m_rom_list = roms;
	}
	else
	{
		printline("ESH:  Unsupported -version paramter, ignoring...");
	}
}

/*
// eshalt constructor
eshalt::eshalt()
{
	m_shortgamename = "eshalt";
	
	// NOTE : this must be static
	static struct rom_def roms[] =
	{
		// Z80 program
		{ "h8_is1.bin", "esh", &m_cpumem[0x0000], 0x2000, 0x114C912B },
		{ "f8_is2.bin", NULL, &m_cpumem[0x2000], 0x2000, 0x7A562F49 },
		// NOTE : f8 is the only rom that has changed, the rest of these load from the
		// esh directory
		
		// character (tile) graphics
		{ "m3_a.bin", "esh", &character[0x0000], 0x1000, 0xA04736D8 },
		{ "l3_b.bin", "esh", &character[0x1000], 0x1000, 0x9366DDE7 },
		{ "k3_c.bin", "esh", &character[0x2000], 0x1000, 0xA936EF01 },
		
		// color lookup prom, video timing proms
		{ "j1_rgb.bin", "esh", &color_prom[0x0000], 0x0200, 0x1E9F795F },
		{ "c5_h.bin", "esh", &miscprom[0x0000], 0x0100, 0xABDE5E4B },
		{ "c6_v.bin", "esh", &miscprom[0x0100], 0x0100, 0x7157BA22 },
		{ NULL }
	};

	m_rom_list = roms;

}

// eshalt2 constructor
eshalt2::eshalt2()
{
	m_shortgamename = "eshalt2";
	
	// NOTE : this must be static
	static struct rom_def roms[] =
	{
		// Z80 program
		{ "h8_1.bin", NULL, &m_cpumem[0x0000], 0x2000, 0x8D27D363 },
		{ "f8_is2.bin", NULL, &m_cpumem[0x2000], 0x2000, 0x0E3B6E62 },
		
		// character (tile) graphics
		{ "m3_a.bin", "esh", &character[0x0000], 0x1000, 0xA04736D8 },
		{ "l3_b.bin", "esh", &character[0x1000], 0x1000, 0x9366DDE7 },
		{ "k3_c.bin", "esh", &character[0x2000], 0x1000, 0xA936EF01 },
		
		// color lookup prom, video timing proms
		{ "j1_rgb.bin", "esh", &color_prom[0x0000], 0x0200, 0x1E9F795F },
		{ "c5_h.bin", "esh", &miscprom[0x0000], 0x0100, 0xABDE5E4B },
		{ "c6_v.bin", "esh", &miscprom[0x0100], 0x0100, 0x7157BA22 },
		{ NULL }
	};

	m_rom_list = roms;
}
*/

// does anything special needed to send an IRQ
void esh::do_irq(unsigned int which_irq)
{
	if (which_irq == 0)
	{
		// Redraws the screen (if needed) on interrupt

		// do we need to blink?
		if (((m_needlineblink || m_needcharblink) && blank_count==0) || 
		 (m_needlineblink && blank_count == 3) || 
		 (m_needcharblink && blank_count==7))
		{
			m_video_overlay_needs_update = true;
		}

		video_blit();
		blank_count++;
		if (blank_count >= 10)
		{
			blank_count = 0;
		}
	
		Z80_ASSERT_IRQ;
	}	
}

// does anything special needed to send an NMI
void esh::do_nmi()
{
	Z80_ASSERT_NMI;
}

void esh::cpu_mem_write(Uint16 addr, Uint8 value)
{
	// if program writes to video memory
//	if (addr > 0xf000 && addr <= 0xf7ff && value!=m_cpumem[addr])  //doesn't seem to be necessary 
	if (addr > 0xf000 && addr <= 0xf7ff)
	{
		m_video_overlay_needs_update = true;
	}
	
	m_cpumem[addr] = value;	
}

Uint8 esh::port_read(Uint16 port)
{
	char s[81];
	Uint8 result = 0;

	switch (port & 0xFF)
	{
	case 0xF0:	// switch 0?
		result = banks[0];
		break;
	case 0xF1:	// switch 1?
		result = banks[1];
		break;
	case 0xF2:	// switch 2?
		result = 0xff;
		break;
	case 0xF3:	// switch 3?
		result = 0xff;
		break;
	case 0xF4:
		result = read_ldv1000();
//		sprintf(s, "%x read from LDV1000", result);
//		printline(s);
		break;
	default:
		sprintf(s, "Port %x being read at PC %x\n", port & 0xFF, Z80_GET_PC);
		printline(s);
		break;
	}

	return result;
}

void esh::port_write(Uint16 port, Uint8 value)
{
	char s[81];
	static unsigned int lastbeep = 0;

//	static Uint8 lastmode=1;  //invalid default to guarantee screen updates start right away
		
	switch (port & 0xFF)
	{
	case 0xF4:
//		sprintf(s, "%x written to LDV1000", value);
//		printline(s);
		write_ldv1000(value);
		break;
	case 0xF5:	
		// bit 0 - Unknown
		// bit 1 - Coin/Extra life/high score beep
		// bit 2 - Unknown
		// bit 3 - turn off/on laserdisc video
		// bit 4 - Unknown
		// bit 5 - Unknown
		// bit 6 - Unknown
		// bit 7 - Unknown

		if (value & 0x02)
		{
			// one second beeps get 61 writes, two second beeps get more (121)
			// so if there have been more than 61 continuous writes, play it again
			if (++lastbeep > 61)
			{
				lastbeep = 0;
				sound_play(S_ESH_BEEP);  // 1 sec. simulated beep, until the real beep is sampled
			}
		}
		if (value & 0x08)
		{
			// this is a temp value until the correct colors are in 
			// and something better is found
			palette_high_bit = 1;
		}
		else 
		{
			palette_high_bit = 0;
		}
		break;

	case 0xF8:
	case 0xFE:
	case 0xFF:
		// 0 and 1 are rapidly written here, not sure why
		break;
	
	// these get written before laserdisc activity, not sure what they do
	case 0xFA:
		break;		
	case 0xFB:
		// turns on action button lights?
		break;
	default:
		sprintf(s, "Port %x being written at PC %x with a value of %x", port & 0xFF, Z80_GET_PC, value);
		printline(s);
		break;
	}
}

void esh::palette_calculate()
{
	SDL_Color temp_color = { 0 };
	//Convert palette rom into a useable palette
	for (int i = 0; i < ESH_COLOR_COUNT; i++)
	{
		Uint8 bit0,bit1,bit2;	

		/* red component */
		bit0 = static_cast<Uint8>((color_prom[i+256] >> 0) & 0x01);
		bit1 = static_cast<Uint8>((color_prom[i+256] >> 1) & 0x01);
		bit2 = static_cast<Uint8>((color_prom[i+256] >> 2) & 0x01);
		temp_color.r = static_cast<Uint8>((0x21 * bit0) + (0x47 * bit1) + (0x97 * bit2));
			
		/* green component */
		bit0 = static_cast<Uint8>(0);
		bit1 = static_cast<Uint8>((color_prom[i+256] >> 3) & 0x01);
		bit2 = static_cast<Uint8>((color_prom[i+256] >> 4) & 0x01);
		temp_color.g = static_cast<Uint8>((0x21 * bit0) + (0x47 * bit1) + (0x97 * bit2));
				
		/* blue component */
		bit0 = static_cast<Uint8>(0);
		bit1 = static_cast<Uint8>((color_prom[i+256] >> 5) & 0x01);
		bit2 = static_cast<Uint8>((color_prom[i+256] >> 6) & 0x01);
		temp_color.b = static_cast<Uint8>((0x21 * bit0) + (0x47 * bit1) + (0x97 * bit2));
			
		//apply gamma correction to make colors brighter overall
		//  Corrected value = 255 * (uncorrected value / 255) ^ (1.0 / gamma)
		temp_color.r = (Uint8) (255 * pow((static_cast<double>(temp_color.r)) / 255, 1/ESH_GAMMA));
		temp_color.g = (Uint8) (255 * pow((static_cast<double>(temp_color.g)) / 255, 1/ESH_GAMMA));
		temp_color.b = (Uint8) (255 * pow((static_cast<double>(temp_color.b)) / 255, 1/ESH_GAMMA));

		palette_set_color(i, temp_color);
		
		// more than color 0 should be transparent, and until someone figures out exactly what the other
		//  color is (surrounding the overlay), then all black will be transparent, which doesn't hurt anything
		if ((temp_color.r == 0) && (temp_color.g == 0) && (temp_color.b == 0))
		{
			palette_set_transparency(i, true);
		}
	}
}

// updates esh's video
void esh::video_repaint()
{
	m_needcharblink = false;
	m_needlineblink = false;
			
	for (int charx = 0; charx < 32; charx++)
	{
		for (int chary = 0; chary < 32; chary++)
		{
			for (int y = 0; y < 8; y++)
			{
				// Memory region F400-F7FF controls the video
				//
				// Low nibble:
				// palette
				//
				// High nibble:
				// bit 4	use tile set 2 (tiles 256-511)
				// bit 5	unknown
				// bit 6	blink every other line
				// bit 7	blink the whole character
					
				Uint8 pixel[8]; 
					
				int palette = (m_cpumem[(chary << 5) + charx + 0xf400] & 0x0f);
				int tile_set = ((m_cpumem[(chary << 5) + charx + 0xf400] >> 0x04) & 0x01);
				int blink_line = ((m_cpumem[(chary << 5) + charx + 0xf400] >> 0x06) & 0x01);
				int blink_char = ((m_cpumem[(chary << 5) + charx + 0xf400] >> 0x07) & 0x01);

				if (blink_char) 
				{
					m_needcharblink = true;
				}
					
				if (blink_line) 
				{
					m_needlineblink = true;
				}

				// this is just an approximation of the character blinking
				if ( (!(blink_char) || blank_count < 7) && ((!(blink_line && y%2 == 0) || blank_count < 3)) && palette)
				{
					int current_char = (m_cpumem[(chary << 5) + charx + 0xf000] | (0x100 * tile_set));
						
					pixel[0] = static_cast<Uint8>(((character[(current_char)*8+y] & 0x80) >> 7) | ((character[(current_char)*8+y+0x1000] & 0x80) >> 6) | ((character[(current_char)*8+y+0x2000] & 0x80) >> 5));
					pixel[1] = static_cast<Uint8>(((character[(current_char)*8+y] & 0x40) >> 6) | ((character[(current_char)*8+y+0x1000] & 0x40) >> 5) | ((character[(current_char)*8+y+0x2000] & 0x40) >> 4));
					pixel[2] = static_cast<Uint8>(((character[(current_char)*8+y] & 0x20) >> 5) | ((character[(current_char)*8+y+0x1000] & 0x20) >> 4) | ((character[(current_char)*8+y+0x2000] & 0x20) >> 3));
					pixel[3] = static_cast<Uint8>(((character[(current_char)*8+y] & 0x10) >> 4) | ((character[(current_char)*8+y+0x1000] & 0x10) >> 3) | ((character[(current_char)*8+y+0x2000] & 0x10) >> 2));
					pixel[4] = static_cast<Uint8>(((character[(current_char)*8+y] & 0x08) >> 3) | ((character[(current_char)*8+y+0x1000] & 0x08) >> 2) | ((character[(current_char)*8+y+0x2000] & 0x08) >> 1));
					pixel[5] = static_cast<Uint8>(((character[(current_char)*8+y] & 0x04) >> 2) | ((character[(current_char)*8+y+0x1000] & 0x04) >> 1) | ((character[(current_char)*8+y+0x2000] & 0x04) >> 0));
					pixel[6] = static_cast<Uint8>(((character[(current_char)*8+y] & 0x02) >> 1) | ((character[(current_char)*8+y+0x1000] & 0x02) >> 0) | ((character[(current_char)*8+y+0x2000] & 0x02) << 1));
					pixel[7] = static_cast<Uint8>(((character[(current_char)*8+y] & 0x01) >> 0) | ((character[(current_char)*8+y+0x1000] & 0x01) << 1) | ((character[(current_char)*8+y+0x2000] & 0x01) << 2));
				}
				else
				{
					memset(pixel, 0x00, 8);
				}
					
				for (int x = 0; x < 8; x++)
				{
					*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + (((chary << 3) + y) * ESH_OVERLAY_W) + ((charx << 3) + x)) = static_cast<Uint8>(pixel[x] | (palette << 3) | (palette_high_bit << 7));
				}
			}
		}
	}

// test - make an 8x8 block of every color
//		for (int x = 0; x < 128; x++)
//		{
//			for (int y = 0; y < 512; y++)
//			{
//				*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + y * ESH_OVERLAY_W + x) = (x>>4) + ((y & 0xfff0)>>1);
//			}
//		}

}

// apply cheat
void esh::patch_roms()
{
	// if a cheat has been requested, modify the ROM.  Note, this must be done after roms are loaded
	if (m_cheat_requested)
	{
		// infinite lives cheat
		m_cpumem[0xCBC] = 0;	// NOP out code that decrements # of remaning lives
		m_cpumem[0xCBD] = 0x18;	// change branch if we're not out of lives to unconditional branch
		printline("Esh infinite lives cheat enabled!");
	}
}

// this gets called when the user presses a key or moves the joystick
void esh::input_enable(Uint8 move)
{
	switch (move)
	{
	case SWITCH_UP:
		banks[1] &= ~0x01;
		break;
	case SWITCH_LEFT:
		banks[1] &= ~0x04;
		break;
	case SWITCH_RIGHT:
		banks[1] &= ~0x08;
		break;
	case SWITCH_DOWN:
		banks[1] &= ~0x02;
		break;
	case SWITCH_START1:
		banks[0] &= ~0x04;
		break;
	case SWITCH_START2:
		break;
	case SWITCH_BUTTON1:
		banks[1] &= ~0x10;
		break;
	case SWITCH_COIN1: 
		banks[0] &= ~0x01;
		break;
	case SWITCH_COIN2: 
		banks[0] &= ~0x02;
		break;
	case SWITCH_SERVICE: 
		break;
	case SWITCH_TEST: 
		banks[0] &= ~0x10;
		break;
	default:
		printline("Error, bug in move enable");
		break;
	}
}  

// this gets called when the user releases a key or moves the joystick back to center position
void esh::input_disable(Uint8 move)
{
	switch (move)
	{
	case SWITCH_UP:
		banks[1] |= 0x01;
		break;
	case SWITCH_LEFT:
		banks[1] |= 0x04;
		break;
	case SWITCH_RIGHT:
		banks[1] |= 0x08;
		break;
	case SWITCH_DOWN:
		banks[1] |= 0x02;
		break;
	case SWITCH_START1: // '1' on keyboard
		banks[0] |= 0x04;
		break;
	case SWITCH_START2: // '2' on keyboard
		break;
	case SWITCH_BUTTON1: // space on keyboard
		banks[1] |= 0x10;
		break;
	case SWITCH_COIN1: 
		banks[0] |= 0x01;
		break;
	case SWITCH_COIN2: 
		banks[0] |= 0x02;
		break;
	case SWITCH_SERVICE: 
		break;
	case SWITCH_TEST: 
		banks[0] |= 0x10;
		break;
	default:
		printline("Error, bug in move disable");
		break;
	}
}
