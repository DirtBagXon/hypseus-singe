/*
 * vip9500sg.cpp
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


// vip9500sg.c
// part of the DAPHNE emulator
// written by Mark Broadhead
// contributions made by Matt Ownby
//
// This code emulates the Hitachi VIP-9550SG laserdisc player which is used in
//  Astron Belt, Galaxy Ranger, and others running on the same hardware

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <queue>
#include "../game/game.h"
#include "vip9500sg.h"
#include "../io/conout.h"
#include "../ldp-out/ldp.h"
#include "../daphne.h"	// for set_quitflag

using namespace std;

// holds serial data returns by the VIP9500
queue <unsigned char> g_qu8VIP9500Output;

char vip9500sg_frame[FRAME_ARRAY_SIZE] = {0}; // holds the digits sent to the VIP9500SG
int vip9500sg_frame_index = 0; 

// default status
unsigned int enter_status = 0;

// whether the laserdisc player is searching or not
bool g_bVIP9500SG_Searching = false;

///////////////////////////////////////////

// retrieves the status from our virtual VIP9500SG
unsigned char read_vip9500sg()
{
	unsigned char result = 0x00;

	if (!g_qu8VIP9500Output.empty())
	{
		result = g_qu8VIP9500Output.front();
		g_qu8VIP9500Output.pop();
	}
	else
	{
		printline("ERROR: VIP9500SG queue read when empty");
	}
	
	return(result);
}

// pushes a value on the vip9500sg queue
bool vip9500sg_queue_push(unsigned char value)
{
	g_qu8VIP9500Output.push(value);

	// doesn't return an error because this almost never can fail
	return true;
}

// sends a byte to our virtual VIP9500SG
void write_vip9500sg (unsigned char value)
{
	char s[81] = { 0 };
	switch (value)
	{
	case 0x25: // Play
		g_ldp->pre_play();
		vip9500sg_queue_push(0xa5); // play success
		break;
	case 0x29: // Step Reverse
		// Astron, GR, and Cobra Command only seem to use this for pause
		g_ldp->pre_pause();
		printline("VIP9500SG: Step Reverse (pause)");
		vip9500sg_queue_push(0xa9); // step backwards success
		break;
	case 0x2b: // Search 
		enter_status = VIP9500SG_SEARCH;
		break;
	case 0x2f: // Stop
		printline("VIP9500SG: Reject received (ignored)");
		vip9500sg_queue_push(0xaf); // stop success
		break;
	case 0x30: // '0'
	case 0x31: // '1'
	case 0x32: // '2'
	case 0x33: // '3'
	case 0x34: // '4'
	case 0x35: // '5'
	case 0x36: // '6'
	case 0x37: // '7'
	case 0x38: // '8'
	case 0x39: // '9'
		vip9500sg_add_digit(value);
		break;
	case 0x41: // Enter
		vip9500sg_enter();
		break;
	case 0x46: // Skip Forward
		enter_status = VIP9500SG_SKIP_FORWARD;
		break;
	case 0x4C:	// frame counter on, NOTE : if disc is stopped this will return an error 0x1D
		printline("VIP9500SG: Frame counter on (ignored)");
		vip9500sg_queue_push(0xcc); // frame counter on success
		break;
	case 0x4d: // Frame counter off, NOTE : if disc is stopped this will return an error 0x1D
		printline("VIP9500SG: Frame counter off (ignored)");
		vip9500sg_queue_push(0xcd); // frame counter off?
		break;
	case 0x50:	// Pause with sound enabled (!!)
	case 0x51:	// very slow motion with sound enabled
	case 0x52:	// slow motion with sound enabled
	case 0x54:	// play faster with sound enabled
	case 0x55:	// play even faster with sound enabled
	case 0x56:	// " " "
	case 0x57:	// " " "
	case 0x58:	// " " "
	case 0x59:	// " " "
		printline("VIP9500SG : Unsupported variable speed command requested!");
		break;
	case 0x53: // Play forward at 1X with sound enabled, note that if disc is stopped this will return an error 0x1D
		g_ldp->pre_play();
		vip9500sg_queue_push(0xd3); // forward 1X success
		break;
	case 0x68: // Reset
		// Do some reset stuff - turn off frame counter etc		
		printline("VIP9500SG: RESET! (ignored)");
		vip9500sg_queue_push(0xe8); // reset success
		break;
	case 0x6b: // Report Current Frame
		{
			unsigned int curframe = g_ldp->get_current_frame();
			vip9500sg_queue_push(0x6b); // frame
			vip9500sg_queue_push((Uint8) ((curframe >> 8) & 0xff)); // high byte of frame
			vip9500sg_queue_push((Uint8) (curframe & 0xff)); // low byte of frame
		}
		break;
	case 0x6e: // UNKNOWN
		vip9500sg_queue_push(0xee); // unknown
		break;
	default:
		sprintf(s,"Unsupported VIP9500SG Command Received: %x", value);
		printline(s);
		break;
	}
}

void vip9500sg_think()
{
	// if we're waiting for a search to complete ...
	if (g_bVIP9500SG_Searching)
	{
		int iStat = g_ldp->get_status();

		// if we're paused, it means we're done seeking
		if (iStat == LDP_PAUSED)
		{
#ifdef DEBUG
			printline("VIP9500SG: Search Complete");
#endif
			vip9500sg_queue_push(0x41); // search success
			vip9500sg_queue_push(0xb0);			
			g_bVIP9500SG_Searching = false;
		}
		// else if we're not seeking as we expect to be, return an error
		else if (iStat != LDP_SEARCHING)
		{
			// if this ever happens, we'll need to find out what the search error result code is :)
			printline("VIP9500SG: search failed and we don't handle this condition so we're aborting");
			set_quitflag();
		}
		// else we're still searching
	}
	// else nothing to do
}

bool vip9500sg_result_ready(void)
{
	bool result = false;

	vip9500sg_think();

	// if the queue is not empty, then we have a result ready
	if (!g_qu8VIP9500Output.empty())
	{
		result = true;
	}
	return result;
}

void vip9500sg_enter(void)
{
	if (enter_status == VIP9500SG_SEARCH)
	{
		// make sure that the ROM checked the search result before sending another search command
		if (!g_bVIP9500SG_Searching)
		{
			// TODO: check for search failure
			g_ldp->pre_search(vip9500sg_frame, false);		
			g_bVIP9500SG_Searching = true;
		}
		else
		{
			printline("VIP9500SG WARNING: ROM did not check search result before sending another search command");
			printline("(therefore we are ignoring the second search command)");
		}

		vip9500sg_frame_index = 0; // reset frame index
	}
	
	else if (enter_status == VIP9500SG_SKIP_FORWARD)
	{
		// TODO: check for skip failure
		vip9500sg_frame[vip9500sg_frame_index] = 0;

		Uint16 frames_to_skip = (Uint16) atoi(vip9500sg_frame);
		
		g_ldp->pre_skip_forward(frames_to_skip);
		
		vip9500sg_frame_index = 0; // reset frame index

		vip9500sg_queue_push(0x41); // skip success
		vip9500sg_queue_push(0xc6);
	}
	
	enter_status = 0x00;
}

// adds a digit to  the frame array that we will be seeking to
// digit should be in ASCII format
void vip9500sg_add_digit(char digit)
{
	if (vip9500sg_frame_index < FRAME_SIZE)
	{
		vip9500sg_frame[vip9500sg_frame_index] = digit;
		vip9500sg_frame_index++;
	}
	else
	{
		char s[81] = { 0 };

		sprintf(s, "Too many digits received for frame! (over %d)", FRAME_SIZE);
		printline(s);
	}
}

