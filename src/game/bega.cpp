/*
* bega.cpp
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

// bega.cpp
// by Mark Broadhead
//
// Bega's Battle Hardware
// uses two 6502's (the second is for sound) that communicate through an am2950
// two ay-3-8910's for sound
// a m6850 Uart for communication with the Sony LDP-1000 laserdisc player
//
// The dedicated version of Cobra Command also runs on this hardware. It's
// much more powerful than Cobra actually needs so DECO released a conversion 
// board that only has one sprite/character generator and one ay-3-8910. The
// conversion runs either the Pioneer LD-V1000 or Pioneer PR8210 (the 8210
// version has an extra pcb on top which includes another 6502 and rom). 

#include <string.h>	// for memset
#include "bega.h"
#include "../cpu/cpu.h"
#include "../cpu/nes6502.h"
#include "../io/conout.h"
#include "../ldp-in/ldp1000.h"
#include "../ldp-out/ldp.h"
#include "../video/palette.h"
#include "../video/video.h"

////////////////

bega::bega()
{
   struct cpudef cpu;

   m_shortgamename = "bega";
   memset(&cpu, 0, sizeof(struct cpudef));
   memset(banks, 0xFF, 3);	// fill banks with 0xFF's
   // turn on diagnostics
   //	banks[2] = 0x7f;

   banks[2] = 0xb8;
   m_game_type = GAME_BEGA;
   m_disc_fps = 29.97;

   m_video_row_offset = -8;	// move overlay up by 16 pixels (8 rows)
   m_video_overlay_width = BEGA_OVERLAY_W;
   m_video_overlay_height = BEGA_OVERLAY_H;
   m_palette_color_count = BEGA_COLOR_COUNT;

   cpu.type = CPU_M6502;
   cpu.hz = BEGA_CPU_HZ / 6; // Bega's CPUs run at 2.5 MHz?
   cpu.irq_period[0] = (1000.0 / 59.94); // no periodic interrupt, we are just using this for vblank (60hz)
   cpu.nmi_period = 0; // nmi generated from coin inputs

   // MPO : seems to me that the serial port can't possibly respond any faster than what I have below
	cpu.irq_period[1] = ((1000.0)*(8+1))/9600.0;	// serial port IRQ (8 data bits, 1 stop bit, 1000 ms pre sec, 9600 bits per second

	cpu_change_interleave(2);

   cpu.initial_pc = 0;
   cpu.must_copy_context = true;	// set this to true when you add multiple 6502's
   cpu.mem = m_cpumem;
   add_cpu(&cpu);	// add 6502 cpu

   memset(&cpu, 0, sizeof(struct cpudef));
   cpu.type = CPU_M6502;
   cpu.hz = BEGA_CPU_HZ / 6; // Bega's CPUs run at 2.5 MHz?
   cpu.irq_period[0] = 0.0; 
   cpu.nmi_period = 2.0;  // this value is unknown... it controls the speed at which sound commands
                          // are sent to the sound chips. 2.0 ms seems close, but might not be exact.
   cpu.initial_pc = 0;
   cpu.must_copy_context = true;	// set this to true when you add multiple 6502's
   cpu.mem = m_cpumem2;
   add_cpu(&cpu);	// add sound 6502 cpu

   struct sounddef soundchip;
   soundchip.type = SOUNDCHIP_AY_3_8910;
   soundchip.hz = BEGA_CPU_HZ / 10; // Bega runs the sound chips at 1.5 MHz
   m_soundchip1_id = add_soundchip(&soundchip);
   m_soundchip2_id = add_soundchip(&soundchip);

   // make chip 1 only play in right speaker
   set_soundchip_volume(m_soundchip1_id, 1, AUDIO_MAX_VOLUME);
   set_soundchip_volume(m_soundchip1_id, 0, 0);

   // make chip 2 only play in left speaker
   set_soundchip_volume(m_soundchip2_id, 0, AUDIO_MAX_VOLUME);
   set_soundchip_volume(m_soundchip2_id, 1, 0);

   ldp_status = 0x00;

   // default ROM version (ver. 3)
   const static struct rom_def bega_roms[] =
   {
      // main cpu roms
      { "an05-3", NULL, &m_cpumem[0x4000], 0x2000, 0xc917a283 },
      { "an04-3", NULL, &m_cpumem[0x6000], 0x2000, 0x935b2b0a },
      { "an03-3", NULL, &m_cpumem[0x8000], 0x2000, 0x79438d80 },
      { "an02-3", NULL, &m_cpumem[0xa000], 0x2000, 0x98ce4ca0 },
      { "an01",   NULL, &m_cpumem[0xc000], 0x2000, 0x15f8921d },
      { "an00-3", NULL, &m_cpumem[0xe000], 0x2000, 0x124a3a36 },

      // sound cpu roms
      { "an06", NULL, &m_cpumem2[0xe000], 0x2000, 0xcbbcd730 },

      // roms for the first character/sprite generator
      { "an0c", NULL, &character1[0x0000], 0x2000, 0x0c127207 },
      { "an0b", NULL, &character1[0x2000], 0x2000, 0x09e4b780 },
      { "an0a", NULL, &character1[0x4000], 0x2000, 0xe429305d },

      // roms for the second character/sprite generator
      { "an09", NULL, &character2[0x0000], 0x2000, 0xb7375fd7 },
      { "an08", NULL, &character2[0x2000], 0x2000, 0xb5518391 },
      { "an07", NULL, &character2[0x4000], 0x2000, 0x6b8ad735 },
      { NULL }
   };
   m_rom_list = bega_roms;

}


// Bega's supports multiple rom revs
void bega::set_version(int version)
{
   if (version == 1) //rev 3
   {
      //already loaded, do nothing
   }
   else if (version == 2)  //rev 1
   {
	   m_shortgamename = "begar1";

      const static struct rom_def bega_roms[] =
      {
         // main cpu roms
         { "an05", NULL, &m_cpumem[0x4000], 0x2000, 0x91a05549 },
         { "an04", NULL, &m_cpumem[0x6000], 0x2000, 0x670966fe },
         { "an03", NULL, &m_cpumem[0x8000], 0x2000, 0xd2d85cdf },
         { "an02", NULL, &m_cpumem[0xa000], 0x2000, 0x84d13c20 },
         { "an01", "bega", &m_cpumem[0xc000], 0x2000, 0x15f8921d },
         { "an00", NULL, &m_cpumem[0xe000], 0x2000, 0x184297f3 },

         // sound cpu roms
         { "an06", "bega", &m_cpumem2[0xe000], 0x2000, 0xcbbcd730 },

         // roms for the first character/sprite generator
         { "an0c", "bega", &character1[0x0000], 0x2000, 0x0c127207 },
         { "an0b", "bega", &character1[0x2000], 0x2000, 0x09e4b780 },
         { "an0a", "bega", &character1[0x4000], 0x2000, 0xe429305d },

         // roms for the second character/sprite generator
         { "an09", "bega", &character2[0x0000], 0x2000, 0xb7375fd7 },
         { "an08", "bega", &character2[0x2000], 0x2000, 0xb5518391 },
         { "an07", "bega", &character2[0x4000], 0x2000, 0x6b8ad735 },
         { NULL }
      };
      m_rom_list = bega_roms;
   }
   else
   {
      printline("BEGA:  Unsupported -version paramter, ignoring...");
   }
}	




cobra::cobra()  //dedicated version of Cobra Command
{
   m_shortgamename = "cobra";
   m_game_issues = "Game does not wook properly (graphics ploblems)";

   const static struct rom_def cobra_roms[] =
   {
      // main cpu roms
      { "au03-2", NULL, &m_cpumem[0x8000], 0x2000, 0x8f0a8fba },
      { "au02-2", NULL, &m_cpumem[0xa000], 0x2000, 0x7db11acf },
      { "au01-2", NULL, &m_cpumem[0xc000], 0x2000, 0x523dd8f6 },
      { "au00-2", NULL, &m_cpumem[0xe000], 0x2000, 0x6c0f1f16 },

      // sound cpu roms
      { "au06", NULL, &m_cpumem2[0xe000], 0x2000, 0xccc94eb0 },

      // roms for the first character/sprite generator
      { "au0c", NULL, &character1[0x0000], 0x2000, 0xd00a2762 },
      { "au0b", NULL, &character1[0x2000], 0x2000, 0x92247877 },
      { "au0a", NULL, &character1[0x4000], 0x2000, 0x6aaedcf3 },

      // roms for the second character/sprite generator
      { "au09", NULL, &character2[0x0000], 0x2000, 0x74e93394 },
      { "au08", NULL, &character2[0x2000], 0x2000, 0x63158274 },
      { "au07", NULL, &character2[0x4000], 0x2000, 0xd4bf12a5 },
      { NULL }
   };
   m_rom_list = cobra_roms;


}

//we need to override bega::set_version so we don't load the wrong roms
void cobra::set_version(int version)
{
   printline("COBRA:  Unsupported -version paramter, ignoring...");
}

roadblaster::roadblaster()  //dedicated version of Cobra Command
{
   m_shortgamename = "roadblaster";

   const static struct rom_def rb_roms[] =
   {
      // main cpu roms
      { "01.bin", NULL, &m_cpumem[0xc000], 0x2000, 0xe4733c49 },
      { "00.bin", NULL, &m_cpumem[0xe000], 0x2000, 0x084d6ae2 },

      // sound cpu roms
      { "02.bin", NULL, &m_cpumem2[0xe000], 0x2000, 0x6c20335d },

		// NOTE : the roms loaded in these two following sections are the same

      // roms for the first character/sprite generator
      { "05-08.bin", NULL, &character1[0x0000], 0x2000, 0x4608b516 },
      { "04-07.bin", NULL, &character1[0x2000], 0x2000, 0xda2c84d9 },
      { "03-06.bin", NULL, &character1[0x4000], 0x2000, 0xd1ff5ffb },

      // roms for the second character/sprite generator
      { "05-08.bin", NULL, &character2[0x0000], 0x2000, 0x4608b516 },
      { "04-07.bin", NULL, &character2[0x2000], 0x2000, 0xda2c84d9 },
      { "03-06.bin", NULL, &character2[0x4000], 0x2000, 0xd1ff5ffb },
      { NULL }
   };
   m_rom_list = rb_roms;

   /*
   Road Blaster dip switches, by Mark Broadhead and Brad Oldham
   (these are active low)

Dipswitch 1 
xx00	1 Coin/1 Credit
xx01	3 Coins/1 Credit
xx10	1 Coin/2 Credits
xx11	2 Coins/1 Credit
	
	bits 2 and 3 are unknown

Dipswitch 2 
0xxx xxxx	Test mode off 
1xxx xxxx	Test mode on (Plays through the game without any input)

x0xx xxxx	Random scenes off
x1xx xxxx	Random scenes on

xx0x xxxx	Attract sound on
xx1x xxxx	Attract sound off

xxx0 xxxx	Difficulty Normal
xxx1 xxxx	Difficulty Hard

xxxx x00x	Bonus car at 20000 points
xxxx x01x	Bonus car at 20000 points, and then every 40000 
xxxx x10x	Bonus car at 15000 points, and then every 30000 
xxxx x11x	Bonus car at 30000 points, and then every 50000 

xxxx xxx0	2 cars
xxxx xxx1	4 cars

	bit 3 is unknown
   */
}

