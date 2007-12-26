/*
* cobraconv.cpp
*
* Copyright (C) 2003 Warren Ondras
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

// cobraconv.cpp
// by Warren Ondras, based on bega.cpp by Mark Broadhead
//
// Cobra Command - Conversion hardware
// uses two 6502's (the second is for sound) that communicate through an am2950
// one ay-3-8910's for sound
//
// The conversion runs either the Pioneer LD-V1000 or Pioneer PR8210 (the 8210
// version has an extra pcb on top which includes another 6502 and rom). 
// It seems that the conversion board doesn't have any color ram, but this is 
// unverified

#include <string.h>
#include "cobraconv.h"
#include "../cpu/cpu.h"
#include "../cpu/nes6502.h"
#include "../io/conout.h"
#include "../ldp-in/ldv1000.h"
#include "../ldp-out/ldp.h"
#include "../video/palette.h"
#include "../video/video.h"

////////////////

cobraconv::cobraconv()
{
	struct cpudef cpu;

	m_shortgamename = "cobraconv";
	memset(banks, 0xFF, 4);	// fill banks with 0xFF's

	//	m_game_type = GAME_BEGA;
	m_disc_fps = 29.97;

	m_video_overlay_width = COBRACONV_OVERLAY_W;
	m_video_overlay_height = COBRACONV_OVERLAY_H;
	m_palette_color_count = COBRACONV_COLOR_COUNT;
	m_video_row_offset = -8;	// 16 pixels, 8 rows

	memset(&cpu, 0, sizeof(struct cpudef));
	cpu.type = CPU_M6502;
	cpu.hz = 2500000; // unverified	
	cpu.irq_period[0] = 0; // we'll generate IRQ's manually in the game driver
	cpu.nmi_period = 0; // no periodic nmi
	cpu.initial_pc = 0;
	cpu.must_copy_context = true;	// set this to true when you add multiple 6502's
	cpu.mem = m_cpumem;
	add_cpu(&cpu);	// add 6502 cpu

	memset(&cpu, 0, sizeof(struct cpudef));
	cpu.type = CPU_M6502;
	cpu.hz = 2500000; // unverified	
	cpu.irq_period[0] = 0;
	cpu.irq_period[1] = 0;
	cpu.nmi_period = 2.0; 
	cpu.initial_pc = 0;
	cpu.must_copy_context = true;	// set this to true when you add multiple 6502's
	cpu.mem = m_cpumem2;
	add_cpu(&cpu);	// add 6502 cpu

	struct sounddef soundchip;
	soundchip.type = SOUNDCHIP_AY_3_8910;
	soundchip.hz = 1500000; // Bega runs the sound chips at 1.5 MHz
	m_soundchip_id = add_soundchip(&soundchip);

	// the ROM expects searching to be extra slow or it will lock up after death scenes
	ldv1000_set_seconds_per_search(1.0);

	//ldp_status = 0x00;

	//just to test some video stuff (remove me)
	m_nvram_begin = &m_cpumem[0x0000];
	m_nvram_size = 0xFFFF;

	// the catch-all!
	m_game_issues = "This game doesn't work.";

	// NOTE : this must be static
	const static struct rom_def roms[] =
	{
		// main cpu roms
		{ "bd00", NULL, &m_cpumem[0xe000], 0x2000, 0x8d9ad777 },
		{ "bd01", NULL, &m_cpumem[0xc000], 0x2000, 0x1b4db507 },
		{ "bd02", NULL, &m_cpumem[0xa000], 0x2000, 0x3d802707 },
		{ "bd03", NULL, &m_cpumem[0x8000], 0x2000, 0xf1b9df77 },

		// sound cpu roms
		{ "bd07", NULL, &m_cpumem2[0xe000], 0x2000, 0x584d714a },

		// roms for the character/sprite generator (conv board only has one)
		{ "bd06", NULL, &character2[0x0000], 0x2000, 0xb1340125 },
		{ "bd05", NULL, &character2[0x2000], 0x2000, 0x98412178 },
		{ "bd04", NULL, &character2[0x4000], 0x2000, 0x33013cc2 },

		// color lookup prom
		{ "vd0-c.bpr", NULL, &color_prom[0x0000], 0x0020, 0x2c27aa0 },

		// unused video timing prom(?), misc. PLD
		{ "vd0-t.bpr", NULL, &miscprom[0x0000], 0x0020, 0x78449942 },
		{ "lp4-2.pld", NULL, &miscprom[0x0100], 0x022A, 0x4bc65ab0 },

		{ NULL }
	};

	m_rom_list = roms;

}


// clocks screen updates and vblank timing
void cobraconv::do_irq(unsigned int which_irq)
{
	// IRQ moved to OnVblank function
	switch (cpu_getactivecpu())
	{
	default:
		// this should never happen
		printline("cobraconv ERROR: unhandled IRQ received");
		break;
	case 0:
		nes6502_irq();
		break;
	case 1:
		nes6502_irq();
		break;
	}
}

void cobraconv::do_nmi()
{
	// used by both cpu's
	nes6502_nmi();
}

Uint8 cobraconv::cpu_mem_read(Uint16 addr)
{
//	static unsigned int loopcount = 0;
	char s[81] = {0};

	Uint8 result = 0;

	switch (cpu_getactivecpu())
	{
	case 0:
		result = m_cpumem[addr];

		// main ram
		if (addr <= 0x0fff)
		{
		}

		// main rom
		else if (addr >= 0x4000)
		{
		}

		// video ram
		else if (addr >= 0x2000 && addr <= 0x3fff)
		{
		}

		// bit 7 - vblank
		// bit 6 = ldv1000 status strobe
		// bit 5 = ldv1000 command strobe
		// bits 1 and 2 - coin inputs (see a976)
		// bit 0 - tilt input
		else if (addr == 0x1001) 
		{
			// vblank is bit 7
			if (ldv1000_is_vsync_active())
			{
				banks[3] |= 0x80;  // set bit 7
			}
			else
			{
				banks[3] &= 0x7F;	// clear bit 7
			}

			// NOTE: status strobe is active when bit 6 is set (see E500 in ROM code for example)
			//		    if (ldv1000_is_status_strobe_active())
			// UPDATE:
			// We have a problem where The IRQ (e53a) and the function that sends commands to the LD-V1000 (E4C1) are
			//  both waiting for the status strobe to begin, and the result is that the function at E4C1 will never
			//  see the status strobe begin (and thus lockup) unless we force the status strobe to be 'always on'.
			// Hopefully one day we can find some schematics for this boardset so we can see what's really supposed
			//  to be going on.
			if (1)
			{
				banks[3] |= 0x40;
			}
			else
			{
				banks[3] &= ~0x40;
			}

			// Command strobe is active when bit 5 is set (see E50E in ROM code for example)
			if (ldv1000_is_command_strobe_active())
			{
				banks[3] |= 0x20;
			}
			else
			{
				banks[3] &= ~0x20;
			}

			result = banks[3];
		}  

		// control panel
		else if (addr == 0x1000)
		{
			result = banks[0];
		}

		// dipswitch 1
		else if (addr == 0x1002)
		{
			result = banks[1];
		}

		// dipswitch 2
		else if (addr == 0x1003)
		{
			result = banks[2];
		}

		// laserdisc player interface
		else if (addr == 0x1004)
		{
			result = read_ldv1000();
		}

		else
		{
			sprintf(s, "CPU 0: Unmapped read from %x", addr);
			printline(s);
		}
		break;
	case 1:
		result = m_cpumem2[addr];

		if (addr == 0xa000)
		{
			result = m_sounddata_latch;
		}
		// Rom
		else if (addr >= 0xe000)
		{
		}

		else
		{
			sprintf(s, "CPU 1: Unmapped read from %x", addr);
			printline(s);
		}
		break;
	}
	return result;
}

void cobraconv::cpu_mem_write(Uint16 addr, Uint8 value)
{
	char s[81] = {0};

	switch (cpu_getactivecpu())
	{
	case 0:
		// main ram
		if (addr <= 0x0fff)
		{
		}

		// video ram
		else if (addr >= 0x2000 && addr <= 0x3fff)
			//  how much video ram does cobraconv have?
			//  and does it have palette ram?  from 0x3000-3fff?
			//	else if (addr >= 0x2000 && addr <= 0x2fff)
		{
			if (value != m_cpumem[addr]) 
			{
				//				sprintf(s, "Video write to %x with value %x", addr, value);
				//				printline(s);
				m_video_overlay_needs_update = true;
			}	
		}

		// 0x1001 bit 1 is LD command strobe? - it occurs before ld commands are sent
		//        other bits for int/ext sync? other?
		else if (addr == 0x1001)  
		{
			// When a coin is inserted, the high nibble of this byte cycles between 3, 2, 1, and 0 as if
			//  it controls a color palette or something (the text changes colors in other versions of this game)
			if ((m_cpumem[0x1001] >> 4) != (value >> 4))
			{
				// the overlay needs update because the game's 8-color palette has changed (but Daphne's hasn't because
				//  we support the full 32 colors)
				m_video_overlay_needs_update = true;
			}

			// Bits 1 and 2 are set and cleared regularly during LD-V1000 activity. Who knows why?
			//sprintf(s, "CPU 0: Unmapped write to %x with value %x at PC %x", addr, value, nes6502_get_pc());
			//printline(s);
		}

		// 0x1002 coin counters and blockers?
		// bit 6 toggles on NMI
		else if (addr == 0x1002)  
		{
		}

		// 0x1005 is sound -- sends a sound number to second CPU, sends 0 to mute
		// (should be easy to hack with samples for now, until we have a real sound core/mixer)
		else if (addr == 0x1005)  
		{
			m_sounddata_latch = value;
			cpu_generate_irq(1, 0); // generate an interrupt on the sound cpu 
		}	

		else if (addr == 0x1003)  
		{
			// ?
		}	

		//LD player data bus
		else if (addr == 0x1004)  
		{
			write_ldv1000(value);
		}


		// main rom
		else if (addr >= 0x4000)
		{
			sprintf(s, "Error! write to main rom at %x", addr);
			printline(s);
		}

		else
		{
			sprintf(s, "CPU 0: Unmapped write to %x with value %x", addr, value);
			printline(s);
		}

		m_cpumem[addr] = value;
		break;
	case 1:
		if (addr == 0x2000)
		{
			audio_write_ctrl_data(m_soundchip_address_latch, value, m_soundchip_id);
		}
		else if (addr == 0x4000)
		{
			m_soundchip_address_latch = value;
		}
		else
		{
			sprintf(s, "CPU 1: Unmapped write to %x with value %x", addr, value);
			printline(s);
		}
		m_cpumem2[addr] = value;
		break;
	}
}

void cobraconv::palette_calculate()
{
	SDL_Color temp_color;

	//Convert palette rom into a useable palette
	for (int i = 0; i < COBRACONV_COLOR_COUNT; i++)
	{
		Uint8 bit0,bit1,bit2;	

		/* red component */
		bit0 = static_cast<Uint8>((color_prom[i] >> 0) & 0x01);
		bit1 = static_cast<Uint8>((color_prom[i] >> 1) & 0x01);
		bit2 = static_cast<Uint8>((color_prom[i] >> 2) & 0x01); 
		temp_color.r = static_cast<Uint8>((0x21 * bit0) + (0x47 * bit1) + (0x97 * bit2));

		/* green component */
		bit0 = static_cast<Uint8>((color_prom[i] >> 3) & 0x01);
		bit1 = static_cast<Uint8>((color_prom[i] >> 4) & 0x01);
		bit2 = static_cast<Uint8>((color_prom[i] >> 5) & 0x01);
		temp_color.g = static_cast<Uint8>((0x21 * bit0) + (0x47 * bit1) + (0x97 * bit2));

		/* blue component */
		bit0 = static_cast<Uint8>(0);
		bit1 = static_cast<Uint8>((color_prom[i] >> 6) & 0x01);
		bit2 = static_cast<Uint8>((color_prom[i] >> 7) & 0x01);
		temp_color.b = static_cast<Uint8>((0x21 * bit0) + (0x47 * bit1) + (0x97 * bit2));

		////apply gamma correction to make colors brighter overall
		//Corrected value = 255 * (uncorrected value / 255) ^ (1.0 / gamma)
		//temp_color.r = (Uint8) (255 * pow((static_cast<double>(temp_color.r)) / 255, 1/COBRACONV_GAMMA));
		//temp_color.g = (Uint8) (255 * pow((static_cast<double>(temp_color.g)) / 255, 1/COBRACONV_GAMMA));
		//temp_color.b = (Uint8) (255 * pow((static_cast<double>(temp_color.b)) / 255, 1/COBRACONV_GAMMA));

		palette_set_color(i, temp_color);
	}

	// transparent color default to 0, so no change needed

}

