/*
 * lair2.cpp
 *
 * Copyright (C) 2003 Paul Blagay
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

// Dragon's Lair 2  / Space Ace '91 Driver
// Coded by Paul Blagay
// Thanks to Robert DiNapoli
// Fabrice Frances & MAME team for I86 core

/*

DL2 source code notes:

The main() function is at 0x13544 (once the startup code relocates the ROM to its new home)

#defines:
OUR_PCB
COIN_CO_PROC
IO_PORT = 0x201
COIN_PORT = 0x202
CNTL_PORT = 0x202
NORMAL_MODE = 0x20

*/

#ifdef WIN32
#pragma warning (disable:4100) // disable warning about unreferenced parameter
#endif

// Win32 doesn't use strcasecmp, it uses stricmp (lame)
#ifdef WIN32
#define strcasecmp stricmp
#endif

#ifdef _XBOX
#include "../xbox/xbox_grafx.h"
#include "../cpu/mamewrap.h"
#endif

#include <stdio.h> 
#include "lair2.h"
#include "../ldp-out/ldp.h"
#include "../ldp-in/ldp1000.h"
#include "../ldp-in/vp932.h"
#include "../io/conout.h"
#include "../io/error.h"
#include "../io/serial.h"	// for controlling a real LDP-1450
#include "../sound/sound.h"
#include "../cpu/cpu.h"
#include "../daphne.h"
#include "../io/sram.h"
#include "../timer/timer.h"	// for debugging
#ifndef _XBOX
#include "../cpu/x86/i86intf.h"
#else
#include "../cpu/i86intf.h"
#endif

#ifdef _XBOX
#include "../xbox/daphne_xbox.h"
#include "../xbox/xfile.h"
#include "../xbox/gamepad.h"
#endif

#ifdef DEBUG
#include "../io/numstr.h"
bool g_bVerbose = false;	// for debugging only
#endif

// Dragons Lair 2/Space Ace 91: Sound Samples
int g_sound_map[] = { S_DL2_WARBLE, 0x9, 
					   S_DL2_ERROR, 0x0, 
					    S_DL2_GOOD, 0x5, 
						 S_DL2_BAD, 0x26, 
						 S_DL2_TIC, 0x4,
						 S_DL2_TOC, 0xa,
						S_DL2_WARN, 0x19,
					   S_DL2_COIN1, 0x3,
					   S_DL2_COIN2, 0x3,
					   S_DL2_COIN3, 0x3,
					   S_DL2_COIN4, 0x3, 
						S_SA91_TIC, 0xb,
						S_SA91_TOC, 0x7	};

int lair2_irq_callback(int irqline);

///////////////////////////
// Debug Stuff

#ifdef CPU_DEBUG

// Dragon's Lair 2 v3.19 function/variable names for debugging
struct addr_name lair2_319_addr_names[] =
{
	{ "_dos_setvect", 0x10112 },
	{ "make_sound", 0x10d6e },
	{ "LDP_Init", 0x10FB6 },
	{ "Fault", 0x10F72 },
	{ "collect_all_treasure_nodes", 0x11f46 },
	{ "start_game", 0x1334a },
	{ "play_game", 0x133bc },
	{ "com_int_2", 0x1354e },
	{ "setvects", 0x135b2 },
	{ "InitSerial", 0x13628 },
	{ "SetSerial", 0x138ec },
	{ "main", 0x13544 },
	{ "eep_setup", 0x140f6 },
	{ NULL, 0 }
};

#endif


////////////////////////////////////////////////////////////////////////////
// Rom structs

// Dragons Lair 2   v2.11
static struct rom_def g_lair2_211_roms[] =
{
	{ "dl2_211.bin", NULL, 0, 0x10000, 0x9f2660a3 },
	{ NULL }
};

// Dragons Lair 2   v3.00
static struct rom_def g_lair2_300_roms[] =
{
	{ "dl2_300.bin", NULL, 0, 0x10000, 0xdec4f2e3 },
	{ NULL }
};

// Dragons Lair 2   v3.14
static struct rom_def g_lair2_314_roms[] =
{
	{ "dl2_314.bin", NULL, 0, 0x10000, 0xaf92b612 },
	{ NULL }
};

// Dragons Lair 2   v3.15
static struct rom_def g_lair2_315_roms[] =
{
	{ "dl2_315.bin", NULL, 0, 0x10000, 0x13ec0600 },
	{ NULL }
};

// MPO : NOTE : v3.17 has the same CRC as v3.19 therefore it is the same ROM
//  This either means there is no v3.17, or we have the wrong v3.17 :)
// Dragons Lair 2   v3.17
//static struct rom_def g_lair2_317_roms[] =
//{
//	{ "dl2_317.bin", NULL, 0, 0x10000, 0xe9453a1b },
//	{ NULL }
//};

// Dragons Lair 2   v3.18
static struct rom_def g_lair2_318_roms[] =
{
	{ "dl2_318.bin", NULL, 0, 0x10000, 0x64706492 },
	{ NULL }
};

