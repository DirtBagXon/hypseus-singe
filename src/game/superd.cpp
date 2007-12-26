/*
 * superd.cpp
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

// superd.cpp
// by Mark Broadhead and Matt Ownby
//

// HARDWARE NOTES:
// The interrupt seems to be triggered by the LD-V1000 command strobe
// The interrupt seems to stay 'raised' until port 8 output is received
// Upon receiving LD-V1000 status strobe, the LD-V1000 status is immediately stored into
//  a latch which can then be read at any time by reading z80 port 4.
// Upon writing to z80 port 0, a byte is put into a latch which is always sent to
//  the LD-V1000 on the next command strobe.

#include <stdio.h>
#include <string.h>
#include "superd.h"
#include "../cpu/generic_z80.h"
#include "../ldp-in/ldv1000.h"
#include "../ldp-out/ldp.h"
#include "../io/conout.h"
#include "../sound/sound.h"
#include "../timer/timer.h"
#include "../video/palette.h"
#include "../video/video.h"

#define SUPERDON_TRANSPARENT_COLOR 15

/////////////////////////////////////////////////////////////////////

#ifdef CPU_DEBUG

// super don function/variable names for debugging
struct addr_name superd_addr_names[] =
{
	{ "Start", 0x0000 },
	{ "SdqStart", 0x0210 },
	{ "MainGameLoop", 0x02A0 },
	{ "CheckExpectedJoystick", 0xF1E },
	{ "CheckExpectedButtons", 0xF32 },
	{ "CheckUnexpectedJoystick", 0xF76 },
	{ "EnableSkipFrames", 0x1083 },
	{ "MakeFrameNum", 0x112E },
	{ "LDV1000izeDigit", 0x115A },
	{ "HandleBadMove", 0x11DD },
	{ "LDV1000_Reject", 0x1426 },
	{ "LDV1000_Play2", 0x142B },
	{ "LDV1000_Play1", 0x1435 },
	{ "LDV1000_1X", 0x143F },
	{ "LDV1000_Stop", 0x1444 },
	{ "LDV1000_SetDisp", 0x1448 },
	{ "LDV1000_DispEnable", 0x1459 },
	{ "LDV1000_Send1AndDisp", 0x145E },
	{ "Send_to_LDV1000", 0x14AE },
	{ "Send2_to_LDV1000", 0x14DB },
	{ "MakeDelay", 0x1512 },
	{ "LDV1000_Search", 0x151B },
	{ "GetFrameDigit", 0x151F },
	{ "LDV1000_Clear", 0x1566 },
	{ "LDV1000_SkipFrames", 0x157D },
	{ "GetFrameNumber", 0x15AB },
	{ "Upate_P1_Score", 0x1698 },
	{ "Update_Score", 0x16A8 },
	{ "(DE)=(DE)+(HL)", 0x1D6B },
	{ "ISR2_Index", 0x4000 },
	{ "Stack0Ptr", 0x4002 },
	{ "Stack1Ptr", 0x4004 },
	{ "Stack2Ptr", 0x4006 },
	{ "Stack3Ptr", 0x4008 },
	{ "Stack4Ptr", 0x400A },
	{ "ExpectedFrame", 0x4050 },
	{ "NeedSearch?", 0x40D1 },
	{ "SkipFrames?", 0x40D2 },
	{ "FrameCalcPtr", 0x40E2 },
	{ "Joystick0", 0x40F0 },	// here is where info from input port 0 (joystick) get stored
	{ "Joystick1", 0x40F1 },
	{ "JoyChanged?1", 0x40F2 },
	{ "JoyChanged?2", 0x40F3 },
	{ "ButtonsCoin0", 0x40F4 }, // 4-8
	{ "Dip1_0", 0x40F8 },		// 8-B
	{ "Dip2_0", 0x40FC },		// C-F
	{ "Move_Accepted?", 0x4086 },
	{ "LDV1000Frame", 0x4100 },
	{ "ApproxCurFrame", 0x4110 },
	{ "Old2Frame", 0x4120 },
	{ "FrameDifference", 0x4130 },
	{ "DeathFrame", 0x4140 },
	{ "Frame2", 0x4120 },
	{ "FrameSubtract", 0x4130 },
	{ "P1Score0", 0x4190 },
	{ "P1Score1", 0x4191 },
	{ "P1Score2", 0x4192 },
	{ "P2Score0", 0x4198 },
	{ "P2Score1", 0x4199 },
	{ "P2Score2", 0x419A },
	{ NULL, 0 }
};

#endif


/////////////////////////////////////////////////////////////////////

// we do not want to clear the IRQ when it gets enabled
int superd_irq_callback(int irqline)
{
	// get rid of warnings
	if (irqline)
	{
	}
	// notice we do NOT clear the irq line here
	return 0xff;
}


// superdon constructor
superd::superd()
{
	struct cpudef cpu;
	
	m_shortgamename = "sdq";
	memset(&cpu, 0, sizeof(struct cpudef));
	memset(banks, 0xFF, 4);	// fill banks with 0xFF's
	m_video_overlay[m_active_video_overlay] = NULL;

	m_disc_fps = 29.97;
	m_game_type = GAME_SUPERD;

	m_video_overlay_width = SUPERD_OVERLAY_W;
	m_video_overlay_height = SUPERD_OVERLAY_H;
	m_palette_color_count = SUPERD_COLOR_COUNT;
	
	cpu.type = CPU_Z80;
	cpu.hz = SUPERD_CPU_HZ;
	cpu.irq_period[0] = SUPERD_IRQ_PERIOD;	// the IRQ is connected to the LD-V1000 command strobe
	cpu.initial_pc = 0;
	cpu.must_copy_context = false;
	cpu.nmi_period = 0.0;
	cpu.mem = m_cpumem;
	add_cpu(&cpu);	// add this cpu to the list

   struct sounddef soundchip;

   soundchip.type = SOUNDCHIP_SN76496;  // Super Don uses the SN76496 sound chip
   soundchip.hz = SUPERD_CPU_HZ / 2;   // the SN76496 uses half the rate that the cpu uses on Super Don
   m_soundchip_id = add_soundchip(&soundchip);

#ifdef CPU_DEBUG
	addr_names = superd_addr_names;
#endif
	ldp_output_latch = 0xff;

#ifdef USE_M80
	m80_set_irq_callback(superd_irq_callback);
#else
	z80_set_irq_callback(superd_irq_callback);
#endif

	ldv1000_enable_instant_seeking();	// enable instantaneous seeks because we can

	// no more issues! :)
//	m_game_issues = "Death scenes are visible between each correct move";

	m_num_sounds = 5;
	m_sound_name[S_SD_COIN] = "sd_coin.wav";
	m_sound_name[S_SD_SUCCEED] = "sd_succeed.wav";
	m_sound_name[S_SD_FAIL] = "sd_fail.wav";
	m_sound_name[S_SDA_SUCCESS_LO] = "sda_success_lo.wav";
	m_sound_name[S_SDA_SUCCESS_HI] = "sda_success_hi.wav";

	// NOTE : this must be static
	static struct rom_def roms[] =
	{
		// main z80 program
		{ "sdq-prog.bin", NULL, &m_cpumem[0x0000], 0x4000, 0x96B931E2 },
		
		// character tiles
		{ "sdq-char.bin", NULL, &character[0x0000], 0x2000, 0x5FB0E440 },

		// color prom		
		{ "sdq-cprm.bin", NULL, &color_prom[0x0000], 0x20, 0x96701569 },
		{ NULL }
	};
	m_rom_list = roms;
   
}

// superdon (short scenes) constructor
sdqshort::sdqshort()
{
	m_shortgamename = "sdqshort";

	// NOTE : this must be static
	static struct rom_def roms[] =
	{
		// main z80 program
		{ "sdq_c45.rom", NULL, &m_cpumem[0x0000], 0x4000, 0x0F4D4832 },
		
		// character tiles
		{ "sdq-char.bin", "sdq", &character[0x0000], 0x2000, 0x5FB0E440 },

		// color prom
		{ "sdq-cprm.bin", "sdq", &color_prom[0x0000], 0x20, 0x96701569 },
		{ NULL }
	};

	m_rom_list = roms;
}

// superdon (short scenes alternate) constructor
sdqshortalt::sdqshortalt()
{
	m_shortgamename = "sdqshortalt";

	// NOTE : this must be static
	static struct rom_def roms[] =
	{
		// main z80 program
		{ "sdq_c45alt.rom", NULL, &m_cpumem[0x0000], 0x4000, 0xB12CE1F8 },
		
		// character tiles
		{ "sdq-char.bin", "sdq", &character[0x0000], 0x2000, 0x5FB0E440 },

		// color prom
		{ "sdq-cprm.bin", "sdq", &color_prom[0x0000], 0x20, 0x96701569 },
		{ NULL }
	};

	m_rom_list = roms;
}

bool superd::init()
{
	cpu_init();
//	g_ldp->set_skip_blanking(true);	// set blanking for skips
	// this is a hack since the real arcade controls this blanking with the video overlay
	// hardware (apparently) but at this time, I can't find any other way to blank the
	// screen during skips.
	// NOTE : some users may not like the blanking so we might want to allow them to turn
	// it off later.  But since the real machine does do it, it's on by default.
	
	return true;
}

void superd::do_irq(unsigned int which_irq)
{
	if (which_irq == 0)
	{
		// NOTE: IRQ stuff moved to OnVblank routine
		/*
		// Redraws the screen (if needed) on interrupt
		video_blit();
		ldp_input_latch = read_ldv1000();
		write_ldv1000(ldp_output_latch);
		Z80_ASSERT_IRQ;
		*/
	}
	else
	{
		printline("ERROR : Illegal IRQ for super don");
	}
}