// another fine cheat by Pinco69
void roadblaster::patch_roms()
{
	if (m_cheat_requested)
	{
		// infinite lives cheat
		m_cpumem[0xC41c] = 0x00; // Dec "0" from total lives!
		printline("RoadBlaster infinite lives cheat enabled!");
	}
} 

// used to set dip switch values
bool bega::set_bank(unsigned char which_bank, unsigned char value)
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



// clocks screen updates and vblank timing
void bega::do_irq(unsigned int which_irq)
{
   if (cpu_getactivecpu() == 0)
   {
      if (which_irq == 0)
      {
         video_blit();
         vblank = true;
      }
      else
      {
         // periodicly check if there is any laserdisc data to read
         if (ldp1000_result_ready())
         {
            // logerror("found ldp data! issuing interrupt\n");
            // if we get new data set the interrupt bit (7), the recieve data full bit (0),
            // the transmit data register empty bit (1), and do an irq
            mc6850_status |= 0x83;
            nes6502_irq();
         }
         else
         {
            if (!(mc6850_status & 0x01))
            {
               // no new data and no data waiting so we set the transmit data register empty bit (1)
               mc6850_status |= 0x02;
            }
         }
      }
   }
   else 
   {
      nes6502_irq();
   }
}

void bega::do_nmi()
{
   nes6502_nmi();
}