// Dragons Lair 2   v3.19
static struct rom_def g_lair2_319_roms[] =
{
	{ "dl2_319.bin", NULL, 0, 0x10000, 0xe9453a1b },
	{ NULL }
};

// Dragons Lair 2   v3.16 Euro
static struct rom_def g_lair2_316e_roms[] =
{
	{ "lair2_316_euro.bin", NULL, 0, 0x10000, 0xd68f1b13 },
	{ NULL }
};

// Dragons Lair 2   v3.19 Euro
static struct rom_def g_lair2_319e_roms[] =
{
	{ "dl2euro3.19.bin", NULL, 0, 0x10000, 0xcc23ad9f },
	{ NULL }
};

// Dragons Lair 2   v3.19 Spanish
static struct rom_def g_lair2_319es_roms[] =
{
	{ "lair2_319_span.bin", NULL, 0, 0x10000, 0x4b9a811d},
	{ NULL }
};

// Space Ace 91
static struct rom_def g_ace91_roms[] =
{
	{ "ace91.bin", NULL, 0, 0x10000, 0xde93a213 },
	{ NULL }
};

// Space Ace 91 Euro
static struct rom_def g_ace91e_roms[] =
{
	{ "sa91euro1.3.bin", NULL, 0, 0x10000, 0x27dd0486 },
	{ NULL }
};

//  soundhook

// Dragons Lair 2  v211
int g_dl2vars_211[] =
{ 0x14CD0
};

// Dragons Lair 2  v300
int g_dl2vars_300[] =
{ 0x14BCE
};

// Dragons Lair 2  v314
int g_dl2vars_314[] =
{ 0x150FA
};

// Dragons Lair 2  v315
int g_dl2vars_315[] =
{ 0x15106
};

// Dragons Lair 2  v317
//int g_dl2vars_317[] =
//{ 0x156c2
//};

// Dragons Lair 2  v318
int g_dl2vars_318[] =
{ 0x154ca
};

// Dragons Lair 2  v319
int g_dl2vars_319[] =
{ 0x156c2
};

// Dragons Lair 2  v316 Euro
int g_dl2vars_316e[] =
{ 0x1509e
};

// Dragons Lair 2  v319 Euro
int g_dl2vars_319e[] =
{ 0x153aa
};

// Dragons Lair 2  v319 Euro Spanish
int g_dl2vars_319es[] =
{ 0x15666
};

// Space Ace 91  (not complete)
int g_ace91vars[] =
{ 0x1be1a
};

// Space Ace 91 Euro  (not complete)
int g_ace91evars[] =
{ 0x1bb94
};

int	*g_Dv = NULL;
int	g_dl2_ver = 0;
int g_dl2_euro = 0;

//////////////////////////////////////////////////////////////////////////

