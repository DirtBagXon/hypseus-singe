/*
 * firefox.cpp
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

// firefox.cpp
// by Mark Broadhead
//

#include <string.h>
#include "firefox.h"
#include "../cpu/cpu.h"
#include "../cpu/mc6809.h"
#include "../io/conout.h"
#include "../ldp-in/vp931.h"
#include "../ldp-out/ldp.h"
#include "../video/video.h"
#include "../video/palette.h"
#include "../cpu/cpu-debug.h"	// for cpu debugging

////////////////

// MEMORY MAP (MAIN CPU)
//----------------------
// 0xC000-0xFFFF = ROM2
// 0x8000-0xBFFF = ROM1
// 0x4400-0x7FFF = ROM0 (the 0x4000-0x43FF part of the ROM cannot be accessed)
// 0x4000-0x43FF = I/O and NVRAM
//	0x42xx = Outputs
//		0x42A0-A8 = DSKLATCH
//		0x4298-9F = WRSOUND
//		0x4290-97 = WRTREG (table ROM select)
//		0x4288-428F = LATCH1
//		NOTE: 0x4280-0x4287 are enabled/disabled based on the high-bit of the byte written to the memory location
//		0x4287 = WRDSK
//		0x4286 = RSTDSK
//		0x4285 = SWDSKL
//		0x4284 = SWDSKR
//		0x4283 = LOCK
//		0x4282 = NVRSTORE
//		0x4281 = RSTSOUND
//		0x4280 = NVRECALL
//		0x4230-38 = AMUCK
//		0x4220-28 = ADCSTART
//		0x4218-1F = DSKREAD
//		0x4210-17 = WDCLK (watchdog?)
//		0x4208-0F = RSTFIRQ
//		0x4200-07 = RSTIRQ
//	0x41xx = Inputs
//		0x41x7 = ADC
//		0x41x6 = RDSOUND
//		0x41x5 = DREAD
//		0x41x4 = OPT1 (dipswitch1)
//		0x41x3 = OPT0 (dipswitch0)
//		0x41x2 = RDIN2
//		0x41x1 = RDIN1
//		0x41x0 = RDIN0
//	0x40xx = NVRAM
//	0x3000-0x3FFF = ROM TABLE input
//	0x2000-0x2FFF = MOTION (graphics)
//	0x1000-0x1FFF = ALPHA (graphics)
//	0x0800-0x0FFF = RAM1
//	0x0000-0x07FF = RAM0

////////////////

// who knows if this is correct or not...
#define FIREFOX_CPU_HZ 4000000

firefox::firefox()
{
	struct cpudef cpu;
	
	m_shortgamename = "firefox";
	memset(&cpu, 0, sizeof(struct cpudef));
	memset(banks, 0xFF, 4);	// fill banks with 0xFF's
//	banks[0] = 0xfb;
	banks[1] = 0x1f;
	banks[4] = 0x00;
	banks[5] = 0x00;
	m_disc_fps = 29.97;

	m_bFIRQLatch = false;
	m_bIRQLatch = false;

	m_video_overlay_width = FIREFOX_OVERLAY_W;
	m_video_overlay_height = FIREFOX_OVERLAY_H;
	m_uVideoOverlayVisibleLines = 480;	// firefox is unusual in that it has a 1:1 vertical line mapping with the laserdisc video
	m_video_row_offset = -16;	// the visible area is from line 16-496 (top and bottom 16 lines are hidden)
	m_palette_color_count = FIREFOX_COLORS;

	cpu.type = CPU_M6809;
	cpu.hz = FIREFOX_CPU_HZ;
	cpu.nmi_period = 0;	// firefox has no NMI

	// MarkB came up with this number, I'm not sure how he arrived at it, but it seems to be too frequent because
	//  LDP commands may be getting sent too frequently for every response from the LDP.
	//cpu.irq_period[0] = 1000.0 / (60.0 * 6.0);

	// It is very difficult to tell from the schematics what events assert the IRQ.
	// After looking at the ROM disassembly, it seems that the IRQ should occur 4 times per field,
	//  so I'm gonna try that for now.
	cpu.irq_period[0] = (1000.0 / (59.94)) / 4;

	cpu.initial_pc = 0;
	cpu.must_copy_context = false;	// set to true for multiple 6809's
	cpu.mem = m_cpumem;
	add_cpu(&cpu);	// add a 6809 cpu
	
	ad_converter_channel = 0;
	current_bank = 0x0000;
	m_game_issues = "Inputs aren't hooked up and the LDP isn't implemented";

	m_nvram_begin = &m_cpumem[0x4000];
	m_nvram_size = 0xff;

	const static struct rom_def firefox_roms[] =
	{
		// main program
		{ "136026.109", NULL, &m_cpumem[0x4000], 0x4000, 0x7639270c },
		{ "136026.110", NULL, &m_cpumem[0x8000], 0x4000, 0xf3102944 },
		{ "136026.111", NULL, &m_cpumem[0xc000], 0x4000, 0x8a230bb5 },

		// rom banks
		{ "136026.101", NULL, &rombank[0x0000], 0x4000, 0x91bba45a },	// 8A
		{ "136026.102", NULL, &rombank[0x4000], 0x4000, 0x5f1e423d },	// 7A
		{ "136026.105", NULL, &rombank[0x8000], 0x4000, 0x83f1d4ed },	// 4A
		{ "136026.106", NULL, &rombank[0xc000], 0x4000, 0xc5d8d417 },	// 3A

		// characters
		{ "136026.125", NULL, &character[0x0000], 0x2000, 0x8a32f9f1 },
		{ NULL }
	};

	m_rom_list = firefox_roms;

	// UPDATE : this is too slow so I'll handle the DAK delay differently
	// This runs 50 cycles each cpu iteration, which I need in order to have the DAK work in a timely way
//	cpu_change_interleave(FIREFOX_CPU_HZ / 50000);

}

firefoxa::firefoxa()
{
	m_shortgamename = "firefoxa";

	const static struct rom_def firefoxa_roms[] =
	{
		// main program
		{ "136026.209", NULL, &m_cpumem[0x4000], 0x4000, 0 },
		{ "136026.210", NULL, &m_cpumem[0x8000], 0x4000, 0 },
		{ "136026.211", NULL, &m_cpumem[0xc000], 0x4000, 0 },

		// rom banks
		{ "136026.201", NULL, &rombank[0x0000], 0x4000, 0 },
		{ "136026.205", NULL, &rombank[0x8000], 0x4000, 0 },
		{ "136026.127", NULL, &rombank[0xc000], 0x2000, 0 },

		// characters
		{ "136026.125", "firefox", &character[0x0000], 0x2000, 0 },
		{ NULL }
	};

	m_rom_list = firefoxa_roms;

}

bool firefox::init()
{
	cpu_init();
	g_ldp->pre_play();	// the VP931 starts playing automatically
	return true;
}

void firefox::do_irq(unsigned int which_irq)
{
	static unsigned int uVblankCounter = 0;

	// every 4th IRQ should have vblank enabled, based on looking at the ROM disassembly
	// vblank is bit 5 of 0x4101, hence the 0x20
	if ((uVblankCounter & 3) == 0)
	{
		banks[1] |= 0x20;
	}
	else
	{
		banks[1] &= ~0x20;
	}

	// we want to space this out far enough away from vblank as to not cause a conflict
	if ((uVblankCounter & 3) == 1)
	{
		// Firefox seems to have its FIRQ tied to DAV becoming active (going low) which happens at or shortly after vblank
		do_firq();
	}

	++uVblankCounter;

	mc6809_irq = 1;
	m_bIRQLatch = true;
}

void firefox::do_nmi()
{
}

void firefox::do_firq()
{
	mc6809_firq = 1;
	m_bFIRQLatch = true;
}

Uint8 firefox::cpu_mem_read(Uint16 addr)
{
//	char s[81] = {0};

	Uint8 result = m_cpumem[addr];

	// Program RAM
	if (addr <= 0x0fff)
	{
	}

	// Graphics/Alphanumerics RAM
	else if (addr >= 0x1000 && addr <= 0x1fff)
	{
	}

	// Motion Object RAM
	else if (addr >= 0x2000 && addr <= 0x27ff)
	{
	}

	// Table Rom
	else if (addr >= 0x3000 && addr <= 0x3fff)
	{
		result = rombank[current_bank | (addr & 0xfff)];
//		sprintf(s, "current bank %x reading m_cpumem[%x], returning rombank[%x]", current_bank, addr, current_bank + (addr - 0x3000));
//		printline(s);
	}

	// RDIN0 from schematics
	// BITS:
	// 7 - left fire / start
	// 6 - right fire / start
	// 5 - left thumb
	// 4 - right thumb
	// 3 - slam/tilt
	// 2 - self-test
	// 1 - spare
	else if (addr == 0x4100)
	{
		result = banks[0];
	}

	// RDIN1 from schematics
	// BITS:
	// 7 - MAINFLAG
	// 6 - SOUNDFLAG
	// 5 - VBLANK (1 = on, 0 = off)
	// 4 - DIAG (active low?)
	// 3 - not connected?
	// 2 - COIN AUX
	// 1 - left coin
	// 0 - right coin
	else if (addr == 0x4101)
	{
		result = banks[1];
	}

	// Disc status, bit 7 - Diskdav, 6 - DAK, 5 - Disk Opr (operational, always enabled if player is up and running?)
	else if (addr == 0x4102)
	{
		result = 0x00;

		// DAV = active low (see E4B9 for support to this theory)
		if (!vp931_is_dav_active()) result |= (1 << 7);

		// DAK is active high, DAK low means the buffer is full
		// (the firefox schematics reverse this and view things from a FULL, active-low viewpoint)
		if (vp931_is_dak_active()) result |= (1 << 6);

		// I _think_ this should always be high
		if (vp931_is_oprt_active()) result |= (1 << 5);
	}

	// Option Sw 1 
	else if (addr == 0x4103)
	{
//		result = banks[2];
	}

	// Option Sw 2 
	else if (addr == 0x4104)
	{
//		result = banks[3];
	}

	// Read VP931 data (DREAD)
	//  and disabled RDDSK (sets it high)
	else if (addr == 0x4105)
	{
		result = read_vp931();
		vp931_change_read_line(false);
	}

	// RDSOUND
	else if (addr == 0x4106)
	{
	}

	// Read A/D Converter (ADC)
	else if (addr == 0x4107)
	{
		switch (ad_converter_channel)
		{
		case 0:
			result = banks[4];
			break;
		case 1:
			result = banks[5];
			break;
		default:
			printline("Invalid A/D Converter channel");
			break;
		}
	}

	// Program ROM
	else if (addr >= 0x4400)
	{
	}

	else
	{
//		sprintf(s, "Unmapped read from %x", addr);
//		printline(s);
	}

	return result;
}

void firefox::cpu_mem_write(Uint16 addr, Uint8 value)
{
	char s[81] = {0};

	// some write locations don't care about the bottom 3 bits
	unsigned int uAddrANDFFF8 = addr & 0xFFF8;

	// Program RAM
	if (addr <= 0x0fff)
	{
		/*
		if ((addr >= 0x4D) && (addr <= 0x4F))
		{
			if (value == 0x11)
			{
#ifdef DEBUG
				set_cpu_trace(1);
#endif // DEBUG
			}
		}
		*/
	}

	// Graphics/Alphanumerics RAM
	else if (addr >= 0x1000 && addr <= 0x1fff)
	{
		m_video_overlay_needs_update = true;
	}

	// Motion Object RAM
	else if (addr >= 0x2000 && addr <= 0x27ff)
	{
	}

	// Color Ram (sprites)
	else if (addr >= 0x2800 && addr <= 0x2aff)
	{
	}
	
	// sprite bank select
	else if (addr == 0x2b00)
	{
	}

	// Color Ram (alphanumerics)
	else if (addr >= 0x2c00 && addr <= 0x2fff)
	{
		palette_modified = true;
	}
	
	// reset IRQ
	else if (uAddrANDFFF8 == 0x4200)
	{
		m_bIRQLatch = false;
	}

	// reset FIRQ
	else if (uAddrANDFFF8 == 0x4208)
	{
		m_bFIRQLatch = false;
	}

	// Reset Watchdog (WDCLK)
	else if (addr == 0x4210)
	{
	}
	
	// DSKREAD
	// (makes RDDSK lo, ie enables it)
	else if (addr == 0x4218)
	{
		vp931_change_read_line(true);
	}
	
	// Start A/D Converter Channel 0/1 (ADCSTART)
	else if (addr >= 0x4220 && addr <= 0x4221)
	{
		ad_converter_channel = addr & 0x01;
	}

	// 4228 not wired to anything

	// AMUCK?
	else if (addr == 0x4230)
	{
	}

	// LATCH0
	else if ((addr >= 0x4280) && (addr <= 0x4287))
	{
		unsigned int D = value & 0x80;

		switch (addr & 7)
		{
			// nvrecall
		case 0:
			break;
			//rstsound
		case 1:
			break;
			// nvrstore
		case 2:
			break;
			// lock
		case 3:
			break;
			// swdskr
		case 4:
			break;
			// swkskl
		case 5:
			break;
		case 6:				// rstdsk
			// rstdsk is active low
			vp931_change_reset_line(D == 0);
			break;
		case 7:				// wrdsk
			// wrdsk is active low
			if (!D)
			{
				write_vp931(m_u8DskLatch);	// this must come before the write line is enabled
				vp931_change_write_line(true);
			}
			// else the write line is being disabled
			else
			{
				vp931_change_write_line(false);
			}
			break;
		default:
			printline("Firefox ERROR 0x4280-0x4287 section");
			break;
		}
	}

	// coin counter (LATCH1)
	else if ((addr >= 0x4288) && (addr <= 0x4289))
	{
	}

	// Led 1-4 (LATCH1)
	else if (addr >= 0x428c && addr <= 0x428f)
	{
		if (value & 0x80)
		{
			sprintf(s,"Led %x off", (addr & 0x03) + 1);
		}
		else 
		{
			sprintf(s,"Led %x on", (addr & 0x03) + 1);
		}
		printline(s);
	}

	// Rom paging @ 3000 (WRTREG)
	else if (addr == 0x4290)
	{
		unsigned int val3 = value & 3;	// lower 2 bits (DBB0 and DBB1) are A12 and A13 respectively

		switch(value & 0x1c)
		{
			case 0x00: /* T0 */
				current_bank = (0x0000 + (0x1000 * val3));
	//			sprintf(s, "bank switch, T0: %x\n", value & 0x3);
	//			printline(s);
				break;
			case 0x04: /* T1 */
				current_bank = (0x4000 + (0x1000 * val3));
	//			sprintf(s, "bank switch, T1: %x\n", value & 0x3);
	//			printline(s);
				break;
			case 0x10: /* T4 */
				current_bank = (0x8000 + (0x1000 * val3));
	//			sprintf(s, "bank switch, T4: %x\n", value & 0x3);
	//			printline(s);
				break;
			case 0x14: /* T5 */
				current_bank = (0xc000 + (0x1000 * val3));
	//			sprintf(s, "bank switch, T5: %x\n", value & 0x3);
	//			printline(s);
				break;
			default:
				sprintf(s,"Invalid bank switch, %x", value);
				printline(s);
				break;
		}
	}

	// WRSOUND
	else if (addr == 0x4298)
	{
	}

	// DSKLATCH (store data in latch to be send to LDP)
	else if (addr == 0x42a0)
	{
		m_u8DskLatch = value;
	}
	
	// Program ROM
	else if (addr >= 0x4400)
	{
		printline("ERROR: Write to program rom!");
	}

	else
	{
//		sprintf(s, "Unmapped write to %x with %x", addr, value);
//		printline(s);
	}


	m_cpumem[addr] = value;
}

