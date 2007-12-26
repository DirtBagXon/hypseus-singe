/*
 * lgp.cpp
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


// lgp.cpp
// by Mark Broadhead
//
// Taito Laser Grand Prix
//
// Uses two Z80's and an LD-V1000
// Main Z80 memory map
// --------------------
// 0000 7fff Rom (a02_01.bin, a02_02.bin, a02_03.bin, a02_04.bin)
// e000 e400 Ram (tile video ram)
// f000 ffff Ram 

#include <string.h>
#include "lgp.h"
#include "../io/conout.h"
#include "../ldp-in/ldv1000.h"
#include "../ldp-out/ldp.h"
#include "../video/palette.h"
#include "../video/video.h"
#include "../sound/sound.h"
#include "../cpu/cpu.h"
#include "../cpu/generic_z80.h"

lgp::lgp()
{
	struct cpudef cpu;
	
	m_shortgamename = "lgp";
	memset(&cpu, 0, sizeof(struct cpudef));
	memset(m_cpumem, 0x00, 0x10000);	// making sure m_cpumem[] is zero'd out
	memset(m_cpumem2, 0x00, 0x10000);	// making sure m_cpumem2[] is zero'd out
	memset(banks, 0x00, 7);	// fill banks with 0xFF's
   palette_modified = true;
	m_disc_fps = 29.97; 

	m_video_overlay_width = LGP_OVERLAY_W;
	m_video_overlay_height = LGP_OVERLAY_H;
	m_palette_color_count = LGP_COLOR_COUNT;
	
	cpu.type = CPU_Z80;
	cpu.hz = 5000000; 
	cpu.irq_period[0] = 16.6666; // interrupt from vblank (60hz)
	cpu.nmi_period = 0; // nmi from sound chip?
	cpu.initial_pc = 0;
	cpu.must_copy_context = true;
	cpu.mem = m_cpumem;
	add_cpu(&cpu);	// add a z80	

	cpu.type = CPU_Z80;
   cpu.hz = 5000000; 
	cpu.irq_period[0] = 0; 
	cpu.nmi_period = 0; 
	cpu.initial_pc = 0;
	cpu.must_copy_context = true;
	cpu.mem = m_cpumem2;
	add_cpu(&cpu);	// add a z80
	
   struct sounddef soundchip;
   soundchip.type = SOUNDCHIP_AY_3_8910;
   soundchip.hz = 2500000; // complete guess
   m_soundchip1_id = add_soundchip(&soundchip);
   m_soundchip2_id = add_soundchip(&soundchip);
   m_soundchip3_id = add_soundchip(&soundchip);
   m_soundchip4_id = add_soundchip(&soundchip);

	m_transparent_color = 0;
   m_ldp_write_latch = 0xff;
   m_ldp_read_latch = 0xff;

	const static struct rom_def lgp_roms[] =
	{
      // main z80 rom
		{ "a02_01.bin", NULL, &m_cpumem[0x0000], 0x2000, 0 },
		{ "a02_02.bin", NULL, &m_cpumem[0x2000], 0x2000, 0 },
		{ "a02_03.bin", NULL, &m_cpumem[0x4000], 0x2000, 0 },
		{ "a02_04.bin", NULL, &m_cpumem[0x6000], 0x2000, 0 },

      // sound cpu roms
		{ "A02_29.bin", NULL, &m_cpumem2[0x0000], 0x2000, 0 },
		//{ "A02_30.bin", NULL, &m_cpumem2[0x2000], 0x2000, 0 },
      
      // tile roms
      { "a02_14.bin", NULL, &m_character[0x0000], 0x2000, 0 },
      { "a02_15.bin", NULL, &m_character[0x2000], 0x2000, 0 },
      { "a02_16.bin", NULL, &m_character[0x4000], 0x2000, 0 },
      { "a02_17.bin", NULL, &m_character[0x6000], 0x2000, 0 },

		{ NULL }
	};

	m_rom_list = lgp_roms;

}

// does anything special needed to send an IRQ
void lgp::do_irq(unsigned int which_irq)
{
	if (which_irq == 0)
	{
		// Redraws the screen (if needed) on interrupt
		recalc_palette();
		m_ldp_read_latch = read_ldv1000();
      write_ldv1000(m_ldp_write_latch);
      video_blit();
		Z80_ASSERT_IRQ;
	}
}

// does anything special needed to send an NMI
void lgp::do_nmi()
{
	Z80_ASSERT_NMI;
}

// reads a byte from the cpu's memory
Uint8 lgp::cpu_mem_read(Uint16 addr)
{
	
   Uint8 result = 0;
	char s[81] = { 0 };
	
	switch (cpu_getactivecpu())
	{
	case 0:
      result = m_cpumem[addr];
      // patch rom to boot faster
      if (addr == 0x3bb2)
      {
         result = 0x01;
      }
   	// main rom
   	if (addr <= 0x7fff)
   	{
   	}

   	// video ram
      else if (addr >= 0xe000 && addr <= 0xe7ff)
      {
      }

      // ldp read
      else if (addr == 0xef80)
      {
         result = m_ldp_read_latch;
      }

      // ?
      else if (addr == 0xefb8)
      {
      }

      // ?
      else if (addr == 0xefd8)
      {
         return banks[0];
      }

      // ?
      else if (addr == 0xefe0)
      {
      }

	   // scratch ram
   	else if (addr >= 0xf000)
	   {
   	}

      else
	   {
         sprintf(s, "CPU0: Unmapped read from %x (PC is %x)", addr, Z80_GET_PC);
   		printline(s);
	   }
      break;
   case 1:
      result = m_cpumem2[addr];
      // rom
      if (addr <= 0x3fff)
      {
      }
      // ram
      else if (addr >= 0x8000 && addr <= 0x83ff)
      {
      }
      // unknown
      else if (addr >= 0x8800 && addr <= 0x8803)
      {
      }
      else
      {
         sprintf(s, "CPU1: Unmapped read from %x (PC is %x)", addr, Z80_GET_PC);
   		printline(s);
	   }
      break;
   }
   return result;
}

// writes a byte to the cpu's memory
void lgp::cpu_mem_write(Uint16 addr, Uint8 value)
{
	char s[81] = { 0 };

	switch (cpu_getactivecpu())
	{
	case 0:
   	m_cpumem[addr] = value;
	   // main rom
   	if (addr <= 0x7fff)
   	{
   		sprintf(s, "Attempted write to main ROM! at %x with value %x", addr, value);
   		printline(s);
   	}

	   // video ram
      else if (addr >= 0xe000 && addr <= 0xe3ff)
      {
         m_video_overlay_needs_update = true;
      }

	   // sprite ram?
      else if (addr >= 0xe400 && addr <= 0xe7ff)
      {
         //m_video_overlay_needs_update = true;
      }

      // ldp write
      else if (addr == 0xef80)
      {
         m_ldp_write_latch = value;
      }

   	// ?
	   //else if (addr >= 0xeff8 && addr <= 0xeffb)
   	//{
   	//}

   	// ?
	   //else if (addr == 0xefd8)
   	//{
	   //}

   	// ?
	   else if (addr == 0xefa0)
	   {
	   }

      // scratch ram
	   else if (addr >= 0xf000)
	   {
	   }

      else
	   {
         m_cpumem[addr] = value;
         sprintf(s, "CPU0: Unmapped write to %x with value %x (PC is %x)", addr, value, Z80_GET_PC);
         printline(s);
	   }
      break;
   case 1:
      // ram
      if (addr >= 0x8000 && addr <= 0x83ff)
      {
      }
      // AY-3-8910 #1
      else if (addr == 0x8400)
      {
         m_soundchip1_address_latch= value;
      }
      else if (addr == 0x8401)
      {
         audio_write_ctrl_data(m_soundchip1_address_latch, value, m_soundchip1_id);
      }
      // AY-3-8910 #2
      else if (addr == 0x8402)
      {
         m_soundchip2_address_latch= value;
      }
      else if (addr == 0x8403)
      {
         audio_write_ctrl_data(m_soundchip2_address_latch, value, m_soundchip2_id);
      }
      // AY-3-8910 #3
      else if (addr == 0x8404)
      {
         m_soundchip3_address_latch= value;
      }
      else if (addr == 0x8405)
      {
         audio_write_ctrl_data(m_soundchip3_address_latch, value, m_soundchip3_id);
      }
      // AY-3-8910 #1
      else if (addr == 0x8406)
      {
         m_soundchip4_address_latch= value;
      }
      else if (addr == 0x8407)
      {
         audio_write_ctrl_data(m_soundchip4_address_latch, value, m_soundchip4_id);
      }
      // unknown
      else if (addr >= 0x8800 && addr <= 0x8803)
      {
      }
      else
      {
         m_cpumem[addr] = value;
         sprintf(s, "CPU1: Unmapped write to %x with value %x (PC is %x)", addr, value, Z80_GET_PC);
         printline(s);
	   }
      break;
   }
}

// reads a byte from the cpu's port
Uint8 lgp::port_read(Uint16 port)
{
//	char s[81] = { 0 };

	port &= 0xFF;	
	
	/*
	switch (port)
	{
	default:
//		sprintf(s, "ERROR: CPU port %x read requested, but this function is unimplemented!", port);
//		printline(s);
      break;
	}
	*/

	return(0);
}