// lair2 class constructor (default the 3.19 roms)
lair2::lair2() :
	m_uSerialBufSize(0),	// the serial buffer starts off being empty
	m_serial_int_enabled(false),	// serial interrupts start off disabled
	m_real_1450(false),	// no real LDP-1450 is attached by default
	// serial hack isn't proper emulation, so it is disabled by default
	m_bSerialHack(false)
{
	m_shortgamename = "lair2";
	memset(m_cpumem, 0, CPU_MEM_SIZE);
	memset(EEPROM_9536, 0, 0x80);
	m_uCoinCount[0] = m_uCoinCount[1] = 0;
	banks[0] = 0xff; // bank 0 is active low
	banks[1] = 0x01; // bank 1 is active high - bit 0 is set for EEP

	m_game_uses_video_overlay = true;
	m_overlay_size_is_dynamic = true;
	m_video_overlay_count = 1;
	m_video_overlay_width = DL2_OVERLAY_W;
	m_video_overlay_height = DL2_OVERLAY_H;
	m_palette_color_count = 256;
	m_video_overlay[m_active_video_overlay] = NULL;

	struct cpudef cpu;
	memset(&cpu, 0, sizeof(struct cpudef));
	cpu.type = CPU_I88;
	cpu.hz = LAIR2_CPU_HZ;
	cpu.irq_period[0] = LAIR2_IRQ_PERIOD;
	cpu.irq_period[1] = ((1000.0)*(8+1))/9600.0;	// serial port IRQ (8 data bits, 1 stop bit, 1000 ms pre sec, 9600 bits per second
	cpu.nmi_period = 0.0;
	cpu.initial_pc = 0xFFFF0;	// the ROM copies itself to 1000:0000 and starts there, but the intial entry is here
	cpu.must_copy_context = false;
	cpu.mem = m_cpumem;

	m_EEPROM_9536 = true;
	m_EEPROM_9536_begin = &EEPROM_9536[0x00];
	m_nvram_filename = "lair2";
	m_nvram_size = 0x80;

	add_cpu(&cpu);	// add this cpu to the list (it will be our only one)

	// add our awesome sound chip!
	struct sounddef def;
	def.type = SOUNDCHIP_PC_BEEPER;
	def.hz = 0;	// not used
	m_soundchip_id = add_soundchip(&def);
	m_port61_val = 0;

	m_disc_fps = 29.97;
	m_game_type = GAME_LAIR2;
	ldp_status = 0x00;

	m_sample_trigger = false;

	m_num_sounds = 13;
	m_sound_name[S_DL2_WARBLE] = "dl2_warble.wav";
	m_sound_name[S_DL2_ERROR] = "dl2_error.wav";
	m_sound_name[S_DL2_GOOD] = "dl2_good.wav";
	m_sound_name[S_DL2_BAD] = "dl2_bad.wav";
	m_sound_name[S_DL2_TIC] = "dl2_tic.wav";
	m_sound_name[S_DL2_TOC] = "dl2_toc.wav";
	m_sound_name[S_DL2_COIN1] = "dl2_coin1.wav";
	m_sound_name[S_DL2_COIN2] = "dl2_coin2.wav";
	m_sound_name[S_DL2_COIN3] = "dl2_coin3.wav";
	m_sound_name[S_DL2_COIN4] = "dl2_coin4.wav";
	m_sound_name[S_DL2_WARN] = "dl2_warn.wav";
	m_sound_name[S_SA91_TIC] = "dl2_tic.wav";
	m_sound_name[S_SA91_TOC] = "dl2_toc.wav";

	// MPO:	we need to default to some version so it doesn't crash ...
	set_version(6);	// v3.19 is the newest DL2 rom so it makes sense to have that as default

#ifdef _XBOX
	set_version(GameOpts[GameInfo.GameIndex].RomVersion);
#endif

	// set mem (yes, we load in at 0xF0000 because the program entry point is at 0xFFFF0)
	g_lair2_211_roms[0].buf = g_lair2_300_roms[0].buf  = 
	g_lair2_314_roms[0].buf = g_lair2_315_roms[0].buf = 
	/*g_lair2_317_roms[0].buf  = */ g_lair2_318_roms[0].buf =
	g_lair2_319_roms[0].buf  = g_lair2_316e_roms[0].buf =
	g_lair2_319e_roms[0].buf = g_lair2_319es_roms[0].buf =
	g_ace91_roms[0].buf = g_ace91e_roms[0].buf = &m_cpumem[0xF0000];

	// ldp1450 text stuff
	memset ( &g_LDP1450_TextControl, 0, sizeof(ldp_text_control));

	// clear the ldp strings
	for (int i=0; i<3; i++)
	{
		g_LDP1450_Strings[i].String[0] = 0;
	}

	i86_set_irq_callback(lair2_irq_callback);
}

// Space Ace 91 class constructor
ace91::ace91()
{
	m_shortgamename = "ace91";
	m_nvram_filename = "ace91";
	m_game_type = GAME_ACE91;	

	m_rom_list = g_ace91_roms;
	g_dl2_euro = 0;
	g_Dv = g_ace91vars;

#ifdef _XBOX
	set_version(GameOpts[GameInfo.GameIndex].RomVersion);
#endif

}

void ace91::set_version(int version)
{
	{
		switch (version)
		{
		case	0:
			break;
		case	1:
			m_shortgamename = "ace91_euro";
			m_nvram_filename = "ace91_euro";
			m_rom_list = g_ace91e_roms;
			g_dl2_euro = 1;
			g_Dv = g_ace91evars;
			break;
		}
		g_dl2_ver = version;
	}
}

bool lair2::init()
{
	cpu_init();
	g_ldp->pre_play();	// the LDP-1450 automatically begins playback
	return true;
}

