/*
 * astron.cpp
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

// astron.cpp
// by Mark Broadhead
//
// Pioneer Astron Belt Hardware
//
// There are 2 different versions of this hardware. One that uses the Pioneer
// LD-V1000 and another that uses the Sega / Hitachi VIP9500SG.
//
// Games include:
//
// Astron Belt   (Hitachi and Pioneer)
// Galaxy Ranger/Star Blazer (Star Blazer Hitachi is undumped if it exists)
// Albegas/Cybernaught (unverified - discs exist (I have one) but to date no 
//			collector that I know has found a romset, rumored to be a prototype, 
//			might also have been a Japanese only release)
// Cobra Command (Special version that runs on Hitachi AB hardware)

#include <string.h>
#include "astron.h"
#include "../io/conout.h"
#include "../ldp-in/ldv1000.h"
#include "../ldp-in/vip9500sg.h"
#include "../ldp-out/ldp.h"
#include "../video/palette.h"
#include "../video/video.h"
#include "../sound/sound.h"
#include "../cpu/cpu.h"
#include "../cpu/generic_z80.h"

astron::astron()
{
	struct cpudef cpu;
	
	m_shortgamename = "astronp";
	
	memset(&cpu, 0, sizeof(struct cpudef));
	memset(banks, 0xFF, 4);	// fill banks with 0xFF's
	memset(sprite, 0x00, 0x10000);	// making sure sprite[] is zero'd out
	memset(used_sprite_color, 0x00, 256);	
	palette_modified = true;
	m_disc_fps = 29.97; 
	m_game_type = GAME_ASTRON;

	m_video_row_offset = -16;	// shift video up by 32 pixels (16 rows)

	m_video_overlay_width = ASTRON_OVERLAY_W;
	m_video_overlay_height = ASTRON_OVERLAY_H;
	m_palette_color_count = ASTRON_COLOR_COUNT;
	
	cpu.type = CPU_Z80;
	cpu.hz = 5000000; // the schematics seem to indicate that its 20MHz / 4
	cpu.irq_period[0] = (1000.0 / 59.94); // interrupt from vblank (60hz)
	cpu.nmi_period = (1000.0 / 59.94); // nmi from LD-V1000 command strobe
	cpu.initial_pc = 0;
	cpu.must_copy_context = false;
	cpu.mem = m_cpumem;
	add_cpu(&cpu);	// add a z80
	
	current_bank = 0;
	m_transparent_color = 0;
	ldp_output_latch = 0xff;

	m_num_sounds = 7;
	m_sound_name[S_AB_PLAYER_SHIP] = "ab_ship.wav";
	m_sound_name[S_AB_PLAYER_FIRE] = "ab_fire.wav";
	m_sound_name[S_AB_ENEMY_FIRE] = "ab_enemy.wav";
	m_sound_name[S_AB_ALARM1] = "ab_alarm1.wav";
	m_sound_name[S_AB_ALARM2] = "ab_alarm2.wav";
	m_sound_name[S_AB_ALARM3] = "ab_alarm3.wav";
	m_sound_name[S_AB_ALARM4] = "ab_alarm4.wav";

		// (this must be static!)
	const static struct rom_def astron_roms[] =
	{
		// main z80 program
		{ "abp-ic03.bin", NULL, &m_cpumem[0x0000], 0x4000, 0xfd0dcfc9 },
		{ "abp-ic10.bin", NULL, &m_cpumem[0x4000], 0x4000, 0xa3746393 },

		// bank switching roms
		{ "5284", "astron", &rombank[0x0000], 0x4000, 0xeec6db27 },
		{ "5284", "astron", &m_cpumem[0x8000], 0x4000, 0xeec6db27 },
		{ "5285", "astron", &rombank[0x4000], 0x4000, 0x820e154e },

		// tile graphics
		{ "5280", "astron", &character[0x0000], 0x0800, 0x583af1ff },
		{ "5281", "astron", &character[0x0800], 0x0800, 0x7b5c820c },

		// sprites
		{ "5286", "astron", &sprite[0x0000], 0x4000, 0x8eb1c28e },
		{ "5338", "astron", &sprite[0x8000], 0x4000, 0x94ca5f9a },

		// color lookup prom
		{ "5279", "astron", &bankprom[0x000], 0x200, 0x8716aeb5 },

		// misc (unused) proms 
		{ "pr-5278.bin", "astron", &miscprom[0x000], 0x100, 0xe81613da },
		{ "pr-5277.bin", "astron", &miscprom[0x100], 0x100, 0xbf2c33ab },
		{ "pr-5276.bin", "astron", &miscprom[0x200], 0x20, 0x91267e8a },
		{ "pr-5275.bin", "astron", &miscprom[0x220], 0x20, 0x0c872a9b },
		{ NULL }
	};

	m_rom_list = astron_roms;

}

blazer::blazer()
{
	m_shortgamename = "blazer";
	m_game_type = GAME_GALAXY;
	m_num_sounds = 8;
	m_sound_name[S_GR_FIRE] = "gr_fire.wav";
	m_sound_name[S_GR_ENEMY_CANNON] = "gr_cannon.wav";
	m_sound_name[S_GR_ENEMY_MINION] = "gr_mineon.wav";
	m_sound_name[S_GR_ENEMY_ATTACK] = "gr_attack.wav";
	m_sound_name[S_GR_ALARM1] = "gr_alarm1.wav";
	m_sound_name[S_GR_ALARM2] = "gr_alarm2.wav";
	m_sound_name[S_GR_ALARM3] = "gr_alarm3.wav";
	m_sound_name[S_GR_ALARM4] = "gr_alarm4.wav";

	// THIS MUST BE STATIC
	const static struct rom_def blazer_roms[] =
	{
		// main z80 program
		{ "epr-5590.bin", NULL, &m_cpumem[0x0000], 0x4000, 0xc06e41bb },
		{ "epr-5591.bin", NULL, &m_cpumem[0x4000], 0x4000, 0xb179d18c },
	
		// bank switching roms
		{ "gr5592.bin", "galaxy", &rombank[0x0000], 0x4000, 0xd13715f8 },
		{ "gr5592.bin", "galaxy", &m_cpumem[0x8000], 0x4000, 0xd13715f8 },
		{ "gr5593.bin", "galaxy", &rombank[0x4000], 0x4000, 0xb0a557aa },
	
		// sprites
		{ "gr5594.bin", "galaxy", &sprite[0x0000], 0x4000, 0x4403ef5a },
		{ "gr5611.bin", "galaxy", &sprite[0x4000], 0x4000, 0xb16bdfe4 },
		{ "gr5595.bin", "galaxy", &sprite[0x8000], 0x4000, 0xccbaec4f },
		{ "gr5612.bin", "galaxy", &sprite[0xc000], 0x4000, 0xe312d080 },

		// tile graphics
		{ "5280", "astron", &character[0x0000], 0x0800, 0x583af1ff },
		{ "5281", "astron", &character[0x0800], 0x0800, 0x7b5c820c },

		// color lookup prom
		{ "5279", "astron", &bankprom[0x000], 0x200, 0x8716aeb5 },

		// misc (unused) proms 
		{ "pr-5278.bin", "astron", &miscprom[0x000], 0x100, 0xe81613da },
		{ "pr-5277.bin", "astron", &miscprom[0x100], 0x100, 0xbf2c33ab },
		{ "pr-5276.bin", "astron", &miscprom[0x200], 0x20, 0x91267e8a },
		{ "pr-5275.bin", "astron", &miscprom[0x220], 0x20, 0x0c872a9b },
		{ NULL }
	};

	m_rom_list = blazer_roms;

}

galaxyp::galaxyp()
{
	m_shortgamename = "galaxyp";
	m_game_type = GAME_GALAXY;
	m_num_sounds = 8;
	m_sound_name[S_GR_FIRE] = "gr_fire.wav";
	m_sound_name[S_GR_ENEMY_CANNON] = "gr_cannon.wav";
	m_sound_name[S_GR_ENEMY_MINION] = "gr_mineon.wav";
	m_sound_name[S_GR_ENEMY_ATTACK] = "gr_attack.wav";
	m_sound_name[S_GR_ALARM1] = "gr_alarm1.wav";
	m_sound_name[S_GR_ALARM2] = "gr_alarm2.wav";
	m_sound_name[S_GR_ALARM3] = "gr_alarm3.wav";
	m_sound_name[S_GR_ALARM4] = "gr_alarm4.wav";

	// THIS MUST BE STATIC
	const static struct rom_def galaxyp_roms[] =
	{
		// main z80 program
		{ "epr-5613.bin", NULL, &m_cpumem[0x0000], 0x4000, 0x6617e702 },
		{ "epr-5614.bin", NULL, &m_cpumem[0x4000], 0x4000, 0x73ad8932 },
	
		// bank switching roms
		{ "gr5592.bin", "galaxy", &rombank[0x0000], 0x4000, 0xd13715f8 },
		{ "gr5592.bin", "galaxy", &m_cpumem[0x8000], 0x4000, 0xd13715f8 },
		{ "gr5593.bin", "galaxy", &rombank[0x4000], 0x4000, 0xb0a557aa },
	
		// sprites
		{ "gr5594.bin", "galaxy", &sprite[0x0000], 0x4000, 0x4403ef5a },
		{ "gr5611.bin", "galaxy", &sprite[0x4000], 0x4000, 0xb16bdfe4 },
		{ "gr5595.bin", "galaxy", &sprite[0x8000], 0x4000, 0xccbaec4f },
		{ "gr5612.bin", "galaxy", &sprite[0xc000], 0x4000, 0xe312d080 },

		// tile graphics
		{ "5280", "astron", &character[0x0000], 0x0800, 0x583af1ff },
		{ "5281", "astron", &character[0x0800], 0x0800, 0x7b5c820c },

		// color lookup prom
		{ "5279", "astron", &bankprom[0x000], 0x200, 0x8716aeb5 },

		// misc (unused) proms 
		{ "pr-5278.bin", "astron", &miscprom[0x000], 0x100, 0xe81613da },
		{ "pr-5277.bin", "astron", &miscprom[0x100], 0x100, 0xbf2c33ab },
		{ "pr-5276.bin", "astron", &miscprom[0x200], 0x20, 0x91267e8a },
		{ "pr-5275.bin", "astron", &miscprom[0x220], 0x20, 0x0c872a9b },
		{ NULL }
	};

	m_rom_list = galaxyp_roms;

}

astronh::astronh()
{
	m_shortgamename = "astron";
	
	cpu_change_nmi(0, 2.0);	// change the NMI period for cpu #0
	// we'll use this to clock the 8251
	rxrdy = txrdy = false;

		// (this must be static!)
	const static struct rom_def astronh_roms[] =
	{
		// main z80 program
		{ "5473c", NULL, &m_cpumem[0x0000], 0x4000, 0xf24baaa },
		{ "5474a", NULL, &m_cpumem[0x4000], 0x4000, 0x5d44603d },

		// bank switching roms
		{ "5284", NULL, &rombank[0x0000], 0x4000, 0xeec6db27 },
		{ "5284", NULL, &m_cpumem[0x8000], 0x4000, 0xeec6db27 },
		{ "5285", NULL, &rombank[0x4000], 0x4000, 0x820e154e },

		// tile graphics
		{ "5280", NULL, &character[0x0000], 0x0800, 0x583af1ff },
		{ "5281", NULL, &character[0x0800], 0x0800, 0x7b5c820c },

		// sprites
		{ "5286", NULL, &sprite[0x0000], 0x4000, 0x8eb1c28e },
		{ "5338", NULL, &sprite[0x8000], 0x4000, 0x94ca5f9a },

		// color lookup prom
		{ "5279", NULL, &bankprom[0x000], 0x200, 0x8716aeb5 },

		// misc (unused) proms 
		{ "pr-5278.bin", NULL, &miscprom[0x000], 0x100, 0xe81613da },
		{ "pr-5277.bin", NULL, &miscprom[0x100], 0x100, 0xbf2c33ab },
		{ "pr-5276.bin", NULL, &miscprom[0x200], 0x20, 0x91267e8a },
		{ "pr-5275.bin", NULL, &miscprom[0x220], 0x20, 0x0c872a9b },
		{ NULL }
	};

	m_rom_list = astronh_roms;

}

galaxy::galaxy()
{
	m_shortgamename = "galaxy";
	m_game_type = GAME_GALAXY;
	m_num_sounds = 8;
	m_sound_name[S_GR_FIRE] = "gr_fire.wav";
	m_sound_name[S_GR_ENEMY_CANNON] = "gr_cannon.wav";
	m_sound_name[S_GR_ENEMY_MINION] = "gr_mineon.wav";
	m_sound_name[S_GR_ENEMY_ATTACK] = "gr_attack.wav";
	m_sound_name[S_GR_ALARM1] = "gr_alarm1.wav";
	m_sound_name[S_GR_ALARM2] = "gr_alarm2.wav";
	m_sound_name[S_GR_ALARM3] = "gr_alarm3.wav";
	m_sound_name[S_GR_ALARM4] = "gr_alarm4.wav";

	const static struct rom_def galaxy_roms[] =
	{
		// main z80 program ...
		{ "gr5633.bin", NULL, &m_cpumem[0x0000], 0x4000, 0x398f6b23 },
		{ "gr5634.bin", NULL, &m_cpumem[0x4000], 0x4000, 0x2c5be1b7 },

		// bank switching roms
		{ "gr5592.bin", NULL, &rombank[0x0000], 0x4000, 0xd13715f8 },
		{ "gr5592.bin", NULL, &m_cpumem[0x8000], 0x4000, 0xd13715f8 },
		{ "gr5593.bin", NULL, &rombank[0x4000], 0x4000, 0xb0a557aa },
	
		// sprites
		{ "gr5594.bin", NULL, &sprite[0x0000], 0x4000, 0x4403ef5a },
		{ "gr5611.bin", NULL, &sprite[0x4000], 0x4000, 0xb16bdfe4 },
		{ "gr5595.bin", NULL, &sprite[0x8000], 0x4000, 0xccbaec4f },
		{ "gr5612.bin", NULL, &sprite[0xc000], 0x4000, 0xe312d080 },

		// tile graphics
		{ "5280", "astron", &character[0x0000], 0x0800, 0x583af1ff },
		{ "5281", "astron", &character[0x0800], 0x0800, 0x7b5c820c },

		// color lookup prom
		{ "5279", "astron", &bankprom[0x000], 0x200, 0x8716aeb5 },

		// misc (unused) proms 
		{ "pr-5278.bin", "astron", &miscprom[0x000], 0x100, 0xe81613da },
		{ "pr-5277.bin", "astron", &miscprom[0x100], 0x100, 0xbf2c33ab },
		{ "pr-5276.bin", "astron", &miscprom[0x200], 0x20, 0x91267e8a },
		{ "pr-5275.bin", "astron", &miscprom[0x220], 0x20, 0x0c872a9b },
		{ NULL }
	};

	m_rom_list = galaxy_roms;			

}

cobraab::cobraab()
{
	m_shortgamename = "cobraab";
	banks[3] = 0xfb;

	m_video_row_offset = -8;	// shift video up by 16 pixels (8 rows)

	const static struct rom_def cobraab_roms[] =
	{

		{ "ic-1.bin", NULL, &m_cpumem[0x0000], 0x4000, 0x079783C7 },
		{ "ic-2.bin", NULL, &m_cpumem[0x4000], 0x2000, 0x40C0b825 },
	
		// tile graphics
		{ "ic-11.bin", NULL, &character[0x0000], 0x0800, 0x5A2E8F4E },
		{ "ic-12.bin", NULL, &character[0x0800], 0x0800, 0x4B89D7Ed },

		// sprites
		{ "ic-7.bin", NULL, &sprite[0x0000], 0x2000, 0x9E12B19C },
		{ "ic-8.bin", NULL, &sprite[0x8000], 0x2000, 0x201041C0 },

		// color lookup prom
		{ "ic-13.bin", NULL, &bankprom[0x000], 0x200, 0x3547a14c },

		// misc (unused) proms 
		{ "pr-5278.bin", "astron", &miscprom[0x000], 0x100, 0xe81613da },
		{ "pr-5277.bin", "astron", &miscprom[0x100], 0x100, 0xbf2c33ab },
		{ "pr-5276.bin", "astron", &miscprom[0x200], 0x20, 0x91267e8a },
		{ "pr-5275.bin", "astron", &miscprom[0x220], 0x20, 0x0c872a9b },
		{ NULL }
	};

	m_rom_list = cobraab_roms;

}

// does anything special needed to send an IRQ
void astron::do_irq(unsigned int which_irq)
{
	if (which_irq == 0)
	{
		// Redraws the screen (if needed) on interrupt
		recalc_palette();
		video_blit();
		Z80_ASSERT_IRQ;
	}
}

// does anything special needed to send an NMI
void astron::do_nmi()
{
	// only do an nmi if nmie is enabled
	if (m_cpumem[0xd801] & 0x40)
	{
		write_ldv1000(ldp_output_latch);
		ldp_input_latch = read_ldv1000();
		Z80_ASSERT_NMI;
	}
}

// we'll use this for clocking the 8251 since it generates the nmi's
void astronh::do_nmi()
{
	clock_8251();
}

// does anything special needed to send an NMI
void astronh::astronh_nmi()
{
	// only do an nmi if nmie is enabled
	if (m_cpumem[0xd801] & 0x40)
	{
//		printline("doin' the NMI");
		
//		z80_set_nmi_line(ASSERT_LINE);
//		z80_execute(1);	// execute a quick instruction
//		z80_set_nmi_line(CLEAR_LINE);
		Z80_ASSERT_NMI;
	}
}

// reads a byte from the cpu's memory
Uint8 astron::cpu_mem_read(Uint16 addr)
{
	Uint8 result = m_cpumem[addr];
//	char s[81] = { 0 };
	
	// main rom
	if (addr <= 0x7fff)
	{
	}

	// 0x8000 to 0xbfff is the bankswitching memory
	else if (addr >= 0x8000 && addr <= 0xbfff)
	{
//		sprintf(s, "Bank rom read at %x, rombank %x, current bank %x", addr, (addr - 0x8000) + (0x4000 * current_bank), current_bank);
//		printline(s);
		result = rombank[(addr - 0x8000) + (0x4000 * current_bank)];
	}

	else if (addr >= 0xc800 && addr <= 0xcfff)
	{
		result = read_ldp(addr);
//		sprintf(s, "LDP read %x", result);
//		printline(s);
	}
	else if (addr == 0xd000)
	{
		result = banks[2];
	}
	else if (addr == 0xd001)
	{
		result = banks[3];
	}
	else if (addr == 0xd002)
	{
		result = banks[0];
	}
	else if (addr == 0xd003)
	{
		result = banks[1];
	}
	
	// obj
	else if (addr >= 0xc000 && addr <= 0xc320)
	{
	}

	// out
	else if (addr >= 0xd800 && addr <= 0xdfff)
	{
	}

	// color
	else if (addr >= 0xe000 && addr <= 0xe1ff)
	{
	}

	// work ram
	else if (addr >= 0xf800 )
	{
	}

	else
	{
//		char s[81] = { 0 };
//		sprintf(s, "Unmapped read from %x", addr);
//		printline(s);
	}

	return result;
}

// writes a byte to the cpu's memory
void astron::cpu_mem_write(Uint16 addr, Uint8 value)
{
	m_cpumem[addr] = value;
	char s[81] = { 0 };

	// main rom
	if (addr <= 0x7fff)
	{
		sprintf(s, "Attempted write to main ROM! at %x with value %x", addr, value);
		printline(s);
	}

	// bank rom
	else if (addr >= 0x8000 && addr <= 0xbfff)
	{
		sprintf(s, "Attempted write to bank ROM! at %x with value %x", addr, value);
		printline(s);
	}

	// disc
	else if (addr >= 0xc800 && addr <= 0xcfff)
	{
//		sprintf(s, "LDP write %x", value);
//		printline(s);
		write_ldp(value, addr);
	}


	// obj (sprite)
	else if (addr >= 0xc000 && addr <= 0xc320)
	{
		m_cpumem[addr] = value;
		m_video_overlay_needs_update = true;
	}
	
	// out
	else if (addr == 0xd800)
	{
//		sprintf(s, "ASTRON: Write to D800 with %x", value);
//		printline(s);

		if (m_game_type == GAME_ASTRON)
		{
			if (((value & 0x0f) == 0x00) && !(value & 0x20))
			{
				sound_play(S_AB_PLAYER_SHIP);
			}
			else if ((value & 0x0f) == 0xb)
			{
				sound_play(S_AB_PLAYER_FIRE);
			}
			else if ((value & 0x0f) == 0xa)
			{
				sound_play(S_AB_ENEMY_FIRE);
			}
			else if ((value & 0x0f) == 0xf)
			{
				sound_play(S_AB_ALARM1);
			}
			else if ((value & 0x0f) == 0xe)
			{
				sound_play(S_AB_ALARM2);
			}
			else if ((value & 0x0f)== 0xd)
				{
				sound_play(S_AB_ALARM3);
			}
			else if ((value & 0x0f) == 0xc)
			{
				sound_play(S_AB_ALARM4);
			}
		}
		else if (m_game_type == GAME_GALAXY)
		{
			if ((value & 0x0f) == 0xb)
			{
				sound_play(S_GR_FIRE);
			}
			else if ((value & 0x0f) == 0x9)
			{
				sound_play(S_GR_ENEMY_CANNON);
			}
			else if ((value & 0x0f) == 0x8)
			{
				sound_play(S_GR_ENEMY_MINION);
			}
			else if ((value & 0x0f) == 0xa)
			{
				sound_play(S_GR_ENEMY_ATTACK);
			}
			else if ((value & 0x0f) == 0xf)
			{
				sound_play(S_GR_ALARM1);
			}
			else if ((value & 0x0f) == 0xe)
			{
				sound_play(S_GR_ALARM2);
			}
			else if ((value & 0x0f) == 0xd)
			{
				sound_play(S_GR_ALARM3);
			}
			else if ((value & 0x0f) == 0xc)
			{
				sound_play(S_GR_ALARM4);
			}
		}

		m_cpumem[addr] = value;
	}
	else if (addr == 0xd801)
	{
		m_cpumem[addr] = value;
	}
	else if (addr == 0xd802)
	{
		m_cpumem[addr] = value;
	}
	else if (addr == 0xd803)
	{
		m_cpumem[addr] = value;
	}

	// color
	else if (addr >= 0xe000 && addr <= 0xe1ff)
	{
		m_cpumem[addr] = value;
		palette_modified = true;
	}

	// fix (character)
	else if (addr >= BASE_VID_MEM_ADDRESS && addr <= BASE_VID_MEM_ADDRESS + 0x7ff)
	{
		if (addr < (BASE_VID_MEM_ADDRESS + 0x400))
		{
			m_cpumem[addr] = value;
		}
		else
		{
			m_cpumem[addr-0x400] = value;
		}
		m_video_overlay_needs_update = true;
	}

	// commented out because our final else does the same thing, so this is redundant
	/*
	// work ram
	else if (addr >= 0xf800 && addr <= 0xffff)
	{
		m_cpumem[addr] = value;
	}
	*/

	else
	{
			m_cpumem[addr] = value;
//			char s[81] = { 0 };
//			sprintf(s, "Unmapped write to %x with value %x", addr, value);
//			printline(s);
	}
}

