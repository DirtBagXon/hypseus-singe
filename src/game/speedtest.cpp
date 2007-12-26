/*
 * speedtest.cpp
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


// speedtest.cpp
// by Matt Ownby

#include <string.h>
#include "speedtest.h"
#include "../ldp-out/ldp.h"
#include "../io/conout.h"
#include "../timer/timer.h"

#define NUM_TESTS 10

// 100 worked for most players, but the Philips choked
#define DELAY_BETWEEN_TESTS 500

// frame is the last frame on the disc you wish to seek to for the speed test
speedtest::speedtest()
{
	m_shortgamename = "speedtest";
	strcpy(m_max_frame, "31500"); // use dragon's lair 24fps disc by default
	m_game_uses_video_overlay = false;
}

void speedtest::set_preset(int i)
{
	switch (i)
	{
	case 0:	// dragon's lair NTSC, 24 fps
		strcpy(m_max_frame, "31500");
		break;
	case 1:	// dragon's lair 2 NTSC, 30 fps
		strcpy(m_max_frame, "46800");
		break;
	default:
		printline("ERROR: unknown preset specified!  Doing Dragon's Lair '83 NTSC");
		break;
	}
}

void speedtest::start()
{
	Uint32 cur_time = 0;
	Uint32 forward[NUM_TESTS] = { 0 };	// results going forward
	Uint32 backward[NUM_TESTS] = { 0 }; // results going backward
	double avg_forward = 0.0;
	double avg_backward = 0.0;
	char s[81] = { 0 };
	int attempt = 0;
	int i = 0;	// generic index for averaging
	double temp = 0.0;	// generic double for averaging

	printline("Executing speed test ...");

	g_ldp->pre_play();	// start the disc playing
	g_ldp->pre_search("00001", true);	// go to frame #1

	while (attempt < NUM_TESTS)
	{
		sprintf(s, "Attempt #%d", attempt);
		printline(s);

		make_delay(DELAY_BETWEEN_TESTS);
		cur_time = refresh_ms_time();
		g_ldp->pre_search(m_max_frame, true);
		forward[attempt] = elapsed_ms_time(cur_time);

		sprintf(s, "It took %u ms to go from frame 1 to frame %s.", forward[attempt], m_max_frame);
		printline(s);

		make_delay(DELAY_BETWEEN_TESTS);
		cur_time = refresh_ms_time();
		g_ldp->pre_search("00001", true);	// go to beginning of disc
		backward[attempt] = elapsed_ms_time(cur_time);

		sprintf(s, "It took %u ms to go from frame %s to frame 1.", backward[attempt], m_max_frame);
		printline(s);

		attempt++;
	}
	printline("*** FINAL RESULTS ***");

	temp = 0.0;	// for averaging
	for (i = 0; i < NUM_TESTS; i++)
	{
		temp += forward[i];
	}
	avg_forward = temp / NUM_TESTS;

	temp = 0.0;
	for (i = 0; i < NUM_TESTS; i++)
	{
		temp += backward[i];
	}
	avg_backward = temp / NUM_TESTS;

	sprintf(s, "Average forward seek speed: %f", avg_forward);
	printline(s);
	sprintf(s, "Average backward seek speed: %f", avg_backward);
	printline(s);
	sprintf(s, "Overall average: %f", (avg_forward+avg_backward)/2.0 );
	printline(s);

	char frame[6] = {0};

	// do a formal seek to 0 using pre_search so that the is_playing boolean gets cleared
	for (i = 0; i < 5; i++)
	{
		frame [i] = (unsigned char) (i + '0');
	}
	
	g_ldp->pre_search(frame, true);
	g_ldp->pre_play();
}