void lair2::set_version(int version)
{
	{
		switch (version)
		{
		case	0:		//315
			m_nvram_filename = m_shortgamename = "lair2_315";
			m_rom_list = g_lair2_315_roms;
			g_Dv = g_dl2vars_315;
			g_dl2_euro = 0;
			break;
		case	1:		//211
			m_nvram_filename = m_shortgamename = "lair2_211";
			m_rom_list = g_lair2_211_roms;
			g_Dv = g_dl2vars_211;
			g_dl2_euro = 0;
			break;
		case	2:		//300
			m_nvram_filename = m_shortgamename = "lair2_300";
			m_rom_list = g_lair2_300_roms;
			g_Dv = g_dl2vars_300;
			g_dl2_euro = 0;
			break;
		case	3:		//314
			m_nvram_filename = m_shortgamename = "lair2_314";
			m_rom_list = g_lair2_314_roms;
			g_Dv = g_dl2vars_314;
			g_dl2_euro = 0;
			break;
//		case	4:		//317
//			m_nvram_filename = m_shortgamename = "lair2_317";
//			m_rom_list = g_lair2_317_roms;
//			g_Dv = g_dl2vars_317;
//			g_dl2_euro = 0;
//			break;
		case	5:		//318
			m_nvram_filename = m_shortgamename = "lair2_318";
			m_rom_list = g_lair2_318_roms;
			g_Dv = g_dl2vars_318;
			g_dl2_euro = 0;
			break;
		default:		//319 (version 6 and the default)
			m_nvram_filename = "lair2_319";
			m_shortgamename = "lair2";	// default version, so it gets the default name
			m_rom_list = g_lair2_319_roms;
			g_Dv = g_dl2vars_319;
			g_dl2_euro = 0;
#ifdef CPU_DEBUG
			addr_names = lair2_319_addr_names;
#endif
			break;
		case	7:		//316 Euro
			m_nvram_filename = m_shortgamename = "lair2_316_euro";
			m_rom_list = g_lair2_316e_roms;
			g_Dv = g_dl2vars_316e;
			g_dl2_euro = 1;
			break;
		case	8:		//319 Euro
			m_nvram_filename = m_shortgamename = "lair2_319_euro";
			m_rom_list = g_lair2_319e_roms;
			g_Dv = g_dl2vars_319e;
			g_dl2_euro = 1;
			break;
		case	9:		//319 Spanish
			m_nvram_filename = m_shortgamename = "lair2_319_span";
			m_rom_list = g_lair2_319es_roms;
			g_Dv = g_dl2vars_319es;
			g_dl2_euro = 0;
			break;
		}
		g_dl2_ver = version;
	}
}

// game-specific command line arguments handled here
bool lair2::handle_cmdline_arg(const char *arg)
{
	bool bResult = true;

	// instead of emulating a LDP-1450, just pass the serial port commands through to the real serial port
	// This is for people who actually have a real 1450 and want to play DL2 on it
	if (strcasecmp(arg, "-real1450")==0)
	{
		m_real_1450 = true;
	}
	// Instead of generating serial port IRQ's, manually store serial data into the memory
	// (requires knowing memory locations in the ROM, so this probably will only be
	// supported on v3.19)
	else if (strcasecmp(arg, "-serialhack")==0)
	{
		// make sure we're using the right rom version to be able to handle this ...
		if (g_Dv == g_dl2vars_319)
		{
			m_bSerialHack = true;
		}
	}
	else bResult = false;

	return bResult;
}