Uint8 bega::cpu_mem_read(Uint16 addr)
{
//   char s[81] = {0};
   Uint8 result;

   if (cpu_getactivecpu() == 0)
   {
      result = m_cpumem[addr];

      // main ram
      if (addr <= 0x0fff)
      {
      }  

      // control panel
      else if (addr == 0x1000)
      {
         return banks[0];
      }

      // dipswitch 1
      else if (addr == 0x1001)
      {
         return banks[1];
      }

      // dipswitch 2
      else if (addr == 0x1002)
      {
         return banks[2];
      }

      // bit 7 is vblank, bit 6 is some other blank
      else if (addr == 0x1003)
      {
		  // If vblank is enabled, the high bit should be set, according to the Bega's Battle ROM at 564A.
		  // At 564A is waits for vblank in order to time how fast the 'continue game' countdown goes.
         if (vblank)
         {
            result = 0x80;
            vblank = false;
         }
         else
         {
            result = 0x00;
         }
      }

      // m6850 status port
      else if (addr == 0x1006)
      {
         result = read_m6850_status();
      }

      // m6850 data port
      else if (addr == 0x1007)
      {
         result = read_m6850_data();
      }

      // color ram
      else if (addr >= 0x1800 && addr <= 0x1837)
      {
      }

      // video ram
      else if (addr >= 0x2000 && addr <= 0x3fff)
      {
      }

      // main rom (0x4000 - 0xFFFF)
      else if (addr >= 0x4000)
      {
      }

      else
      {
//         sprintf(s, "CPU: 0  - Unmapped read from %x", addr);
//         printline(s);
      }
   }

   // sound cpu
   else
   {
      result = m_cpumem2[addr];

      // 2048 bytes of scratch ram
      if (addr <= 0x07ff)
      {
      }

      else if (addr == 0xa000)
      {
         result = m_sounddata_latch;
      }

      // main rom (0xe000 - 0xffff)
      else if (addr >= 0xe000)
      {
      }

      else
      {
         //sprintf(s, "CPU: 1  - Unmapped read from %x", addr);
         //printline(s);
      }
   }

   return result;
}