// writes a byte to the cpu's port
void lgp::port_write(Uint16 port, Uint8 value)
{
//	char s[81] = { 0 };

	port &= 0xFF;

	/*
	switch (port)
	{
	default:
//		sprintf(s, "ERROR: CPU port %x write requested (value %x) but this function is unimplemented!", port, value);
//		printline(s);
		break;
	}
	*/
}

// updates lgp's video
void lgp::video_repaint()
{
	//This should be much faster
	SDL_FillRect(m_video_overlay[m_active_video_overlay], NULL, m_transparent_color);

   for (int chary = 0; chary < 32; chary++)
   {
      for (int charx = 0; charx < 32; charx++)
      {
         //draw_8x8(charx + 32 * chary, charx * 8, chary * 8);
         int current_char = (m_cpumem[(chary << 5) + charx + 0xe000]);

         draw_8x8(current_char, charx*8, chary*8);
      }
   }
}

void lgp::draw_8x8(int character_number, int xcoord, int ycoord)
{
	Uint8 pixel[8] = {0};
	
	for (int y = 0; y < 8; y++)
	{
		if ((y + ycoord) < LGP_OVERLAY_H)
		{
			Uint8 byte1 = m_character[character_number*8+y];
			Uint8 byte2 = m_character[character_number*8+y+0x2000];
			Uint8 byte3 = m_character[character_number*8+y+0x4000];
			Uint8 byte4 = m_character[character_number*8+y+0x6000];

			pixel[0] = static_cast<Uint8>(((byte4 & 0x80) >> 4) | ((byte3 & 0x80) >> 5) | ((byte2 & 0x80) >> 6) | ((byte1 & 0x80) >> 7));
			pixel[1] = static_cast<Uint8>(((byte4 & 0x40) >> 3) | ((byte3 & 0x40) >> 4) | ((byte2 & 0x40) >> 5) | ((byte1 & 0x40) >> 6));
			pixel[2] = static_cast<Uint8>(((byte4 & 0x20) >> 2) | ((byte3 & 0x20) >> 3) | ((byte2 & 0x20) >> 4) | ((byte1 & 0x20) >> 5));
			pixel[3] = static_cast<Uint8>(((byte4 & 0x10) >> 1) | ((byte3 & 0x10) >> 2) | ((byte2 & 0x10) >> 3) | ((byte1 & 0x10) >> 4));
			pixel[4] = static_cast<Uint8>(((byte4 & 0x08) >> 0) | ((byte3 & 0x08) >> 1) | ((byte2 & 0x08) >> 2) | ((byte1 & 0x08) >> 3));
			pixel[5] = static_cast<Uint8>(((byte4 & 0x04) << 1) | ((byte3 & 0x04) << 0) | ((byte2 & 0x04) >> 1) | ((byte1 & 0x04) >> 2));
			pixel[6] = static_cast<Uint8>(((byte4 & 0x02) << 2) | ((byte3 & 0x02) << 1) | ((byte2 & 0x02) << 0) | ((byte1 & 0x02) >> 1));
			pixel[7] = static_cast<Uint8>(((byte4 & 0x01) << 3) | ((byte3 & 0x01) << 2) | ((byte2 & 0x01) << 1) | ((byte1 & 0x01) << 0));

			for (int x = 0; x < 8; x++)
			{
				if ((pixel[x]) && ((x + xcoord) < LGP_OVERLAY_W))
				{
					*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + ((ycoord + y) * LGP_OVERLAY_W) + (xcoord + x)) = ((Uint8) pixel[x]);
				}
			}
		}
	}
}