void firefox::palette_calculate()
{
	SDL_Color color;
	for (int x = 0; x < FIREFOX_COLORS; x++)
	{
		color.r = m_cpumem[0x2c00 + x];
		color.g = m_cpumem[0x2d00 + x];
		color.b = m_cpumem[0x2e00 + x] & 0xfd;
		palette_set_color(x, color);
	}
}

// updates firefox's video
void firefox::video_repaint()
{
	if (palette_modified)
	{
		palette_calculate();
		palette_finalize();
	}

	for (int charx = 0; charx < 64; charx++)
	{
		for (int chary = 0; chary < 64; chary++)
		{
			for (int x=0; x < 4; x++)
			{
				for (int y = 0; y < 8; y++)
				{
					Uint8 left_pixel = static_cast<Uint8>((character[m_cpumem[chary * 64 + charx + 0x1000]*32+x+4*y] & 0xf0) >> 4);
					Uint8 right_pixel = static_cast<Uint8>((character[m_cpumem[chary * 64 + charx + 0x1000]*32+x+4*y] & 0x0f));

					*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + (((chary) * 8 + y) * FIREFOX_OVERLAY_W) + ((charx) * 8 + x * 2)) = left_pixel;
					*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + (((chary) * 8 + y) * FIREFOX_OVERLAY_W) + ((charx) * 8 + x * 2 + 1)) = right_pixel;

//						*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + (((chary) * 8 + y) * FIREFOX_OVERLAY_W) + ((charx - 10) * 16 + x * 4 + 0)) = left_pixel;
//						*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + (((chary) * 8 + y) * FIREFOX_OVERLAY_W) + ((charx - 10) * 16 + x * 4 + 1)) = left_pixel;
//						*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + (((chary) * 8 + y) * FIREFOX_OVERLAY_W) + ((charx - 10) * 16 + x * 4 + 2)) = right_pixel;
//						*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + (((chary) * 8 + y) * FIREFOX_OVERLAY_W) + ((charx - 10) * 16 + x * 4 + 3)) = right_pixel;
				}
			}
		}
	}
}