void cobraconv::video_repaint()
{
	/*	if (palette_updated)
	{
	recalc_palette();
	m_video_overlay_needs_update = true;
	palette_updated = false;
	}  */

	//fast screen clear
	SDL_FillRect(m_video_overlay[m_active_video_overlay], NULL, 0);

	// draw sprites first(?)
	draw_sprites(0x2800, character2);

	// draw tiles
	for (int charx = 0; charx < 32; charx++)
	{
		// don't draw the first line of tiles (this is where the sprite data is)
		for (int chary = 1; chary < 32; chary++)
		{
			// draw 8x8 tiles from tile/sprite generator 2
			int current_character = m_cpumem[chary * 32 + charx + 0x2800] + 256 * (m_cpumem[chary * 32 + charx + 0x2c00] & 0x03);
			draw_8x8(current_character, 
				character2, 
				charx*8, chary*8, 
				0, 0, 
				(m_cpumem[0x1001] >> 4) & 3); // this is a decent guess about the color selection

			// draw 8x8 tiles from tile/sprite generator 1
			current_character = m_cpumem[chary * 32 + charx + 0x2000] + 256 * (m_cpumem[chary * 32 + charx + 0x2400] & 0x03);
			draw_8x8(current_character, 
				character2, 
				//charx*8, chary*8,  // x/y swapped vs Bega's Battle hardware
				chary*8, charx*8, 
				0, 0, 
				(m_cpumem[0x1001] >> 4) & 3); // this is a decent guess about the color selection
		}
	}
}