void lgp::recalc_palette()
{
   SDL_Color temp_color;
	//Convert palette rom into a useable palette
	for (int i = 0; i < LGP_COLOR_COUNT; i++)
	{
		temp_color.r = static_cast<Uint8>((i * 23) % 256);
		temp_color.g = static_cast<Uint8>((i * 100) % 256);
		temp_color.b = static_cast<Uint8>((i * 34) % 256);
		palette_set_color(i, temp_color);
	}
   palette_finalize();

   palette_modified = false;
}

// this gets called when the user presses a key or moves the joystick
void lgp::input_enable(Uint8 move)
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
	case SWITCH_START1: // '1' on keyboard
		break;
	case SWITCH_BUTTON1: // space on keyboard
		break;
	case SWITCH_BUTTON2: // left shift
		break;
	case SWITCH_BUTTON3: // left alt
		break;
	case SWITCH_COIN1: 
		banks[0] = 0xf0;
      cpu_generate_nmi(0);
      break;
	case SWITCH_COIN2: 
		break;
	case SWITCH_SERVICE: 
		break;
	case SWITCH_TEST: 
		break;
	}
}  

// this gets called when the user releases a key or moves the joystick back to center position
void lgp::input_disable(Uint8 move)
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
	case SWITCH_START1: // '1' on keyboard
		break;
	case SWITCH_BUTTON1: // space on keyboard
		break;
	case SWITCH_BUTTON2: // left shift
		break;
	case SWITCH_BUTTON3: // left alt
		break;
	case SWITCH_COIN1: 
		banks[0] = 0x0;
		break;
	case SWITCH_COIN2: 
		break;
	case SWITCH_SERVICE: 
		break;
	case SWITCH_TEST: 
		break;
	}
}

// used to set dip switch values
bool lgp::set_bank(Uint8 which_bank, Uint8 value)
{
	bool result = true;
	
	switch (which_bank)
	{
	case 0:	// bank A
		banks[3] = (Uint8) (value ^ 0xFF);	// dip switches are active low
		break;
	case 1:	// bank B
		banks[4] = (Uint8) (value ^ 0xFF);	// switches are active low
		break;
	default:
		printline("ERROR: Bank specified is out of range!");
		result = false;
		break;
	}
	
	return result;
}