void bega::cpu_mem_write(Uint16 addr, Uint8 value)
{
   char s[81] = {0};

   if (cpu_getactivecpu() == 0)
   {
      // main ram (0 - 0x0FFF)
      if (addr <= 0x0fff)
      {
      }

      // coin blockers
      else if (addr == 0x1000)
      {
      }

      // sound data
      else if (addr == 0x1004)
      {
         m_sounddata_latch = value;
         cpu_generate_irq(1, 0); // generate an interrupt on the sound cpu 
      }

      // m6850 control port
      else if (addr == 0x1006)
      {
         write_m6850_control(value);
      }

      // m6850 data port
      else if (addr == 0x1007)
      {
         write_m6850_data(value);
      }

	  /*
	  Color Notes from IRC chat on 27 Sep 2007:
	<BuzzQ> Red:
	<BuzzQ> 3/2 - 4.7k
	<BuzzQ> 4/2 - 2.978k
	<BuzzQ> 5/2 - 1.495k
	<BuzzQ> Green:
	<BuzzQ> 7/6 - 4.7k
	<BuzzQ> 8/6 - 2.981k
	<BuzzQ> 9/6 - 1.495k
	<BuzzQ> Blue:
	<BuzzQ> 11/10 - 2.988k
	<BuzzQ> 12/10 - 1.494k
	<Mark_B> ok, these are the values I came up with... 8c 46 2d
	<Mark_B> to find them it's (resistance for the bit / total parallel
          resistance) * 0xff
	<Mark_B> that's a little bit different from the 97 47 21 that is the Daphne code
	  */

      // color ram
      else if (addr >= 0x1800 && addr <= 0x1837)
      {
         SDL_Color temp_color;

         value = ~value;

         int bit0,bit1,bit2;
         /* red component */
         bit0 = (value >> 0) & 0x01;
         bit1 = (value >> 1) & 0x01;
         bit2 = (value >> 2) & 0x01;
         temp_color.r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
         /* green component */
         bit0 = (value >> 3) & 0x01;
         bit1 = (value >> 4) & 0x01;
         bit2 = (value >> 5) & 0x01;
         temp_color.g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
         /* blue component */
         bit0 = 0;
         bit1 = (value >> 6) & 0x01;
         bit2 = (value >> 7) & 0x01;
         temp_color.b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;	

         palette_set_color(addr & 0xff, temp_color);
         palette_finalize();	// we need to do this here because it takes place outside palette_calculate

         m_video_overlay_needs_update = true;
      }

      // video ram
      else if (addr >= 0x2000 && addr <= 0x3fff)
      {
         m_video_overlay_needs_update = true;
      }

      // main rom (0x4000 - 0xFFFF)
      else if (addr >= 0x4000)
      {
         sprintf(s, "Error! write to main rom at %x", addr);
         printline(s);
      }

      else
      {
//         m_cpumem[addr] = value;	// MPO : seems redundant
//         sprintf(s, "CPU 0 - Unmapped write to %x with value %x", addr, value);
//         printline(s);
      }

      m_cpumem[addr] = value;
   }

   // sound cpu
   else
   {
      // 2048 bytes of scratch ram
      if (addr <= 0x07ff)
      {
      }
      else if (addr == 0x2000)
      {
         audio_write_ctrl_data(m_soundchip1_address_latch, value, m_soundchip1_id);
      }
      else if (addr == 0x4000)
      {
         m_soundchip1_address_latch = value;
      }
      else if (addr == 0x6000)
      {
         audio_write_ctrl_data(m_soundchip2_address_latch, value, m_soundchip2_id);
      }
      else if (addr == 0x8000)
      {
         m_soundchip2_address_latch = value;
      }
      // generate NMI?
      else if (addr == 0xa000)
      {
      }
      // main rom (0xE000 - 0xFFFF)
      else if (addr >= 0xe000)
      {
         sprintf(s, "Error! write to main rom at %x", addr);
         printline(s);
      }
      else
      {
         m_cpumem2[addr] = value;
         //sprintf(s, "CPU 1 - Unmapped write to %x with value %x", addr, value);
         //printline(s);
      }

      m_cpumem2[addr] = value;
   }
}