// used to set dip switch values
bool cobraconv::set_bank(unsigned char which_bank, unsigned char value)
{
	bool result = true;

	switch (which_bank)
	{
	case 0:	// bank A
		banks[1] = (unsigned char) (value ^ 0xFF);	// dip switches are active low
		break;
	case 1:	// bank B
		banks[2] = (unsigned char) (value ^ 0xFF);	// switches are active low
		break;
	default:
		printline("ERROR: Bank specified is out of range!");
		result = false;
		break;
	}

	return result;
}
// this gets called when the user presses a key or moves the joystick
void cobraconv::input_enable(Uint8 move)
{
	switch (move)
	{
	case SWITCH_UP:
		banks[0] &= ~0x01; 
		break;
	case SWITCH_LEFT:
		banks[0] &= ~0x08; 
		break;
	case SWITCH_RIGHT:
		banks[0] &= ~0x04; 
		break;
	case SWITCH_DOWN:
		banks[0] &= ~0x02; 
		break;
	case SWITCH_START1:
		banks[0] &= ~0x40; 
		break;
	case SWITCH_START2:
		banks[0] &= ~0x80; 
		break;
	case SWITCH_BUTTON1: 
		banks[0] &= ~0x10; 
		break;
	case SWITCH_BUTTON2: 
		banks[0] &= ~0x20; 
		break;
	case SWITCH_BUTTON3: 
		//		banks[3] &= ~0x10; 
		break;
	case SWITCH_COIN1: 
		banks[3] &= ~0x04; 
		cpu_generate_nmi(0);
		break;
	case SWITCH_COIN2: 
		banks[3] &= ~0x02;	// right coin chute is bit 1 (see a97f)
		cpu_generate_nmi(0);
		break;
	case SWITCH_SERVICE: 
		//		banks[3] &= ~0x08; 
		break;
	case SWITCH_TILT:
		banks[3] |= 0x01;  // yes, I think tilt is active high (see a2de in ROM for tilt detector)
		break;
	default:
		//		printline("Error, bug in move enable");
		break;
	}
}

