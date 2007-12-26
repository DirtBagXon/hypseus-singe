/*
 * ffr.cpp
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


// ffr.cpp
// by Matt Ownby

// just a little exercise to try to see if I can get a clip from the Freedom Fighter laserdisc
// to play correctly (using skipping)

#include "ffr.h"
#include "../daphne.h"	// for get_quitflag
#include "../ldp-out/ldp.h"
#include "../timer/timer.h"
#include "../io/conout.h"

struct clip_boundary
{
	unsigned int start;
	unsigned int end;
};

struct clip_boundary ffr_clips[] =
{
	//{ 3449, 3546 },	// more of an intro
	{ 3530, 3546 },	// less of an intro
	{ 3557, 3568 },
	{ 3589, 3598 },
	{ 3617, 3626 },
	{ 3665, 3670 },
	{ 3707, 3712 },
	{ 3749, 3756 },
	{ 3797, 3802 },
	{ 3841, 3846 },
	{ 3883, 3890 },
	{ 3929, 3934 },
	{ 3975, 3980 },
	{ 4017, 4024 },
	{ 4063, 4068 },
	{ 4109, 4114 },
	{ 4151, 4158 },
	{ 4197, 4202 },
	{ 4243, 4248 },
	{ 4285, 4292 },
	{ 4331, 4336 },
	{ 4377, 4382 },
	{ 4419, 4426 },
	{ 4465, 4470 },
	{ 4511, 4516 },
	{ 4553, 4560 },
	{ 4599, 4604 },
	{ 4647, 4650 },
	{ 0, 0 }
};

// super don test clip
struct clip_boundary sdq_clips[] =
{
	{ 5900, 5984 },
	{ 6065, 6300 },
	// note : rom goes from 6341 to 6533
	{ 6341, 6352 },
	{ 6354, 6533 },

	{ 6642, 6739 },	// rom 6641
	{ 6808, 6850 },	// rom 6810
	{ 10400, 10434 },
	{ 10485, 10551 },
	{ 10602, 10705 },
	{ 10756, 10824 },
	{ 10908, 10950 },	// rom 10915
	{ 11034, 11076 },	// rom 11041
	{ 11148, 11160 },	// rom goes to 11147 which is a bad frame
	{ 16100, 16162 },
	{ 16209, 16268 },	// rom 16213
	{ 16328, 16355 },	// rom 16328
	{ 16415, 16533 },	// rom 16416
	{ 16579, 16658 },	// rom 16584
	{ 16704, 16938 },	// rom 16709
	{ 16968, 17026 },	// rom 16969
	{ 17058, 17100 },	// rom 17057 (1 frame too early)
	{ 0, 0 }
};

struct clip_boundary sdqshort_clips[] =
{
	{ 5961, 5984 },
	{ 6065, 6075 },
	{ 0, 0 }
};

// constructor
ffr::ffr() : m_uTimer(0)
{
	m_shortgamename = "ffr";
	m_disc_fps = 29.97;	// Freedom Fighter disc is 29.97 fps
	m_game_uses_video_overlay = false;

	// default to freedom fighter disc
	m_pClips = ffr_clips;
}

//#define SEARCH_METHOD 1

// here's where we do the search loop
void ffr::start()
{
	bool finished = false;
	int index = 0;
	char s[6] = { 0 };	// holds frame
	unsigned int uThinkCounter = 0;	// so we can check input regularly ...
//	int i = 0;
	
	g_ldp->pre_play();	// make sure the disc is playing
	
	m_uTimer = GET_TICKS();

	// new behavior: loop until user quits so we can test this thing exhaustively
	while (!get_quitflag())
	{
		finished = false;
		index = 0;
#ifdef SEARCH_METHOD	
		while (!finished && !get_quitflag())
		{
			unsigned int start_frame = m_pClips[index].start;

			// 0 means we're done
			if (start_frame)
			{
				sprintf(s, "%05d", start_frame);
				g_ldp->pre_search(s, true);	// do the seek
				g_ldp->pre_play();	// and start playing

				// sleep, waiting for frame number to get to last frame of clip
				while (g_ldp->get_current_frame() < m_pClips[index].end)
				{
					think();
					++uThinkCounter;

					// if it's time to check to see if we need to quit
					if (uThinkCounter > 30)
					{
						SDL_check_input();
						uThinkCounter = 0;
					}
				}

				index++;	// go to the next clip, initiate next search
			}
			else
			{
				finished = true;
			}
		}
#else
		sprintf(s, "%05d", m_pClips[0].start);
		g_ldp->pre_search(s, true);	// do the seek
		g_ldp->pre_play();	// and start playing

		while (!finished && !get_quitflag())
		{
			// sleep, waiting for frame number to get to last frame of clip
			while ((g_ldp->get_current_frame() < m_pClips[index].end) && (!get_quitflag()))
			{
				think();

					++uThinkCounter;

					// if it's time to check to see if we need to quit
					if (uThinkCounter > 30)
					{
						SDL_check_input();
						uThinkCounter = 0;
					}

			}

			// we're now on the last frame of the sequence

		// safety check ...
		if (g_ldp->get_current_frame() != m_pClips[index].end)
		{
			printline("FFR FAILED! Current frame was past where it should've been!");
			finished = true;
			break;
		}

// 3546 to 3557

			// if we have another sequence to skip to, then do so		
			if (m_pClips[index+1].start)
			{
				g_ldp->pre_skip_forward((Uint16) (m_pClips[index+1].start - m_pClips[index].end));
			}
			else
			{
				finished = true;
			}

			index++;	// go to the next clip
		}
#endif
	} // end loop until user quits
}

void ffr::think()
{
	static const unsigned int uTICK_MS = 1;	// how many ms equals 1 virtual millisecond (this can be used to slow things down)
	unsigned int uTicks = GET_TICKS();
	unsigned int uDiff = uTicks - m_uTimer;

	if (uDiff > uTICK_MS)
	{
		g_ldp->pre_think();	// do a virtual millisecond
		m_uTimer += uTICK_MS;

		// if we're really lagged, it probably means we got interrupted, so just set the timer equal ...
		if (uDiff > 60)
		{
			m_uTimer = uTicks;
		}
	}

	// only delay if we're caught up
	else
	{
		MAKE_DELAY(1);
	}
}

void ffr::set_preset(int iPreset)
{
	switch (iPreset)
	{
	default:
		break;
	case 1:
		m_pClips = sdq_clips;
		break;
	case 2:
		m_pClips = sdqshort_clips;
		break;
	}
}