void lair2::patch_roms()
{		
	#ifndef _XBOX
	if (strcasecmp(m_shortgamename, "lair2") == 0)
	#else
	if (strcmp(m_shortgamename, "lair2") == 0)
	#endif
	{
		switch (g_dl2_ver)
		{
		case	0:			//315
			// cut wait on coins
			//m_cpumem[0x1120f + 1] = 0x1;
			//m_cpumem[0x111fa] = 0x1;

			// return from eep funcs() - not needed.. 
			// these take FOREVER to execute.. seems to be
			// flash mem for saving stats
			// m_cpumem[0x13ec6] = 0xc3;
			// m_cpumem[0x13f0c] = 0xc3;
			break;
		case	1:			//211
			// cut down wait count for coins.
			//m_cpumem[0x11a3e + 1] = 0x1;
			//m_cpumem[0x11a27 + 1] = 0x1;

			// return from eep funcs() - not needed.. 
			// m_cpumem[0x146f4] = 0xc3;
			// m_cpumem[0x1473a] = 0xc3;
			break;
		case	2:			//300
			// cut down wait count for coins.
			//m_cpumem[0x111db + 1] = 0x1;
			//m_cpumem[0x111f2 + 1] = 0x1;

			// return from eep funcs() - not needed.. 
			// m_cpumem[0x13d04] = 0xc3;
			// m_cpumem[0x13d4a] = 0xc3;
			break;
		case	3:			//314
			// cut down wait count for coins.
			//m_cpumem[0x111f9 + 1] = 0x1;
			//m_cpumem[0x1120f + 1] = 0x1;

			// return from eep funcs() - not needed.. 
			// m_cpumem[0x13ec6] = 0xc3;
			// m_cpumem[0x13f0c] = 0xc3;
			break;
		case	4:			//317
			// cut down wait count for coins.
			//m_cpumem[0x11227 + 1] = 0x1;
			//m_cpumem[0x1123d + 1] = 0x1;

			// return from eep funcs() - not needed.. 
			// m_cpumem[0x140b0] = 0xc3;
			// m_cpumem[0x140f6] = 0xc3;
			break;
		case	5:		//318
			// cut down wait count for coins.
			//m_cpumem[0x111f9 + 1] = 0x1;
			//m_cpumem[0x1120f + 1] = 0x1;

			// return from eep funcs() - not needed.. 
			// m_cpumem[0x13eb4] = 0xc3;
			// m_cpumem[0x13efa] = 0xc3;
			break;
		case	6:   //319
			// cut down wait count for coins.
			//m_cpumem[0x11227 + 1] = 0x1;
			//m_cpumem[0x1123d + 1] = 0x1;

			// return from eep funcs() - not needed.. 
			// m_cpumem[0x140f6] = 0xc3;
			// m_cpumem[0x140b0] = 0xc3;

			// FIX compiler bugs that make laserdisc access problematic

			// FIX Send_Byte_2_LDP() function
			// NOTE : this first fix isn't necessary because the bug doesn't seem to cause any harm in this instance
			memmove(&m_cpumem[0xF0B93], &m_cpumem[0xF0B92], 85);	// make room for extra opcode
			// first jnz doesn't need to be changed 'cause label is inside moved code
			m_cpumem[0xF0b9a]--;	// jnz 10be8
			m_cpumem[0xF0ba0]--;	// call 10f20
			m_cpumem[0xf0ba7]--;	// jnz 10b82
			m_cpumem[0xf0bb1]--;	// call 13732
			m_cpumem[0xf0bbb]--;	// call 13642
			m_cpumem[0xf0bc5]--;	// call 10f20
			m_cpumem[0xf0bcc]--;	// call 10b7f
			m_cpumem[0xf0bd6]--;	// call 13732
			m_cpumem[0xf0be0]--;	// call 10f20
			m_cpumem[0xf0be6]--;	// call 10b46
			m_cpumem[0xf0b91] = 0xFE;	// INC AL instead of INC AX
			m_cpumem[0xf0b92] = 0xC0;

			// FIX Wait_4_LDP() function
			// This fix is necessary to get passed LDP initialization reliably
			memmove(&m_cpumem[0xF0C39], &m_cpumem[0xF0C38], 17);	// make room for extra opcode
			m_cpumem[0xF0c37] = 0xFE;	// INC AL instead of INC AX
			m_cpumem[0xF0c38] = 0xC0;
			m_cpumem[0xF0C3A]--;	// jnz 10C4A
			m_cpumem[0xF0C40]--;	// time_delay function
			m_cpumem[0xF0C47]--;	// jnz 10C28
			m_cpumem[0xF0C49]--;	// jmp 10c52
			break;
		case	7:    //316 euro
			// cut down wait count for coins.
			//m_cpumem[0x10ef9 + 1] = 0x1;
			//m_cpumem[0x10F0f + 1] = 0x1;

			// return from eep funcs() - not needed.. 
			// m_cpumem[0x13ba4] = 0xc3;
			// m_cpumem[0x13bea] = 0xc3;
			break;
		case	8:    //319 euro
			// cut down wait count for coins.
			//m_cpumem[0x10F27 + 1] = 0x1;
			//m_cpumem[0x10F3d + 1] = 0x1;

			// return from eep funcs() - not needed.. 
			// m_cpumem[0x13dae] = 0xc3;
			// m_cpumem[0x13df4] = 0xc3;
			break;
		case	9:    //319 euro spanish
			// cut down wait count for coins.
			//m_cpumem[0x11227 + 1] = 0x1;
			//m_cpumem[0x1123d + 1] = 0x1;

			// return from eep funcs() - not needed.. 
			// m_cpumem[0x14056] = 0xc3;
			// m_cpumem[0x1409c] = 0xc3;
			break;
		}
	}	
	else
	{
		switch (g_dl2_ver)
		{
		case	0:
			// cut down wait count for coins.
//			m_cpumem[0x0F28 + 1] = 0x1;
//			m_cpumem[0x0F3E + 1] = 0x1;

			// return from eep funcs() - not needed.. 
//			m_cpumem[0x3dae] = 0xc3;
//			m_cpumem[0x3df4] = 0xc3;
			break;
		case	1:
			// cut down wait count for coins.
//			m_cpumem[0x0F28 + 1] = 0x1;
//			m_cpumem[0x0F3E + 1] = 0x1;

			// return from eep funcs() - not needed.. 
//			m_cpumem[0x3dae] = 0xc3;
//			m_cpumem[0x3df4] = 0xc3;
			break;
		}
	}
}


///////////////////////////////////////////////////////////

int g_dl2_irq_val = 0;	// which IRQ the i86 code should activate
// this needs to be global because the callback needs to access it

// irqline will always be 0, it won't help us unfortunately
int lair2_irq_callback(int irqline)
{
	i86_set_irq_line(0, CLEAR_LINE);	// first order of business, clear the IRQ line
	return g_dl2_irq_val;
}