// this gets called when the user releases a key or moves the joystick back to center position
void cobraconv::input_disable(Uint8 move)
{
	switch (move)
	{
	case SWITCH_UP:
		banks[0] |= 0x01; 
		break;
	case SWITCH_LEFT:
		banks[0] |= 0x08; 
		break;
	case SWITCH_RIGHT:
		banks[0] |= 0x04; 
		break;
	case SWITCH_DOWN:
		banks[0] |= 0x02; 
		break;
	case SWITCH_START1: 
		banks[0] |= 0x40; 
		break;
	case SWITCH_START2: 
		banks[0] |= 0x80; 
		break;
	case SWITCH_BUTTON1: 
		banks[0] |= 0x10; 
		break;
	case SWITCH_BUTTON2: 
		banks[0] |= 0x20; 
		break;
	case SWITCH_BUTTON3: 
		//		banks[3] |= 0x10; 
		break;
	case SWITCH_COIN1: 
		banks[3] |= 0x04; 
		break;
	case SWITCH_COIN2: 
		banks[3] |= 0x02;	// right coin chute is bit 1 (see a97f)
		break;
	case SWITCH_SERVICE: 
		//		banks[3] |= 0x08; 
		break;
	case SWITCH_TILT: 
		banks[3] &= ~0x01;
		break;
	default:
		//		printline("Error, bug in move enable");
		break;
	}
}

