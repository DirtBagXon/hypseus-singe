/*
 * laireuro.cpp
 *
 * Copyright (C) 2001-2007 Mark Broadhead
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


// laireuro.cpp
// by Mark Broadhead

#include <string.h>	// for memset
#include "laireuro.h"
#include "../cpu/cpu.h"
#include "../cpu/generic_z80.h"
#include "../ldp-in/vp932.h"
#include "../ldp-out/ldp.h"
#include "../io/conout.h"
#include "../sound/sound.h"
#include "../video/palette.h"

ctc_chip g_ctc;
dart_chip g_dart;
Uint8 g_int_vec;
Uint8 g_soundchip_id;

laireuro::laireuro()
{
	struct cpudef cpu;
	struct sounddef sound;

	memset(&g_ctc,0,sizeof(ctc_chip));
	
	m_shortgamename = "laireuro";
	memset(&cpu, 0, sizeof(struct cpudef));
	memset(m_banks, 0xFF, 4);	// m_banks are active low

	m_disc_fps = 25.0; 
	m_game_type = GAME_LAIREURO;

	cpu.type = CPU_Z80;
	cpu.hz = LAIREURO_CPU_HZ;
	cpu.nmi_period = 1000 / 50; 
	cpu.must_copy_context = false;
	cpu.mem = m_cpumem;
	add_cpu(&cpu);	

	cpu_change_interleave(100); // make it update the irqs every 1/3 of a ms

	m80_set_irq_callback(laireuro_irq_callback);

	memset(&sound,0,sizeof(sound));
    sound.type = SOUNDCHIP_TONEGEN;
	g_soundchip_id = add_soundchip(&sound);

	m_video_overlay_width = LAIREURO_OVERLAY_W;
	m_video_overlay_height = LAIREURO_OVERLAY_H;
	m_uVideoOverlayVisibleLines = LAIREURO_OVERLAY_H;	// since it's PAL, this must be explicitly set (576 / 2)
	m_palette_color_count = LAIREURO_COLOR_COUNT;

	ctc_init(1000.0 / LAIREURO_CPU_HZ, 0, 0, 2.0 * 1000.0 / LAIREURO_CPU_HZ, 0);

	const static struct rom_def laireuro_roms[] =
	{
		// z80 program
		{ "elu45.bin", NULL, &m_cpumem[0x0000], 0x2000, 0x4D3A9EAC },
		{ "elu46.bin", NULL, &m_cpumem[0x2000], 0x2000, 0x8479612B },
		{ "elu47.bin", NULL, &m_cpumem[0x4000], 0x2000, 0x6a66f6b4 },
		{ "elu48.bin", NULL, &m_cpumem[0x6000], 0x2000, 0x36575106 },

		// character tiles
		{ "elu33.bin", NULL, &m_character[0x0000], 0x2000, 0xe7506d96 },
		{ NULL }
	};

	m_rom_list = laireuro_roms;

}

aceeuro::aceeuro()
{
	m_shortgamename = "aceeuro";
//	m_game_type = GAME_ACEEURO;	
	
	const static struct rom_def aceeuro_roms[] =
	{
		// z80 program
		{ "sa_u45a.bin", NULL, &m_cpumem[0x0000], 0x2000, 0x41264d46 },
		{ "sa_u46a.bin", NULL, &m_cpumem[0x2000], 0x2000, 0xbc1c70cf },
		{ "sa_u47a.bin", NULL, &m_cpumem[0x4000], 0x2000, 0xff3f77c7 },
		{ "sa_u48a.bin", NULL, &m_cpumem[0x6000], 0x2000, 0x8c83ac81 },
		{ "sa_u49a.bin", NULL, &m_cpumem[0x8000], 0x2000, 0x03b58fc3 },

		// character tiles
		{ "sa_u33a.bin", NULL, &m_character[0x0000], 0x2000, 0xa8c14612 },
		{ NULL }
	};
	m_rom_list = aceeuro_roms;
}

void laireuro::do_irq(Uint32 which)
{
	if (g_ctc.channels[which].interrupt)
	{
		g_int_vec = g_ctc.int_vector | (which << 1);
		Z80_ASSERT_IRQ;
	}
	if (which == 2)
	{
		static bool send = true;
		if (send)
		{
			if (g_dart.transmit_int)
			{
            // signal transmit buffer empty
				g_int_vec = g_dart.int_vector | 0x08;
				Z80_ASSERT_IRQ;
			}
		}
		else
		{
			if (vp932_data_available())
			{
				g_int_vec = g_dart.int_vector | 0x0c;
				Z80_ASSERT_IRQ;
			}
		}
		send = !send;
	}
}

void laireuro::do_nmi()
{
	// Redraws the screen (if needed) on interrupt
	video_blit();

	// Italian DL doesn't like it if coins held too long
    if (~(m_banks[1] & 0x04))
	{
		static int coinCount = 0;
		if (coinCount >= 6)
		{
			m_banks[1] |= 0x04;
			coinCount = 0;
		}
		coinCount++;
	}
    if (~(m_banks[1] & 0x08))
	{
		static int coinCount = 0;
		if (coinCount >= 6)
		{
			m_banks[1] |= 0x08;
			coinCount = 0;
		}
		coinCount++;
	}
}

Uint8 laireuro::cpu_mem_read(Uint16 addr)
{
	Uint8 result = m_cpumem[addr];

	// main rom (must be > 0 because it's unsigned)
	if (addr <= 0x9fff)
	{
	}

	// ram
	else if (addr >= 0xa000 && addr <= 0xa7ff)
	{
	}

	// video ram
	else if (addr >= 0xc000 && addr <= 0xc7ff)
	{
		// MPO : video shouldn't change on a READ.  I'm assuming this was an oversight.
		//m_video_overlay_needs_update = true;
	}
	
	// ?
	else if (addr == 0xe000)
	{
		result = 0;
	}
	// sel cp a
	else if (addr >= 0xe080 && addr <= 0xe087)
	{
		result = m_banks[0];
	}
	// sel cp b
	else if (addr >= 0xe088 && addr <= 0xe08f)
	{
		result = m_banks[1];
		// bit 6 is enter/status-
		// bit 7 is ready/command-
		result &= 0x3f;
	}	
	// sel op switch a
	else if (addr >= 0xe090 && addr <= 0xe097)
	{
		result = m_banks[2];
	}		
	// sel op switch b
	else if (addr >= 0xe098 && addr <= 0xe09f)
	{
		result = m_banks[3];
	}		
	// read disc data
	else if (addr >= 0xe0a0 && addr <= 0xe0a7)
	{
	}	
	else
	{
		char s[81] = { 0 };
		sprintf(s, "Unmapped read from %x", addr);
		printline(s);
	}

	return result;
}

void laireuro::cpu_mem_write(Uint16 addr, Uint8 value)
{	
	char s[81] = {0};

	// main rom (must be > 0 because it's unsigned)
	if (addr <= 0x9fff)
	{
		sprintf(s,"ERROR: WRITE TO MAIN ROM at %x with %x(PC is %x)",addr, value,Z80_GET_PC);
//		printline(s);
	}

	// ram
	else if (addr >= 0xa000 && addr <= 0xa7ff)
	{
		m_cpumem[addr] = value;
	}

	// video ram
	else if (addr >= 0xc000 && addr <= 0xc7ff)
	{
		m_video_overlay_needs_update = true;
		m_cpumem[addr] = value;
	}
	
	// wt led 1
	else if (addr == 0xe000)
	{
	}
	// wt led 2
	else if (addr == 0xe008)
	{
	}	
	// wt ext led 1
	else if (addr == 0xe010)
	{
	}		
	// wt ext led 2
	else if (addr == 0xe018)
	{
	}		
	// disc wt
	else if (addr == 0xe020)
	{
	}	
	// wt misc
	else if (addr == 0xe028)
	{
		m_wt_misc = value;
		m_video_overlay_needs_update = true;
		m_cpumem[addr] = value;
	}	
	// watchdog reset
	else if (addr == 0xe030)
	{
	}

	// ?
	else if (addr == 0xe036)
	{
	}

	// ?
	else if (addr == 0xe037)
	{
	}

	else
	{
		char s[81] = { 0 };
		sprintf(s, "Unmapped write to %x with %x", addr, value);
		printline(s);
	}
}

void laireuro::port_write(Uint16 port, Uint8 value)
// Called whenever the emulator wants to output to a port
{

	char s[81] = { 0 };

	port &= 0xFF;	// strip off high byte

	switch(port)
	{
	// CTC I/O
	case 0x00:	
	case 0x01:	
	case 0x02:	
	case 0x03:	
		ctc_write(port & 0xff, value);
		break;
	// UART (SIO) 
	case 0x80:
	case 0x81:
	case 0x82:
	case 0x83:
		dart_write((port >> 1) & 0x01, port & 0x01, value);
		break;
	default:
		sprintf(s, "LAIREURO: Unsupported Port Output-> %x:%x (PC is %x)", port, value, Z80_GET_PC);
		printline(s);
		break;

	}		
}

Uint8 laireuro::port_read(Uint16 port)
// Called whenever the emulator wants to read from a port
{

    char s[81] = { 0 };
    unsigned char result = 0;

	port &= 0xFF;	// strip off high byte

    switch(port)
    {
	// CTC I/O
	case 0x00:	
	case 0x01:	
	case 0x02:	
	case 0x03:	
		result = ctc_read(port & 0xff);
		break;
	// UART (SIO) 
	case 0x80:
		result = vp932_read();
		break;
	case 0x81:
	case 0x82:
	case 0x83:
	default:
		sprintf(s, "LAIREURO: Unsupported Port Input-> %x (PC is %x)", port, Z80_GET_PC);
		printline(s);
		break;
    }

    return(result);

}

void laireuro::palette_calculate()
{
	SDL_Color colors[LAIREURO_COLOR_COUNT];

	colors[0].r = 0;
	colors[0].g = 0;
	colors[0].b = 0;

	colors[1].r = 255;
	colors[1].g = 0;
	colors[1].b = 0;
 		
	colors[2].r = 0;
	colors[2].g = 255;
	colors[2].b = 0;
	
	colors[3].r = 255;
	colors[3].g = 255;
	colors[3].b = 0;
	
	colors[4].r = 0;
	colors[4].g = 0;
	colors[4].b = 255;
	
	colors[5].r = 255;
	colors[5].g = 0;
	colors[5].b = 255;
	
	colors[6].r = 0;
	colors[6].g = 255;
	colors[6].b = 255;
	
	colors[7].r = 255;
	colors[7].g = 255;
	colors[7].b = 255;

	for (int x = 0; x < LAIREURO_COLOR_COUNT; x++)
	{
      palette_set_color(x, colors[x]);
	}
}

// updates laireuro's video
void laireuro::video_repaint()
{
	int charx_offset = 0;
	int chary_offset = 0;
	for (int charx = charx_offset; charx < 21 + charx_offset; charx++)
	{
		for (int chary = chary_offset; chary < 9 + chary_offset; chary++)
		{
			int y = 0;
			for (y = 0; y < 16; y++)
			{
				int x = 0;
				for (x = 0; x < 8; x++)
				{
					// bit 0 of wt misc turns on/off character generator
					if (m_wt_misc & 0x04)
					{
                  Uint8 pixel = static_cast<Uint8>(m_character[(m_cpumem[chary * 64 + charx * 2 + 0xc001]*16+y) | ((m_wt_misc & 0x02) << 11)] & (0x01 << x));
						*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + (((chary - chary_offset) * 32 + y * 2) * LAIREURO_OVERLAY_W) + (charx - charx_offset) * 16 + (x * 2) + (charx - charx_offset)) = pixel?m_cpumem[chary * 64 + charx * 2 + 0xc002]:((m_wt_misc & 0x02)?0:4);
						*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + (((chary - chary_offset) * 32 + y * 2) * LAIREURO_OVERLAY_W) + (charx - charx_offset) * 16 + (x * 2) + 1 + (charx - charx_offset)) = pixel?m_cpumem[chary * 64 + charx * 2 + 0xc002]:((m_wt_misc & 0x02)?0:4);
						*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + (((chary - chary_offset) * 32 + y * 2 + 1) * LAIREURO_OVERLAY_W) + (charx - charx_offset) * 16 + (x * 2) + (charx - charx_offset)) = pixel?m_cpumem[chary * 64 + charx * 2 + 0xc002]:((m_wt_misc & 0x02)?0:4);
						*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + (((chary - chary_offset) * 32 + y * 2 + 1) * LAIREURO_OVERLAY_W) + (charx - charx_offset) * 16 + (x * 2) + 1 + (charx - charx_offset)) = pixel?m_cpumem[chary * 64 + charx * 2 + 0xc002]:((m_wt_misc & 0x02)?0:4);
					}
					else 
					{
						*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + (((chary - chary_offset) * 32 + y * 2) * LAIREURO_OVERLAY_W) + (charx - charx_offset) * 16 + (x * 2) + (charx - charx_offset)) = 0;
						*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + (((chary - chary_offset) * 32 + y * 2) * LAIREURO_OVERLAY_W) + (charx - charx_offset) * 16 + (x * 2) + 1 + (charx - charx_offset)) = 0;
						*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + (((chary - chary_offset) * 32 + y * 2 + 1) * LAIREURO_OVERLAY_W) + (charx - charx_offset) * 16 + (x * 2) + (charx - charx_offset)) = 0;
						*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + (((chary - chary_offset) * 32 + y * 2 + 1) * LAIREURO_OVERLAY_W) + (charx - charx_offset) * 16 + (x * 2) + 1 + (charx - charx_offset)) = 0;
					}
				}
				*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + (((chary - chary_offset) * 32 + y * 2) * LAIREURO_OVERLAY_W) + (charx - charx_offset) * 16 + (x * 2) + (charx - charx_offset)) = ((m_wt_misc & 0x04)?4:0);
				*((Uint8 *) m_video_overlay[m_active_video_overlay]->pixels + (((chary - chary_offset) * 32 + y * 2 + 1) * LAIREURO_OVERLAY_W) + (charx - charx_offset) * 16 + (x * 2) + (charx - charx_offset)) = ((m_wt_misc & 0x04)?4:0);
			}
		}
	}
}

// this gets called when the user presses a key or moves the joystick
void laireuro::input_enable(Uint8 move)
{
	switch (move)
	{
	case SWITCH_UP:
		m_banks[0] &= ~0x01; 
		break;
	case SWITCH_LEFT:
		m_banks[0] &= ~0x04;
		break;
	case SWITCH_RIGHT:
		m_banks[0] &= ~0x08;
		break;
	case SWITCH_DOWN:
		m_banks[0] &= ~0x02; 
		break;
	case SWITCH_BUTTON1: 
		m_banks[0] &= ~0x10; 
		break;
	case SWITCH_START1: 
		m_banks[1] &= ~0x01;
		break;
	case SWITCH_START2: 
		m_banks[1] &= ~0x02;
		break;
	case SWITCH_COIN1: 
		m_banks[1] &= ~0x04;
		break;
	case SWITCH_COIN2: 
		m_banks[1] &= ~0x08;
		break;
	case SWITCH_TEST: 
		m_banks[0] &= ~0x80;
		break;
	default:
		printline("Error, bug in move enable");
		break;
	}
}  

// this gets called when the user releases a key or moves the joystick back to center position
void laireuro::input_disable(Uint8 move)
{
	switch (move)
	{
	case SWITCH_UP:
		m_banks[0] |= 0x01; 
		break;
	case SWITCH_LEFT:
		m_banks[0] |= 0x04;
		break;
	case SWITCH_RIGHT:
		m_banks[0] |= 0x08;
		break;
	case SWITCH_DOWN:
		m_banks[0] |= 0x02; 
		break;
	case SWITCH_BUTTON1: 
		m_banks[0] |= 0x10; 
		break;
	case SWITCH_START1: 
		m_banks[1] |= 0x01;
		break;
	case SWITCH_START2: 
		m_banks[1] |= 0x02;
		break;
	case SWITCH_COIN1: 
		m_banks[1] |= 0x04;
		break;
	case SWITCH_COIN2: 
		m_banks[1] |= 0x08;
		break;
	case SWITCH_TEST: 
		m_banks[0] |= 0x80;
		break;
	default:
		printline("Error, bug in move enable");
		break;
	}
}

Sint32 laireuro_irq_callback(int irqline)
{
	m80_set_irq_line(CLEAR_LINE);
	return g_int_vec;
}

void ctc_write(Uint8 channel, Uint8 value)
{
#ifdef DEBUG
	char s[81] = "";
#endif // DEBUG

	// load a time constant
	if (g_ctc.channels[channel].load_const)
	{
		g_ctc.channels[channel].load_const = false;
		g_ctc.channels[channel].time_const = value;
		ctc_update_period(channel);
#ifdef DEBUG		
		sprintf(s,"CTC time constant of %x loaded on channel %x", g_ctc.channels[channel].time_const, channel);
		printline(s);
#endif
	}
	// only channel 0 can set the vector
	else if (!(value & 0x01) && channel == 0)
	{
		g_ctc.int_vector = value & 0xf8;
#ifdef DEBUG		
		sprintf(s,"CTC interrupt vector set to %x", g_ctc.int_vector);
		printline(s);
#endif
	}
	// it's a control word
	else
	{
		g_ctc.channels[channel].interrupt = (value & 0x80) ? true:false;
		g_ctc.channels[channel].mode = (value & 0x40) ? COUNTER:TIMER;
		g_ctc.channels[channel].prescaler = (value & 0x20) ? 256:16;
		g_ctc.channels[channel].load_const = (value & 0x04) ? true:false;
		
		
		if (g_ctc.channels[channel].interrupt)
		{
#ifdef DEBUG		
			sprintf(s,"CTC interrupt enabled on channel %x", channel);
			printline(s);
#endif
		}
		// Reset
		if (value & 0x02)
		{
			g_ctc.channels[channel].time_const = 0;
         cpu_change_irq(0, channel, 0);
		}
		ctc_update_period(channel);
	}
}

Uint8 ctc_read(Uint8 channel)
{	
	return g_ctc.channels[channel].time_const;
}

void dart_write(bool b, bool command, Uint8 data)
{
	char s[81] = "";
	if (command)
	{
		switch (g_dart.next_reg)
		{
		case 0:
			g_dart.next_reg = data & 0x07;
			switch ((data >> 3) & 0x07)
			{
			case 0:
				break;
			case 1:
				break;
			case 2:
				break;
			case 3:
				break;
			case 4:
				break;
			case 5:
				break;
			case 6:
				break;
			case 7:
				break;
			}
			break;
		case 1:
			g_dart.next_reg = 0;
			g_dart.ext_int = (data >> 0) & 0x01;
			g_dart.transmit_int = (data >> 1) & 0x01;
			break;
		case 2:
         // only channel b can set the vector
         if (b)
			{
				g_dart.int_vector = data;
#ifdef DEBUG		
				sprintf(s,"DART interrupt vector set to %x", g_ctc.int_vector);
				printline(s);
#endif	
			}
			g_dart.next_reg = 0;
			break;
		case 3:
			g_dart.next_reg = 0;
			break;
		case 4:
			g_dart.next_reg = 0;
			sprintf(s,"DART register 4 written with %x", data);
			printline(s);
			break;
		case 5:
			g_dart.next_reg = 0;
			break;
		case 6:
			g_dart.next_reg = 0;
			break;
		case 7:
			g_dart.next_reg = 0;
			break;
		}
	}
	else
	{
		if (data && !b)
		{
			vp932_write(data);
		}
		else 
		{
		}
	}
}

void ctc_init(double clock, double trig0, double trig1, double trig2, double trig3)
{
	g_ctc.clock = clock;
	g_ctc.channels[0].trig = trig0;
	g_ctc.channels[1].trig = trig1;
	g_ctc.channels[2].trig = trig2;
	g_ctc.channels[3].trig = trig3;
}

void ctc_update_period(Uint8 channel)
{
	double new_period = 0;
	if (g_ctc.channels[channel].mode == TIMER)
	{
		new_period = g_ctc.channels[channel].time_const * g_ctc.channels[channel].prescaler * g_ctc.clock;
	}
	else
	{
		new_period = g_ctc.channels[channel].time_const * g_ctc.channels[channel].trig;
	}
	
	if (g_ctc.channels[channel].interrupt)
	{
		cpu_change_irq(0, channel, new_period);
#ifdef DEBUG		
		char s[81] = { 0 };
		sprintf(s,"Set up Irq %x with period of %f", channel , new_period);
		printline(s);
#endif
	}
	else 
	{
		// when interrupts are disabled...
		cpu_change_irq(0, channel, 0);
		if (channel == 2)
		{
         // the baud rate gets divided by the UART by 16, but we need two interrupts per 
			// transaction, one for input and one for output. Also baud is bits per second,
			// and we need bytes per second.
			cpu_change_irq(0, 2, new_period * 16 * 8 / 2);
		}
		if (channel == 0) // sound
		{
			audio_write_ctrl_data(0, (Uint32)(1000 / new_period / 2), g_soundchip_id);
		}
#ifdef DEBUG
		char s[81] = { 0 };
		sprintf(s,"Set up Irq %x with period of %f", channel ,0);
		printline(s);
#endif
	}
}

void laireuro::set_version(int version)
{
	if (version == 1) 
	{
		//already loaded, do nothing
	}
	else if (version == 2)  // italian version
	{
		m_shortgamename = "lair_ita";
		const static struct rom_def lair_ita_roms[] =
		{
		// z80 program
		{ "lair_ita_45.bin", NULL, &m_cpumem[0x0000], 0x2000, 0x2ed85958 },
		{ "elu46.bin", "laireuro", &m_cpumem[0x2000], 0x2000, 0x8479612b },	// the same as laireuro
		{ "elu47.bin", "laireuro", &m_cpumem[0x4000], 0x2000, 0x6a66f6b4 },	// the same as laireuro
		{ "lair_ita_48.bin", NULL, &m_cpumem[0x6000], 0x2000, 0x0c0b3011 },

		// character tiles
		{ "elu33.bin", "laireuro", &m_character[0x0000], 0x2000, 0xe7506d96 },	// same as laireuro
		{ NULL }
		};

		m_rom_list = lair_ita_roms;
	}
	else if (version == 3)
	{
		m_shortgamename = "lair_d2";
		const static struct rom_def lair_d2_roms[] =
		{
			// z80 program
			{ "elu45_d2.bin", NULL, &m_cpumem[0x0000], 0x2000, 0x329b354a },
			{ "elu46.bin", "laireuro", &m_cpumem[0x2000], 0x2000, 0x8479612B },
			{ "elu47.bin", "laireuro", &m_cpumem[0x4000], 0x2000, 0x6a66f6b4 },
			{ "elu48.bin", "laireuro", &m_cpumem[0x6000], 0x2000, 0x36575106 },	

			// character tiles
			{ "elu33.bin", "laireuro", &m_character[0x0000], 0x2000, 0xe7506d96 },
			{ NULL }
		};

		m_rom_list = lair_d2_roms;
	}
	else
	{
		printline("Unsupported -version paramter, ignoring...");
	}
}	

// used to set dip switch values
bool laireuro::set_bank(Uint8 which_bank, Uint8 value)
{
	bool result = true;
	
	switch (which_bank)
	{
	case 0:	// bank A
		m_banks[2] = ~value;	
		break;
	case 1:	// bank B
		m_banks[3] = ~value;
		break;
	default:
		printline("ERROR: Bank specified is out of range!");
		result = false;
		break;
	}
	
	return result;
}