// raise IRQ line
void lair2::do_irq(unsigned int which_irq)
{
	if (m_bSerialHack)
	{
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
		while (ldp1000_result_ready())
		{
			// NOTE : only works on little endian!
			unsigned char u8Val = read_ldp1000();
			unsigned char *ccbuf2 = &m_cpumem[0x1A050];
			Sint16 *p_endbuf2 = (Sint16 *) &m_cpumem[0x1596E];
			Sint16 *p_RxCnt2 = (Sint16 *) &m_cpumem[0x1596A];

			// this code modeled after dragon's lair 2 rom source code
			ccbuf2[*p_endbuf2] = u8Val;
			*p_endbuf2 = (*p_endbuf2 + 1) % 0x100;
			++(*p_RxCnt2);
		}
#endif
	}

	// cpu timer IRQ
	if (which_irq == 0)
	{
		g_dl2_irq_val = 0x1C;	// the value of the TIMER_INT
		i86_set_irq_line(0, ASSERT_LINE);
	}

	// serial port IRQ (COM 2)
	else if ((which_irq == 1) && (!m_bSerialHack))
	{
		// if we have room in our buffer to receive more stuff
		if (m_uSerialBufSize < DL2_BUF_SIZE)
		{
			int serial_val = 0;

			// populate our buffer with all serial characters that are waiting
			while (serial_val != -1)
			{
				serial_val = -1;

				if (g_dl2_euro)
				{
					if (vp932_data_available())
					{
						serial_val = vp932_read();
					}
				}
				else
				{
					// if we're connected to a real 1450 ... read the value now
					if (m_real_1450)
					{
						// if there is a character waiting in the buffer to be read ...
						if (serial_rx_char_waiting())
						{
							serial_val = serial_rx();
						}
					}

					// otherwise just read from our emulated 1450
					else if (ldp1000_result_ready())
					{
						serial_val = read_ldp1000();
					}
					// else no char is available
				}

				// if we got a new character from the serial port, then add it to our buffer
				if (serial_val != -1)
				{
					m_u8SerialBuf[m_uSerialBufSize] = serial_val;
					++m_uSerialBufSize;
				}
			} // end while we have characters waiting to be read

			// if our buffer is not empty, generate an IRQ to encourage ROM to empty it :)
			// NOTE : after reviewing DL2 source code, it seems ok to generate interrupts
			//  if the buffer is not empty, even if no new character has been received.
			// Not only that, buf this seems like the only way to ensure that the buffer is
			//  emptied in a timely manner.
			if (m_uSerialBufSize > 0)
			{
				// if serial port should generate interrupts
				if (m_serial_int_enabled)
				{
					g_dl2_irq_val = 0x0B;	// COM2
					i86_set_irq_line(0, ASSERT_LINE);
				}
				// else wait until serial interrupts have been enabled
			}
		}
		// else we have no room
	}
   
	if (m_game_uses_video_overlay && m_video_overlay_needs_update)
	    video_blit(); 
}


void lair2::port_write(Uint16 port, Uint8 value)
{
	switch(port)
	{
		// ICR
	case 0x20:
		// 0x20 gets sent to port 0x20 when a byte has been read from the serial port ISR,
		//  to indicate an End-Of-Interrupt section (right before interrupts are re-enabled)
		if (value == 0x20)
		{
			// If there are more serial characters waiting, then generate an interrupt to
			//  get them processed immediately.  This will hopefully solve some latency
			//  problems.
			if (m_uSerialBufSize > 0)
			{
				// if serial port should generate interrupts
				if (m_serial_int_enabled)
				{
					g_dl2_irq_val = 0x0B;	// COM2
					i86_set_irq_line(0, ASSERT_LINE);
				}
				// else wait until serial interrupts have been enabled
			}

		}
		break;
		// sound control registers
	case 0x42:
	case 0x43:	
		if (value == 0xb6 && m_prefer_samples)
			m_sample_trigger = false;
	case 0x61:
		if (!m_prefer_samples)
			audio_write_ctrl_data(port, value, m_soundchip_id);
		else
		{
			if (port == 0x42)
			{
				if (m_sample_trigger)
				{	
					for (unsigned int i = 0; i < sizeof(g_sound_map); i+=2)
					{
						if (g_sound_map[i + 1] == value)
						{
							value = i;
							break;
						}
					}

					sound_play (g_sound_map[value]);
				}
				else
					m_sample_trigger = true;
			}
		}
		if (port == 0x61) m_port61_val = value;	// for safety reasons only
		break;
	case 0x201:
		break;
	case 0x202:
		// if the 'coinco' status should be hidden
		if (value & 0x40)
		{
			/*
#ifdef DEBUG
			static unsigned int uOldTime = 0;
			unsigned int uElapsed = elapsed_ms_time(uOldTime);
			uOldTime = GET_TICKS();
			string s = "Elapsed mSec since last coin activity: " + numstr::ToStr(uElapsed);
			printline(s.c_str());
#endif
			*/

			for (int i = 0; i < NUM_COIN_SLOTS; i++)
			{
				// decrement each coin count by 1 when the status is hidden.
				// This may not be how the 'coinco' is supposed to behave, but it does
				//  give the DL2 rom what i wants, and avoids dropped coins.
				if (m_uCoinCount[i] > 0) --m_uCoinCount[i];
			}
			banks[1] &= ~0x3C;	// clear all coin bits (hide them)
		}
		// else the 'coinco' status is being shown, so make sure the coin bits are up to date
		else
		{
			unsigned int uORVal = (1 << 2);	// start with coin slot 1's bit
			for (int i = 0; i < NUM_COIN_SLOTS; i++)
			{
				// if there is a coin insertion waiting for this coin ...
				if (m_uCoinCount[i] > 0)
				{
					banks[1] |= uORVal;
				}
				uORVal <<= 1;	// shift left by 1 to go to the next coin slot
			}
		}
		EEPROM_9536_write(value);   // 9536 EEPROM emulation
		break;

   // COM2 port 0x2F8
	case	0x2F8:			// transmit/receive
	{
#ifdef DEBUG
//		if (value == 0x67) g_bVerbose = true;
#endif
		// write to LDP
		// and set our interupt
		if (g_dl2_euro)
		{
			vp932_write(value);
		}
		else
		{
			// if we're hooked up to a real LDP-1450, send directly to the serial port
			if (m_real_1450)
			{
				serial_tx(value);
				while (!serial_rx_char_waiting()) SDL_Delay(1);	// TESTING...
			}

			// otherwise send to our emulated LDP
			else
			{
				write_ldp1000(value);
			}
		}
		break;
	}
	case	0x2F8 + DL2_IER:		// Interrupt enable
		m_serial_int_enabled = (value != 0);	// 0 disables interrupts, non-zero enables them
		break;
	case	0x2F8 + DL2_IIR:		// Interrupt ID
	case	0x2F8 + DL2_LCR:		// Line control
	case	0x2F8 + DL2_MCR:		// Modem control
	case	0x2F8 + DL2_LSR:		// Line Status
	case	0x2F8 + DL2_MSR:		// Modem Status
		break;						// divisor latch high

	default:
        break;
	}		
}