void cobraconv::OnVblank()
{
	video_blit();
	ldv1000_report_vsync();
}

void cobraconv::OnLDV1000LineChange(bool bIsStatus, bool bIsEnabled)
{
	// If the status strobe has hit, then do an IRQ
	// NOTE : I am just guessing that the IRQ is caused by the status strobe, and
	//  I think this guess may be incorrect, because the IRQ handler waits for the status strobe.
	// However, I am fairly confident that the IRQ is not caused by vblank because other parts of the ROM loop waiting for vblank.
	// The IRQ handler seems to only be interested in reading the status from the LD-V1000, so causing the IRQ on the
	//  status strobe is good enough for now.
	if ((bIsStatus) && (bIsEnabled))
	{
		// do an IRQ
		cpu_generate_irq(0, 0);
	}
}

void cobraconv::draw_8x8(int character_number, Uint8 *character_set, int xcoord, int ycoord,
						 int xflip, int yflip, int color)
{
	Uint8 pixel[8] = {0};

	for (int y = 0; y < 8; y++)
	{
		Uint8 byte1 = character_set[character_number*8+y];
		Uint8 byte2 = character_set[character_number*8+y+0x2000];
		Uint8 byte3 = character_set[character_number*8+y+0x4000];

		pixel[0] = static_cast<Uint8>(((byte1 & 0x01) << 2) | ((byte2 & 0x01) << 1) | ((byte3 & 0x01) << 0));
		pixel[1] = static_cast<Uint8>(((byte1 & 0x02) << 1) | ((byte2 & 0x02) << 0) | ((byte3 & 0x02) >> 1));
		pixel[2] = static_cast<Uint8>(((byte1 & 0x04) << 0) | ((byte2 & 0x04) >> 1) | ((byte3 & 0x04) >> 2));
		pixel[3] = static_cast<Uint8>(((byte1 & 0x08) >> 1) | ((byte2 & 0x08) >> 2) | ((byte3 & 0x08) >> 3));
		pixel[4] = static_cast<Uint8>(((byte1 & 0x10) >> 2) | ((byte2 & 0x10) >> 3) | ((byte3 & 0x10) >> 4));
		pixel[5] = static_cast<Uint8>(((byte1 & 0x20) >> 3) | ((byte2 & 0x20) >> 4) | ((byte3 & 0x20) >> 5));
		pixel[6] = static_cast<Uint8>(((byte1 & 0x40) >> 4) | ((byte2 & 0x40) >> 5) | ((byte3 & 0x40) >> 6));
		pixel[7] = static_cast<Uint8>(((byte1 & 0x80) >> 5) | ((byte2 & 0x80) >> 6) | ((byte3 & 0x80) >> 7));

		for (int x = 0; x < 8; x++)
		{
			if (pixel[x])
			{
				*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + ((ycoord + (yflip ? y : (7-y))) * COBRACONV_OVERLAY_W) + (xcoord + (xflip ? (7-x) : x))) = pixel[x] + (8*color);
			}
		}
	}
}