void superd::cpu_mem_write(Uint16 Addr, Uint8 Value)
// Called whenever the Z80 emulator wants to write to memory
{	
	m_cpumem[Addr] = Value;

	// if the cpu writes to video memory, update the screen on the next interrupt
	if (Addr >= 0x5c00 && Addr <= 0x5fff)
	{
		m_video_overlay_needs_update = true;
	}

}

void superd::port_write(Uint16 Port, Uint8 Value)
// Called whenever the emulator wants to output to a port
{

    char s[81] = { 0 };
	static int snd_succeed_count = 0;	// a cheat to make sure we only play the succeed sample once
	static int snd_coin_count = 0;	// a cheat to make sure we only play the coin sample once

    Port &= 0xFF;	// strip off high byte

    switch(Port)
	{
	
	case 0x00:	//	Send command to LVD-1000
		ldp_output_latch = Value;
		break;


   // super don's sound chip is the sn76496
   case 0x04: 
      if (!m_prefer_samples)
      {
         // Send data to sound chip
         audio_writedata(m_soundchip_id, Value);
      }

      else
      {
         // The sound chip sends a bunch of data to port 4 to control the sound chip.
		   // All we do here is look for unique data to figure out which sample we should play.
		   // It turns out that only 8's are sent for the coin sound, only 11's are sent for the
		   // successful move sound, and only 0xC1 is send for the bad move sound.
		   // Also, about eight 0x11's are sent for the successful move, so we make sure we only
		   // play a sound for 1 out of every eight 0x11's we get.  Same type of deal for the coin
		   // sound.
         if (Value == 0x08)
		   {
   			if (snd_coin_count == 0)
	   		{
		   		sound_play(S_SD_COIN);
			   }

	   		snd_coin_count++;
   			if (snd_coin_count >= 2)
		   	{
			   	snd_coin_count = 0;
			   }
		   }
		   if (Value == 0x11)
		   {
			   if (snd_succeed_count == 0)
			   {
				   sound_play(S_SD_SUCCEED);
			   }

			   snd_succeed_count++;
			   if (snd_succeed_count >= 8)
			   {
				   snd_succeed_count = 0;
			   }
		   }
		   if (Value == 0xc1)
		   {
			   printline("Playing fail sound");
			   sound_play(S_SD_FAIL);
		   }
		
		   //alternate sounds for alternate ROM set (same counter should be okay)
		   //you get more points and a higher-pitched sound if your reaction is quick
		   if (Value == 0x12)
		   {
   			if (snd_succeed_count == 0)
	   		{
		   		sound_play(S_SDA_SUCCESS_LO);
			   }

			   snd_succeed_count++;
			   if (snd_succeed_count >= 8)
   			{
	   			snd_succeed_count = 0;
		   	}
   		}
	   	
		   if (Value == 0x0F)
	   	{
	   		if (snd_succeed_count == 0)
		   	{
			   	sound_play(S_SDA_SUCCESS_HI);
		   	}

			   snd_succeed_count++;
   			if (snd_succeed_count >= 8)
   			{
	   			snd_succeed_count = 0;
		   	}
   		}
      }
		break;
		

	case 0x08:	//	Latch for outputs to vid hardware and coin counters

		// bit 6 is set, clear the IRQ
		if ((Value >> 6) & 0x01)
		{
			Z80_CLEAR_IRQ;
//			z80_set_irq_line(0, CLEAR_LINE);
		}
		// bit 7 controls whether LD video is displayed
		if ((Value >> 7) & 0x01)
		{
//			palette_set_transparency(SUPERDON_TRANSPARENT_COLOR, true);
		}
		else 
		{
			// NOTE : this gets blanked _after_ a skip command is issued
//			palette_set_transparency(SUPERDON_TRANSPARENT_COLOR, false);
		}
		break;

	case 0x0c:	//	Output to HD46505
		break;

	case 0x0d:	//	Output to HD46505
		break;

	default:
        sprintf(s, "SUPERDON: Unsupported Port Output-> %x:%x (PC is %x)", Port, Value, Z80_GET_PC);
        printline(s);
        break;

	}		
}