Uint8 astron::read_ldp(Uint16 addr)
{
//	char s[81] = {0};
		
		Uint8 result = ldp_input_latch;
//		sprintf(s, "Read from player %x at pc: %x", result, Z80_GET_PC);
//		printline(s);

		return result;
}

Uint8 astronh::read_ldp(Uint16 addr)
{
	Uint8 result = 0x00;
	
	switch (addr & 0x01)
	{
	case 0x00:
		result = read_8251_data();
		break;
	case 0x01:
		result = read_8251_status();
		break;
	default:
		printline("ERROR: ASTRONH read_ldp()! check disc address");
		break;
	}

	return result;
}

void astron::write_ldp(Uint8 value, Uint16 addr)
{
//	char s[81] = {0};
	
//	sprintf(s, "Write to player %x at pc %x", value, Z80_GET_PC);
//	printline(s);
	ldp_output_latch = value;
}

void astronh::write_ldp(Uint8 value, Uint16 addr)
{
	switch (addr & 0x01)
	{
	case 0x00:
		write_8251_data(value);
		break;
	case 0x01:
		write_8251_control(value);
		break;
	default:
		printline("ERROR: ASTRONH write_ldp()! check disc address");
		break;
	}
}

// reads a byte from the cpu's port
Uint8 astron::port_read(Uint16 port)
{
	char s[81] = { 0 };

	port &= 0xFF;	
	sprintf(s, "ERROR: CPU port %x read requested, but this function is unimplemented!", port);
	printline(s);

	return(0);
}

