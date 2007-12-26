/*
* lair.cpp
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

// Dragon's Lair/Space Ace driver for DAPHNE
// started by Matt Ownby
// contributions made by Mark Broadhead, Robert DiNapoli, Jeff Kulczycki
// If you don't see your name here, feel free to add it =]

#ifdef WIN32
#pragma warning (disable:4100) // disable warning about unreferenced parameter
#endif

// Win32 doesn't use strcasecmp, it uses stricmp (lame)
#ifdef WIN32
#define strcasecmp stricmp
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lair.h"
#include "../ldp-in/ldv1000.h"
#include "../ldp-in/pr7820.h"
#include "../ldp-out/ldp.h"
#include "../scoreboard/scoreboard_collection.h"
#include "../video/led.h"
#include "../video/palette.h"
#include "../io/conout.h"
#include "../io/error.h"
#include "../sound/sound.h"
#include "../cpu/cpu.h"
#include "../cpu/generic_z80.h"

//////////////////////////////////////////////////////////////////////////

// lair class constructor (default the rev F2 roms)
lair::lair() :
m_bUseAnnunciator(false),
m_pScoreboard(NULL)
{
	m_shortgamename = "lair";
	memset(m_cpumem, 0, CPU_MEM_SIZE);
	m_switchA = 0x22;
	m_switchB = 0xD8;
	m_joyskill_val = 0xFF;	// all input cleared
	m_misc_val = 0x7F;	// bit 7 and 6 alternated, and bits 0-3 all set (bits 4-5 are unused)

	// Scoreboard starts off being visible
	m_bScoreboardVisibility = true;

	struct cpudef cpu;
	memset(&cpu, 0, sizeof(struct cpudef));
	cpu.type = CPU_Z80;
	cpu.hz = LAIR_CPU_HZ;
	cpu.irq_period[0] = LAIR_IRQ_PERIOD;
	cpu.nmi_period = (1000.0 / 60.0);	// dragon's lair has no NMI.  We use this to time the LD-V1000 strobes since it's convenient
	cpu.initial_pc = 0;
	cpu.must_copy_context = false;
	cpu.mem = m_cpumem;
	add_cpu(&cpu);	// add this cpu to the list (it will be our only one)

	struct sounddef soundchip;
	soundchip.type = SOUNDCHIP_AY_3_8910;  // Dragon's Lair hardware uses the ay-3-8910
	soundchip.hz = LAIR_CPU_HZ / 2;   // DL halves the CPU clock for the sound chip
	m_soundchip_id = add_soundchip(&soundchip);

	m_disc_fps = 23.976;
	m_game_type = GAME_LAIR;

	m_game_uses_video_overlay = false;	// this game doesn't (by default) use video overlay
	m_video_overlay_needs_update = false;

	ldv1000_enable_instant_seeking();	// make the LD-V1000 perform instantaneous seeks because we can
	m_status_strobe_timer = 0;
	m_uses_pr7820 = false;  // only used by lairalt()
	m_leds_cleared = false; //hack so we can execute the clear command just once

	m_num_sounds = 3;
	m_sound_name[S_DL_CREDIT] = "dl_credit.wav";
	m_sound_name[S_DL_ACCEPT] = "dl_accept.wav";
	m_sound_name[S_DL_BUZZ] = "dl_buzz.wav";

	// ROM images for Dragon's Lair (this must be static!)
	const static struct rom_def lair_roms[] =
	{
		{ "dl_f2_u1.bin", NULL, &m_cpumem[0x0000], 0x2000, 0xF5EA3B9D },
		{ "dl_f2_u2.bin", NULL, &m_cpumem[0x2000], 0x2000, 0xDCC1DFF2 },
		{ "dl_f2_u3.bin", NULL, &m_cpumem[0x4000], 0x2000, 0xAB514E5B },
		{ "dl_f2_u4.bin", NULL, &m_cpumem[0x6000], 0x2000, 0xF5EC23D2 },
		{ NULL }
	};

	m_rom_list = lair_roms;

}

void lair::set_version(int version)
{
	//Since other game classes are derived from this one,
	//we need to make sure the right game is loaded for these
	//alternate rom versions
	if (strcasecmp(m_shortgamename, "lair") == 0)
	{
		if (version == 1) //rev F2
		{
			//rev F2 is already loaded, do nothing
		}
		else if (version == 2)  //rev F
		{
			m_shortgamename = "lair_f";
			const static struct rom_def roms[] =
			{
				{ "dl_f_u1.bin", NULL, &m_cpumem[0x0000], 0x2000, 0x06fc6941 }, 
				{ "dl_f2_u2.bin", "lair", &m_cpumem[0x2000], 0x2000, 0xdcc1dff2 }, //same as rev F2
				{ "dl_f2_u3.bin", "lair", &m_cpumem[0x4000], 0x2000, 0xab514e5b }, //same as rev F2
				{ "dl_f_u4.bin", NULL, &m_cpumem[0x6000], 0x2000, 0xa817324e }, 
				{ NULL }
			};
			m_rom_list = roms;
		}
		else if (version == 3)  //rev E
		{
			m_shortgamename = "lair_e";
			static struct rom_def roms[] =
			{
				{ "dl_e_u1.bin", NULL, &m_cpumem[0x0000], 0x2000, 0x02980426 },
				{ "dl_e_u2.bin", NULL, &m_cpumem[0x2000], 0x2000, 0x979d4c97 },
				{ "dl_e_u3.bin", NULL, &m_cpumem[0x4000], 0x2000, 0x897bf075 },
				{ "dl_e_u4.bin", NULL, &m_cpumem[0x6000], 0x2000, 0x4ebffba5 },
				{ NULL }
			};
			m_rom_list = roms;
		}
	}
	else
	{
		//call the generic non-functional method instead
		game::set_version(version);
	}
}

void lair::patch_roms()
{
	if (m_fastboot)
	{
		m_cpumem[0x121b] = 0x00;
		m_cpumem[0x1235] = 0x00;
	}
}

// dragon's lair enhanced class constructor
dle11::dle11()
{
	m_shortgamename = "dle11";
	m_game_type = GAME_DLE1;

	// NOTE : this must be static
	static struct rom_def roms[] =
	{
		{ "dle11u1l.bin", NULL, &m_cpumem[0x0000], 0x2000, 0x9E65B33D },
		{ "dle11u2l.bin", NULL, &m_cpumem[0x2000], 0x2000, 0xF16FA36F },
		{ "dle11u3l.bin", NULL, &m_cpumem[0x4000], 0x2000, 0xB8D07A16 },
		{ "dle11u4l.bin", NULL, &m_cpumem[0x6000], 0x2000, 0x20FC79B7 },
		{ NULL }
	};

	m_rom_list = roms;

}

void dle11::patch_roms()
{
	bool passed_test = false;

	// NOW check to make sure the DLE readme file is present and unaltered
	// Dave Hallock requested this check to make sure sites don't remove his readme file
	// and distribute DLE.

	passed_test = verify_required_file("readme11.txt", "dle11", 0x4BF84551);

	// if they failed the test then exit daphne
	if (!passed_test)
	{
		printerror("DLE readme11.txt file is missing or altered.");
		printerror("Please get the original readme11.txt file from www.d-l-p.com, thanks.");
		set_quitflag();
	}

	if (m_fastboot)
	{
		m_cpumem[0x121b] = 0x00;
		m_cpumem[0x1235] = 0x00;
	}
}

// dragon's lair enhanced v2.x class constructor
dle2::dle2()
{
	// We need to differentiate between GAME_LAIR and GAME_DLE2 because 
	//  print_board_info() only works with rev F2 (and probably DLE v1 too).
	// It is known not to work with DLE v2
	m_game_type = GAME_DLE2;

	m_shortgamename = "dle21";

	// NOTE : this must be static
	static struct rom_def roms[] =
	{
		{ "DLE21_U1.bin", NULL, &m_cpumem[0x0000], 0x2000, 0x4F8AF481 },
		{ "DLE21_U2.bin", NULL, &m_cpumem[0x2000], 0x2000, 0x5FCA04C5 },
		{ "DLE21_U3.bin", NULL, &m_cpumem[0x4000], 0x2000, 0xC14F36B3 },
		{ NULL }
	};

	m_rom_list = roms;

}

void dle2::set_version(int version)
{

	if (version == 1) //rev 2.1
	{
		//rev 2.1 is already loaded, do nothing
	}
	else if (version == 0)  //rev 2.0
	{
		m_shortgamename = "dle20";

		static struct rom_def roms[] =
		{
			{ "DLE20_U1.bin", NULL, &m_cpumem[0x0000], 0x2000, 0x0ACA15B4 },
			{ "DLE20_U2.bin", NULL, &m_cpumem[0x2000], 0x2000, 0x1CEA7622 },
			{ "DLE20_U3.bin", NULL, &m_cpumem[0x4000], 0x2000, 0x21A7E7CA },
			{ "DLE20_U4.bin", NULL, &m_cpumem[0x6000], 0x2000, 0x4AE26073 },
			{ NULL }
		};

		m_rom_list = roms;
	}
	else
	{
		printline("LAIR 2.x:  Unsupported -version paramter, ignoring...");
	}
}

// DLE v2.x test to make sure readme20.txt file is present and unaltered
void dle2::patch_roms()
{
	bool passed_test = false;

	if (strcasecmp(m_shortgamename, "dle20") == 0)
	{
		if (!(passed_test = verify_required_file("readme20.txt", "dle20", 0x51C50010)))
		{
			printerror("DLE readme20.txt file is missing or altered.");
			printerror("Please get the original file from http://www.d-l-p.com.  Thanks.");
			set_quitflag();
		}
	}
	else
	{
		if (!(passed_test = verify_required_file("readme21.txt", "dle21", 0xA68F0D21)))
		{
			printerror("DLE readme21.txt file is missing or altered.");
			printerror("Please get the original file from http://www.d-l-p.com.  Thanks.");
			set_quitflag();
		}
	}
}

// Space Ace class constructor
ace::ace()
{
	m_shortgamename = "ace";
	m_game_type = GAME_ACE;	
	m_switchA = 0x3D;	// 2 coins/credit, attract mode audio always on
	m_switchB = 0xFE;	// 5 lives	

	// NOTE : this must be static
	const static struct rom_def ace_roms[] =
	{
		{ "sa_a3_u1.bin", NULL, &m_cpumem[0x0000], 0x2000, 0x427522D0 },
		{ "sa_a3_u2.bin", NULL, &m_cpumem[0x2000], 0x2000, 0x18D0262D },
		{ "sa_a3_u3.bin", NULL, &m_cpumem[0x4000], 0x2000, 0x4646832D },
		{ "sa_a3_u4.bin", NULL, &m_cpumem[0x6000], 0x2000, 0x57DB2A79 },
		{ "sa_a3_u5.bin", NULL, &m_cpumem[0x8000], 0x2000, 0x85CBCDC4 },
		{ NULL }
	};

	m_rom_list = ace_roms;

}

// ace supports multiple rom revs
void ace::set_version(int version)
{
	if (version == 1) //rev A3
	{
		//rev A3 is already loaded, do nothing
	}
	else if (version == 2)  //rev A2
	{
		m_shortgamename = "ace_a2";
		const static struct rom_def ace_roms[] =
		{
			{ "sa_a2_u1.bin", NULL, &m_cpumem[0x0000], 0x2000, 0x71b39e27 },
			{ "sa_a3_u2.bin", "ace", &m_cpumem[0x2000], 0x2000, 0x18D0262D },
			{ "sa_a3_u3.bin", "ace", &m_cpumem[0x4000], 0x2000, 0x4646832D },
			{ "sa_a3_u4.bin", "ace", &m_cpumem[0x6000], 0x2000, 0x57DB2A79 },
			{ "sa_a3_u5.bin", "ace", &m_cpumem[0x8000], 0x2000, 0x85CBCDC4 },
			{ NULL }
		};
		m_rom_list = ace_roms;
	}
	else if (version == 3)  //rev A
	{
		m_shortgamename = "ace_a";
		const static struct rom_def ace_roms[] =
		{
			{ "sa_a_u1.bin", NULL, &m_cpumem[0x0000], 0x2000, 0x8eb1889e },
			{ "sa_a3_u2.bin", "ace", &m_cpumem[0x2000], 0x2000, 0x18D0262D },
			{ "sa_a3_u3.bin", "ace", &m_cpumem[0x4000], 0x2000, 0x4646832D },
			{ "sa_a3_u4.bin", "ace", &m_cpumem[0x6000], 0x2000, 0x57DB2A79 },
			{ "sa_a3_u5.bin", "ace", &m_cpumem[0x8000], 0x2000, 0x85CBCDC4 },
			{ NULL }
		};
		m_rom_list = ace_roms;
	}
	else
	{
		printline("ACE:  Unsupported -version paramter, ignoring...");
	}
}	



// Space Ace class constructor
sae::sae()
{
	m_shortgamename = "sae";
	m_game_type = GAME_SAE;	
	m_switchA = 0x66;	
	m_switchB = 0x98;	

	// NOTE : this must be static
	const static struct rom_def sae_roms[] =
	{
		{ "sae10_u1.bin", NULL, &m_cpumem[0x0000], 0x2000, 0xCBC5E425 },
		{ "sae10_u2.bin", NULL, &m_cpumem[0x2000], 0x2000, 0x71A26F47 },
		{ "sae10_u3.bin", NULL, &m_cpumem[0x4000], 0x2000, 0xBAC5CDD8 },
		{ "sae10_u4.bin", NULL, &m_cpumem[0x6000], 0x2000, 0xE18380F9 },
		{ "sae10_u5.bin", NULL, &m_cpumem[0x8000], 0x2000, 0x8A536CB0 },
		{ NULL }
	};

	m_rom_list = sae_roms;

}

void ace::patch_roms()
{
	if (m_fastboot)
	{
		m_cpumem[0x121b] = 0x00;
		m_cpumem[0x123a] = 0x00;
	}
}

void sae::patch_roms()
{
	bool passed_test = false;

	// NOW check to make sure the SAE readme file is present and unaltered
	// Dave Hallock requested this check to make sure sites don't remove his readme file
	// and distribute SAE.

	passed_test = verify_required_file("readme.txt", "sae", 0xCA4E20E6);

	// if they failed the test then exit daphne
	if (!passed_test) 
	{
		printerror("The SAE readme.txt file is missing or altered.");
		printerror("Please get the original file from www.d-l-p.com, thanks.");
		set_quitflag();
	}
}

lairalt::lairalt()
{
	m_shortgamename = "lair_a";
	m_uses_pr7820 = true;

	// NOTE : this must be static
	static struct rom_def roms[] =
	{
		{ "dl_a_u1.bin", NULL, &m_cpumem[0x0000], 0x2000, 0xd76e83ec },
		{ "dl_a_u2.bin", NULL, &m_cpumem[0x2000], 0x2000, 0xa6a723d8 },
		{ "dl_a_u3.bin", NULL, &m_cpumem[0x4000], 0x2000, 0x52c59014 },
		{ "dl_a_u4.bin", NULL, &m_cpumem[0x6000], 0x2000, 0x924d12f2 },
		{ "dl_a_u5.bin", NULL, &m_cpumem[0x8000], 0x2000, 0x6ec2f9c1 },
		{ NULL }
	};

	m_rom_list = roms;
	//cpu.nmi_period = 0 // dragon's lair has no NMI, and the PR-7820 has 
	//no strobes to keep track of.  
	//FIXME:  How can we disable the NMI for lairalt()?

	set_bank(0, 0xFF);  //set more reasonable defaults for this ROM rev
	set_bank(1, 0xF7);

}

// for lairalt, we support multiple rom revs
void lairalt::set_version(int version)
{
	if (version == 1) //rev A
	{
		//rev A is already loaded, do nothing
	}
	else if (version == 2)  //rev B
	{
		m_shortgamename = "lair_b";
		const static struct rom_def roms[] =
		{
			{ "dl_a_u1.bin", "lair_a", &m_cpumem[0x0000], 0x2000, 0xd76e83ec }, //same as rev a
			{ "dl_b_u2.bin", NULL, &m_cpumem[0x2000], 0x2000, 0x6751103d },
			{ "dl_a_u3.bin", "lair_a", &m_cpumem[0x4000], 0x2000, 0x52c59014 }, //same as rev a
			{ "dl_a_u4.bin", "lair_a", &m_cpumem[0x6000], 0x2000, 0x924d12f2 }, //same as rev a
			{ "dl_a_u5.bin", "lair_a", &m_cpumem[0x8000], 0x2000, 0x6ec2f9c1 }, //same as rev a
			{ NULL }
		};
		m_rom_list = roms;
	}
	else if (version == 3)  //rev C
	{
		m_shortgamename = "lair_c";

		static struct rom_def roms[] =
		{
			{ "dl_c_u1.bin", NULL, &m_cpumem[0x0000], 0x2000, 0xcebfe26a },
			{ "dl_b_u2.bin", "lair_b", &m_cpumem[0x2000], 0x2000, 0x6751103d }, //same as rev B
			{ "dl_a_u3.bin", "lair_a", &m_cpumem[0x4000], 0x2000, 0x52c59014 }, //same as rev A
			{ "dl_a_u4.bin", "lair_a", &m_cpumem[0x6000], 0x2000, 0x924d12f2 }, //same as rev A
			{ "dl_a_u5.bin", "lair_a", &m_cpumem[0x8000], 0x2000, 0x6ec2f9c1 }, //same as rev A
			{ NULL }
		};
		m_rom_list = roms;
	}
	else if (version == 4)  //rev D
	{
		m_uses_pr7820 = false;  //also LD-V1000?
		m_shortgamename = "lair_d";

		static struct rom_def roms[] =
		{
			{ "dl_d_u1.bin", NULL, &m_cpumem[0x0000], 0x2000, 0x0b5ab120 },
			{ "dl_d_u2.bin", NULL, &m_cpumem[0x2000], 0x2000, 0x93ebfffb },
			{ "dl_d_u3.bin", NULL, &m_cpumem[0x4000], 0x2000, 0x22e6591f },
			{ "dl_d_u4.bin", NULL, &m_cpumem[0x6000], 0x2000, 0x5f7212cb },
			{ "dl_d_u5.bin", NULL, &m_cpumem[0x8000], 0x2000, 0x2b469c89 },
			{ NULL }
		};
		m_rom_list = roms;
	}	
	else if (version == 5)  // this set is from the DL machine with serial # 1
	{
		m_shortgamename = "lair_n1";

		static struct rom_def roms[] =
		{
			{ "dl_n1_u1.bin", NULL, &m_cpumem[0x0000], 0x2000, 0xa1856eac },
			{ "dl_n1_u2.bin", NULL, &m_cpumem[0x2000], 0x2000, 0x1b34406f },
			{ "dl_n1_u3.bin", NULL, &m_cpumem[0x4000], 0x2000, 0xcf3f4d3c },
			{ "dl_n1_u4.bin", NULL, &m_cpumem[0x6000], 0x2000, 0xa98880c5 },
			{ "dl_n1_u5.bin", NULL, &m_cpumem[0x8000], 0x2000, 0x17b7336b },
			{ NULL }
		};
		m_rom_list = roms;
	}
	else if (version == 6)  // an early set with an unknown revision designation
	{
		m_shortgamename = "lair_x";

		static struct rom_def roms[] =
		{
			{ "dl_x_u1.bin", NULL, &m_cpumem[0x0000], 0x2000, 0xEA6D5498 },
			{ "dl_x_u2.bin", NULL, &m_cpumem[0x2000], 0x2000, 0xFFE84A95 },
			{ "dl_x_u3.bin", NULL, &m_cpumem[0x4000], 0x2000, 0x6363FD84 },
			{ "dl_x_u4.bin", NULL, &m_cpumem[0x6000], 0x2000, 0x84CABB86 },
			{ "dl_x_u5.bin", NULL, &m_cpumem[0x8000], 0x2000, 0x8CC8F073 },
			{ NULL }
		};
		m_rom_list = roms;
	}
}


// lairalt old revs don't use the LDV1000, so there are no strobe -presets
void lairalt::set_preset(int preset)
{
	printline("NOTE: lairalt has no presets defined.  The -preset option will be ignored.");
}


///////////////////////////////////////////////////////////

// raise IRQ line
void lair::do_irq(unsigned int which_irq)
{
	// assume that the IRQ is 0 since we only have 1 IRQ
	Z80_ASSERT_IRQ;
}

// Dragon's Lair has no NMI
// We just use the timer as a convenience to keep track of the LD-V1000 status strobe
void lair::do_nmi()
{
	if (!m_uses_pr7820)
	{
		m_status_strobe_timer = get_total_cycles_executed(0);
	}

	// If the game does not use a video overlay
	if (!m_game_uses_video_overlay)
	{
		m_pScoreboard->RepaintIfNeeded();
	}

	// else if we are using a video overlay
	else
	{
		video_blit();
	}

}

void lair::cpu_mem_write(Uint16 Addr, Uint8 Value)
// Called whenever the Z80 emulator wants to write to memory
{
	// if we're writing to RAM or hardware
	if (Addr >= 0xA000)
	{
		// if we're writing to RAM
		if (Addr < 0xB000)
		{
			switch(Addr)
			{
				// We do not emulate the sound chip correctly.  Instead we do a cheat which gives us the benefit
				// of less CPU cycles and also better quality sound.
				// A01C gets written to every time a beep is supposed to be played, and A01D gets the memory location where
				// the beep data is, so whenever A01C gets a 1 written to it, we check the value of A01D to see which type
				// of beep we are supposed to play.  If anyone ever re-assembled the Dragon's Lair ROM, this scheme would
				// likely break.
			case 0xA01C:
				if (Value & 1)
				{
					int index = m_cpumem[0xA01D] | (m_cpumem[0xA01E] << 8);	// pointer to audio data

					if (m_prefer_samples)
					{
						// only the 'accept' sound data has a D5 as the second byte
						if (m_cpumem[index+1] == 0xD5)
						{
							sound_play(S_DL_ACCEPT);
						}
						// only the 'credit' sound data has a 0x66 as the second byte
						else if (m_cpumem[index+1] == 0x66)
						{
							sound_play(S_DL_CREDIT);
						}
						// only the 'buzz' sound data has a 0x99 as its third byte
						else if (m_cpumem[index+1] == 0x99)
						{
							sound_play(S_DL_BUZZ);
						}
						// else unknown sound, play an error
						else
						{
							printline("WARNING : Unknown dragon's lair sound!");
						}
					}
				}
				break;
			} // end switch

			m_cpumem[Addr] = Value; // always store to RAM if we write to it
		}

		// ROM
		else
		{
			switch (Addr)
			{
				// real sound chip data is sent here
			case 0xE000:
				if (!m_prefer_samples)
					audio_write_ctrl_data(m_soundchip_address_latch, Value, m_soundchip_id);
				break;

				// I believe this controlled whether the bus was in input or output mode but it is not important
				// for emulation
			case 0xE008:
				// it seems to be safe to totally ignore this =]
				break;

				// sound chip I/O (also controls dip switches)
			case 0xE010:
				m_soundchip_address_latch = Value;
				switch (Value)
				{
				case 0x0E:
					// read dip-switch 1 and put result at 0xC000
					m_cpumem[0xC000] = m_switchA;
					break;
				case 0x0F:
					// read dip-switch 2 and put result at 0xC000
					m_cpumem[0xC000] = m_switchB;
					break;
				default:
					break;
				}
				break;

				// laserdisc control
			case 0xE020:
				if (m_uses_pr7820)
				{
					write_pr7820(Value);
				}
				else
				{
					write_ldv1000(Value);
				}
				break;

				// E030-E03F are all LED's on the Dragon's Lair scoreboard
			case 0xE030:
			case 0xE031:
			case 0xE032:
			case 0xE033:
			case 0xE034:
			case 0xE035:
				m_pScoreboard->update_player_score(Addr & 7, Value & 0x0F, 1);
				break;

			case 0xE036:	// 10's position of how many credits are deposited
			case 0xE037:	// 1's position of how many credits are deposited
				m_pScoreboard->update_credits(Addr & 1, Value & 0x0F);
				break;

			case 0xE038:
			case 0xE039:
			case 0xE03A:
			case 0xE03B:
			case 0xE03C:
			case 0xE03D:
				// if this isn't an annunciator command or if we are using an annunciator
				if ((Value != 0xCC) || (m_bUseAnnunciator))
				{
					m_pScoreboard->update_player_score(Addr & 7, (Value & 0x0F), 0);
				}
				// else do something with the LED's
				else
				{
					if (Addr == 0xE03B) change_led(false, false, true);	// ACE SKILL
					else if (Addr == 0xE03D) change_led(false, true, false);	// CAPTAIN SKILL
				}
				break;

				// lives of player 1
			case 0xE03E:
				if ((Value != 0xCC) || (m_bUseAnnunciator))
				{
					m_pScoreboard->update_player_lives(Value & 0x0F, 0);
				}
				else
				{
					// show cadet skill LED
					change_led(true, false, false);
				}
				break;
				// lives of player 2
			case 0xE03F:
				// Clears LEDs 
				if (Value == 0xCC)
				{
					// not needed for emulation
					//change_led(false, false, false);
					if (m_bUseAnnunciator)
					{
						// WDO: For some reason, this causes the annunciator LEDs
						// to flash too quickly, so they are just barely visible.
						// Therefore, we are executing it just on the first call,
						// so the initial state will be all LEDs off when the game boots.
						if (!m_leds_cleared)
						{
							m_leds_cleared = true;
							m_pScoreboard->update_player_lives(Value & 0x0F, 1);
						}
					}
				}
				else
				{
					m_pScoreboard->update_player_lives(Value & 0x0F, 1);
				}
				break;
			default:
				{
					char s[160] = { 0 };
					sprintf(s, "Unknown hardware output at %x, value of %x, PC %x", Addr, Value, Z80_GET_PC);
					printline(s);
				}
				break;
			} // end switch

			// update overlay if a repaint is needed
			m_video_overlay_needs_update = m_pScoreboard->is_repaint_needed();

		} // end writing to E000
	} // end if writing at an address >= A000

	// if we are trying to write below 0xA000, it means we are trying to write to ROM
	else
	{
		char s[160];
		sprintf(s, "Error, program attempting to write to ROM (%x), PC is %x", Addr, Z80_GET_PC);
		printline(s);
	}	
}


// Called whenever the Z80 emulator wants to read from memory
Uint8 lair::cpu_mem_read(Uint16 Addr)
{

	register Uint8 result;	// I intentionally left this uninitialized for speed

	// RAM/ROM
	if (Addr < 0xC000)
	{
		result = m_cpumem[Addr];
	}

	// the memory range of 0xC000 to somewhere less than 0xE000 is used by Lair/Ace to read external input
	// such as the joystick, the dip switches, or the laserdisc player
	else
	{
		switch(Addr)
		{
		case 0xC008:	// joystick/spaceace skill query ...
			result = m_joyskill_val;
			break;
		case 0xC010:
			result = read_C010();
			break;
		case 0xC020:
			result = read_ldv1000();
			break;
		default:
			result = m_cpumem[Addr];
			break;
		}	// end switch
	} // end if addr > 0xC000 ...

	return(result);
}

// this is necessary so that we can use it as a function pointer
SDL_Surface *lair_get_active_overlay()
{
	return g_game->get_active_video_overlay();
}

bool lair::init()
{
	bool bResult = true;

	cpu_init();

	IScoreboard *pScoreboard = ScoreboardCollection::GetInstance(m_pLogger,
		lair_get_active_overlay,
		false,	// we aren't thayer's quest
		m_bUseAnnunciator,
		get_scoreboard_port());

	if (pScoreboard)
	{
		// if video overlay is enabled, it means we're using an overlay scoreboard
		if (m_game_uses_video_overlay)
		{
			ScoreboardCollection::AddType(pScoreboard, ScoreboardFactory::OVERLAY);
		}
		// else if we're not using VLDP, then display an image scoreboard
		else if (!g_ldp->is_vldp())
		{
			ScoreboardCollection::AddType(pScoreboard, ScoreboardFactory::IMAGE);
		}

		// if user has also requested a hardware scoreboard, then enable that too
		if (get_scoreboard() != 0)
		{
			ScoreboardCollection::AddType(pScoreboard, ScoreboardFactory::HARDWARE);
		}

		m_pScoreboard = pScoreboard;
	}
	else
	{
		bResult = false;
	}

	return bResult;
}

// Initialize overlay scoreboard if -useroverlaysb command line switch used.
void lair::init_overlay_scoreboard()
{
	m_game_uses_video_overlay = true;
	m_overlay_size_is_dynamic = true;

	// note : in the past, m_video_overlay_count was set to 1 because the overlay scoreboard would update only part of the SDL surface
	//  with changes and leave the rest intact.
	// While this was more efficient, it also made the new IScoreboard interface more difficult to design,
	//  so I've decided that the entire scoreboard overlay will be redrawn any time there is a change.
	// I am hoping that today's faster cpu's will make this choice a win for everyone.

	m_video_overlay_width = 320;
	m_video_overlay_height = 240;
	m_palette_color_count = 256;

	// this is used to enable visibility on the overlay scoreboard, which is now enabled by default
	//    set_overlay_scoreboard(true);
}

void lair::palette_calculate()
{
	SDL_Color temp_color;

	// fill color palette with sensible grey defaults
	for (int i = 0; i < 256; i++)
	{
		temp_color.r = (unsigned char) i;
		temp_color.g = (unsigned char) i;
		temp_color.b = (unsigned char) i;

		palette_set_color(i, temp_color);
	}
}

// frees any images we loaded in, etc
void lair::shutdown()
{
	// IMPORTANT: the scoreboard must always be shut down even if in VLDP mode,
	//  because it closes the parallel port.
	if (m_pScoreboard)
	{
		m_pScoreboard->PreDeleteInstance();
	}

	cpu_shutdown();
}

// redraws the scoreboard on the screen
void lair::video_repaint()
{
	// if there is an overlay (for overlay scoreboard)
	if (m_video_overlay[m_active_video_overlay])
	{
		Uint32 cur_w = g_ldp->get_discvideo_width() >> 1;	// width overlay should be
		Uint32 cur_h = g_ldp->get_discvideo_height() >> 1;	// height overlay should be
		char s[128] = {0};

		// if the width or height of the mpeg video has changed since we last were here (ie, opening a new mpeg)
		// then reallocate the video overlay buffer
		if ((cur_w != m_video_overlay_width) || (cur_h != m_video_overlay_height))
		{
			if (g_ldp->lock_overlay(1000))
			{
				m_video_overlay_width = cur_w;
				m_video_overlay_height = cur_h;

				sprintf(s, "%s : Re-allocated overlay surface (%d x %d)...",
					m_shortgamename, m_video_overlay_width, m_video_overlay_height);

				printline(s);

				video_shutdown();

				if (! video_init())
				{
					printline("Fatal Error trying to re-allocate overlay surface!");
					set_quitflag();
				}

				g_ldp->unlock_overlay(1000);	// unblock game video overlay
			}
			else
			{
				sprintf(s, "%s : Timed out trying to get a lock on the yuv overlay", m_shortgamename);
				printline(s);
			}
		} // end if dimensions are incorrect
	}

	// we invalidate to force a repaint, because this function should _always_ repaint
	m_pScoreboard->Invalidate();
	m_pScoreboard->RepaintIfNeeded();
}

// basically 'preset' is a macro to set a bunch of other options; useful as a good shortcut
void lair::set_preset(int preset)
{
	if (preset == 1)
	{
		printline("LD-V1000 strobes enabled!");
	}
	else if (preset == 2)
	{
		printline("WARNING: You've requested that the LD-V1000 strobes be disabled, but this option has been removed!");
		printline("(instant strobes were incompatible with seek delay, and not accurate emulation anyway)");
	}
}

// used to set dip switch values
bool lair::set_bank(unsigned char which_bank, unsigned char value)
{
	bool result = true;

	switch (which_bank)
	{
	case 0:	// bank A
		m_switchA = (unsigned char) (value ^ 0xFF);	// in dragon's lair, dip switches are active low
		break;
	case 1:	// bank B
		m_switchB = (unsigned char) (value ^ 0xFF);	// switches are active low
		break;
	default:
		printline("ERROR: Bank specified is out of range!");
		result = false;
		break;
	}

	return result;
}

bool ace::handle_cmdline_arg(const char *arg)
{
	bool bRes = false;

	// annunciator only supported on space ace
	if (strcasecmp(arg, "-use_annunciator")==0)
	{
		m_bUseAnnunciator = true;
		bRes = true;
	}

	return bRes;
}

Uint8 lair::read_C010()
// emulates the C010 memory location
// bit 0 = player 1 button, bit 1 = player 2 button (clear = button is depressed)
// bit 2 = coin 1 inserted, bit 3 = coin 2 inserted
// bit 6 = status strobe, bit 7 = command strobe
{
	// Are we using the LD-V1000 ?
	if (!m_uses_pr7820)
	{
		// Nothing to do for LD-V1000 strobes.
		// (it's now handled in the OnLDV1000LineChange function).

		// Why is the 'instant strobe' option gone?
		// It's because instant strobes will only work with instant seeking, so if the user uses seek delay,
		//  the ROM will cause the seek to 'time out' almost instantly instead of waiting a decent amount of time.
		// Accurate strobes are better anyway because they are.. well.. accurate, and that's one of Daphne's goals.
	}
	else  //pr-7820 handler
	{
		// the PR-7820 doesn't generate strobes, so this function is much simpler.
		// all it does is set bit 7 (READY) low when it's not busy searching
		// (if the game CPU is halted during searches, then this will never be 
		//  set high anyway.  Maybe in the future...)

		if (read_pr7820_ready())
		{
			m_misc_val |= 0x80;
		}
		else
		{
			m_misc_val &= ~0x80;
		}
	}

	return(m_misc_val);

}

void lair::input_enable(Uint8 move)
{

	switch(move)
	{
	case SWITCH_UP:
		m_joyskill_val &= (unsigned char) ~1;	// clear bit 0
		break;
	case SWITCH_DOWN:
		m_joyskill_val &= (unsigned char) ~2;	// clear bit 1
		break;
	case SWITCH_LEFT:
		m_joyskill_val &= (unsigned char) ~4;	// clear bit 2
		break;
	case SWITCH_RIGHT:
		m_joyskill_val &= (unsigned char) ~8;	// clear bit 3
		break;
	case SWITCH_START1: // PLAYER 1
		m_misc_val &= ~1;	// clear bit 0 ...
		break;
	case SWITCH_START2:
		m_misc_val &= ~2;	// clear bit 1
		break;
	case SWITCH_BUTTON1: // SWORD
		m_joyskill_val &= (unsigned char) ~0x10;	// clear bit 4
		break;
	case SWITCH_BUTTON3:
		m_bScoreboardVisibility = !m_bScoreboardVisibility;
		m_pScoreboard->ChangeVisibility(m_bScoreboardVisibility);
		m_video_overlay_needs_update |= m_pScoreboard->is_repaint_needed();
		break;
	case SWITCH_COIN1: 
		m_misc_val &= (unsigned char) ~0x04;	
		break;
	case SWITCH_COIN2: 
		m_misc_val &= (unsigned char) ~0x08;	
		break;
	case SWITCH_SKILL1:	// cadet
		m_joyskill_val &= (unsigned char) ~0x20;	// clear bit 5
		break;
	case SWITCH_SKILL2:	// captain
		m_joyskill_val &= (unsigned char) ~0x40;	// clear bit 6
		break;
	case SWITCH_SKILL3:	// space ace
		m_joyskill_val &= (unsigned char) ~0x80;	// clear bit 7
		break;
	case SWITCH_SERVICE:
		if (m_game_type == GAME_LAIR)
		{
			m_switchA ^= 0x80;	// toggle diagnostics switch (to start/stop diagnostics)
		}
		else if (m_game_type == GAME_ACE)
		{
			m_switchB ^= 0x80;	// toggle diagnostics switch (to start/stop diagnostics)
		}
		break;
	default:
		//unused key, take no action

		//printline("Error, bug in Dragon's Lair's input enable");
		break;
	}
}


void lair::input_disable(Uint8 move)
{

	switch(move)
	{
	case SWITCH_UP:
		m_joyskill_val |= 1;	// set bit 0
		break;
	case SWITCH_DOWN:
		m_joyskill_val |= 2;	// set bit 1
		break;
	case SWITCH_LEFT:
		m_joyskill_val |= 4;	// set bit 2
		break;
	case SWITCH_RIGHT:
		m_joyskill_val |= 8;	// set bit 3
		break;
	case SWITCH_START1:
		m_misc_val |= 1; // PLAYER 1
		break;
	case SWITCH_START2:
		m_misc_val |= 2;	// set bit 1
		break;
	case SWITCH_BUTTON1: // SWORD
		m_joyskill_val |= 0x10;	// set bit 4
		break;
	case SWITCH_COIN1: 
		m_misc_val |= 0x04;	
		break;
	case SWITCH_COIN2: 
		m_misc_val |= 0x08;	
		break;
	case SWITCH_SKILL1:	// cadet
		m_joyskill_val |= (unsigned char) 0x20;	// set bit 5
		break;
	case SWITCH_SKILL2:	// captain
		m_joyskill_val |= (unsigned char) 0x40;	// set bit 6
		break;
	case SWITCH_SKILL3:	// space ace
		m_joyskill_val |= (unsigned char) 0x80;	// set bit 7
		break;
	default:
		//unused key, take no action

		//printline("Error, bug in Dragon's Lair's move disable");
		break;
	}
}

void lair::OnVblank()
{
	// in order to make OnLDV1000LineChange work
	ldv1000_report_vsync();
}

void lair::OnLDV1000LineChange(bool bIsStatus, bool bIsEnabled)
{
	m_misc_val &= 0x3F;	// clear all strobes to initialize

	// if the status strobe has changed
	if (bIsStatus)
	{
		if (bIsEnabled)
		{
			m_misc_val |= 0x40;	// set status strobe
		}
		// else no strobes are active
	}
	// else the command strobe has changed
	else
	{
		if (bIsEnabled)
		{
			m_misc_val |= 0x80;	// set command strobe
		}
	}
}

