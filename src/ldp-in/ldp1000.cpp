/*
 * ldp1000.cpp
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

// ldp1000.cpp
// part of the DAPHNE emulator
// written by Mark Broadhead
//
// lDP-1450 command and text handling added by Paul Blagay 2003
//
// This code emulates the Sony LDP-1000 laserdisc player which is used in
// Bega's Battle, Cobra Command (dedicated), and Road Blaster.
// It is also compatible with the Sony LDP-1450, used in Dragon's Lair 2 and
//  Space Ace '91.

#include <stdio.h>
#include <stdlib.h>	// for atoi
#include <string.h>	// memset, strcpy
#include "../game/game.h"
#include "ldp1000.h"
#include "../io/conout.h"
#include "../io/numstr.h"
#include "../ldp-out/ldp.h"
#include "../video/video.h"
#include "../daphne.h"

#include <queue>	// for queueing up LDP responses

using namespace std;	// for make <queue> work

queue <unsigned char> g_qu8LdpOutput;	// what LDP returns to user
unsigned int ldp1000_repeat_frame;
unsigned int ldp1000_repeat_start_frame;
ldp1000_state g_uLDP1000State = LDP1000_STATE_NORMAL;
int g_iLDP1000TimesToRepeat = 0;

char ldp1000_frame[FRAME_ARRAY_SIZE] = {0}; // holds the digits sent to the LDP1000
int ldp1000_frame_index = 0; 

// if we get a new search while a current search is aborting, then we stash it in here
char g_ldp1000_queued_frame[FRAME_ARRAY_SIZE] = {0};

// LDP1450 - Text Display (up to 3 lines)
// NOTE, you will need to handle the rendering of these strings
// in your engine.
ldp_text			g_LDP1450_Strings[3];
ldp_text_control	g_LDP1450_TextControl;

// default status
static unsigned char enter_status = 0x00;

unsigned int g_ldp1000_clear_cycles = 0;
unsigned int g_ldp1000_number_cycles = 0;
unsigned int g_ldp1000_enter_cycles = 0;
unsigned int g_ldp1000_play_cycles = 0;

bool g_bLDP1000_Waiting4Event = false;

///////////////////////////////////////////

// clear all the crap
void reset_ldp1450_globals()
{
	// ldp1450 text stuff
	memset ( &g_LDP1450_TextControl, 0, sizeof(ldp_text_control));

	// clear the ldp strings
	for (int i=0; i<3; i++)
		g_LDP1450_Strings[i].String[0] = 0;
}

// should get called by the cpu emulation before emulation begins to get our cycle delays set up
void reset_ldp1000()
{
	Uint32 cpu_hz = get_cpu_hz(0);
	double dCyclesPerMs = cpu_hz / 1000.0;	// cycles per milisecond

	// Compute how many CPU cycles we must delay before ACK'ing certain commands
	// (Bega's Battle relies on us having some latency after a clear command for example)
	// NOTE : this values come from the LDP1000A programming manual, they may be different for the LDP-1450
	g_ldp1000_clear_cycles = (unsigned int) ((dCyclesPerMs * 4.3) + 0.5);
	g_ldp1000_number_cycles = (unsigned int) ((dCyclesPerMs * 1.2) + 0.5);
	g_ldp1000_enter_cycles = (unsigned int) ((dCyclesPerMs * 2.6) + 0.5);
	g_ldp1000_play_cycles = (unsigned int) ((dCyclesPerMs * 2.0) + 0.5);

}

void ldp1000_event_callback(void *unused)
{
	// we got our event, we're done ...
	g_bLDP1000_Waiting4Event = false;
}

// create some latency so that commands don't ACK immediately
void ldp1000_make_ack_latency(unsigned int uCycles)
{
	g_bLDP1000_Waiting4Event = true;
	cpu_set_event(0, uCycles, ldp1000_event_callback, NULL);
}

////////////////////////////////////////////////////////

// retrieves the status from our virtual LDP1000
unsigned char read_ldp1000()
{
	unsigned char result = 0x00;

	// make sure queue is not empty and that we're not waiting for an event
	if ((!g_qu8LdpOutput.empty()) && (!g_bLDP1000_Waiting4Event))
	{
		result = g_qu8LdpOutput.front();
		g_qu8LdpOutput.pop();
	}
	// error, queue was empty, this should never happen
	else
	{
		printline("ERROR: LDP1000 read when empty, this should never happen");
		set_quitflag();	// game drivers should always check to see if a response is pending before calling this, force dev to deal with this
	}

	/*
	// handy for debugging, keep commented out for performance reasons
	string msg = " [R: 0x" + numstr::ToStr(result, 16) + "]";
	printline(msg.c_str());
	*/

	return(result);
}