// writes a byte to the cpu's port
void astron::port_write(Uint16 port, Uint8 value)
{
	char s[81] = { 0 };

	port &= 0xFF;
	
	switch (port & 0xFF)
	{
	case 0x00:	// astron switches rom banks with the D0 bit here
	case 0x01:	// at 0x01 too?
		current_bank = value & 0x01;
		break;
	default:
		sprintf(s, "ERROR: CPU port %x write requested (value %x) but this function is unimplemented!", port, value);
		printline(s);
		break;
	}
}

void astron::palette_calculate()
{
	// check how many colors are used in total to see if we can cheat and only use one 8 bit palette
	bool used_tile_color[256] = {0};
	int total_sprite_colors = 0;
	int total_tile_colors = 0;
	int x = 0;
	int y = 0;
	
	// first, check the sprites
	for (x = 0x0000; x < 0x8000; x++)
	{
		Uint8 data_lo = sprite[x];
		Uint8 data_high = sprite[x + 0x8000];
			
		used_sprite_color[((data_lo & 0x0f) | ((data_high << 0x04) & 0xff))] = true;
		used_sprite_color[((data_lo >> 0x04) | ((data_high & 0xf0) & 0xff))] = true;
	}
		
	// now, the tiles
	for (x = 0x000; x < 0x200; x++)
	{
		used_tile_color[bankprom[x]] = true;
	}

	// count up the total colors		
	for (x = 0; x < 256; x++)
	{
		if (used_sprite_color[x] == true)
		{
			total_sprite_colors++;
		}
		if (used_tile_color[x] == true)
		{
			total_tile_colors++;
		}
	}
		
	//	color conversion for all 2^12 possible colors
	for (int i = 0; i < 4096; i++)
	{
		Uint8 bit0,bit1,bit2,bit3;	

		// red component
		bit0 = static_cast<Uint8>((i >> 3) & 0x01);
		bit1 = static_cast<Uint8>((i >> 2) & 0x01);
		bit2 = static_cast<Uint8>((i >> 1) & 0x01);
		bit3 = static_cast<Uint8>((i >> 0) & 0x01);
		palette_lookup[i].r = static_cast<Uint8>((0x8f * bit0) + (0x43 * bit1) + (0x1f * bit2) + (0x0e * bit3));
		// green component
		bit0 = static_cast<Uint8>((i >> 7) & 0x01);
		bit1 = static_cast<Uint8>((i >> 6) & 0x01);
		bit2 = static_cast<Uint8>((i >> 5) & 0x01);
		bit3 = static_cast<Uint8>((i >> 4) & 0x01);
		palette_lookup[i].g = static_cast<Uint8>((0x8f * bit0) + (0x43 * bit1) + (0x1f * bit2) + (0x0e * bit3));
		// blue component
		bit0 = static_cast<Uint8>((i >> 11) & 0x01);
		bit1 = static_cast<Uint8>((i >> 10) & 0x01);
		bit2 = static_cast<Uint8>((i >> 9) & 0x01);
		bit3 = static_cast<Uint8>((i >> 8) & 0x01);
		palette_lookup[i].b = static_cast<Uint8>((0x8f * bit0) + (0x43 * bit1) + (0x1f * bit2) + (0x0e * bit3));
	}
		
	// if the total number of colors is under 0xff we can cheat and have them all in one palette
	if ((total_tile_colors + total_sprite_colors) < 0xff)
	{
		char s[81] = {0};
		sprintf(s, "total used colors 0x%x - compressing palette", total_tile_colors + total_sprite_colors);
		printline(s);
		compress_palette = true;

		// check each tile color and if we need it remap it to a sprite color
		for (x = 0; x < ASTRON_COLOR_COUNT; x++)
		{
			if (used_tile_color[x] == true)
			{
				while (used_sprite_color[y] == true)
				{
					y++;
				}
			
				mapped_tile_color[x] = y;
				palette_set_color(y, palette_lookup[((x & 0x07) << 1) | ((x & 0x38) << 2) | ((x & 0xc0) << 4)]);
					// since color 0 is moved we need to find our new transparent color
				if (x == 0)
				{
					palette_set_transparency(m_transparent_color, false);	// make old transparent color non-transparent
					m_transparent_color = y;
					palette_set_transparency(m_transparent_color, true);
				}
				y++;
			}
		}
	}
	else
	{
		char s[81] = {0};
		sprintf(s, "total used colors 0x%x - cannot compress palette!", total_tile_colors + total_sprite_colors);
		printline(s);
		compress_palette = false;
	}
}