void bega::palette_calculate()
{
	// the color palette for begas is set by the ROM when memory is written, so we can't set it statically

	// setup transparency stuff
	palette_set_transparency(0, false);	// change default color 0 to non-transparent
	palette_set_transparency(BEGA_TRANSPARENT_COLOR, true);	
}

// updates bega's video
void bega::video_repaint()
{	
   //This is much faster!
   SDL_FillRect(m_video_overlay[m_active_video_overlay], NULL, BEGA_TRANSPARENT_COLOR); // note:  using transparent color

   // now the sprites
   draw_sprites(0x3800, character1);
   draw_sprites(0x3be0, character1);
   draw_sprites(0x2800, character2);
   draw_sprites(0x2be0, character2);

   // draw tiles first
   for (int charx = 0; charx < 32; charx++)
   {
      // don't draw the first or last lines of tiles (this is where the sprite data is)
      for (int chary = 1; chary < 31; chary++)
      {
         int current_character;

         // draw 8x8 tiles from tile/sprite generator 2
         current_character = m_cpumem[chary * 32 + charx + 0x2800] + 256 * (m_cpumem[chary * 32 + charx + 0x2c00] & 0x03);
         draw_8x8(current_character, 
            character2, 
            charx*8, chary*8, 
            0, 0, 
            6); // this isn't the correct color... i'm not sure where color comes from right now

         // draw 8x8 tiles from tile/sprite generator 1
         current_character = m_cpumem[chary * 32 + charx + 0x3800] + 256 * (m_cpumem[chary * 32 + charx + 0x3c00] & 0x03);
         draw_8x8(current_character, 
            character1, 
            charx*8, chary*8, 
            0, 0, 
            6); // this isn't the correct color... i'm not sure where color comes from right now

         // draw 8x8 tiles from tile/sprite generator 1
         //current_character = m_cpumem[chary * 32 + charx + 0x2000] + 256 * 2;
         //draw_8x8(current_character, 
         //   character2, 
         //   chary*8, charx*8, 
         //   0, 0, 
         //   6); // this isn't the correct color... i'm not sure where color comes from right now

         // draw 8x8 tiles from tile/sprite generator 1
         //current_character = m_cpumem[chary * 32 + charx + 0x3000] + 256 * (m_cpumem[chary * 32 + charx + 0x3c00] & 0x03);
         //draw_8x8(current_character, 
         //  character1, 
         //   chary*8, charx*8, 
         //   0, 0, 
         //   6); // this isn't the correct color... i'm not sure where color comes from right now			

      }
   }
}