Uint8 superd::port_read(Uint16 Port)
// Called whenever the emulator wants to read from a port
{

    char s[81] = { 0 };
    unsigned char result = 0;

	Port &= 0xFF;	// strip off high byte

    switch(Port)
    {

	case 0x00:	//	Joystick
		result = banks[0];
		break;

	case 0x01:	//	Buttons/Coins
		result = banks[1];
		break;

	case 0x02:	//	Dipswitch 1
		result = banks[2];
		break;

	case 0x03:	//	Dipswitch 2
		result = banks[3];
		break;

	case 0x04:	//	Read ldp-1000 status
		result = ldp_input_latch;
		break;

    default:
        sprintf(s, "SUPERD: Unsupported Port Input-> %x (PC is %x)", Port, Z80_GET_PC);
        printline(s);
        break;
    }

    return(result);

}

// used to set dip switch values
bool superd::set_bank(unsigned char which_bank, unsigned char value)
{
	bool result = true;
	
	switch (which_bank)
	{
	case 0:	// bank A
		banks[2] = (unsigned char) (value ^ 0xFF);	// dip switches are active low
		break;
	case 1:	// bank B
		banks[3] = (unsigned char) (value ^ 0xFF);	// switches are active low
		break;
	default:
		printline("ERROR: Bank specified is out of range!");
		result = false;
		break;
	}
	
	return result;
}