void astron::recalc_palette()
{
	if (palette_modified)
	{
		m_video_overlay_needs_update = true;

		// load current palette with values from the lookup according to color ram
		for (int i = 0; i < 256; i++)
		{
			int j;
			j = 0xe000 + (i * 0x02);
			
			// only update the sprite colors if they are used
			if (used_sprite_color[i])
			{
				palette_set_color(i, palette_lookup[m_cpumem[j] | ((m_cpumem[j + 1] & 0x0f) << 8)]);
			}
		}

		palette_finalize();
	}
	palette_modified = false;
}

// updates astron's video
void astron::video_repaint()
{
	SDL_FillRect(m_video_overlay[m_active_video_overlay], NULL, m_transparent_color);

	// The sprites are bottom priority so we draw them first
	// START modified Mame code
	// check if sprites need to be drawn
	int spr_number,sprite_bottom_y,sprite_top_y;
	Uint8 *sprite_base;

	for (spr_number = 0;spr_number < 32;spr_number++)
	{
		sprite_base = m_cpumem + BASE_SPRITE_MEM_ADDRESS + 0x10 * spr_number;
		sprite_top_y = sprite_base[SPR_Y_TOP];
		sprite_bottom_y = sprite_base[SPR_Y_BOTTOM];
		if (sprite_bottom_y && (sprite_bottom_y-sprite_top_y > 0))
		{
			draw_sprite(spr_number);
		}
	}
	// END modified Mame code
		
	Uint8 pixel[8];

	// loop through video memory and draw characters
	for (int charx = 0; charx < 32; charx++)
	{
		for (int chary = 0; chary < 32; chary++)
		{
			for (int y = 0; y < 8; y++)
			{
				int current_character = chary * 32 + charx + BASE_VID_MEM_ADDRESS;
					
				pixel[0] = static_cast<Uint8>(bankprom[((character[m_cpumem[current_character]*8+y] & 0x80) >> 7) | ((character[m_cpumem[current_character]*8+y+0x800] & 0x80) >> 6) | (((m_cpumem[current_character]) & 0xf8) >> 0x01) | ((m_cpumem[0xd801] & 0x20) << 2)]);
				pixel[1] = static_cast<Uint8>(bankprom[((character[m_cpumem[current_character]*8+y] & 0x40) >> 6) | ((character[m_cpumem[current_character]*8+y+0x800] & 0x40) >> 5) | (((m_cpumem[current_character]) & 0xf8) >> 0x01) | ((m_cpumem[0xd801] & 0x20) << 2)]);
				pixel[2] = static_cast<Uint8>(bankprom[((character[m_cpumem[current_character]*8+y] & 0x20) >> 5) | ((character[m_cpumem[current_character]*8+y+0x800] & 0x20) >> 4) | (((m_cpumem[current_character]) & 0xf8) >> 0x01) | ((m_cpumem[0xd801] & 0x20) << 2)]);
				pixel[3] = static_cast<Uint8>(bankprom[((character[m_cpumem[current_character]*8+y] & 0x10) >> 4) | ((character[m_cpumem[current_character]*8+y+0x800] & 0x10) >> 3) | (((m_cpumem[current_character]) & 0xf8) >> 0x01) | ((m_cpumem[0xd801] & 0x20) << 2)]);
				pixel[4] = static_cast<Uint8>(bankprom[((character[m_cpumem[current_character]*8+y] & 0x08) >> 3) | ((character[m_cpumem[current_character]*8+y+0x800] & 0x08) >> 2) | (((m_cpumem[current_character]) & 0xf8) >> 0x01) | ((m_cpumem[0xd801] & 0x20) << 2)]);
				pixel[5] = static_cast<Uint8>(bankprom[((character[m_cpumem[current_character]*8+y] & 0x04) >> 2) | ((character[m_cpumem[current_character]*8+y+0x800] & 0x04) >> 1) | (((m_cpumem[current_character]) & 0xf8) >> 0x01) | ((m_cpumem[0xd801] & 0x20) << 2)]);
				pixel[6] = static_cast<Uint8>(bankprom[((character[m_cpumem[current_character]*8+y] & 0x02) >> 1) | ((character[m_cpumem[current_character]*8+y+0x800] & 0x02) >> 0) | (((m_cpumem[current_character]) & 0xf8) >> 0x01) | ((m_cpumem[0xd801] & 0x20) << 2)]);
				pixel[7] = static_cast<Uint8>(bankprom[((character[m_cpumem[current_character]*8+y] & 0x01) >> 0) | ((character[m_cpumem[current_character]*8+y+0x800] & 0x01) << 1) | (((m_cpumem[current_character]) & 0xf8) >> 0x01) | ((m_cpumem[0xd801] & 0x20) << 2)]);
			
				// draw the remapped colors instead of the regular if our palette is compressed
				if (compress_palette)
				{
					for (int x = 0; x < 8; x++)
					{
						if (pixel[x])
						{
							*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + ((chary * 8 + y) * ASTRON_OVERLAY_W) + (charx * 8 + x)) = mapped_tile_color[pixel[x]];
								
							//char s[81] = {0};
							//sprintf(s, "drawing color %x", mapped_tile_color[pixel[x]]);
							//printline(s);
						}
					}
				}
				else
				{
					for (int x = 0; x < 8; x++)
					{
						if (pixel[x])
						{
							*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + ((chary * 8 + y) * ASTRON_OVERLAY_W) + (charx * 8 + x)) = pixel[x];
						}
					}
				}					
			}
		}
	}
}