// this gets called when the user presses a key or moves the joystick
void bega::input_enable(Uint8 move)
{
   switch (move)
   {
   case SWITCH_UP:
      banks[0] &= ~0x02; 
      break;
   case SWITCH_LEFT:
      banks[0] &= ~0x04; 
      break;
   case SWITCH_RIGHT:
      banks[0] &= ~0x08; 
      break;
   case SWITCH_DOWN:
      banks[0] &= ~0x01; 
      break;
   case SWITCH_START1: // '1' on keyboard
      banks[1] &= ~0x20; 
      break;
   case SWITCH_START2: // '2' on keyboard
      banks[1] &= ~0x10; 
      break;
   case SWITCH_BUTTON1: 
      banks[0] &= ~0x40; 
      break;
   case SWITCH_BUTTON2: 
      banks[0] &= ~0x20; 
      break;
   case SWITCH_BUTTON3: 
      banks[0] &= ~0x10; 
      break;
   case SWITCH_COIN1: 
      banks[1] &= ~0x40; 
      cpu_generate_nmi(0);
      break;
   case SWITCH_COIN2: 
      banks[1] &= ~0x80; 
      cpu_generate_nmi(0);
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
void bega::input_disable(Uint8 move)
{
   switch (move)
   {
   case SWITCH_UP:
      banks[0] |= 0x02; 
      break;
   case SWITCH_LEFT:
      banks[0] |= 0x04; 
      break;
   case SWITCH_RIGHT:
      banks[0] |= 0x08; 
      break;
   case SWITCH_DOWN:
      banks[0] |= 0x01; 
      break;
   case SWITCH_START1: // '1' on keyboard
      banks[1] |= 0x20; 
      break;
   case SWITCH_START2: // '2' on keyboard
      banks[1] |= 0x10; 
      break;
   case SWITCH_BUTTON1: 
      banks[0] |= 0x40; 
      break;
   case SWITCH_BUTTON2: 
      banks[0] |= 0x20; 
      break;
   case SWITCH_BUTTON3: 
      banks[0] |= 0x10; 
      break;
   case SWITCH_COIN1: 
      banks[1] |= 0x40; 
      break;
   case SWITCH_COIN2: 
      banks[1] |= 0x80; 
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

void bega::draw_8x8(int character_number, Uint8 *character_set, int xcoord, int ycoord,
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
            *((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + ((ycoord + (yflip ? y : (7-y))) * BEGA_OVERLAY_W) + (xcoord + (xflip ? (7-x) : x))) = pixel[x] + (8*color);
         }
      }
   }
}

void bega::draw_16x16(int character_number, Uint8 *character_set, int xcoord, int ycoord,
                      int xflip, int yflip, int color)
{
   Uint8 pixel[16] = {0};

   for (int y = 0; y < 16; y++)
   {
      Uint8 byte1 = character_set[character_number*32+y];
      Uint8 byte2 = character_set[character_number*32+y+0x2000];
      Uint8 byte3 = character_set[character_number*32+y+0x4000];
      Uint8 byte4 = character_set[character_number*32+y+16];
      Uint8 byte5 = character_set[character_number*32+y+0x2000+16];
      Uint8 byte6 = character_set[character_number*32+y+0x4000+16];

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
            *((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + ((ycoord + (yflip ? y : (15-y))) * BEGA_OVERLAY_W) + (xcoord + (xflip ? (15-x) : x))) = pixel[x] + (8*color);
         }

      }
   }
}

void bega::draw_sprites(int offset, Uint8 *character_set)
{
   for (int sprites = 0; sprites < 0x32; sprites += 4)
   {
      // check to make sure the sprite fits in the boundry of our overlay
      if ((m_cpumem[offset + sprites] & 0x01) && (m_cpumem[offset + sprites + 3] < 240) && (m_cpumem[offset + sprites + 2] >= 8) && (m_cpumem[offset + sprites + 2] < 232))
      {		
         draw_16x16(m_cpumem[offset + sprites + 1],
            character_set,
            m_cpumem[offset + sprites + 3], 
            m_cpumem[offset + sprites + 2], 
            m_cpumem[offset + sprites] & 0x04, 
            m_cpumem[offset + sprites] & 0x02,
            6); // this isn't the correct color... i'm not sure where color comes from right now
      }
   }
}

Uint8 bega::read_m6850_status()
{
   return mc6850_status;
}

void bega::write_m6850_control(Uint8 data)
{
   if ((data & 0x03) == 0x03)
   {
      printline("MC6850: Master Reset!");
      mc6850_status = 0x02;
   }
   else
   {
      switch (data & 0x03)
      {
      case 0x00:
         printline("MC6850: clock set to x1");
         break;
      case 0x01:
         printline("MC6850: clock set to x16");
         break;
      case 0x02:
         printline("MC6850: clock set to x32");
         break;
      }

      switch (data & 0x1c)
      {
      case 0x00:
         printline("MC6850: 7 Bits+Even Parity+2 Stop Bits");
         break;
      case 0x04:
         printline("MC6850: 7 Bits+Odd Parity+2 Stop Bits");
         break;
      case 0x08:
         printline("MC6850: 7 Bits+Even Parity+1 Stop Bits");
         break;
      case 0x0c:
         printline("MC6850: 7 Bits+Odd Parity+1 Stop Bits");
         break;
      case 0x10:
         printline("MC6850: 8 Bits+2 Stop Bits");
         break;
      case 0x14:
         printline("MC6850: 8 Bits+1 Stop Bits");
         break;
      case 0x18:
         printline("MC6850: 8 Bits+Even Parity+1 Stop Bits");
         break;
      case 0x1c:
         printline("MC6850: 8 Bits+Odd Parity+1 Stop Bits");
         break;
      }

      switch (data & 0x60)
      {
      case 0x00:
         printline("MC6850: /RTS=low, Transmitting Interrupt Disabled");
         break;
      case 0x20:
         printline("MC6850: /RTS=low, Transmitting Interrupt Enabled");
         break;
      case 0x40:
         printline("MC6850: /RTS=high, Transmitting Interrupt Disabled");
         break;
      case 0x60:
         printline("MC6850: /RTS=low, Transmits break level on Transmit Data Output, Transmitting Interrupt Disabled");
         break;
      }

      if (data & 0x80)
      {
         printline("MC6850: Recieve Interrupt Enabled");
      }
      else 
      {
         printline("MC6850: Recieve Interrupt Disabled");
      }
   }	
}

Uint8 bega::read_m6850_data()
{
   // if there is new data get it from the ldp, otherwise return the old data
   if (mc6850_status & 0x01)
   {
      ldp_status = read_ldp1000();
   }

   // reading from the data register clears the interrupt bit (7) and the recieve data register full bit (0)
   mc6850_status &= 0x7e;
   return ldp_status;
}

void bega::write_m6850_data(Uint8 data)
{
   //	char s[81];
   //	sprintf(s, "Sending LDP %x", data);
   //	printline(s);

   // writing to the data register clears the interrupt bit (7) and the transmit data register empty bit (1)
   mc6850_status &= 0x7d;
   write_ldp1000(data);
}