// calculates color palette
void superd::palette_calculate()
{

	SDL_Color temp_color;
	//Convert palette rom into a useable palette
	for (int i = 0; i < SUPERD_COLOR_COUNT; i++)
	{
		Uint8 bit0,bit1,bit2;	

		/* red component */
		bit0 = static_cast<Uint8>((color_prom[i] >> 5) & 0x01);
		bit1 = static_cast<Uint8>((color_prom[i] >> 6) & 0x01);
		bit2 = static_cast<Uint8>((color_prom[i] >> 7) & 0x01);
		temp_color.r = static_cast<Uint8>((0x24 * bit0) + (0x4a * bit1) + (0x91 * bit2));
		/* green component */
		bit0 = static_cast<Uint8>((color_prom[i] >> 2) & 0x01);
		bit1 = static_cast<Uint8>((color_prom[i] >> 3) & 0x01);	
		bit2 = static_cast<Uint8>((color_prom[i] >> 4) & 0x01);
		temp_color.g = static_cast<Uint8>((0x24 * bit0) + (0x4a * bit1) + (0x91 * bit2));
		/* blue component */
		bit0 = static_cast<Uint8>(0);
		bit1 = static_cast<Uint8>((color_prom[i] >> 0) & 0x01);
		bit2 = static_cast<Uint8>((color_prom[i] >> 1) & 0x01);
		temp_color.b = static_cast<Uint8>((0x24 * bit0) + (0x4a * bit1) + (0x91 * bit2));
		palette_set_color(i, temp_color);
	}
	
	palette_set_transparency(0, false);	// color 0 is yellow, and is not transparent
	palette_set_transparency(SUPERDON_TRANSPARENT_COLOR, true);
}