// this gets called when the user presses a key or moves the joystick
void astron::input_enable(Uint8 move)
{
	switch (move)
	{
	case SWITCH_UP:
		banks[1] &= ~0x08; 
		break;
	case SWITCH_LEFT:
		banks[1] &= ~0x02;
		break;
	case SWITCH_RIGHT:
		banks[1] &= ~0x01;
		break;
	case SWITCH_DOWN:
		banks[1] &= ~0x04; 
		break;
	case SWITCH_START1: // '1' on keyboard
		banks[0] &= ~0x10; 
		break;
	case SWITCH_START2: // '2' on keyboard
		banks[0] &= ~0x40;
		break;
	case SWITCH_BUTTON1: // space on keyboard
		banks[1] &= ~0x10;
		break;
	case SWITCH_COIN1: 
		banks[0] &= ~0x01;
		break;
	case SWITCH_COIN2: 
		banks[0] &= ~0x02;
		break;
	case SWITCH_SERVICE: 
		banks[0] &= ~0x08;
		break;
	case SWITCH_TEST: 
		banks[0] &= ~0x04;
		break;
	}
}  

// this gets called when the user releases a key or moves the joystick back to center position
void astron::input_disable(Uint8 move)
{
	switch (move)
	{
	case SWITCH_UP:
		banks[1] |= 0x08; 
		break;
	case SWITCH_LEFT:
		banks[1] |= 0x02;
		break;
	case SWITCH_RIGHT:
		banks[1] |= 0x01;
		break;
	case SWITCH_DOWN:
		banks[1] |= 0x04; 
		break;
	case SWITCH_START1: // '1' on keyboard
		banks[0] |= 0x10; 
		break;
	case SWITCH_START2: // '2' on keyboard
		banks[0] |= 0x40;
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
		banks[0] |= 0x08;
		break;
	case SWITCH_TEST: 
		banks[0] |= 0x04;
		break;
	}
}

