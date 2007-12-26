/*
 * cliff.cpp
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

// cliff.cpp

// Cliffy memory map:
//
// 0x0000-0x9FFF Program ROMs
// 0xE000-0xE7FF Battery-backed non-volatile RAM (5126)
// 0xE800-0xEFFF Volatile RAM (2128)

#include <stdio.h>
#include <string.h>
#include "cliff.h"
#include "../io/conout.h"
#include "../sound/sound.h"
#include "../video/tms9128nl.h"
#include "../ldp-in/pr8210.h"
#include "game.h"
#include "../ldp-out/ldp.h"
#include "../cpu/cpu.h"
#include "../cpu/generic_z80.h"

#ifdef DEBUG
#include "../cpu/cpu-debug.h"
#endif

/////////////////////////////////////////////////

// cliff constructor
cliff::cliff()
{
	struct cpudef cpu;
	
	m_shortgamename = "cliff";
	memset(&cpu, 0, sizeof(struct cpudef));
	// initialize m_blips
	m_blips = 0;
	m_blips_count = 0;
	m_banks_index = 0;
	m_frame_val = 0;

	m_uLastSoundIdx = 0;

	m_banks[0] = 0xFF;  // bank 0 (dip switchs 1,2)
	m_banks[1] = 0; // anthony has these all on
	m_banks[2] = 0xFF & DIP22_UNKNOWN & DIP23_UNKNOWN; // bank 2 (dips 20-27)
	// this is how Anthony and Bo's machines are both configured

	m_banks[3] = 0xFF & DIP18_SHORTSCENES & DIP19_BUYIN; 

	m_banks[4] = 0xFF & DIP5_DIFFICULTY2; // bank 4 (dips 4-11) aka H11  (first bank for Anthony) all off
	m_banks[5] = 0xFF; // bank 5 (button data)
	m_banks[6] = 0xFF; // bank 6 (joystick data)
	m_banks[7] = 0xFF; // bank 7
	m_banks[8] = 0xFF; // bank 8
	m_banks[9] = 0xFF; // bank 9

	m_disc_fps = 29.97;
	m_game_type = GAME_CLIFF;

	m_video_overlay_width = TMS9128NL_OVERLAY_W;
	m_video_overlay_height = TMS9128NL_OVERLAY_H;
	m_palette_color_count = TMS_COLOR_COUNT;

	cpu.type = CPU_Z80;
	cpu.hz = CLIFF_CPU_HZ;
	cpu.initial_pc = 0;
	cpu.must_copy_context = false;
	cpu.irq_period[0] = CLIFF_IRQ_PERIOD;
	cpu.nmi_period = CLIFF_NMI_PERIOD;
	cpu.mem = m_cpumem;
	add_cpu(&cpu);	// add z80 cpu

	m_num_sounds = 3;
	m_sound_name[S_C_CORRECT] = "cliff_correct.wav";
	m_sound_name[S_C_WRONG] = "cliff_wrong.wav";
	m_sound_name[S_C_STARTUP] = "cliff_startup.wav";
	
	m_nvram_begin = &m_cpumem[0xE000];
	m_nvram_size = 0x800;

	// NOTE : this must be static
	static struct rom_def roms[] =
	{
		{ "cliff_u1.bin", NULL, &m_cpumem[0x0000], 0x2000, 0xA86EC38F },
		{ "cliff_u2.bin", NULL, &m_cpumem[0x2000], 0x2000, 0xB8D33B6B },
		{ "cliff_u3.bin", NULL, &m_cpumem[0x4000], 0x2000, 0x75A64CD2 },
		{ "cliff_u4.bin", NULL, &m_cpumem[0x6000], 0x2000, 0x906B2AF1 },
		{ "cliff_u5.bin", NULL, &m_cpumem[0x8000], 0x2000, 0x5922E710 },
		{ NULL }
	};

	m_rom_list = roms;

}

// goal to go constructor
gtg::gtg()
{
	m_shortgamename = "gtg";
	m_game_type = GAME_GTG;
	m_game_issues = "When we fixed Cliff, we broke this game, sorry! hehe";
	
	disc_side = 1;  //default to side 1 if no -preset is used
	e1ba_accesscount = 0;

	// this must be static!
	const static struct rom_def roms[] =
	{
		{ "gtg.rm0", NULL, &m_cpumem[0], 0x2000, 0 },
		{ "gtg.rm1", NULL, &m_cpumem[0x2000], 0x2000, 0 },
		{ "gtg.rm2", NULL, &m_cpumem[0x4000], 0x2000, 0 },
		{ "gtg.rm3", NULL, &m_cpumem[0x6000], 0x2000, 0 },
		{ "gtg.rm4", NULL, &m_cpumem[0x8000], 0x2000, 0 },
		{ NULL }
	};

	m_rom_list = roms;
}

// cliffalt constructor
cliffalt::cliffalt()
{	
	m_shortgamename = "cliffalt";
	memset(m_banks, 0xFF, sizeof(m_banks));	// clear all dip switches

	// FIXME : set dip switches to a sensible value here

	// NOTE : this must be static
	static struct rom_def roms[] =
	{
		{ "cliff_alt_0.bin", NULL, &m_cpumem[0x0000], 0x2000, 0x27CAA67C },
		{ "cliff_alt_1.bin", NULL, &m_cpumem[0x2000], 0x2000, 0x6E5F1515 },
		{ "cliff_alt_2.bin", NULL, &m_cpumem[0x4000], 0x2000, 0x045F895D },
		{ "cliff_alt_3.bin", NULL, &m_cpumem[0x6000], 0x2000, 0x54CDB4A1 },
		{ NULL }
	};

	m_rom_list = roms;

}

// cliffalt2 constructor - Added by Buzz
cliffalt2::cliffalt2()
{	
	m_shortgamename = "cliffalt2";
	memset(m_banks, 0xFF, sizeof(m_banks));	// clear all dip switches

	// FIXME : set dip switches to a sensible value here

	// NOTE : this must be static
	static struct rom_def roms[] =
	{
		{ "cliff_alt2_0.bin", NULL, &m_cpumem[0x0000], 0x2000, 0x598D57FC },
		{ "cliff_alt2_1.bin", NULL, &m_cpumem[0x2000], 0x2000, 0x7BCE618E },
		{ "cliff_alt2_2.bin", NULL, &m_cpumem[0x4000], 0x2000, 0x65D2B984 },
		{ "cliff_alt2_3.bin", NULL, &m_cpumem[0x6000], 0x2000, 0xF43A5269 },
		{ NULL }
	};

	m_rom_list = roms;

}

// resets cliff hanger
void cliff::reset()
{
	cpu_reset();
	video_shutdown();
	video_init();	// restart the video, because otherwise cliff won't reboot
	pr8210_reset();  // makes sure audio is in the correct state
}

// resets goal to go
void gtg::reset()
{
	cpu_reset();
	e1ba_accesscount = 0;  //reset the frame/chapter read count 
	video_shutdown();
	video_init();	// restart the video
	pr8210_reset();  // makes sure audio is in the correct state
}

// when z80 outputs to a port, this gets called
void cliff::port_write(Uint16 Port, Uint8 Value)
{

	char s[81] = { 0 };

	Port &= 0xFF;	// strip off high byte

	switch(Port)
	{
	// this unsigned char is written to the screen at the specified coordinates (set by writing to port 0x54)
	case 0x44:
		tms9128nl_write_port0(Value);
		break;

		// play sound, this is still a big hack
	case 0x46:
		// make sure we don't play a sample that is already being played
		if ((unsigned int) (Value & 0xF) != m_uLastSoundIdx)
		{
			m_uLastSoundIdx = Value & 0xF;

			// the lower bits control which sound to play
			switch (m_uLastSoundIdx)
			{
			case 1:
				sound_play(S_C_CORRECT);
				break;
			case 2:
				sound_play(S_C_WRONG);
				break;
			case 3:
				sound_play(S_C_STARTUP);
				break;
			default:
				// no sound
				break;
			}
		}

		// if bit 4 is set, it tells the video hardware to maintain transparency
		if ((Value & 0x10) == 0x10)
		{
			tms9128nl_set_transparency();
		}
		break;

		// outputting to port 0x54 is the same as outputting to
		// port #1 on the TMS chip (see reference)
	case 0x54:
		tms9128nl_write_port1(Value);
		break;

	// get current frame number from the LDP
	// FIXME : this can receive multiple bytes.  What do they mean?  for example, 0xAA or 0x00
	case 0x57: //get frame from LDP
		{
			m_frame_val = pr8210_get_current_frame();
			g_ldp->framenum_to_frame(m_frame_val, m_frame_str);
			sprintf(s,"Playing Frame: %s", m_frame_str);
			tms9128nl_outcommand(s,43,23);
		}
		break;
	case 0x60:

            // make sure that the bank request is in range
            if (Value < CLIFF_BANK_COUNT)
            {
                m_banks_index = Value;
            }

			// It appears that Cliffy hardware required sending a 0xF to this port
			// to clear any previous results, and then sending the desired bank
			// shortly thereafter.  See 0x24BB for an example of this in action.
			// (obviously we do not need to emulate this restriction)
            else if (Value == 0xF)
            {
            }

            else
            {
                sprintf(s, "A bank out of range was requested! %x", Value);
                printline(s);
            }
            break;

		// PR-8210 port that we can ignore with our current scheme
		// NOTE : On further reflection, I am not sure if this port is related to
		// the PR-8210 output at all hehe
	case 0x64:
//		printf("0x64 was written to at PC %x, value %x\n", Z80_GET_PC, Value);
		break;
		
		// This port gets written to by 1's and 0's.
		// My guess is that a 1 raises the PR-8210 output and a 0 lowers it.
		// Assuming the ROM works with a real PR-8210, it should be safe for us
		// to ignore the 0 and only pay attention to the 1.
	case 0x66:
//		printf("0x66 written to at PC %x, value %x\n", Z80_GET_PC, Value);
		if (Value == 1)
		{
			cliff_do_blip();
		}
		break;

	// we used to detect the blip in this function but I discovered the behavior
	// exhibited by this function was not consistent across different ROM revisions
	// of cliffy.  Port 0x66 seems to be the correct way to detect blips.
	case 0x6A:
		break;
	case 0x6E:
            //	outcommand("Led On");
//            		printline("LED On");
            break;
	case 0x6F:
            		//printline("LED Off");
            break;
	default:
            sprintf(s, "CLIFF: Unsupported Port Output-> %x : %x", Port, Value);
			printline(s);
            break;
	}
}


Uint8 cliff::port_read(Uint16 Port)
// Called whenever the emulator wants to read from a port
{
    char s[81] = { 0 };
    unsigned char result = 0;
	Port &= 0xFF;	// strip off high byte

    switch(Port)
    {
        // don't know what it is, but it's annoying =]
    case 0x39:
        break;

        // read from video memory
    case 0x45:
        result = tms9128nl_getvidmem();
        break;

    case 0x52: //return high byte of frame
        result = (unsigned char) (m_frame_str[0] & 0x0F);
        
        // if the LDP is not busy/seeking, then we need to OR 0xF8 to this byte, or
        // the program will think the LDP is busy and will not return the rest of the frame.
        // (if the LDP is busy though, the upper 5 bits must be clear)
        if (m_frame_val != 0)
        {
			result |= 0xf8;
		}
        break;
    case 0x51: //return mid byte of frame
		result = (unsigned char) ((m_frame_str[1] & 0xF) << 4);
		result |= ( (unsigned char) m_frame_str[2] & (unsigned char) 0x0F);
        break;
    case 0x50: //return frame low byte of frame
        result = (unsigned char) ((m_frame_str[3] & 0xF) << 4);
		result |= (m_frame_str[4] & 0xF);
        break;

	// UNKNOWN (gets read by IRQ routine but doesn't seem to get used for anything)
    case 0x53:
        break;

		// something to do with video
    case 0x55:
        break;

        // read dip switches
    case 0x62:
        result = m_banks[m_banks_index];
        break;
    default:
        sprintf(s, "CLIFF: Unsupported Port Input-> %x (PC is %x)", Port, Z80_GET_PC);
        printline(s);
        break;
    }
    return(result);
}

void cliff::do_nmi()
{	
	video_blit();	// vsync

	// if the TMS chip is producing interrupts, then we do an NMI
	if (tms9128nl_int_enabled())
	{
		  // SEND QUICK NMI PULSE HERE
		  Z80_ASSERT_NMI;
	}

}

void cliff::do_irq(unsigned int which_irq)
{
	// this is kind of a meaningless check since there is only 1 z80 irq
	if (which_irq == 0)
	{
		Z80_ASSERT_IRQ;
	}
}

#ifdef DEBUG
// the the purpose of debugging/disassembling the ROM
void cliff::cpu_mem_write(Uint16 addr, Uint8 value)
{
//	if (addr == 0xE116) set_cpu_trace(1);
	m_cpumem[addr] = value;
}
#endif

void outcommand(char *s) //dangit Matt, stop ripping out my useful routines =]
{ //this needs fixing anyway

	static int row=1,commandnum=0;

    int slength,x;
    char strtoprint[25],string2[25];
    //gotoxy(43,row++);
    if (row==21) row=1;
    slength=strlen(s);
    strcpy(strtoprint,s);
    for(x=slength;x<=18;x++)
        strcat(strtoprint," ");
    sprintf(string2,"%d) %s\n",commandnum++,strtoprint);
	tms9128nl_outcommand(string2,45,row);
}


// process a blip for the PR-8210
void cliff::cliff_do_blip()
{
	static Uint64 total_cycles = 0;

	Uint8 blip_value = 0;
	Uint64 cur_total_cycles = get_total_cycles_executed(0);
	
	// check to make sure flush_cpu_timers was not called
	if (cur_total_cycles > total_cycles)
	{
		Uint64 ec = cur_total_cycles - total_cycles;

		//printf("ec is %u\n", ec);	// for debugging purposes only

		// 12000 is our arbitrary upper limit for getting a valid blip (it looks like it's safe to go as low at 7000)
		// The elapsed cycles are greater than this number, we didn't get any blip at all
		if (ec < 12000)
		{
			// 5500 is our arbitrary boundary between a 0 and a 1.  If it's over this number, it's a 1, otherwise it's a 0
			if (ec > 5500)	// I had to raise this number for cliffalt's benefit
			{
				blip_value = 1;
			}

			m_blips = m_blips << 1;	// shift to the left one to make room for new blip
			m_blips |= blip_value;
			m_blips_count++;	// we got a blip so advance the pointer

			// if buffer is filled, send command
			if (m_blips_count > 9)
			{
				pr8210_command(m_blips);
				m_blips_count = 0;
			}
		} // if we got a blip at all
		
		// else we got no blip, so reset
		else
		{
			m_blips_count = 0;	// reset if timed out?
		}
	}
	
	// else if flush_cpu_timers was called, we assume (dangerously?) that the blip was far
	// away and therefore doesn't count as a blip
	// If you ever have problems w/ the PR-8210 interpreting commands, this is probably the
	// place to start looking.  My tests show that it works fine though.

	total_cycles = cur_total_cycles;
}


// set's service mode in Cliff Hanger
// enabled: 0 = off, 1 = on
void cliff::cliff_set_service_mode(int enabled)
{

	// turn on service mode
	if (enabled)
	{
		printline("Enabling service mode");
		m_banks[3] &= DIP12_SERVICE;
	}

	// turn off service mode
	else
	{
		printline("Disabling service mode");
		m_banks[3] |= ~DIP12_SERVICE;
	}

}

// sets switch test mode in Cliffy
// enabled: 0 = off, 1 = on
void cliff::cliff_set_test_mode(int enabled)
{
	if (enabled)
	{
		printline("Enabling test mode");
		m_banks[3] &= DIP13_SWITCHES;
	}
	else
	{
		printline("Disabling test mode");
		m_banks[3] |= ~DIP13_SWITCHES;
	}
}

// this gets called when the user presses a key or moves the joystick
void cliff::input_enable(Uint8 move)
{
	static unsigned char service_enabled = 0;	// start disabled
	static unsigned char test_enabled = 0;
	
	switch (move)
	{
	case SWITCH_UP:
		m_banks[6] &= ~1; // clear bit 0
		break;
	case SWITCH_LEFT:
		m_banks[6] &= ~8;
		break;
	case SWITCH_RIGHT:
		m_banks[6] &= ~2;
		break;
	case SWITCH_DOWN:
		m_banks[6] &= ~4; // clear bit 2
		break;
	case SWITCH_START1:
	case SWITCH_BUTTON2:	// for convenience.. so player doesn't have to press start1 for feet
		m_banks[5] &= ~4; // clear bit 2
		break;
	case SWITCH_START2:
		m_banks[5] &= ~8;	// clear bit 3
		break;
	case SWITCH_BUTTON1:
		m_banks[5] &= ~16;	// only press one of the hands buttons
								// if we press both, the high score section is ruined
		break;
	case SWITCH_COIN1:
		m_banks[5] &= ~1; // Coin 1 and Coin 2 might be reversed
		break;
	case SWITCH_COIN2:
		m_banks[5] &= ~2;  
		break;
	case SWITCH_SERVICE:
		service_enabled = (unsigned char) (service_enabled ^ 1);	// flip bit 0		
		cliff_set_service_mode(service_enabled);		
		break;
	case SWITCH_TEST:
		test_enabled = (unsigned char) (test_enabled ^ 1);
		cliff_set_test_mode(test_enabled);
		break;
	case SWITCH_TILT:
		m_banks[5] &= ~128; //clear bit 5
		break;

	default:
		char s[81] = { 0 };
		sprintf(s, "Bug in Cliffy's input enable.  Input was %x", move);
		printline(s);
		break;
	}
}

// this gets called when the user releases a key or moves the joystick back to center position
void cliff::input_disable(Uint8 move)
{
	switch (move)
	{
	case SWITCH_UP:
		m_banks[6] |= 1;	// set bit 0
		break;
	case SWITCH_LEFT:
		m_banks[6] |= 8;
		break;
	case SWITCH_RIGHT:
		m_banks[6] |= 2;
		break;
	case SWITCH_DOWN:
		m_banks[6] |= 4;	// set bit 2
		break;
	case SWITCH_START1:
	case SWITCH_BUTTON2:	// for convenience.. so player doesn't have to press start1 for feet
		m_banks[5] |= 4;
		break;
	case SWITCH_START2:
		m_banks[5] |= 8;
		break;
	case SWITCH_BUTTON1:
		m_banks[5] |= 16;
		break;
	case SWITCH_COIN1:	
		m_banks[5] |= 1;
		break;
	case SWITCH_COIN2:
		m_banks[5] |= 2;
		break;
	case SWITCH_SERVICE:
	case SWITCH_TEST:
		break;
	case SWITCH_TILT:
		m_banks[5] |= 128;
		break;
	default:
		char s[81] = { 0 };
		sprintf(s, "Error, bug in Cliffy's input disable, input was %x", move);
		printline(s);
		break;
	}
}

// used to set dip switch values
bool cliff::set_bank(unsigned char which_bank, unsigned char value)
{
	bool result = true;
	
	switch (which_bank)
	{
	case 0:	// h11
		m_banks[4] = (unsigned char) (value ^ 0xFF);	// switches are active low
		break;
	case 1:	// g11
		m_banks[3] = (unsigned char) (value ^ 0xFF);	// switches are active low
		break;
	case 2:	// f11
		m_banks[2] = (unsigned char) (value ^ 0xFF);	// switches are active low
		break;
	case 3:	// e11
		m_banks[1] = (unsigned char) (value ^ 0xFF);	// switches are active low
		break;
	default:
		printline("ERROR: Bank specified is out of range!");
		result = false;
		break;
	}
	
	return result;
}

void cliff::palette_calculate()
{
	tms9128nl_palette_calculate();
}

void cliff::video_repaint()
{
	tms9128nl_video_repaint();
}

// post-rom loading adjustment
void cliff::patch_roms()
{
	// if a cheat has been requested, modify the ROM.
	if (m_cheat_requested)
	{
		// infinite lives cheat
		m_cpumem[0xD36] = 0;	// NOP out code that decrements # of remaning lives :)
		m_cpumem[0xD37] = 0;
		m_cpumem[0xD38] = 0;

		printline("Cliff hanger infinite lives cheat enabled!");
	}

// MATT : commented out since we can now support more than one ROM set
//	m_cpumem[0x5b7] = 0x05;		// Barbadel: HACK: reduce delay to prevent hearing the mpeg before the cliffy logo
//	m_cpumem[0x5b8] = 0x24;

	// Replaces 0 characters with another blank character.  This makes the hints display properly while we try
	// to figure out how to get the driver working properly.
//	m_cpumem[0xe0b] = 0x8a; // Barbadel: HACK to get the hints to display properly
//	m_cpumem[0xe11] = 0x8a;
//	m_cpumem[0xe35] = 0x8a;
//	m_cpumem[0xe3C] = 0x8a;

	// skip diagnostics if fast boot is enabled
	if (m_fastboot)
	{
		m_cpumem[8] = 0;
		m_cpumem[9] = 0;
		m_cpumem[0xA] = 0;
	}

}

Uint8 gtg::cpu_mem_read(Uint16 addr)
{
 	Uint8 result = m_cpumem[addr];

	//mem hack for GtG disc side detection
	//(this address is filled by GtG's ISR when a chapter code is received from the
	//frame detect hardware.  The chapter code is '01' for side 1, and '77' for side 2)
 	if (addr == 0xe1ba)
 	{
 		e1ba_accesscount++;

		// MATT : only report the current side of the disc if the disc is playing, otherwise
		// it will interfere with the self-test
		if (g_ldp->get_status() == LDP_PLAYING)

 		//don't mess with the value the first few times the location is read -- it screws up the self-test
//		if (e1ba_accesscount > 29) 
		{
			if (disc_side==1)
				result = 0x01;
			else
				result = 0x77;
 		}

// 		printline("e1ba read");
 	}
 	return result;
}

void gtg::set_preset(int val)
{
	if (val==1) disc_side=1;
	else if (val==2) disc_side=2;
}