void superd::video_repaint()
{
	for (int charx = 0; charx < 32; charx++)
	{
		for (int chary = 0; chary < 32; chary++)
		{
			for (int x=0; x < 4; x++)
			{
				for (int y = 0; y < 8; y++)
				{
					Uint8 left_pixel = static_cast<Uint8>((character[m_cpumem[chary * 32 + charx + 0x5c00]*32+x+4*y] & 0xf0) >> 4);
					Uint8 right_pixel = static_cast<Uint8>((character[m_cpumem[chary * 32 + charx + 0x5c00]*32+x+4*y] & 0x0f));
					*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + ((chary * 8 + y) * SUPERD_OVERLAY_W) + (charx * 8 + x * 2)) = left_pixel;
					*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + ((chary * 8 + y) * SUPERD_OVERLAY_W) + (charx * 8 + x * 2 + 1)) = right_pixel;
				}
			}
		}
	}
}

// this gets called when the user presses a key or moves the joystick
void superd::input_enable(Uint8 move)
{
	switch (move)
	{
	case SWITCH_UP:
		banks[0] &= ~0x80; 
		break;
	case SWITCH_LEFT:
		banks[0] &= ~0x02;
		break;
	case SWITCH_RIGHT:
		banks[0] &= ~0x20;
		break;
	case SWITCH_DOWN:
		banks[0] &= ~0x08; 
		break;
	case SWITCH_START1: // '1' on keyboard
		banks[1] &= ~0x80; 
		break;
	case SWITCH_START2: // '2' on keyboard
		banks[1] &= ~0x40;
		break;
	case SWITCH_BUTTON1: // space on keyboard
		banks[1] &= ~0x20;
		break;
	case SWITCH_COIN1: 
		banks[1] &= ~0x08;
		break;
	case SWITCH_COIN2: 
		banks[1] &= ~0x04;
		break;
	case SWITCH_SERVICE: 
		banks[1] &= ~0x01;
		break;
	case SWITCH_TEST: 
		banks[1] &= ~0x02;
		break;
	default:
		printline("Error, bug in move enable");
		break;
	}
}  

// this gets called when the user releases a key or moves the joystick back to center position
void superd::input_disable(Uint8 move)
{
	switch (move)
	{
	case SWITCH_UP:
		banks[0] |= 0x80; 
		break;
	case SWITCH_LEFT:
		banks[0] |= 0x02;
		break;
	case SWITCH_RIGHT:
		banks[0] |= 0x20;
		break;
	case SWITCH_DOWN:
		banks[0] |= 0x08; 
		break;
	case SWITCH_START1: // '1' on keyboard
		banks[1] |= 0x80; 
		break;
	case SWITCH_START2: // '2' on keyboard
		banks[1] |= 0x40;
		break;
	case SWITCH_BUTTON1: // space on keyboard
		banks[1] |= 0x20;
		break;
	case SWITCH_COIN1: 
		banks[1] |= 0x08;
		break;
	case SWITCH_COIN2: 
		banks[1] |= 0x04;
		break;
	case SWITCH_SERVICE: 
		banks[1] |= 0x01;
		break;
	case SWITCH_TEST: 
		banks[1] |= 0x02;
		break;
	default:
		printline("Error, bug in move enable");
		break;
	}
}

void superd::OnVblank()
{
	ldv1000_report_vsync();

	// Redraws the screen (if needed)
	video_blit();
}

void superd::OnLDV1000LineChange(bool bIsStatus, bool bIsEnabled)
{
	if (bIsEnabled)
	{
		// if the status strobe has become enabled, then read command from ldv1000 into latch
		if (bIsStatus)
		{
			ldp_input_latch = read_ldv1000();
		}
		// else the command strobe has become enabled, so send command to ldv1000 and do an IRQ
		else 
		{
			write_ldv1000(ldp_output_latch);
			Z80_ASSERT_IRQ;
		}
	}
	// else the line is disabled which we don't care about
}