// this gets called when the user presses a key or moves the joystick
void cobraab::input_enable(Uint8 move)
{
	switch (move)
	{
	case SWITCH_UP:
		banks[1] &= ~0x08; 
		break;
	case SWITCH_LEFT:
		banks[1] &= ~0x02;
		break;
	case SWITCH_RIGHT:
		banks[1] &= ~0x01;
		break;
	case SWITCH_DOWN:
		banks[1] &= ~0x04; 
		break;
	case SWITCH_START1: // '1' on keyboard
		banks[0] &= ~0x10; 
		break;
	case SWITCH_START2: // '2' on keyboard
		banks[0] &= ~0x20;
		break;
	case SWITCH_BUTTON1: // space on keyboard
		banks[1] &= ~0x10;
		break;
	case SWITCH_BUTTON2: // space on keyboard
		banks[0] &= ~0x40;
		break;
	case SWITCH_COIN1: 
		banks[0] &= ~0x01;
		break;
	case SWITCH_COIN2: 
		banks[0] &= ~0x02;
		break;
	case SWITCH_SERVICE: 
		banks[0] &= ~0x08;
		break;
	case SWITCH_TEST: 
		banks[0] &= ~0x04;
		break;
	}
}  

// this gets called when the user releases a key or moves the joystick back to center position
void cobraab::input_disable(Uint8 move)
{
	switch (move)
	{
	case SWITCH_UP:
		banks[1] |= 0x08; 
		break;
	case SWITCH_LEFT:
		banks[1] |= 0x02;
		break;
	case SWITCH_RIGHT:
		banks[1] |= 0x01;
		break;
	case SWITCH_DOWN:
		banks[1] |= 0x04; 
		break;
	case SWITCH_START1: 
		banks[0] |= 0x10; 
		break;
	case SWITCH_START2: 
		banks[0] |= 0x20;
		break;
	case SWITCH_BUTTON1: 
		banks[1] |= 0x10;
		break;
	case SWITCH_BUTTON2: 
		banks[0] |= 0x40;
		break;
	case SWITCH_COIN1: 
		banks[0] |= 0x01;
		break;
	case SWITCH_COIN2: 
		banks[0] |= 0x02;
		break;
	case SWITCH_SERVICE: 
		banks[0] |= 0x08;
		break;
	case SWITCH_TEST: 
		banks[0] |= 0x04;
		break;
	}
}