// this gets called when the user presses a key or moves the joystick
void firefox::input_enable(Uint8 move)
{
	switch (move)
	{
	case SWITCH_UP:
		banks[4] = 0x00;
		break;
	case SWITCH_LEFT:
		banks[5] = 0x00;
		break;
	case SWITCH_RIGHT:
		banks[5] = 0xff;
		break;
	case SWITCH_DOWN:
		banks[4] = 0xff;
		break;
	case SWITCH_BUTTON1:	// left fire
		banks[0] &= ~0x80; 
		break;
	case SWITCH_BUTTON2:	// right fire
		banks[0] &= ~0x40;
		break;
	case SWITCH_BUTTON3:	// left thumb
		banks[0] &= ~0x20; 
		break;
		// TODO : right thumb
	case SWITCH_TILT: 
		banks[0] &= ~0x08; 
		break;
	case SWITCH_TEST:
		banks[0] &= ~0x02; 
		break;
	case SWITCH_COIN1: 
		banks[1] &= ~0x02; // left coin
		break;
	case SWITCH_COIN2: 
		banks[1] &= ~0x01; // right coin
		break;
	default:
		break;
	}
}  

// this gets called when the user releases a key or moves the joystick back to center position
void firefox::input_disable(Uint8 move)
{
	switch (move)
	{
	case SWITCH_UP:
		banks[4] = 0x7f;	// centered
		break;
	case SWITCH_LEFT:
		banks[5] = 0x7f;
		break;
	case SWITCH_RIGHT:
		banks[5] = 0x7f;
		break;
	case SWITCH_DOWN:
		banks[4] = 0x7f;
		break;
	case SWITCH_BUTTON1:	// left fire
		banks[0] |= 0x80; 
		break;
	case SWITCH_BUTTON2:	// right fire
		banks[0] |= 0x40;
		break;
	case SWITCH_BUTTON3:	// left thumb
		banks[0] |= 0x20; 
		break;
		// TODO : right thumb
	case SWITCH_TILT: 
		banks[0] |= 0x08; 
		break;
	case SWITCH_TEST:
		banks[0] |= 0x02; 
		break;
	case SWITCH_COIN1: 
		banks[1] |= 0x02; // left coin
		break;
	case SWITCH_COIN2: 
		banks[1] |= 0x01; // right coin
		break;
	default:
		break;
	}
}

void firefox::OnVblank()
{
	video_blit();

	// not really used for emulated vblank because the timing is still somewhat unknown

	vp931_report_vsync();
}

// used to set dip switch values
bool firefox::set_bank(unsigned char which_bank, unsigned char value)
{
	bool result = true;
	
	switch (which_bank)
	{
	case 0:	// dipswitch 1
		banks[2] = (unsigned char) (value ^ 0xFF);	// dip switches are active low
		break;
	case 1:	// dipswitch 2
		banks[3] = (unsigned char) (value ^ 0xFF);	// switches are active low
		break;
	default:
		printline("ERROR: Bank specified is out of range!");
		result = false;
		break;
	}
	
	return result;
}
