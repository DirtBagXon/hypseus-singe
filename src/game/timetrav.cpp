/*
 * timetrav.cpp
 *
 * Copyright (C) 2005 Mark Broadhead
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

#include <string.h> // for memset
#include <stdio.h>	// for sprintf (we shouldn't use sprintf anymore)
#include "timetrav.h"
#include "../ldp-out/ldp.h"
#include "../io/conout.h"
#include "../sound/sound.h"
#include "../video/palette.h"
#include "../video/video.h"

// Time Traveler

timetrav::timetrav()
{
	m_shortgamename = "timetrav";
   memset(m_cpumem, 0, CPU_MEM_SIZE);

   struct cpudef cpu;
   memset(&cpu, 0, sizeof(struct cpudef));
	cpu.type = CPU_I88;
	cpu.hz = TIMETRAV_CPU_HZ;
	cpu.irq_period[0] = 0; 
	cpu.irq_period[1] = 0;	
	cpu.nmi_period = (1000.0 / 59.94);	
	cpu.initial_pc = 0xFFFF0;
	cpu.must_copy_context = false;
	cpu.mem = m_cpumem;
	add_cpu(&cpu);	// add this cpu to the list (it will be our only one)

   m_disc_fps = 29.97;
//	m_game_type = GAME_TIMETRAV;
   m_game_uses_video_overlay = true;
   m_video_overlay_width = 320;	// default values we start with for video overlay
   m_video_overlay_height = 240;
   m_palette_color_count = 256;
   m_video_overlay_count = 1;
   m_overlay_size_is_dynamic = true;	// this game does dynamically change its overlay size
	
	static struct rom_def g_timetrav_roms[] =
	{
		{ "TT061891.BIN", NULL, &m_cpumem[0xc0000], 0x40000, 0x00000000 },
		{ NULL }
	};
	m_rom_list = g_timetrav_roms;
}

void timetrav::do_nmi()
{
}

Uint8 timetrav::cpu_mem_read(Uint32 addr)
{
	char s[80];
	Uint8 result = m_cpumem[addr];

	// Scratch ram
   if (addr < 0x10000)
	{
	}
   // ROM
   else if (addr >= 0xc0000)
	{
	}
	else 
	{
		sprintf(s, "Unmapped read from %x", addr);
		printline(s);
	}
	return (result);
}

void timetrav::cpu_mem_write(Uint32 addr, Uint8 value)
{
	char s[80];

   m_cpumem[addr] = value;

	// Scratch ram
   if (addr < 0x10000)
	{
	}
   // ROM
	else if (addr >= 0xc0000)
	{
		sprintf(s, "Write to rom at %x with %x!", addr, value);
		printline(s);
	}
	else 
	{
		sprintf(s, "Unmapped write to %x with %x", addr, value);
		printline(s);
	}
}

void timetrav::port_write(Uint16 port, Uint8 value)
{
	char s[80];
   static char display_string[9] = {0};

   switch(port)
   {
      case 0x1180:
      case 0x1181:
      case 0x1182:
      case 0x1183:
      case 0x1184:
      case 0x1185:
      case 0x1186:
      case 0x1187:
         m_video_overlay_needs_update = true;
         display_string[port & 0x07] = value;
         draw_string(display_string, 0, 0, get_active_video_overlay());
         video_blit();
         break;
      default:
         sprintf(s, "Unmapped write to port %x, value %x", port, value);
		   printline(s);
         break;
   }
}

Uint8 timetrav::port_read(Uint16 port)
{
	char s[80];

	unsigned char result = 0;

		sprintf(s, "Unmapped read from port %x", port);
		printline(s);
		return (result);
}


// used to set dip switch values
bool timetrav::set_bank(unsigned char which_bank, unsigned char value)
{
	bool result = true;
	
	if (which_bank == 0) 
	{
	}
	else
	{
		printline("ERROR: Bank specified is out of range!");
		result = false;
	}

	return result;
}

void timetrav::input_disable(Uint8 move)
{
}

void timetrav::input_enable(Uint8 move)
{
}

void timetrav::palette_calculate()
{
	SDL_Color temp_color;

	// fill color palette with schlop because we only use colors 0 and 0xFF for now
	for (int i = 0; i < 256; i++)
	{
		temp_color.r = (unsigned char) i;
		temp_color.g = (unsigned char) i;
		temp_color.b = (unsigned char) i;
		
		palette_set_color(i, temp_color);
	}
}