void cobraab::patch_roms()
{
	// if a cheat has been requested, modify the ROM. Note, this must be done after roms are loaded
	if (m_cheat_requested)
	{
		// infinite lives cheat
		m_cpumem[0x865] = 0; // NOP out code that decrements # of remaning lives
		m_cpumem[0x866] = 0;
		m_cpumem[0x867] = 0;
		printline("Cobraab infinite lives cheat enabled!");
	}
}

// used to set dip switch values
bool astron::set_bank(Uint8 which_bank, Uint8 value)
{
	bool result = true;
	
	switch (which_bank)
	{
	case 0:	// bank A
		banks[2] = (Uint8) (value ^ 0xFF);	// dip switches are active low
		break;
	case 1:	// bank B
		banks[3] = (Uint8) (value ^ 0xFF);	// switches are active low
		break;
	default:
		printline("ERROR: Bank specified is out of range!");
		result = false;
		break;
	}
	
	return result;
}

// START modified Mame code
void astron::draw_sprite(int spr_number)
{
	int sy,row,height,src;
	Uint8 *sprite_base;
	int skip;	/* bytes to skip before drawing each row (can be negative) */


	sprite_base	= m_cpumem + BASE_SPRITE_MEM_ADDRESS + 0x10 * spr_number;

	src = sprite_base[SPR_GFXOFS_LO] + (sprite_base[SPR_GFXOFS_HI] << 8);

	skip = sprite_base[SPR_SKIP_LO] + (sprite_base[SPR_SKIP_HI] << 8);

	height = sprite_base[SPR_Y_BOTTOM] - sprite_base[SPR_Y_TOP];

	sy = sprite_base[SPR_Y_TOP] + 1;

	for (row = 0; row < height; row++)
	{
		int x, y;
		int src2;

		src = src2 = src + skip;

		// draw the sprites 3 pixel to the left so the cobra command radar lines up correctly
		x = sprite_base[SPR_X_LO] + ((sprite_base[SPR_X_HI] & 0x01) << 8) - 3;
		y = sy+row;

		while (1)
		{
			int data_lo, data_high, data;

			data = sprite[src2 & 0x7fff];
			data_lo = sprite[src2 & 0x7fff];
			data_high = sprite[(src2 & 0x7fff) + 0x8000];

			// stop drawing when the sprite data is 0xff
			if ((data_lo == 0xff) && (data_high == 0xff)) 
			{
				break;
			}
			
			Uint8 pixel1 = (data_lo >> 0x04) | (data_high & 0xf0);
			Uint8 pixel2 = (data_lo & 0x0f) | (data_high << 0x04);

			// draw these guys backwards
			if (src & 0x8000)
			{
				Uint8 temp_pixel;
				temp_pixel = pixel1;
				pixel1 = pixel2;
				pixel2 = temp_pixel;
				
				src2--;
			}
			else
			{
				src2++;
			}

			// draw the pixel - don't draw if its black as to not cover up any tiles
			if (pixel1)
			{
				*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + (y * ASTRON_OVERLAY_W) + x) = pixel1;
			}
			x++;
			if (pixel2)
			{
				*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + (y * ASTRON_OVERLAY_W) + x) = pixel2;
			}
			x++;
		}
	}
}
// END modified Mame code