Uint8 lair2::port_read(Uint16 port)
{
	unsigned char result = 0;

	// handle ports	
	switch(port)
	{
	case 0x61:
		result = m_port61_val;	// for safety reasons ...
		break;
	case	0x201:
		result = banks[0];
		break;

	case	0x202:
		// port 0x202
		// bit 0 - EEP data
		// bit 1 - unknown
		// bit 2 - coin 1
		// bit 3 - coin 2
		// bit 4 - coin 3
		// bit 5 - coin 4
		// bits 6-7 - unknown
		result = banks[1];
		break;

		// COM2 port 0x2F8
	case	0x2F8:
		// if we have a character waiting, then return the character
		if (m_uSerialBufSize > 0)
		{
			result = m_u8SerialBuf[0];	// first char in buf is always the oldest in the buffer
			memmove(m_u8SerialBuf, m_u8SerialBuf+1, m_uSerialBufSize - 1);	// move remaining bytes to front
			--m_uSerialBufSize;	// buffer now has 1 less byte in it
		}

		// this should never happen, so it's safe to put an error to notify us if something weird is going on
		else printline("LAIR2.CPP WARNING : tried to read from serial port when no char was waiting");
		break;

	case	0x2F8 + DL2_IER:		// Interrupt enable
		if (m_serial_int_enabled) result = 1;	// this reg will have a 1 if the interrupt is enabled
		break;
	case	0x2F8 + DL2_LCR:		// Line control
		break;

	case	0x2F8 + DL2_IIR:		// Interrupt ID
		// if we have a character from the LDP waiting to be read then return a 4 (not sure why 4, but... that's what the code expects)
		if (m_uSerialBufSize > 0)
		{
			result = 4;
		}
		// else a 0 is returned as usual
		break;

	case	0x2F8 + DL2_LSR:		// Line Status (returning DSR is safe)
	case	0x2F8 + DL2_MSR:		// Modem Status (returning CTS is safe)
		result = DL2_CTS | DL2_DSR;
		break;

	default:
		break;
	}

	return(result);
}

void lair2::cpu_mem_write(Uint32 Addr, Uint8 Value)
{
	m_cpumem[Addr] = Value; // always store to RAM if we write to it
}

// used to set dip switch values
bool lair2::set_bank(unsigned char which_bank, unsigned char value)
{
	bool result = true;
	
	printline("ERROR: Dragon's Lair 2 uses onscreen setup");
	result = false;

	return result;
}


void lair2::input_enable(Uint8 move)
{
	switch(move)
	{
	case SWITCH_UP:
      banks[0] &=  ~0x01;	
		break;
	case SWITCH_DOWN:
		banks[0] &=  ~0x02;	
		break;
	case SWITCH_LEFT:
		banks[0] &=  ~0x04;	
		break;
	case SWITCH_RIGHT:
		banks[0] &=  ~0x08;	
		break;
	case SWITCH_COIN1:
		++m_uCoinCount[0];
		//banks[1] |= 0x04;
		break;
	case SWITCH_COIN2:
		++m_uCoinCount[1];
		//banks[1] |= 0x08;	
		break;
	case SWITCH_BUTTON1:
		banks[0] &=  ~0x40;	
		break;
	case SWITCH_START1: 
		banks[0] &=  ~0x10;	
		break;
	case SWITCH_START2: 
		banks[0] &=  ~0x20;	
		break;
	case SWITCH_SERVICE: 
		banks[0] &=  ~0x80;	
		break;
	default:
		break;
	}
}