// pushes a value on the ldp1000 queue
bool ldp1000_queue_push(unsigned char value)
{
	g_qu8LdpOutput.push(value);
	return true;	// the queue can't easily overflow, so we won't check for it
}

// sends a byte to our virtual LDP1000
void write_ldp1000 (unsigned char value)
{

	/*
	// handy for debugging, keep commented out for performance reasons
	string msg = "[T: 0x" + numstr::ToStr(value, 16) + "] ";
	newline();
	outstr(msg.c_str());
	*/

	// text control
	// XY Position tags
	if (g_LDP1450_TextControl.TextGetXY)
	{
		if (g_LDP1450_TextControl.TextGotXpos == 0)
		{
			g_LDP1450_TextControl.TextGotXpos = 1;
			g_LDP1450_TextControl.TextXPos = value;
		}
		else if (g_LDP1450_TextControl.TextGotYpos == 0)
		{
			g_LDP1450_TextControl.TextGotYpos = 1;
			g_LDP1450_TextControl.TextYPos = value;
		}
		else
		{
			// Text Mode.. we are using this for scale
			// for now.. 
			g_LDP1450_TextControl.TextMode = value;
			g_LDP1450_TextControl.TextGetXY = 0;

			// set scale..
			if (value == 0x030)
				g_LDP1450_TextControl.Scale = 2.0f;		// 3 lines of 10 chars
			else
				g_LDP1450_TextControl.Scale = 3.0f;		// 1 line of 20 chars
		}
		return;
	}

	// if we've received 0x80 0x02 0x??
	else if (g_LDP1450_TextControl.bGotSetWindow)
	{
		// the expected argument here is 00 (Dragon's Lair 2), but we don't know what this actually does,
		//  so we accept the argument and ignore it.
		g_LDP1450_TextControl.bGotSetWindow = false;
		return;
	}
	
	// string to fill out??
	// ignore first char, then store the rest off
	else if (g_LDP1450_TextControl.TextGotString)
	{
		// 00,0a,14 - 1st,2nd,3rd lines
		if ( (value == 0x00 || value == 0x0a || value == 0x14) && g_LDP1450_TextControl.TextGot00 == 0)
		{
			// set our line position
			g_LDP1450_TextControl.TextLine = value / 10;
			g_LDP1450_TextControl.TextGot00 = 1;
		}
		else
		{
			// store off chars until we get a null
			if (value != 0x1a)	//string complete cmd
			{
				// make sure we have a valid ascii char or special LDP1450 character
				if (value>=0x20 || value == 0x13)  
				{
					g_LDP1450_TextControl.TextString[g_LDP1450_TextControl.TextStringPointer] = value;
					g_LDP1450_TextControl.TextStringPointer++;
				}
			}
			else
			{
				int spaces = 0;
				int i = 0;
				int x = 0;
				
				// if its a string of spaces, then clear all strings
				for (i=0; i<32; i++)
				{
					if (g_LDP1450_TextControl.TextString[i] == 32)
						spaces++;
				}

				// clear the text strings
				if (spaces > 20)
				{
					for (i=0; i<3; i++)
						for (x = 0; x<11; x++)
							g_LDP1450_Strings[i].String[x] = 32;
				}
				else	// set it to draw
				{
					// copy the string over
					strcpy (g_LDP1450_Strings[g_LDP1450_TextControl.TextLine].String, g_LDP1450_TextControl.TextString);
					g_LDP1450_Strings[g_LDP1450_TextControl.TextLine].x = 20 + (g_LDP1450_TextControl.TextXPos * 7	);
					g_LDP1450_Strings[g_LDP1450_TextControl.TextLine].y = 20 + ((g_LDP1450_TextControl.TextYPos * 5) + 
																				g_LDP1450_TextControl.TextLine * 38);
				}

				SDL_Surface *overlay;

				if (NULL != (overlay = g_game->get_active_video_overlay()))
				{
					int overlay_ldp1450_x = (int) (g_LDP1450_TextControl.TextXPos * 3.3 - 12); // 1450 text positioning for overlay
					int overlay_ldp1450_y = (int) (g_LDP1450_TextControl.TextYPos * 3.8 - 10);

					// MPO : calling video_repaint directly probably isn't a good idea
					//g_game->video_repaint();

					// if using 720x480, text must be shifted to the right slightly
					if ((int) g_ldp->get_discvideo_width() == 720)
						overlay_ldp1450_x += 15;
					else
						overlay_ldp1450_x -= 7;

					// Display LDP1450 overlay
					for (i=0; i<3; i++)
					{
						draw_singleline_LDP1450(g_LDP1450_Strings[i].String, overlay_ldp1450_x, overlay_ldp1450_y, overlay);				
						overlay_ldp1450_y += OVERLAY_LDP1450_LINE_SPACING;         // line spacing
					}	
					
					g_game->set_video_overlay_needs_update(true);
					g_game->video_blit();
				}

				// clear vars
				g_LDP1450_TextControl.TextStringPointer = 0;
				memset(g_LDP1450_TextControl.TextString, 0, 128);
				g_LDP1450_TextControl.TextGotString = 0;
				g_LDP1450_TextControl.TextGot00 = 0;
			}
		}
		return;
	}

	// regular commands
	switch (value)
	{
	case 0x00:	//LDP1450 - text handling
		if (g_LDP1450_TextControl.TextCommand == true)
		{			
			g_LDP1450_TextControl.TextGetXY = 1;
			g_LDP1450_TextControl.TextGotXpos = g_LDP1450_TextControl.TextGotYpos = 0;	// reset vars for use
			g_LDP1450_TextControl.TextCommand = false;
		}
		else printline("WARNING: ldp1000 received unexpected 0x00");
		break;
	case 0x01:	//LDP1450 - Text handling
		if (g_LDP1450_TextControl.TextCommand == true)
		{
			g_LDP1450_TextControl.TextGotString = 1;
			g_LDP1450_TextControl.TextCommand = false;
		}
		else printline("WARNING: ldp1000 received unexpected 0x01");
		break;
	case 0x02:	// LDP1450 - text 'set window' command
		if (g_LDP1450_TextControl.TextCommand)
		{
			g_LDP1450_TextControl.bGotSetWindow = true;
		}
		else printline("WARNING: ldp1000 received unexpected 0x02");
		break;
	case 0x0a:	// ldp1450 - 2nd line
	case 0x14:	// ldp1450 - 3rd line
	case 0x1a:	// ldp1450 - text sent..
		break;
	case 0x24:	// audio_mute_on
		g_ldp->disable_audio1();	
		g_ldp->disable_audio2();
		break;
	case 0x25:	// audio_mute_off
		g_ldp->enable_audio1();	
		g_ldp->enable_audio2();
		break;
	case 0x27:	// video on
		// ignored
		break;
	case 0x28:	// Stop Codes Enable
		break;
	case 0x29:	// Stop Codes Disable
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
		ldp1000_add_digit(value);
		ldp1000_make_ack_latency(g_ldp1000_number_cycles);
		ldp1000_queue_push(0x0a); // ack
		break;
	case 0x3a: // Play
		g_ldp->pre_play();
		ldp1000_make_ack_latency(g_ldp1000_enter_cycles);
		ldp1000_queue_push(0x0a); // ack
		break;
	case 0x40: // Enter
		ldp1000_make_ack_latency(g_ldp1000_enter_cycles);
		ldp1000_queue_push(0xa);	// ack
		ldp1000_enter();
		break;
	case 0x43: // Search 
		enter_status = LDP1000_SEARCH;
		ldp1000_make_ack_latency(g_ldp1000_enter_cycles);
		ldp1000_queue_push(0x0a); // ack
		break;
	case 0x44: // Repeat
		enter_status |= LDP1000_REPEAT;
		ldp1000_queue_push(0x0a); // ack
		break;
	case 0x46: // Channel 1 on
		g_ldp->enable_audio1();
		ldp1000_queue_push(0x0a);	// ack
		break;
	case 0x47: // Channel 1 off
		g_ldp->disable_audio1();
		ldp1000_queue_push(0x0a);
		break;
	case 0x48: // Channel 2 on
		g_ldp->enable_audio2();
		ldp1000_queue_push(0x0a);	// ack
		break;
	case 0x49: // Channel 2 off
		g_ldp->disable_audio2();
		ldp1000_queue_push(0x0a);
		break;
 	case 0x4F: // LDP1450 - Still()
		g_ldp->pre_pause();	// return 0xB if the disc isn't playing
		ldp1000_queue_push(0x0a);	// ack
		break;
	case 0x55:	// 'frame mode' (unknown function)
		// ignored
		break;
	case 0x56: // C.L. (reset)
		// if this command has been used to interrupt a search, then wait until the search finishes
		if (g_uLDP1000State == LDP1000_STATE_SEARCHING)
		{
			printline("LDP1000 NOTE: C.L. (0x56) sent during search");

			// switch us into the 'aborting' state
			g_uLDP1000State = LDP1000_STATE_SEARCH_ABORTING;
		}
		ldp1000_make_ack_latency(g_ldp1000_clear_cycles);	// bega's battle relies on some latency after the clear command
		ldp1000_queue_push(0x0a); // ack
		break;
	case 0x60: // Addr Inq (get current frame number)
		{
			unsigned int uCurFrame = g_ldp->get_current_frame();
			string f = numstr::ToStr(uCurFrame, 10, FRAME_SIZE);	// with leading zeroes
			ldp1000_queue_push(f[0]); // M1
			ldp1000_queue_push(f[1]); // M2
			ldp1000_queue_push(f[2]); // M3
			ldp1000_queue_push(f[3]); // M4
			ldp1000_queue_push(f[4]); // M5
		}
		break;
	case 0x62:	// motor on
		// ignored
		break;
	case 0x67:	// LDP1450 - Status Inquiry
		ldp1000_queue_push(0x80);
		ldp1000_queue_push(0);
		ldp1000_queue_push(0x10);
		ldp1000_queue_push(0);	// disc is in, door closed
		// if the disc is paused
		if (g_ldp->get_status() == LDP_PAUSED)
		{
			ldp1000_queue_push(0x20); // paused
		}
		else if (g_ldp->get_status() == LDP_PLAYING)
		{
			ldp1000_queue_push(1);	// playing
		}
		// 8a 00 00 00 00 = no disc in the player
		else
		{
			// if this happens, you need to account for the other status codes
			printline("ERROR : ldp1000 status was queried but we aren't prepared to handle it");

			// return "paused" status just so we don't get caught in an endless loop
			ldp1000_queue_push(0x20); // paused
		}
		break;
	case 0x6e:	// CX_ON	??
		break;
	case 0x80:	// text start
		g_LDP1450_TextControl.TextCommand = true;
		break;
	case 0x81:	// LDP1450 - Turn on text
		g_LDP1450_TextControl.LDP1450_DisplayOn = true;
		break;
	case 0x82:	// LDP1450 - Turn off text
		g_LDP1450_TextControl.LDP1450_DisplayOn = false;
		break;
	default:
		string msg = "WARNING : Unimplemented Sony LDP command received: 0x" +
			numstr::ToStr(value, 16);
		printline(msg.c_str());	// force developer to address this situation to get rid of err msg
		ldp1000_queue_push(0x0a); // ack
		break;
	}
}