void cobraconv::draw_16x32(int character_number, Uint8 *character_set, int xcoord, int ycoord,
						   int xflip, int yflip, int color)
{
	Uint8 pixel[16] = {0};

	for (int b = 0; b < 8; b+=2) // laid out as four 8-pixel high blocks
	{
		for (int y = 0; y < 8; y++)
		{
			Uint8 byte1 = character_set[character_number*32+y+(b*8)];
			Uint8 byte2 = character_set[character_number*32+y+0x2000+(b*8)];
			Uint8 byte3 = character_set[character_number*32+y+0x4000+(b*8)];
			Uint8 byte4 = character_set[character_number*32+y+((b+1)*8)];
			Uint8 byte5 = character_set[character_number*32+y+0x2000+((b+1)*8)];
			Uint8 byte6 = character_set[character_number*32+y+0x4000+((b+1)*8)];

			pixel[0]  = static_cast<Uint8>((((byte1 & 0x01) << 2) | ((byte2 & 0x01) << 1) | ((byte3 & 0x01) << 0)));
			pixel[1]  = static_cast<Uint8>((((byte1 & 0x02) << 1) | ((byte2 & 0x02) << 0) | ((byte3 & 0x02) >> 1)));
			pixel[2]  = static_cast<Uint8>((((byte1 & 0x04) << 0) | ((byte2 & 0x04) >> 1) | ((byte3 & 0x04) >> 2)));
			pixel[3]  = static_cast<Uint8>((((byte1 & 0x08) >> 1) | ((byte2 & 0x08) >> 2) | ((byte3 & 0x08) >> 3)));
			pixel[4]  = static_cast<Uint8>((((byte1 & 0x10) >> 2) | ((byte2 & 0x10) >> 3) | ((byte3 & 0x10) >> 4)));
			pixel[5]  = static_cast<Uint8>((((byte1 & 0x20) >> 3) | ((byte2 & 0x20) >> 4) | ((byte3 & 0x20) >> 5)));
			pixel[6]  = static_cast<Uint8>((((byte1 & 0x40) >> 4) | ((byte2 & 0x40) >> 5) | ((byte3 & 0x40) >> 6)));
			pixel[7]  = static_cast<Uint8>((((byte1 & 0x80) >> 5) | ((byte2 & 0x80) >> 6) | ((byte3 & 0x80) >> 7)));
			pixel[8]  = static_cast<Uint8>((((byte4 & 0x01) << 2) | ((byte5 & 0x01) << 1) | ((byte6 & 0x01) << 0)));
			pixel[9]  = static_cast<Uint8>((((byte4 & 0x02) << 1) | ((byte5 & 0x02) << 0) | ((byte6 & 0x02) >> 1)));
			pixel[10] = static_cast<Uint8>((((byte4 & 0x04) << 0) | ((byte5 & 0x04) >> 1) | ((byte6 & 0x04) >> 2)));
			pixel[11] = static_cast<Uint8>((((byte4 & 0x08) >> 1) | ((byte5 & 0x08) >> 2) | ((byte6 & 0x08) >> 3)));
			pixel[12] = static_cast<Uint8>((((byte4 & 0x10) >> 2) | ((byte5 & 0x10) >> 3) | ((byte6 & 0x10) >> 4)));
			pixel[13] = static_cast<Uint8>((((byte4 & 0x20) >> 3) | ((byte5 & 0x20) >> 4) | ((byte6 & 0x20) >> 5)));
			pixel[14] = static_cast<Uint8>((((byte4 & 0x40) >> 4) | ((byte5 & 0x40) >> 5) | ((byte6 & 0x40) >> 6)));
			pixel[15] = static_cast<Uint8>((((byte4 & 0x80) >> 5) | ((byte5 & 0x80) >> 6) | ((byte6 & 0x80) >> 7)));

			for (int x = 0; x < 16; x++)
			{
				if (pixel[x])
				{
					*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + ((ycoord + (8-y)+(b*4)) * COBRACONV_OVERLAY_W) + (xcoord + (xflip ? (15-x) : x))) = pixel[x] + (8*color);
				}			
			}
		}
	}
}
void cobraconv::draw_sprites(int offset, Uint8 *character_set)
{
	for (int sprites = 0; sprites < 0x32; sprites += 4)
	{
		if ((m_cpumem[offset + sprites] & 0x01) && (m_cpumem[offset + sprites + 3] < 240))
		{		
			//			char s[80];
			//			sprintf(s, "sprite %x, char=%x, x=%x, y=%x", sprites, m_cpumem[offset + sprites + 1],
			//				m_cpumem[offset + sprites + 3], m_cpumem[offset + sprites + 2]);
			//			printline(s);
			draw_16x32(m_cpumem[offset + sprites + 1],
				character_set,
				m_cpumem[offset + sprites + 3], 
				m_cpumem[offset + sprites + 2], 
				m_cpumem[offset + sprites] & 0x04,
				m_cpumem[offset + sprites] & 0x02,
				0); // this isn't the correct color... i'm not sure where color comes from right now
		}
	}
}