// very minimal 8251 implementation, just to get Astron working
void astronh::write_8251_data(Uint8 data)
{
	char s[81] = { 0 };
	sprintf(s, "ASTRONH: 8251_write_data() with %x", data);
//	printline(s);
	// writing to the 8251 resets txrdy
	txrdy = false;
	write_vip9500sg(data);
}

Uint8 astronh::read_8251_data(void)
{
	char s[81] = { 0 };
	Uint8 result = read_vip9500sg();
	sprintf(s, "ASTRONH: 8251_read_data with %x", result);
//	printline(s);
	// reading from the 8251 resets rxrdy
	rxrdy = false;
	return result;
}

void astronh::write_8251_control(Uint8 control)
{
	char s[81] = { 0 };
	static bool init = false;

	// initialize chip with first control instruction after reset
	if (!init)
	{
		printline("8251 Reset!");
		if (control == 0x8f)
		{
			printline("8251 mode selected as 1.5 stop bits/parity disabled/8 bit characters/64x baud factor");
		}
		else if (control == 0x4e)
		{
			printline("8251 mode selected as 1 stop bit/parity disabled/8 bit characters/16x baud factor");
		}
		else
		{
			sprintf(s, "8251 attempted to initialize with mode %x - unsupported", control);
			printline(s);
		}
		init = true;
	}
	// otherwise its a command
	else
	{
		if (control & 0x01)
		{
		//	printline("8251: Transmit Enable");
			if (!transmit_enable)
			{
				// when transmit is enabled it makes txrdy true
				transmit_enable = true;
				txrdy = true;
				astronh_nmi();
			}
		}
		else
		{
			//printline("8251: Transmit Disable");
			transmit_enable = false;
			txrdy = false;
		}
		
		// DTR pin is not connected on ASTRON
		//if (control & 0x02)
		//{
		//	printline("8251: /DTR = 0");
		//}
		//else
		//{
		//	printline("8251: /DTR = 1");
		//}

		if (control & 0x04)
		{
		//	printline("8251: Recieve Enable");
			recieve_enable = true;
		}
		else
		{
		//	printline("8251: Recieve Disable");
			recieve_enable = false;
		}
		
		// I don't think ASTRON uses the Break Character... we'll see
		if (control & 0x08)
		{
			printline("8251: Sent Break Character!");
		}
		else
		{
			//printline("8251: Break Character Normal Operation");
		}
		
		// I haven't seen astron reset the error flag yet
		if (control & 0x10)
		{
			printline("8251: Reset Error Flag!");
		}
		else
		{
//			printline("8251: Error Flag Normal Operation");
		}
		
		// not sure what this does
		if (control & 0x20)
		{
//			printline("8251: RTS = 0");
		}
		else
		{
			printline("8251: RTS = 1");
		}
		
		if (control & 0x40)
		{
			printline("8251: Internal Reset");
			init = false;
		}
		else
		{
		//	printline("8251: Reset Normal Operation");
		}
		
		// Hunt mode should only be enabled in Sync mode
		if (control & 0x80)
		{
			printline("8251 ERROR: Hunt Mode!");
		}
		else
		{
		}
	}	
}

Uint8 astronh::read_8251_status(void)
{
	// with astron all we care about are the bottom two bits of status (rxrdy, txrdy)
	Uint8 result = (0x00 | txrdy | (rxrdy << 0x01));

	return result;
}

void astronh::clock_8251(void)
{
	// periodically check the status of the 8251 

	// if we have data ready from the player inform the 8251
	if ((vip9500sg_result_ready()) && (!rxrdy) && (recieve_enable))
	{
		rxrdy = true;
		astronh_nmi();
	}
	// or if neither rxrdy or txrdy are active turn txrdy on
	else if ((!txrdy) && (transmit_enable))
	{
		txrdy = true;
		astronh_nmi();
	}
}