void ldp1000_think()
{
	// if we're repeating ...
	if (g_uLDP1000State == LDP1000_STATE_REPEATING)
	{
		if (g_ldp->get_current_frame() >= ldp1000_repeat_frame)
		{
			// we finished one repeat
			if (g_iLDP1000TimesToRepeat > 0)
			{
				g_iLDP1000TimesToRepeat--;
			}
			// else it could be 0 or -1, in either case we do not want to decrement
			
			// start over if we still need to repeat
			if (g_iLDP1000TimesToRepeat != 0)
			{
				char f[FRAME_ARRAY_SIZE] = {0};
				sprintf(f, "%i", ldp1000_repeat_start_frame);

				// if _blocking_ search succeeds, then play ...
				if (g_ldp->pre_search(f, true))
				{
					g_ldp->pre_play();
				}
				// else if search fails ...
				else
				{
					// I'm not sure if this is the way that the Sony LDP handles a looping error,
					//  but looping errors aren't expected anyway, so it's probably ok
					//  to have unpredictable results at this point.
					ldp1000_queue_push(2);	// error
					g_uLDP1000State = LDP1000_STATE_NORMAL;
				}
			}
			// or if we are done, pause
			else
			{
				g_ldp->pre_pause();
				g_uLDP1000State = LDP1000_STATE_NORMAL;
			}

			// we got to the desired frame
			ldp1000_queue_push(0x01); // completion
		}
	}

	// else if we're waiting for a search to complete (or an aborted search)
	else if ((g_uLDP1000State == LDP1000_STATE_SEARCHING) ||
		(g_uLDP1000State == LDP1000_STATE_SEARCH_ABORTING))
	{
		int iStat = g_ldp->get_status();

		// if we're paused, it means we're done seeking
		if (iStat == LDP_PAUSED)
		{
			// if we're waiting for a search to complete
			if (g_uLDP1000State == LDP1000_STATE_SEARCHING)
			{
#ifdef DEBUG
				printline("LDP1000: Search Complete");
#endif
				ldp1000_queue_push(1);	// search complete
			}
			// else the search has aborted, so we don't want to return a '1'

			// if we were waiting for the search to complete due to the search being aborted
			//  or if we have no queued frame, then reset the state to normal
			if ((g_uLDP1000State != LDP1000_STATE_SEARCH_ABORTING) ||
				(g_ldp1000_queued_frame[0] == 0))
			{
				g_uLDP1000State = LDP1000_STATE_NORMAL;
			}
			// else we need to do our queued frame now
			else
			{
				printline("LDP1000: Queued search is now being executed");
				if (g_ldp->pre_search(g_ldp1000_queued_frame, false))
				{
					g_uLDP1000State = LDP1000_STATE_SEARCHING;
				}
				else
				{
					printline("LDP1000: Queued search failed");
					ldp1000_queue_push(2);	// search error
					g_uLDP1000State = LDP1000_STATE_NORMAL;
				}
			}
		}
		// else if we're not seeking as we expect to be, return an error
		else if (iStat != LDP_SEARCHING)
		{
			// if we're waiting for a search to complete
			if (g_uLDP1000State == LDP1000_STATE_SEARCHING)
			{
				ldp1000_queue_push(2);	// error
			}
			// else we were aborting, so the error doesn't matter
			g_uLDP1000State = LDP1000_STATE_NORMAL;
		}
		// else we're still searching
	}
}

