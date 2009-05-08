/*
 * badlands.cpp
 *
 * Copyright (C) 2001-2005 Mark Broadhead
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

// badlands.cpp
// by Mark Broadhead
//

#include <string.h>
#include <math.h>
#include "badlands.h"
#include "../cpu/cpu.h"
#include "../cpu/mc6809.h"
#include "../io/conout.h"
#include "../ldp-in/ldv1000.h"
#include "../ldp-out/ldp.h"
#include "../video/palette.h"
#include "../video/video.h"
#include "../video/led.h"
#include "../sound/sound.h"

////////////////

badlands::badlands()
{
	struct cpudef cpu;

	m_shortgamename = "badlands";	
	memset(banks, 0xFF, 3);	// fill banks with 0xFF's
	m_game_type = GAME_BADLANDS;
	m_disc_fps = 29.97;

	m_video_overlay_width = BADLANDS_OVERLAY_W;
	m_video_overlay_height = BADLANDS_OVERLAY_H;
	m_palette_color_count = BADLANDS_COLOR_COUNT;

	memset(&cpu, 0, sizeof(struct cpudef));
	cpu.type = CPU_M6809;
	cpu.hz = BADLANDS_CPU_HZ / 4;
	cpu.initial_pc = 0;
	cpu.must_copy_context = false;
	cpu.irq_period[0] = (1000.0 / 59.94); // irq from strobe
	cpu.irq_period[1] = (1000.0 / 8.0 / 59.94); // firq 8 times per vblank
	cpu.nmi_period = (1000.0 / 59.94); // nmi from vblank
	cpu.mem = m_cpumem;
	add_cpu(&cpu);	// add 6809 cpu

   struct sounddef soundchip;

   soundchip.type = SOUNDCHIP_SN76496;  // Badlands uses the SN76496 sound chip
   soundchip.hz = BADLANDS_CPU_HZ / 8;   
   m_soundchip_id = add_soundchip(&soundchip);

	firq_on = false;
	irq_on = false;
	nmi_on = false;

	m_num_sounds = 1;
	m_sound_name[S_BL_SHOT] = "bl_shot.wav";
	
	shoot_led = false;
	shoot_led_overlay = false;
	shoot_led_numlock = false;
	char_base = 0x4000;
	charx_offset = 6;
	chary_offset = 2;

	// this must be static
	const static struct rom_def badlands_roms[] =
	{
		// main 6809 program
		{ "badlands.a13", NULL, &m_cpumem[0xC000], 0x2000, 0xA44776D6 },
		{ "badlands.a14", NULL, &m_cpumem[0xE000], 0x2000, 0x82CB4614 },

		// tile graphics
		{ "badlands.c8", NULL, &character[0x0000], 0x2000, 0x590209FE },
		{ "badlands.c4", NULL, &color_prom[0x0000], 0x20, 0x6757BE8D },
		
		{ NULL }
	};

	m_rom_list = badlands_roms;

}

badlandp::badlandp()
{
	m_shortgamename = "badlandp";	
	irq_on = firq_on = nmi_on = true;
	char_base = 0x2000;
	charx_offset = 12;
	chary_offset = 1;
	
	// we need to set a different type because the hack in add_digit isn't needed for this version
	m_game_type = GAME_BADLANDP;

   // this must be static
	const static struct rom_def badlandp_roms[] =
	{
		// main 6809 program
		{ "bl_hit_7a.bin", NULL, &m_cpumem[0xC000], 0x2000, 0xA135E444 },
		{ "bl_hit_6a.bin", NULL, &m_cpumem[0xE000], 0x2000, 0x4c287f37 },

		// tile graphics
		{ "bl_hit_9c.bin", NULL, &character[0x0000], 0x2000, 0x44C3441E },
		{ "bl_hit_4f.bin", NULL, &color_prom[0x0000], 0x20, 0x226f881 },
		
		{ NULL }
	};

	m_rom_list = badlandp_roms;

}

void badlands::do_irq(unsigned int which_irq)
{
	switch (which_irq)
	{
	case 0: // irq (jumps to 0xE383 on Badlands ROMs)
		if (irq_on)
		{
			mc6809_irq = 1;
		}
		break;
	case 1: // firq (jumps to 0xE7B0 on Badlands ROMs)
		if (firq_on)
		{
			mc6809_firq = 1;
		}
		break;
	default:
		printline("Invalid IRQ set in badlands.cpp!");
		break;
	}
}

void badlands::do_nmi()
{
	if (nmi_on)	// jumps to 0xE7C9 on Badlands ROMs
	{
		mc6809_nmi = 1;
	}

	video_blit();	// the NMI runs at the same period as the monitor vsync
}

Uint8 badlands::cpu_mem_read(Uint16 addr)
{
//	char s[81] = {0};

	Uint8 result = m_cpumem[addr];

	// Dipswitch 2
	if (addr == 0x0000)
	{
		result = banks[2];
	}
	// Dipswitch 1
	else if (addr == 0x0800)
	{
		result = banks[1];
	}

	// Laserdisc (in)
	else if (addr == 0x1000)
	{
		result = read_ldv1000();
	}

	// controls
	else if (addr == 0x1800)
	{
		result = banks[0];
	}


	return result;
}

void badlands::cpu_mem_write(Uint16 addr, Uint8 value)
{
	char s[81] = {0};

   // sound data
	if (addr == 0x0000 && !m_prefer_samples)
	{
		// reverse all the bits (the chip is wired up so D0 goes to D7, D1 to D6, etc...)
		value = ((value & 0x1) << 7) | ((value & 0x2) << 5) | ((value & 0x4) << 3) | ((value & 0x8) << 1) 
			| ((value & 0x10) >> 1) | ((value & 0x20) >> 3) | ((value & 0x40) >> 5) | ((value & 0x80) >> 7);
		audio_writedata(m_soundchip_id, value);
	}

   // Laserdisc (out)
	else if (addr == 0x0800)
	{
		write_ldv1000(value);
	}

	// shoot led
	else if (addr == 0x1000)
	{
		update_shoot_led(value);
	}  

	// Counter 2
	else if (addr == 0x1001)
	{
	}
	// Counter 1
	else if (addr == 0x1002)
	{
	}

	// DSP On
	else if (addr == 0x1003)
	{
		if (value)
		{
			palette_set_transparency(0, false);	// disable laserdisc video
		}
		else
		{
			palette_set_transparency(0, true);	// enable laserdisc video
		}
	}

	// nmi on
	else if (addr == 0x1004)
	{
		if (value)
		{
			nmi_on = true;
		}
		else
		{
			nmi_on = false;		
		}
	}

	// MUT
	else if (addr == 0x1005)
	{
		if (value)
		{
			// TODO:
			// Mute audio
		}
		else
		{
			// TODO:
			// Unmute audio
		}
	}

	// irq on
	else if (addr == 0x1006)
	{
		if (value)
		{
			irq_on = true;
		}
		else
		{
			irq_on = false;		
		}
	}
	// firq on
	else if (addr == 0x1007)
	{
		if (value)
		{
			firq_on = true;
		}
		else
		{
			firq_on = false;		
		}
	}

	// sound chip - I'm not sure what is the difference between 0x0000 and 0x1800, they both send the same data
	else if (addr == 0x1800)
	{
		//value = ((value & 0x1) << 7) | ((value & 0x2) << 5) | ((value & 0x4) << 3) | ((value & 0x8) << 1) | ((value & 0x10) >> 1) | ((value & 0x20) >> 3) | ((value & 0x40) >> 5) | ((value & 0x80) >> 7);
		//audio_writedata(m_soundchip_id, value);
      
      // 0xE7 seems to trigger the shot sound (once registers have been loaded)
       //sprintf(s, "Write to %x with %x", addr, value);
      //printline(s);
        if (value == 0xE7 && m_prefer_samples) 
			sound_play(S_BL_SHOT);  
	}

   // video memory is being written to, so the screen needs to be updated
	else if (addr >= 0x4000 && addr <= 0x47ff)
	{
		m_video_overlay_needs_update = true;
	}

   // RAM
	else if (addr >= 0x4800 && addr <= 0x4fff)
	{
	}

   // AFR = watchdog?
   else if (addr == 0x5800)
   {
   }

   else
   {
    	sprintf(s, "Write to %x with %x", addr, value);
      printline(s);
   }

	m_cpumem[addr] = value;
}

Uint8 badlandp::cpu_mem_read(Uint16 addr)
{
	char s[81] = {0};

	Uint8 result = m_cpumem[addr];
   
	// Laserdisc 
	if (addr == 0x0000)
	{
		result = read_ldv1000();
	}	
	// controls
	else if (addr == 0x0c00)
	{
		result = banks[0];
	}
	// dipswitch
	else if (addr == 0x1000)
	{
		result = banks[1];
	}
	// video ram
	else if (addr >= 0x2000 && addr <= 0x27ff)
	{
	}
	// scratch ram
	else if (addr >= 0x2800 && addr <= 0x2fff)
	{
	}
	// ROM
   else if (addr >= 0xc000)
   {
   }
	else 
	{
		sprintf(s, "Read from %x", addr);
      printline(s);
	}

	return result;
}

void badlandp::cpu_mem_write(Uint16 addr, Uint8 value)
{
	char s[81] = {0};
	
	// Laserdisc 
	if (addr == 0x0400)
	{
		write_ldv1000(value);
	}	
	// coin counter
	else if (addr == 0x0800)
	{
	}	
	// ?
	else if (addr == 0x0801)
	{
	}	
	// shoot led
	else if (addr == 0x0802)
	{
		update_shoot_led(value);
	}	
	// display disable
	else if (addr == 0x0803)
	{
		if (value)
		{
			palette_set_transparency(0, false);	// disable laserdisc video
		}
		else
		{
			palette_set_transparency(0, true);	// enable laserdisc video
		}
	}	
	// ?
	else if (addr == 0x0804)
	{
	}	
	// ?
	else if (addr == 0x0805)
	{
	}	
	// ?
	else if (addr == 0x0806)
	{
	}	
	// ?
	else if (addr == 0x0807)
	{
	}	
	// sound data
	else if (addr == 0x1400)
	{
		audio_writedata(m_soundchip_id, value);
	}		
	// sound data
	else if (addr == 0x1800)
	{
		// the same data sent to 0x1400 is sent here too
	}	
	// video ram
	else if (addr >= 0x2000 && addr <= 0x27ff)
	{
		m_video_overlay_needs_update = true;
	}	
	// scratch ram
	else if (addr >= 0x2800 && addr <= 0x2fff)
	{
	}
	else
	{
    	sprintf(s, "Write to %x with %x", addr, value);
      printline(s);
   }

	m_cpumem[addr] = value;
}

void badlands::palette_calculate()
{
	SDL_Color temp_color = { 0 };
	//Convert palette rom into a useable palette
	for (int i = 0; i < BADLANDS_COLOR_COUNT; i++)
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
			
		//apply gamma correction to make colors brighter overall
		//  Corrected value = 255 * (uncorrected value / 255) ^ (1.0 / gamma)
		temp_color.r = (Uint8) (255 * pow((static_cast<double>(temp_color.r)) / 255, 1/BADLANDS_GAMMA));
		temp_color.g = (Uint8) (255 * pow((static_cast<double>(temp_color.g)) / 255, 1/BADLANDS_GAMMA));
		temp_color.b = (Uint8) (255 * pow((static_cast<double>(temp_color.b)) / 255, 1/BADLANDS_GAMMA));

		palette_set_color(i, temp_color);
	}
}

// updates badlands's video
void badlands::video_repaint()
{
	for (int charx = charx_offset; charx < 40 + charx_offset; charx++)
	{
		for (int chary = chary_offset; chary < 30 + chary_offset; chary++)
		{
			for (int x=0; x < 4; x++)
			{
				for (int y = 0; y < 8; y++)
				{
					Uint8 left_pixel = static_cast<Uint8>((character[m_cpumem[chary * 64 + charx + char_base]*32+x+4*y] & 0xf0) >> 4);
					Uint8 right_pixel = static_cast<Uint8>((character[m_cpumem[chary * 64 + charx + char_base]*32+x+4*y] & 0x0f));
					*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + (((chary - chary_offset) * 8 + y) * BADLANDS_OVERLAY_W) + ((charx  - charx_offset) * 8 + x * 2)) = left_pixel;
					*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + (((chary - chary_offset) * 8 + y) * BADLANDS_OVERLAY_W) + ((charx  - charx_offset) * 8 + x * 2 + 1)) = right_pixel;
				}
			}
		}
	}

   if (shoot_led)
	{
		const char* t = "SHOOT!";
		draw_string(t, 20, 17, m_video_overlay[m_active_video_overlay]);
	}
}

// this gets called when the user presses a key or moves the joystick
void badlands::input_enable(Uint8 move)
{
	switch (move)
	{
	case SWITCH_START1: // '1' on keyboard
		banks[0] &= ~0x08; 
		break;
	case SWITCH_START2: // '2' on keyboard
		banks[0] &= ~0x10; 
		break;
	case SWITCH_BUTTON1: // space on keyboard
		banks[0] &= ~0x20; 
		break;
	case SWITCH_BUTTON2: // make this the same as start2
		banks[0] &= ~0x10;  //(so joystick users can skip the intro)
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
void badlands::input_disable(Uint8 move)
{
	switch (move)
	{
	case SWITCH_START1: // '1' on keyboard
		banks[0] |= 0x08; 
		break;
	case SWITCH_START2: // '2' on keyboard
		banks[0] |= 0x10; 
		break;
	case SWITCH_BUTTON1: // space on keyboard
		banks[0] |= 0x20; 
		break;
	case SWITCH_BUTTON2: // make this the same as start2
		banks[0] |= 0x10;  //(so joystick users can skip the intro)
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
	case SWITCH_TEST:  // test mode is entered my pressing both Start 1 and 2 during boot
		break;
	default:
		printline("Error, bug in move enable");
		break;
	}
}

// used to set dip switch values
bool badlands::set_bank(unsigned char which_bank, unsigned char value)
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

void badlands::reset()
{
	cpu_reset();
	reset_ldv1000();
}

void badlands::set_preset(int preset)
{
//we're using the -preset option to control Shoot lamp emulation
// -preset 0 = none
// -preset 1 = overlay
// -preset 2 = numlock led

	if (preset == 1)
	{
		shoot_led_overlay = true;
	}
	else if (preset == 2)
	{
		shoot_led_numlock = true;
		enable_leds(true);
	}

}

void badlands::update_shoot_led(Uint8 value)
{
	static bool ledstate = false;
	if (value)
	{
		if (!ledstate)
		{
			if (shoot_led_overlay)
			{
				shoot_led = true;
			}
			else if (shoot_led_numlock)
			{
				change_led(true, false, false);
			}
			ledstate = true;
		}
	}
	else
	{
		if (ledstate)
		{
			if (shoot_led_overlay)
			{
				shoot_led = false;
			}
			else if (shoot_led_numlock)
			{
				change_led(false, false, false);
			}
			ledstate = false;
		}
	}
}
