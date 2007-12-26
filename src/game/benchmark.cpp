/*
 * benchmark.cpp
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

// benchmark.cpp
// by Matt Ownby

// basically just starts playing the disc :)
// Used to test VLDP efficiency
// VLDP must be recompiled to enable framerate reporting

#include <string.h>
#include "../daphne.h"	// for get_quitflag/set_quitflag
#include "../io/conout.h"
#include "../ldp-out/ldp.h"
#include "benchmark.h"
#include "../io/input.h"
#include "../video/palette.h"
#include "../video/video.h"
#include "../timer/timer.h"

benchmark::benchmark() :
	m_seconds_to_run(6)	// how long the benchmark runs by default
{
	m_shortgamename = "benchmark";
	m_disc_fps = 29.97;
	m_game_uses_video_overlay = false;
}

void benchmark::start()
{
	g_ldp->play();	// start the disc playing immediately
	
	Uint32 timer = refresh_ms_time();
	Uint32 ms_to_run = m_seconds_to_run * 1000;	// how many ms to run the test ...

	if (m_game_uses_video_overlay)
	{
		// draw overlay on all surfaces
		for (int i = 0; i < m_video_overlay_count; i++)
		{
			m_video_overlay_needs_update = true;
			video_blit();
		}
	}

	// run for 60 seconds
	while (!get_quitflag() && (elapsed_ms_time(timer) < ms_to_run))
	{
		SDL_check_input();
		SDL_Delay(1000);	// don't hog CPU
	}
}

void benchmark::set_preset(int val)
{
	switch (val)
	{
	case 1:	// benchmark test using video overlay
		m_game_uses_video_overlay = true;
		m_video_overlay_width = 320;	// sensible default
		m_video_overlay_height = 240;	// " " "
		m_palette_color_count = 256;
		m_overlay_size_is_dynamic = true;	// this 'game' does reallocate the size of its overlay
		m_video_overlay_needs_update = true;	// we only need to update it once
		break;
	default:
		printline("Unknown preset!");
		break;
	}
}

void benchmark::palette_calculate()
{
	SDL_Color color;
	color.r = color.b = color.g = 255;	// white
	palette_set_color(255, color);	// set it
}

void benchmark::video_repaint()
{
	Uint32 cur_w = g_ldp->get_discvideo_width() >> 1;	// width overlay should be
	Uint32 cur_h = g_ldp->get_discvideo_height() >> 1;	// height overlay should be

	// if the width or height of the mpeg video has changed since we last were here (ie, opening a new mpeg)
	// then reallocate the video overlay buffer
	if ((cur_w != m_video_overlay_width) || (cur_h != m_video_overlay_height))
	{
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
		}
	} // end if dimensions are incorrect

	SDL_FillRect(m_video_overlay[m_active_video_overlay], NULL, 0);	// erase anything on video overlay
	Uint8 *pos = (Uint8 *) m_video_overlay[m_active_video_overlay]->pixels;
	
	// alternate between transparent and non-transparent, skip 2 pixels
	for (int count = 0; count < (m_video_overlay[m_active_video_overlay]->w * m_video_overlay[m_active_video_overlay]->h); count += 2)
	{
		*(pos++) = 0;	// transparent
		*(pos++) = 0xFF;	// white
	}
}