bool ldp1000_result_ready(void)
{
	bool result = false;

	ldp1000_think();	// this is a good place to call this function since it gets called a lot

	// if the queue is not empty and we're not waiting for an event,
	//  then we've got a result to return from the player
	if ((!g_qu8LdpOutput.empty()) && (!g_bLDP1000_Waiting4Event))
	{
		result = true;
	}
	return result;
}

void ldp1000_enter(void)
{
//	char s[81] = {0};

	if (enter_status == LDP1000_SEARCH)
	{
		ldp1000_frame[ldp1000_frame_index] = 0;

		// safety check: make sure we're not trying to search without checking the previous result
		if (g_uLDP1000State != LDP1000_STATE_SEARCHING)
		{
			if (g_uLDP1000State != LDP1000_STATE_SEARCH_ABORTING)
			{
				// begin search
				if (g_ldp->pre_search(ldp1000_frame, false))
				{
					g_uLDP1000State = LDP1000_STATE_SEARCHING;
				}
				// else if search fails
				else
				{
					ldp1000_queue_push(2);	// error
					g_uLDP1000State = LDP1000_STATE_NORMAL;
				}
			}
			// else the search is aborting so we need to queue up the new search
			else
			{
				// if there is no queued frame, then we can use it ...
				if (g_ldp1000_queued_frame[0] == 0)
				{
					// copy frame into queued frame ...
					memcpy(g_ldp1000_queued_frame, ldp1000_frame, sizeof(g_ldp1000_queued_frame));
					printline("LDP1000: next search request is queued until first search finishes aborting");
				}
				// else there is already a frame queued up, and we can't have more than one
				//  because the original player did not support this! (didn't even support queueing hehe)
				else
				{
					printline("LDP1000 ERROR: tried to queue up two frames, this should never happen!");
					set_quitflag();	// abort Daphne so the user realizes there is a big problem here
				}
			}
		}
		// If calling program is trying to search without checking for search results, then we have to intervene ...
		// (Bega's Battle will attempt to do this if there is enough artificial seek delay added)
		else
		{
			printline("LDP1000 WARNING: caller didn't wait for search to complete, so we'll ignore the search request");
		}
		ldp1000_frame_index = 0; // reset frame index
		enter_status = 0;

	}

	else if (enter_status == LDP1000_REPEAT_NUMBER)
	{
		ldp1000_frame[ldp1000_frame_index] = 0;	// null terminate to be safe

		// if no argument is specified (Bega's does this), then we default to '1'
		if (ldp1000_frame_index == 0)
		{
			g_iLDP1000TimesToRepeat = 1;
		}
		else
		{
			g_iLDP1000TimesToRepeat = (int) numstr::ToUint32(ldp1000_frame);	// this # cannot be negative, but the var we store it in is negative for convenience

			// if 0 is the argument supplied, it means to repeat endlessly
			if (g_iLDP1000TimesToRepeat == 0)
			{
				g_iLDP1000TimesToRepeat = -1;	// -1 will mean to repeat an infinite amount of times
			}
			// else repeat the # of times specified
		}
		
//		sprintf(s, "LDP1000: Repeat %i times\n", g_iLDP1000TimesToRepeat); 
//		printline(s);

		g_ldp->pre_play();

		ldp1000_frame_index = 0; // reset frame index
		g_uLDP1000State = LDP1000_STATE_REPEATING;
		enter_status = 0;
	}
	
	else if (enter_status == LDP1000_REPEAT)
	{
		ldp1000_frame[ldp1000_frame_index] = 0;
		
//		sprintf(s, "LDP1000: Repeat at frame %i\n", atoi(ldp1000_frame)); 
//		printline(s);

		ldp1000_repeat_frame = atoi(ldp1000_frame);
		ldp1000_repeat_start_frame = g_ldp->get_current_frame();

		ldp1000_frame_index = 0; // reset frame index

		enter_status = LDP1000_REPEAT_NUMBER;
	}

	// else we got an 'enter' for something unknown
	else
	{
		printline("WARNING : ldp1000_enter() called for an unknown command");
		set_quitflag();	// force dev to deal with this problem :)
	}
}

// adds a digit to  the frame array that we will be seeking to
// digit should be in ASCII format
void ldp1000_add_digit(char digit)
{
	if (ldp1000_frame_index < FRAME_SIZE)
	{
		ldp1000_frame[ldp1000_frame_index] = digit;
		ldp1000_frame_index++;
	}
	else
	{
		// if this happens on a regular basis, we can comment out this msg
		printline("WARNING: ldp1000_add_digit() received too many digits, ignoring");
	}
}