void lair2::input_disable(Uint8 move)
{
	switch(move)
	{
	case SWITCH_UP:
      banks[0] |=  0x01;	
		break;
	case SWITCH_DOWN:
		banks[0] |=  0x02;	
		break;
	case SWITCH_LEFT:
		banks[0] |=  0x04;	
		break;
	case SWITCH_RIGHT:
		banks[0] |=  0x08;	
		break;
	case SWITCH_COIN1:
		//banks[1] &=  ~0x04;
		break;
	case SWITCH_COIN2:
		//banks[1] &=  ~0x08;
		break;
	case SWITCH_BUTTON1: 
		banks[0] |=  0x40;	
		break;
	case SWITCH_START1: 
		banks[0] |=  0x10;	
		break;
	case SWITCH_START2: 
		banks[0] |=  0x20;	
		break;
	case SWITCH_SERVICE: 
		banks[0] |=  0x80;	
		break;
	default:
		break;
	}
}

void lair2::video_repaint()
{
	Uint32 cur_w = g_ldp->get_discvideo_width() >> 1;	// width our overlay should be
	Uint32 cur_h = g_ldp->get_discvideo_height() >> 1;	// height our overlay should be

	// If the width or height of the mpeg video has changed since we last were
    // here (ie, opening a new mpeg) then reallocate the video overlay buffer.
	if ((cur_w != m_video_overlay_width) || (cur_h != m_video_overlay_height))
	{
		printline("LAIR2 : Surface does not match disc video, re-allocating surface!");
		
		// in order to re-initialize our video we need to stop the yuv callback
		if (g_ldp->lock_overlay(1000))
		{
			m_video_overlay_width = cur_w;
			m_video_overlay_height = cur_h;
			video_shutdown();
			if (!video_init()) set_quitflag();	// safety check
			g_ldp->unlock_overlay(1000);	// unblock game video overlay
		}
		
		// if the yuv callback is not responding to our stop request
		else
		{
			printline("LAIR2 : Timed out trying to get a lock on the yuv overlay");
		}
	} // end if video resizing is required
}

void lair2::EEPROM_9536_write(Uint8 value)
{
      // NV RAM saving 
      // uses the 9536 EEPROM
      // bit 0 = data
      // bit 1 = clock
      // bit 2 = CS
      // bit 3 = PRE
      // bits 4-7 unknown

	static Uint8 nv_opcode = 0xff;
	static Uint16 nv_data = 0;
	static Uint16 nv_address = 0;
	static int address_count = 0;
	static int bit_count = 0;
	static Uint8 old = 0x00;
	int org = 1;

	if (value & 0x04) // check chip select - while this is high be are still getting data for the same command
      {
         if ((value & 0x02) && !(old & 0x02))   // check if clock transitioned, if it did we've got new data
         {
            if (nv_opcode == 0xff)
            {
               nv_data = (nv_data << 1) | (value & 0x01);
               if (nv_data & 0x04) // bit 2 is start bit, so when it appears we have a valid opcode
               {
                  nv_opcode = nv_data & 0x03;
               }
            }
            else if (address_count < 9 - org)// we have the opcode, now we can get the address
            {
               nv_address <<= 1;      // add new data
               nv_address |= (value & 0x01);            
               address_count++;
               if (address_count == 2 && (nv_opcode == 0 || nv_opcode == 3))
               {
                  char s[81] = {0};
                  sprintf(s, "EEP unhandled OPCode %x with address %x", nv_opcode, nv_address);
                  banks[1] |= 0x01; // set bit 0 high to indicate we aren't busy
                  printline(s);
               }

               banks[1] = (banks[1] & ~0x01) | ((EEPROM_9536[nv_address] >> 15) & 0x01);
            }         
            // we have the opcode, and the address so we can figure out how many data bits we need
            else if (address_count == 9 - org)
            {
               // read
               if (nv_opcode == 2)
               {
                  banks[1] = (banks[1] & ~0x01) | ((EEPROM_9536[nv_address] >> (((org + 1) * 8) - bit_count - 1)) & 0x01);
               }   
               // write
               if (nv_opcode == 1)
               {
                  if (bit_count == 0)
                  {
                     EEPROM_9536[nv_address] = 0;
                  }
                  EEPROM_9536[nv_address] = (EEPROM_9536[nv_address] << 1) | (value & 0x01);
                  banks[1] |= 0x01; // set bit 0 high to indicate we aren't busy
               }               
               bit_count++;
            }
         }
      }      
      else // CS is disabled so erase everything
      {
         nv_opcode = 0xff;
         nv_address = 0;
         nv_data = 0;
         bit_count = 0;
         address_count = 0;
      }

	  old = value;
}
