/*
 * seektest.cpp
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

// seektest.cpp
// by Matt Ownby

// used to test mpeg seeking accuracy

#include <string.h>
#include "../io/conout.h"
#include "../ldp-out/ldp.h"
#include "seektest.h"
#include "../daphne.h"	// for get_quitflag
#include "../io/input.h"
#include "../video/palette.h"
#include "../video/video.h"
#include "../timer/timer.h"

// Win32 doesn't use strcasecmp, it uses stricmp (lame)
#ifdef WIN32
#define strcasecmp stricmp
#endif

////////////////////////////////////////////////////////////////////////////////

// frames for Dragon's Lair 2 multiple mpeg seektest
int g_dl2_frames[] = 
{
	1,
	721,
	2102,
	4427,
	4477,
	4527,
	4627,
	4677,
	4727,
	4777,
	4827,
	4877,
	4927,
	4977,
	5027,
	5077,
	5127,
	5177,
	5227,
	5277,
	5327,
	5377,
	5427,
	-1
};

////////////////////////////////////////////////////////////////////////////////

seektest::seektest()
:
	m_locked(true),
	m_overlay(true),
	m_frame_offset(0),
	m_multimpeg(false),
	m_multimpeg_frames(NULL)
{
	m_shortgamename = "seektest";
	m_disc_fps = 29.97;	// all these values ought to be changed.. use a preset
	m_early1 = 1;
	m_early2 = 1;
	m_late1 = 1;
	m_late2 = 1;
	strcpy(m_name, "[Undefined game]");
	m_video_overlay_width = 320;	// sensible default
	m_video_overlay_height = 240;	// " " "
	m_palette_color_count = 256;
	m_overlay_size_is_dynamic = true;	// this 'game' does reallocate the size of its overlay
}

void seektest::start()
{
	go(m_early1);	// seek to the first frame to get us started

	// if we're just doing a standard seektest
	if (!m_multimpeg)
	{
		while (!get_quitflag())
		{
			m_video_overlay_needs_update = true;
			video_blit();
			SDL_check_input();
			g_ldp->think_delay(10);	// don't hog cpu, and advance timer
		}
	}

	// if this is a multimpeg seektest...
	else
	{
		int index = 0;
		char frame[6];
		unsigned int timer = 0;

		// go until we do all the frames or until we are aborted early
		while ((m_multimpeg_frames[index] != -1) && (!get_quitflag()))
		{
			g_ldp->framenum_to_frame(m_multimpeg_frames[index], frame);
			g_ldp->pre_search(frame, true);
			g_ldp->pre_play();
			timer = refresh_ms_time();

			// wait for a bit to allow user to view a portion of the video to see that it is in sync
			while ((elapsed_ms_time(timer) < 2000) && !get_quitflag())
			{
				m_video_overlay_needs_update = true;
				video_blit();
				SDL_check_input();
				g_ldp->think_delay(10);	// don't hog CPU
			}
			index++;
		}
	}
}

// do the search, adjusting by the frame offset
void seektest::go(Uint16 target_frame)
{
	char s[81] = { 0 };

	target_frame = (Uint16) (target_frame + m_frame_offset);
	sprintf(s, "%05u", target_frame);

	g_ldp->pre_search(s, true);
}

void seektest::input_enable(Uint8 input)
{
	switch (input)
	{
	case SWITCH_UP:
		// if we're currently looking at early1, then seek to early2
		if ((g_ldp->get_current_frame() + m_frame_offset) == m_early1)
		{
			if (m_locked)
			{
				go((Uint16) (m_early2 - (m_frame_offset << 1)));
			}
			else
			{
				m_frame_offset = m_early2 - g_ldp->get_current_frame();
			}
		}
		else
		{
			if (m_locked)
			{
				go((Uint16) (m_early1 - (m_frame_offset << 1)));
			}
			else
			{
				m_frame_offset = m_early1 - g_ldp->get_current_frame();
			}
		}
		break;
	case SWITCH_DOWN:
		// if we're currently looking at late1, then seek to late2
		if ((g_ldp->get_current_frame() + m_frame_offset) == m_late1)
		{
			if (m_locked)
			{
				go((Uint16) (m_late2 - (m_frame_offset << 1)));
			}
			else
			{
				m_frame_offset = m_late2 - g_ldp->get_current_frame();
			}
		}
		else
		{
			if (m_locked)
			{
				go((Uint16) (m_late1 - (m_frame_offset << 1)));
			}
			else
			{
				m_frame_offset = m_late1 - g_ldp->get_current_frame();
			}
		}
		break;
	case SWITCH_LEFT:
		if (m_locked)
		{
			go((Uint16) (g_ldp->get_current_frame() - 1 - m_frame_offset));
		}
		else
		{
			m_frame_offset--;
		}
		break;
	case SWITCH_RIGHT:
		if (m_locked)
		{
			go((Uint16) (g_ldp->get_current_frame() + 1 - m_frame_offset));
		}
		else
		{
			m_frame_offset++;
		}
		break;
	case SWITCH_BUTTON1:
		// toggle locked status
		if (m_locked)
		{
			m_locked = false;
		}
		else
		{
			m_locked = true;
		}
		break;
	case SWITCH_BUTTON2:	// toggle video overlay
		m_overlay = !m_overlay;
		break;
	case SWITCH_START1:
		g_ldp->pre_play();
		g_ldp->pre_change_speed(1, 1);	// play at 1X
		break;
	case SWITCH_START2:
		g_ldp->pre_play();
		g_ldp->pre_change_speed(2, 1);	// play at 2X
		break;
	case SWITCH_SKILL1:
		g_ldp->pre_play();
		g_ldp->pre_change_speed(4, 1);	// play at 4X
		break;
	case SWITCH_SKILL2:
		g_ldp->pre_play();
		g_ldp->pre_change_speed(8, 1);	// play at 8X
		break;
	}
}

void seektest::input_disable(Uint8 input)
{
	// get rid of warnings
	if (input)
	{
	}
}

// set which disc you're testing
void seektest::set_preset(int val)
{
	switch (val)
	{
	case 0:	// dragon's lair
		m_disc_fps = 23.976;
		m_early1 = 323;
		m_early2 = 322;
		m_late1 = 31615;
		m_late2 = 31616;
		strcpy(m_name, "Dragon's Lair NTSC");
		break;
	case 1:	// space ace
		m_disc_fps = 23.976;
		m_early1 = 1161;	// end of attract mode
		m_early2 = 1162;	// first frame of There's Borf's Ship
		m_late1 = 33185;	// black screen (end of victory)
		m_late2 = 33186;	// first frame of Dexter scene in final showdown
		strcpy(m_name, "Space Ace '83 NTSC");
		break;
	case 2:	// super don
		m_disc_fps = 29.97;
		m_early1 = 5400;	// black frame
		m_early2 = 5401;	// purple-blue gradiant frame 'super don quixote'
		m_late1 = 35850;	// black frame
		m_late2 = 35851;	// first frame of witch and the announcer saying 'game over'
		strcpy(m_name, "Super Don Quix-ote");
		break;
	case 3:	// cliff hanger
		m_disc_fps = 29.97;
		m_early1 = 1544;	// last frame of attract mode with white text
		m_early2 = 1545;	// first frame of 1st stage still with white text
		m_late1 = 49665;	// blank screen
		m_late2 = 49666;	// first grey slide at end of disc
		strcpy(m_name, "Cliff Hanger");
		break;
	case 4:	// astron belt
		m_disc_fps = 29.97;
		m_early1 = 1940;	// space with small ship on left of screen
		m_early2 = 1941;	// first frame of the first explosion on the disc
		m_late1 = 51330;	// black screen
		m_late2 = 51331;	// first frame of right speaker test
		strcpy(m_name, "Astron Belt");
		break;
	case 5:	// galaxy ranger/star blazer
		m_disc_fps = 29.97;
		m_early1 = 1017;	// black screen
		m_early2 = 1018;	// start of launch scene right after titles
		m_late1 = 51667;	// black screen
		m_late2 = 51668;	// first frame of right speaker test
		strcpy(m_name, "Galaxy Ranger / Star Blazer");
		break;
	case 6:	// Thayer's Quest
		m_disc_fps = 29.97;
		m_early1 = 716;	// logo with light to the upper left of the T
		m_early2 = 717;	// logo with light having moved to the h a little bit
		m_late1 = 49990;	// relics of quoid
		m_late2 = 49991;	// sorsabal telling the black magician to walk into the camera :)
		strcpy(m_name, "Thayer's Quest Arcade NTSC");
		break;
	case 7:	// Cobra Command
		m_disc_fps = 29.97;
		m_early2 = 300;	// black frame
		m_early1 = 301;	// first frame that's not black.. helicopter coming over mountains
		m_late1 = 46152;	// a totally white frame
		m_late2 = 46153;	// a totally black frame
		strcpy(m_name, "Cobra Command / Thunderstorm");
		break;
	case 8:	// Esh's Aurunmilla (info from Antonio "Ace64" Carulli)
		m_disc_fps = 29.97;
		m_early1 = 450;	// black frame
		m_early2 = 451;	// first frame of attract
//		m_late1 = 39154;	// blue frame, last frame of last scene (before final game over)
//		m_late2 = 39155;	// black frame
		m_late1 = 39348;	// last frame of last game over
		m_late2 = 39349;	// black frame
		strcpy(m_name, "Esh's Aurunmilla");
		break;
	case 9:	// Badlands
		m_disc_fps = 29.97;
		m_early2 = 150;	// black frame
		m_early1 = 151;	// first frame of attract
		m_late1 = 44249;	// last frame of color test
		m_late2 = 44250;	// first frame of cross hatch
		strcpy(m_name, "Badlands");
		break;
	case 10:	// Bega's Battle
		m_disc_fps = 29.97;
		m_early1 = 300;	// first frame of attract
		m_early2 = 299;	// black frame
		m_late1 = 46733;	// last frame of starburst
		m_late2 = 46734;	// black frame
		strcpy(m_name, "Bega's Battle");
		break;
	case 11:	// Freedom Fighter
		m_disc_fps = 29.97;
		m_early1 = 30;	// yellow slide
		m_early2 = 31;	// magenta slide
		m_late1 = 43049;	// last non-black frame on disc (blue, train going into sky)
		m_late2 = 43050;	// black frame (second to last on disc)
		strcpy(m_name, "Freedom Fighter");
		break;
	case 12:	// GP World
		m_disc_fps = 29.97;
		m_early1 = 5039; // black
		m_early2 = 5040; // first frame of logo
		m_late1 = 39569; // "next race"
		m_late2 = 39570; // black
		strcpy(m_name, "GP World");
		break;
	case 13:	// Dragon's Lair PAL (Original)
		m_disc_fps = 25.0;	// PAL speed
		m_early1 = 323 - 152;
		m_early2 = 322 - 152;
		// don't have any idea what the end frames should be
		strcpy(m_name, "Dragon's Lair PAL (Original)");
		break;
	case 14:	// Dragon's Lair PAL (Software Corner)
		m_disc_fps = 25.0;
		m_early1 = 323 - 230;
		m_early2 = 322 - 230;
		// don't know what to do for late frames
		strcpy(m_name, "Dragon's Lair PAL (Software Corner)");
		break;
	case 15:	// Interstellar
		m_disc_fps = 29.97;
		m_early1 = 1800; // black
		m_early2 = 1801; // first frame of attract
		m_late1 = 38340; // black
		m_late2 = 38341; // color test
		strcpy(m_name, "Interstellar");
		break;
	case 16:	// Dragon's Lair 2 (one single large mpeg2, obviously not appropriate for split mpegs)
		m_disc_fps = 29.97;
		m_early1 = 6;	// Dragon's Lair II TIMEWARP logo
		m_early2 = 7;	// white grid on black background
		m_late1 = 46349;	// last frame of test pattern
		m_late2 = 46350;	// totally black frame
		strcpy(m_name, "Dragon's Lair 2 (single mpeg)");
		break;
	case 17:	// MACH 3 (thanks to Troy_G for this info)
		m_disc_fps = 29.97;
		m_early1 = 2341;	// last frame of split screen attract mode
		m_early2 = 2342;	// black screen (according to Matteo Marioni)
		//m_early2 = 2343;	// black screen
		m_late1 = 52649;	// black screen
		m_late2 = 52650;	// color bars	 (according to Matteo Marioni)
		//m_late2 = 52651;	// color bars
		break;
	case 18:	// Dragon's Lair 2 (multiple mpegs)
		m_multimpeg = true;
		m_multimpeg_frames = g_dl2_frames;
		strcpy(m_name, "Dragon's Lair 2 (multiple mpeg)");
		break;
	case 19:	// Us Vs Them
		m_disc_fps = 29.97;
		m_early1 = 30;	// black frame interlaced with first visible frame of city
		m_early2 = 29;	// all black frame
		m_late1 = 53580;	// black frame interlaced with last frame of president
		m_late2 = 53581;	// all black frame
		strcpy(m_name, "Us vs Them");
		break;
	case 20:	// Space Ace '91
		m_disc_fps = 29.97;
		// NOTE : THESE FRAMES ARE UNVERIFIED, I CAME UP WITH THEM BY USING THE FORMULA IN FRAMEMOD.CPP!!!
		m_early1 = 1686;	// interlaced frame before "there's borfs ship"
		m_early2 = 1687;	// first full frame of "there's borfs ship"
		strcpy(m_name, "Space Ace '91");
		break;
	case 21:	// Time Traveler
		m_disc_fps = 29.97;
		m_early1 = 352;
		m_early2 = 353;
		m_late1 = 53645;
		m_late2 = 53646;
		strcpy(m_name, "Time Traveler");
		break;
	case 22:	// Mad Dog 1
		m_disc_fps = 29.97;
		m_early1 = 137;
		m_early2 = 138;
		m_late1 = 38970;
		m_late2 = 38971;
		strcpy(m_name, "Mad Dog 1");
		break;
	default:
		printline("SEEKTEST ERROR : unknown preset");
		break;
	}
}

// game-specific command line arguments handled here
bool seektest::handle_cmdline_arg(const char *arg)
{
	bool result = true;	// assume true unless otherwise specified

	// it's hard to remember numbers, so this is an alternate way to specify which disc you want

	if (strcasecmp(arg, "-lair")==0)
	{
		set_preset(0);
	}
	else if (strcasecmp(arg, "-ace")==0)
	{
		set_preset(1);
	}
	else if (strcasecmp(arg, "-sdq")==0)
	{
		set_preset(2);
	}
	else if (strcasecmp(arg, "-cliff")==0)
	{
		set_preset(3);
	}
	else if (strcasecmp(arg, "-astron")==0)
	{
		set_preset(4);
	}
	else if (strcasecmp(arg, "-galaxy")==0)
	{
		set_preset(5);
	}
	else if (strcasecmp(arg, "-tq")==0)
	{
		set_preset(6);
	}
	else if (strcasecmp(arg, "-cobra")==0)
	{
		set_preset(7);
	}
	else if (strcasecmp(arg, "-esh")==0)
	{
		set_preset(8);
	}
	else if (strcasecmp(arg, "-badlands")==0)
	{
		set_preset(9);
	}
	else if (strcasecmp(arg, "-bega")==0)
	{
		set_preset(10);
	}
	else if (strcasecmp(arg, "-ffr")==0)
	{
		set_preset(11);
	}
	else if (strcasecmp(arg, "-gpworld")==0)
	{
		set_preset(12);
	}
	else if (strcasecmp(arg, "-dlpal")==0)
	{
		set_preset(13);
	}
	else if (strcasecmp(arg, "-dlsc")==0)
	{
		set_preset(14);
	}
	else if (strcasecmp(arg, "-interstellar")==0)
	{
		set_preset(15);
	}
	else if (strcasecmp(arg, "-lair2")==0)
	{
		set_preset(18);	// 16 isn't as mainstream
	}
	else if (strcasecmp(arg, "-mach3")==0)
	{
		set_preset(17);
	}
	else if (strcasecmp(arg, "-uvt")==0)
	{
		set_preset(19);
	}
	else if (strcasecmp(arg, "-ace91")==0)
	{
		set_preset(20);
	}
	else if (strcasecmp(arg, "-timetrav")==0)
	{
		set_preset(21);
	}
	else if (strcasecmp(arg, "-maddog")==0)
	{
		set_preset(22);
	}

	// no match
	else
	{
		result = false;
	}

	return result;
}


void seektest::palette_calculate()
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

// redraws video
void seektest::video_repaint()
{
	Uint32 cur_w = g_ldp->get_discvideo_width() >> 1;	// width overlay should be
	Uint32 cur_h = g_ldp->get_discvideo_height() >> 1;	// height overlay should be
	char s[81] = { 0 };

	// if the width or height of the mpeg video has changed since we last were here (ie, opening a new mpeg)
	// then reallocate the video overlay buffer
	if ((cur_w != m_video_overlay_width) || (cur_h != m_video_overlay_height))
	{
		printline("SEEKTEST : Surface does not match mpeg, re-allocating surface!"); // remove me
		if (g_ldp->lock_overlay(1000))
		{
			m_video_overlay_width = cur_w;
			m_video_overlay_height = cur_h;
			video_shutdown();
			if (!video_init())
			{
				printline("Fatal Error, trying to re-create the surface failed!");
				set_quitflag();
			}
			g_ldp->unlock_overlay(1000);	// unblock game video overlay
		}
		else
		{
			printline("SEEKTEST ERROR : Timed out trying to get a lock on the yuv overlay");
			return;
		}
	} // end if dimensions are incorrect

	SDL_FillRect(m_video_overlay[m_active_video_overlay], NULL, 0);	// erase anything on video overlay

	// if video overlay is enabled
	if (m_overlay)
	{
		draw_string(m_name, 1, 1, m_video_overlay[m_active_video_overlay]);	// draw the game name

		sprintf(s, "%u x %u", cur_w << 1, cur_h << 1);
		draw_string(s, 1, 2, m_video_overlay[m_active_video_overlay]);	// draw the resolution of the mpeg

		// only present the additional options if this isn't a multimpeg test
		if (!m_multimpeg)
		{
			sprintf(s, "Current frame : %d", g_ldp->get_current_frame() + m_frame_offset);
			if (m_locked)
			{
				strcat(s, " (LOCKED)");
			}
			else
			{
				strcat(s, " (UNLOCKED)");
			}
			draw_string(s, 1, 3, m_video_overlay[m_active_video_overlay]);
		
			if (m_frame_offset != 0)
			{
				sprintf(s, "* Adjust framefile by %d frames *", m_frame_offset);
				draw_string(s, 1, 4, m_video_overlay[m_active_video_overlay]);
			}

			draw_string("1      - Play the disc at 1X", 1, 8, m_video_overlay[m_active_video_overlay]);
			draw_string("2      - Play the disc at 2X", 1, 9, m_video_overlay[m_active_video_overlay]);
			draw_string("SKILL1 - Play the disc at 4X", 1, 10, m_video_overlay[m_active_video_overlay]);
			draw_string("SKILL2 - Play the disc at 8X", 1, 11, m_video_overlay[m_active_video_overlay]);
			draw_string("FIRE1  - Lock/Unlock frame", 1, 12, m_video_overlay[m_active_video_overlay]);
			draw_string("FIRE2  - Toggle Text Overlay", 1, 13, m_video_overlay[m_active_video_overlay]);
			sprintf(s, "UP     - toggle between %u and %u", m_early1, m_early2);
			draw_string(s, 1, 14, m_video_overlay[m_active_video_overlay]);

			sprintf(s, "DOWN   - toggle between %u and %u", m_late1, m_late2);
			draw_string(s, 1, 15, m_video_overlay[m_active_video_overlay]);
		
			draw_string("LEFT   - steps down one frame", 1, 16, m_video_overlay[m_active_video_overlay]);
			draw_string("RIGHT  - steps up one frame", 1, 17, m_video_overlay[m_active_video_overlay]);
		}
	} // end if overlay is enabled
	
	// else if overlay is disabled, don't draw anything
}
