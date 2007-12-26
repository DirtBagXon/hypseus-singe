/*
 * thayers.cpp
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

// thayers.cpp
// by Mark Broadhead 
// based, in part, on information by Jeff Kulczycki
//

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "thayers.h"
#include "../io/conout.h"
#include "../ldp-in/ldv1000.h"
#include "../timer/timer.h"
#include "../sound/ssi263.h"
#include "../ldp-out/ldp.h"
#include "../video/palette.h"
#include "../video/video.h"
#include "../cpu/cpu.h"
#include "../cpu/generic_z80.h"

thayers::thayers()
{
	struct cpudef cpu;
	
	m_shortgamename = "tq";
	memset(&cpu, 0, sizeof(struct cpudef));
	memset(banks, 0xFF, 2);	// fill banks with 0xFF's
	banks[0] = 0x0;	// put it on free play
	m_disc_fps = 29.97;
	m_game_type = GAME_THAYERS;

	cpu.type = CPU_Z80;
	cpu.hz = THAYERS_CPU_HZ;
	cpu.irq_period[0] = (1000.0 / 60.0);	// TQ has no clock-driven IRQ, but we'll use this to time vsync instead
	cpu.nmi_period = 30.0; // the actual value here isn't important
	cpu.initial_pc = 0;
	cpu.must_copy_context = false;
	cpu.mem = m_cpumem;
	add_cpu(&cpu);

	cpu.type = CPU_COP421;
	cpu.hz = THAYERS_CPU_HZ / 2 / 32; // the cop clock is divided by 2 externally and 32 internally
	cpu.nmi_period = 0;
	cpu.must_copy_context = false;
	cpu.mem = coprom;
	add_cpu(&cpu);

	m_irq_status = 0x3f;
	ldv1000_enable_instant_seeking();
    m_use_speech = true;   // Even though not truly emulated, speech synthesis is the default.
    m_show_speech_subtitle = false;  // Flag to toggle on/off subtitled speech text.
    m_game_issues = "Use PageUp, PageDown to change speech volume. F9 toggles speech subtitle on/off";

	m_game_uses_video_overlay = false;
    m_use_overlay_scoreboard = false;   // overlay scoreboard option must be enabled from cmd line

	// default ROM images for TQ (this must be static!)
	const static struct rom_def tq_roms[] =
	{
		{ "tq_u33.bin", NULL, &m_cpumem[0x0000], 0x8000, 0x82df5d89 },
		{ "tq_u1.bin", NULL, &m_cpumem[0xc000], 0x2000, 0xe8e7f566 },
		{ "tq_cop.bin", NULL, &coprom[0x0000], 0x0400, 0x6748e6b3 },
		{ NULL }
	};

	m_rom_list = tq_roms;
}

// this is necessary so that we can use it as a function pointer
SDL_Surface *tq_get_active_overlay()
{
	return g_game->get_active_video_overlay();
}

bool thayers::init()
{
    bool result = false;

    // The "-nosound" arg could have been specified, so don't crank up
    // the synthesizer unless audio is enabled. If -notqspeech was present
    // on the command line, m_use_speech will be false.
    if (is_sound_enabled())
	{
        result = ssi263_init(m_use_speech);
	}
    else
    {
        // The -nosound option must have been present.
        result = ssi263_init(false);
        m_use_speech = false;
        m_show_speech_subtitle = true;
    }

	// if sound initialization succeeded
    if (result)
    {
	    cpu_init();

		IScoreboard *pScoreboard = ScoreboardCollection::GetInstance(m_pLogger,
			tq_get_active_overlay,
			true,	// we are thayers quest
			false,	// not using annunciator, that's just for Space Ace
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
			result = false;
		}
    }
	// else sound initialization failed

    return result;
}

// Initialize overlay scoreboard if -useroverlaysb command line switch used.
void thayers::init_overlay_scoreboard()
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
	m_use_overlay_scoreboard = true;
}

// As with the original, speech synthesis is on by default. But in the interest
// of flexibility, provide a way to turn it off from the command line...
void thayers::no_speech()
{
    m_use_speech = false;
    m_show_speech_subtitle = true;
    m_game_issues = NULL;
}

void thayers::shutdown()
{
	if (m_pScoreboard)
	{
		m_pScoreboard->PreDeleteInstance();
	}
	cpu_shutdown();
}

// TQ supports multiple rom revs
void thayers::set_version(int version)
{
	if (version == 1) //standard set
	{
		//already loaded, do nothing
	}
	else if (version == 2)  //alternate
	{
	// this might be an older version - it contains secret 'warps' like pressing '1' when
	// druce asks you where to begin

		m_shortgamename = "tq_alt";
		const static struct rom_def tq_roms[] =
		{
			{ "tq_u33.bin", "tq", &m_cpumem[0x0000], 0x8000, 0x82df5d89 },
			{ "tq_u1.bin", NULL, &m_cpumem[0xc000], 0x2000, 0x33817e25 }, //this is the changed ROM
			{ "tq_cop.bin", "tq", &coprom[0x0000], 0x0400, 0x6748e6b3 },
			{ NULL }
		};

		m_rom_list = tq_roms;
	}
	else if (version == 3)  //JnK's swearing hack
	{
		m_shortgamename = "tq_swear";
		const static struct rom_def tq_roms[] =
		{
			{ "tq_u33_mod.bin", NULL, &m_cpumem[0x0000], 0x8000, 0x7be981df }, //this is the changed ROM
			{ "tq_u1.bin", "tq", &m_cpumem[0xc000], 0x2000, 0xe8e7f566 },
			{ "tq_cop.bin", "tq", &coprom[0x0000], 0x0400, 0x6748e6b3 },
			{ NULL }
		};

		m_rom_list = tq_roms;
	}
	else
	{
		printline("TQ:  Unsupported -version paramter, ignoring...");
	}
}	

// TQ's IRQ isn't raised by a clock, so this function actually isn't applicable
// But we are using it as a timer for the vsync to get the LD-V1000 strobes accurate
void thayers::do_irq(unsigned int which)
{
	// if this function is called, it means it's time for another simulated vsync
	ldv1000_report_vsync();
}

// does anything special needed to send an IRQ
void thayers::thayers_irq()
{
	Z80_ASSERT_IRQ;
}

// Thayer's doesn't have an nmi, so we'll use this for SSI263 and message timing
void thayers::do_nmi()
{
	if (m_game_uses_video_overlay)
	{
		// check to see if the overlay needs to be redrawn
		video_blit();
	}
	else
	{
		// else check to see if the scoreboard needs to be updated
		m_pScoreboard->RepaintIfNeeded();
	}

	m_message_timer++;

	// Clear any existing message after a few seconds
	if (m_message_timer == 200)
	{
		char t[60] = {0};

		memset(t, 0x20, 59);	// set the string to a bunch of blanks

		if (m_game_uses_video_overlay)
		{
			draw_string(t, 0, 17, m_video_overlay[m_active_video_overlay]);
		}
	}

    thayers_irq();
}

// reads a byte from the cpu's memory
Uint8 thayers::cpu_mem_read(Uint16 addr)
{
	if (addr == 0xbe17) // CHEAT - We need this since the CPU isn't recieving status from the COP420
		return 0x01;

	return m_cpumem[addr];
}

// writes a byte to the cpu's memory
void thayers::cpu_mem_write(Uint16 addr, Uint8 value)
{
	m_cpumem[addr] = value;
}

// reads a byte from the cpu's port
Uint8 thayers::port_read(Uint16 port)
{
	Uint8 result = 0;

	port &= 0xFF;	// strip off high byte

	switch (port)
	{
	    case 0x40:  // Interrupt request status (active if bit clear).
                    // Bit 2 SSI-263 Request Data.
                    // Bit 3 always high.
                    // Bit 4 /TIMER INT.
                    // Bit 5 /DATA RDY INT.
                    // Bit 6 /CART PRES.
                    // Others are unused.
		    result = m_irq_status;
		    break;
	    case 0x80:	// Read data from COP421 - keyboard data should appear here
		    result = cop_write_latch;
		    break;
    //	case 0xe0:	// Doesn't really exist... not in schematics
    //		break;
	    case 0xf0:	// Read Data from LD-V1000
		    result = read_ldv1000();
		    break;
	    case 0xf1:  // DIP Switch B, Coin Slots, READY Status
                    // Bits 0-3 Switch B.
                    // Bit 4 coin 1.
                    // Bit 5 coin 2.
                    // Bit 6 ld-v1000 status strobe
                    // Bit 7 ld-v1000 command strobe

			result = banks[1] | 0xc0;	// set the status/command strobe bits to indicate that they are not active

			// if status strobe is active ..
			if (ldv1000_is_status_strobe_active())
			{
				result &= 0xBF;	// enable status strobe (clear bit 6)
			}

			// if command strobe is active ..
			else if (ldv1000_is_command_strobe_active())
			{
				result &= 0x7F;	// enable command strobe (clear bit 7)
			}
		    break;
	    case 0xf2:	// Switch A
		    result = banks[0]; 
		    break;
	    default:
		    char s[81];
    	
		    sprintf(s, "ERROR: CPU port %x read requested, but this function is unimplemented!", port);
		    printline(s);
	}

	return result;
}

// writes a byte to the cpu's port
void thayers::port_write(Uint16 port, Uint8 value)
{
	char s[81] = { 0 };

	port &= 0xFF;	// strip off high byte

	switch (port)
	{
	case 0x00:	// Ports 00-04 output to speech chip
        // As far as I can tell, it's reg0 of the SSI-263 that issues an IRQ.
        // So always check on return if an IRQ needs to be raised.
		ssi263_reg0(value, &m_irq_status);

        if (! (m_irq_status & 0x04))
            thayers_irq();
		break;
	case 0x01:
		ssi263_reg1(value);
		break;
	case 0x02:
		ssi263_reg2(value);
		break;
	case 0x03:	
		ssi263_reg3(value);
		break;
	case 0x04:
		ssi263_reg4(value);
		break;
	case 0x20:	// Bits 5-7 write to COP421 ports G0-G2, Bit 2 /BANKSELT
		if (value == 0x20)
			cop_write_latch = 0xfa;
		break;
	case 0x40:	//	Bit 0 Latch for /ENCARTDET, Bit 1 Latch for RESOS, Other bits look unused....
		break;
//	case 0x80:	// Write data to COP421
//		cop_read_latch = value;
//		break;
	case 0xa0:	// TIMER_INT_L 
		m_irq_status |= 0x10;
		break;
	case 0xc0:	// DATA_READY_INT_L 
		m_irq_status |= 0x20;
		cop_write_latch = 0x00;
		break;
	case 0xe0:	// Doesn't really exist? Not in schematics
		break;
	case 0xf3:	// Interrupt Triger
		thayers_irq(); 
//		printline("Got self INT");
		break;
	case 0xf4:	// Write data to LD-V1000
		write_ldv1000(value);
//		sprintf(s, "THAYERS: Write to LD-V1000: %x", value);
//		printline(s);
		break;
	case 0xf5:	// Latch, only bits 4-7 are used, bit 4 coin counter, bit 5 enable writes to LDP, bit 6 LD Enter, bit 7 INT/EXT(LD-V1000)
		break;
	case 0xf6:	// DEN 1 - To scoreboard
		write_scoreboard(static_cast<Uint8>((value & 0x70) >> 4), static_cast<Uint8>(value & 0x0f), 0);
		break;
	case 0xf7:	// DEN 2 - To scoreboard
		write_scoreboard(static_cast<Uint8>((value & 0x70) >> 4), static_cast<Uint8>(value & 0x0f), 1);
		break;
	default:
		sprintf(s, "ERROR: CPU port %x write requested (value %x) at pc %x", port, value, Z80_GET_PC);
		printline(s);
		break;
	}
}
 
// Fetch and clean up text from the SSI-263 speech buffer.
void thayers::show_speech_subtitle()
{
    if (m_show_speech_subtitle)
    {
        char text[SSI_PHRASE_BUF_LEN];
        int len;

        if (m_message_timer < 200)
        {
            // Erase previous message that's still showing.
            memset(text, 0x20, 59);
            text[60] = '\0';

			if (m_game_uses_video_overlay)
			{
				draw_string(text, 0, 17, m_video_overlay[m_active_video_overlay]);
			}

        }

        // TQ stores a copy of the text to be synthesized in game RAM at 0xa500.
        // 0xa6d3 holds the # of characters in the buffer.
        len = m_cpumem[0xa6d3];
        text[len + 1] = '\0';

        // The buffer holds "raw" text that still has characters that get parsed
        // out when phoneme rules are applied, so strip them out before display.
        speech_buffer_cleanup((char *) &m_cpumem[0xa500], text, len);

        // Reset the timer so text will be cleared after a few seconds.
		m_message_timer = 0;

		// Make sure m_video_overlay pointer array is not NULL
		if (m_game_uses_video_overlay)
		{
			draw_string(text, 0, 17, m_video_overlay[m_active_video_overlay]);
		}

#ifdef SSI_DEBUG
        printline(text);
#endif
    }
}

// Remove SSI-263 delimiter/control/inflection characters to make text more readable.
void thayers::speech_buffer_cleanup(char *src, char *dst, int len)
{
    char *tmp = src;
    int i;

    for (i = 0; i < len; src++, i++)
    {
        // Remove "/()" characters to tidy up the output. Also apply a sanity
        // check to ensure all are "printable" (i.e. 7-bit ASCII text).
        if (*src == '/' || *src == '(' || *src == ')' || *src < ' ' || *src > '~')
            continue;

        if (*src == ',' && src - tmp > 3 && ! strncmp(src - 4, " THE", 4))
        {
            // Sometimes " THE,SOMETEXT" shows up. It just ain't proper grammer,
            // so pretty it up :-)
            *dst = ' ';
        }
        else if (*src == ',' && *(src + 1) != ' ')
        {
            // Pretty up comma punctuation.
            *dst++ = *src;
            *dst = ' ';
        }
        else
            *dst = *src;

        dst++;
    }

    *dst = '\0';
}

void thayers::palette_calculate()
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

void thayers::video_repaint()
{
	// if m_game_uses_video_overlay is false, then m_video_overlay_width will be 0, which means
	//  g_ldp->lock_overlay will get called before VLDP has been initialized which is not good.
	if (m_game_uses_video_overlay)
	{
		Uint32 cur_w = g_ldp->get_discvideo_width() >> 1;	// width our overlay should be
		Uint32 cur_h = g_ldp->get_discvideo_height() >> 1;	// height our overlay should be

		// If the width or height of the mpeg video has changed since we last were
		// here (ie, opening a new mpeg) then reallocate the video overlay buffer.
		if ((cur_w != m_video_overlay_width) || (cur_h != m_video_overlay_height))
		{
			printline("THAYERS : Surface does not match disc video, re-allocating surface!");

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
				printline("THAYERS : Timed out trying to get a lock on the yuv overlay");
			}
		} // end if video resizing is required
	} // else game isn't using video overlay at this time

	// An 'issues' screen can pop-up before m_pScoreboard has been instantiated.
	if (m_pScoreboard)
	{
		// by definition, video_repaint must force a repaint, hence why we call invalidate first
		m_pScoreboard->Invalidate();
		m_pScoreboard->RepaintIfNeeded();
	}
}

// this handles _any_ keypress on the keyboard
void thayers::process_keydown(SDLKey key)
{
    static int volume = 64;

	key_press = true;

	// Check to see if value of key is a letter between the a and z keys
	// (there are no SDLK_A or SDLK_Z keys.  the keys are returned raw without checking to see
	// if shift or caps lock is enabled)
	if (key >= SDLK_a && key <= SDLK_z)
	{
		cop_write_latch = static_cast<Uint8>(key - SDLK_SPACE);	// convert lowercase keys to uppercase
		m_irq_status &= ~0x20;
		thayers_irq();
	}

	// check to see if key is a number on the top row of the keyboard (not keypad)
	else if (key >= SDLK_0 && key <= SDLK_9)
	{
		cop_write_latch = static_cast<Uint8>(key);
		m_irq_status &= ~0x20;
		thayers_irq();
	}
    // only looking for single keypress, not range
    else switch(key)
    {
        case SDLK_ESCAPE:
            // escape quits daphne =]
    	    set_quitflag();
            break;

        case SDLK_F1:
    	    // blue button 1
	        cop_write_latch = 0x80;
	        m_irq_status &= ~0x20;
	        thayers_irq();
            break;

        case SDLK_F2:
            // blue button 2
	        cop_write_latch = 0x81;
	        m_irq_status &= ~0x20;
	        thayers_irq();
	        break;

        case SDLK_F3:
    	    // blue button 3
	        cop_write_latch = 0x82;
	        m_irq_status &= ~0x20;
	        thayers_irq();
            break;

        case SDLK_F4:
	        // blue button 4
	        cop_write_latch = 0x83;
	        m_irq_status &= ~0x20;
	        thayers_irq();
            break;

        case SDLK_F5:
    	    // insert coin left chute
	        banks[1] &= ~0x10;
            break;

        case SDLK_F6:
    	    // insert coin right chute
	        banks[1] &= ~0x20;
            break;

        case SDLK_F9:
    	    // show/hide overlay speech buffer subtitle
	        // to suppress warning message, as only care about keyup
            break;

        case SDLK_F10:
    	    // show/hide overlay "scoreboard"
	        // to suppress warning message, as only care about keyup
            break;

        case SDLK_F12:
    	    // take a screen shot
	        g_ldp->request_screenshot();
            break;

        case SDLK_PAGEUP:
            if (is_sound_enabled())
            {
                // speech volume up
                volume += volume >= AUDIO_MAX_VOLUME ? 0 : 8;
				set_soundchip_nonvldp_volume(volume);
            }

            break;

        case SDLK_PAGEDOWN:
            if (is_sound_enabled())
            {
                // speech volume down
                volume -= volume == 0 ? 0 : 8;
				set_soundchip_nonvldp_volume(volume);
            }

            break;

        default:
	        // else we recognized no keys so print an error
	        char s[81] = { 0 };

	        sprintf(s, "THAYERS: Unhandled keypress: %x", key);
	        printline(s);
            break;
	}
}

// this is called when the user releases a key
void thayers::process_keyup(SDLKey key)
{
	// left coin chute release
	if (key == SDLK_F5)
	{
		banks[1] |= 0x10;
	}

	// right coin chute release
	if (key == SDLK_F6)
	{
		banks[1] |= 0x20;
	}

	// show/hide overlay speech buffer subtitle
	if (key == SDLK_F9)
	{
        // Allow subtitled text to be shown/hid only if text-to-speech engine enabled.
        if (m_use_speech)
		    m_show_speech_subtitle = ! m_show_speech_subtitle;
	}

    // show/hide "Time Units" MPEG overlay, if option enabled
    if (key == SDLK_F10 && key_press && m_use_overlay_scoreboard)
    {
		m_bScoreboardVisibility = !m_bScoreboardVisibility;
		m_pScoreboard->ChangeVisibility(m_bScoreboardVisibility);
		m_video_overlay_needs_update |= m_pScoreboard->is_repaint_needed();
    }

	key_press = false;
}

// used to set dip switch values
bool thayers::set_bank(unsigned char which_bank, unsigned char value)
{
	bool result = true;
	
	switch (which_bank)
	{
	case 0:	
		banks[0] = (unsigned char) (value ^ 0xFF);	// dip switches are active low
		break;
	case 1:	
		banks[1] = (unsigned char) ((((value ^ 0xFF) & 0x0f)) | (banks[1] & 0xf0));	// dip switches are active low
		break;
	default:
		printline("ERROR: Bank specified is out of range!");
		result = false;
		break;
	}
	
	return result;
}

// Thayer's uses the scoreboard a little different than DL/SA so we need to unpack the data
void thayers::write_scoreboard(Uint8 address, Uint8 data, int den)
{
	if (address <= 5)
	{
		m_pScoreboard->update_player_score(address, data, den);
	}	
	else if (den == 0 && address <= 7)
	{
		m_pScoreboard->update_player_lives(data, address - 6);
	}
	else if (den == 1 && address <= 7)
	{
		m_pScoreboard->update_credits(address - 6 , data);
	}
	else
	{
		char s[81] = { 0 };

		sprintf(s, "THAYERS: Unsupported write to scoreboard: Address %x Data %x ", address, data);
		printline(s);
	}

	// let scoreboard interface decide if it needs to be repainted or not
	m_video_overlay_needs_update = m_pScoreboard->is_repaint_needed();

}

// These functions are used with the COP cpu core
void thayers::thayers_write_d_port(unsigned char d)
{
	// Data Ready INT
	if (!(d & 0x02))
	{
		m_irq_status &= ~0x20;
	}
	// Timer INT
	if (!(d & 0x01))
	{
		m_irq_status &= ~0x10;
	}
	// Do interrupt if either of the above are active
	if ((d & 0x03) != 0x03)
	{
		thayers_irq();
	}
}

void thayers::thayers_write_l_port(unsigned char l)
{
	if (l)
	{
	}
}

unsigned char thayers::thayers_read_l_port()
{
//	return cop_read_latch;
	return 0;
}

unsigned char thayers::thayers_read_g_port()
{
//	return cop_g_read_latch;
	return 0;
}

void thayers::thayers_write_so_bit(unsigned char) 
{
//	printline("Not implemented");
}

unsigned char thayers::thayers_read_si_bit()
{
	return 0;
}

void thayers::thayers_write_g_port(unsigned char) 
{
//	printline("Not implemented");
}
