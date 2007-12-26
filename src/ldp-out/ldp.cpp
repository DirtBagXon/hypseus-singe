
/*
 * ldp.cpp
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

// LDP.CPP
// by Matt Ownby

// The code in this file translates general LDP functions into LDP-specific functions
// Part of the Daphne emulator

#ifdef DEBUG
#include <assert.h>
#include "../io/numstr.h"
#include "../cpu/cpu-debug.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../io/serial.h"
#include "../io/my_stdio.h"
#include "ldp.h"
#include "../timer/timer.h"
#include "../io/conout.h"
#include "framemod.h"
#include "../game/game.h"
#include "../game/boardinfo.h"
#include "../cpu/cpu.h"
#include "../cpu/generic_z80.h"

#ifdef WIN32
#pragma warning (disable:4100)	// disable the warning about unreferenced parameters
#endif

// generic ldp constructor
ldp::ldp() :
	need_serial(false),
	serial_initialized(false),
	player_initialized(false),
	m_bIsVLDP(false),
	blitting_allowed(true),
	skipping_supported(false),
	skip_instead_of_search(false),
	max_skippable_frames(0),
	m_last_try_frame(0),
	m_last_seeked_frame(0),
//	m_play_cycles(0),
	m_play_time(0),
	m_start_time(-1),	// set to something invalid to force us to call pre_init
	m_status(LDP_STOPPED),
	search_latency(0),
	m_stop_on_quit(false),
	m_discvideo_width(640),
	m_discvideo_height(480),
	m_use_nonblocking_searching(true),
	m_dont_get_search_result(false),
	m_sram_continuous_update(false),
	m_noldp_timer(0),
	m_uCurrentFrame(0),
	m_uCurrentOffsetFrame(0),
	m_uElapsedMsSincePlay(0),
	m_uBlockedMsSincePlay(0),
	m_bWaitingForVblankToPlay(false),
	m_iSkipOffsetSincePlay(0),
	m_uMsFrameBoundary(0),
	m_uElapsedMsSinceStart(0),
	m_uVblankCount(0),
	m_uVblankMiniCount(0),
	m_uMsVblankBoundary(0),
	m_uFramesToSkipPerFrame(0),
	m_uFramesToStallPerFrame(0),
	m_uStallFrames(0),
	m_bPreInitCalled(false)
{
	m_bug_log.clear();	// probably redundant, but what the heck..

	// NOTE : GET_TICKS must not be called in this constructor because on the gp2x, get ticks isn't available until video is initialized
}

ldp::~ldp()
{
	pre_shutdown();
}

// Initializes and "pings" the LDP/VLDP and returns true if the player is online and detected
// or false if the LDP could not be initialized
bool ldp::pre_init()
{

	bool result = true;	// assume everything works until we find out otherwise
	bool temp = true;	// needed to make && work the way we want below

	// If we are controlling a real LDP,
	// or if we are controlling a combo and DVD initialized properly,
	// then initialize the serial port here
	if (need_serial)
	{
		printline("NOTE : You are attempting to use DAPHNE with a real laserdisc player!");
		printline("If you don't have DAPHNE plugged into a real laserdisc player,");
		printline("you should be using VLDP instead.");
		serial_initialized = serial_init(get_serial_port(), get_baud_rate());
		temp = serial_initialized;
	}
	
	player_initialized = init_player();
	result = temp && player_initialized;
	m_start_time = GET_TICKS();
	m_uElapsedMsSincePlay = 0;
	m_uBlockedMsSincePlay = 0;
	m_uElapsedMsSinceStart = 0;
	m_uMsVblankBoundary = 0;
	m_bPreInitCalled = true;
	m_bWaitingForVblankToPlay = false;
	m_iSkipOffsetSincePlay = 0;
	m_uMsFrameBoundary = 0;
	m_uVblankCount = 0;
	m_uVblankMiniCount = 0;
	m_uFramesToSkipPerFrame = 0;
	m_uFramesToStallPerFrame = 0;
	m_uStallFrames = 0;

	return(result);

}

// initializes the player after serial has been initialized
// this is so that the main init function can initialize the serial without having each LDP
// initialize its own serial
bool ldp::init_player()
{
	return(true);
}

// may stop the player, shuts down the serial if it's open, then calls shutdown_player for player-specific stuff
void ldp::pre_shutdown()
{
	if (player_initialized)
	{
		// if stop on quit has been requested stop the player from playing
		if (m_stop_on_quit)
		{
			pre_stop();
		}
		
		// if serial has been initialized, shut it down now
		if (serial_initialized)
		{
			serial_close();
			serial_initialized = false;
		}
	
		shutdown_player();
		player_initialized = false;
	}
	// else we were never initialized
}

// does any player specific stuff  
void ldp::shutdown_player()
{
}

// see .h file for usage
bool ldp::pre_search(const char* pszFrame, bool block_until_search_finishes)
{
	char frame[FRAME_ARRAY_SIZE] = { 0 };
	Uint16 frame_number = 0;	// frame to search to
	bool result = false;
	char s1[81] = { 0 };

	// safety check, if they try to search without checking the search result ...
	if (m_status == LDP_SEARCHING)
	{
		printline("LDP : tried to search without checking for search result first! that's bad!");
		printline(frame);
		
		// this is definitely a Daphne bug if this happens, so log it!
		m_bug_log.push_back("LDP.CPP, pre_search() : tried to search without checking for search result first!");
		return false;
	}
	// end safety check

	// Get the frame that we are on now, before we do the seek
	// This is needed in order to calculate artificial seek delay
	// We must do this before we change 'm_status' due to the way get_current_frame is designed
	m_last_seeked_frame = m_uCurrentFrame;

	// we copy here so that we can safely null-terminate
	strncpy(frame, pszFrame, 5);
	frame[5] = 0;	// terminate the string ...
	frame_number = (Uint16) atoi(frame);

	// If we're seeking to the frame we're already on, then don't seek
	// This optimizes performance for many games and especially improves the coin insert delay
	//  for Dragon's Lair 2.
	if ((m_status == LDP_PAUSED) && (frame_number == m_uCurrentFrame))
	{
		printline("LDP NOTE: ignoring seek because we're already on that frame");
		m_status = LDP_PAUSED;	// just to be safe
		return true;
	}

	m_status = LDP_SEARCHING;	// searching can take a while

	// If we need to alter the frame # before searching
	if (need_frame_conversion())
	{
		Uint16 unadjusted_frame = frame_number;
		frame_number = (Uint16) do_frame_conversion(frame_number);
		framenum_to_frame(frame_number, frame);
		sprintf(s1, "Search to %d (formerly %d) received", frame_number, unadjusted_frame);
	}
	else
	{
		sprintf(s1, "Search to %d received", frame_number);
	}

	outstr(s1);

	// notify us if we're still using outdated blocking searching
	if (block_until_search_finishes) outstr(" [blocking] ");

	// if it's Dragon's Lair/Space Ace, print the board we are on
	if ((g_game->get_game_type() == GAME_LAIR) || (g_game->get_game_type() == GAME_DLE1)
		|| (g_game->get_game_type() == GAME_DLE2)
		|| (g_game->get_game_type() == GAME_ACE))
	{
		Uint8 *cpumem = get_cpu_mem(0);	// get the memory for the first (and only)
		outstr(" - ");
		print_board_info(cpumem[0xA00E], cpumem[0xA00F], cpumem[Z80_GET_IY]);
	}
	else
	{
		newline();
	}

	// If the user requested a delay before seeking, make it now
	if (search_latency > 0)
	{
#ifdef DEBUG
		// search latency needs to be reworked so that think() is getting called ...
		assert(false);
#endif
//		make_delay(get_search_latency());
		printline("WARNING : search latency needs to be redesigned, it is currently disabled");
	}

	m_last_try_frame = (Uint16) atoi(frame);	// the last frame we tried to seek to becomes this current one

	// HERE IS WHERE THE SEARCH ACTUALLY TAKES PLACE

	int difference = frame_number - m_uCurrentFrame;	// how many frames we'd have to skip

	// if the player supports skipping AND user has requested the we skip instead of searching when possible
	// AND if we can skip forward instead of seeking ...
	if (skipping_supported && skip_instead_of_search && ((difference <= max_skippable_frames) && (difference > 1)))
	{
		result = pre_skip_forward((Uint16) difference);
	}

	// otherwise do a regular search
	else
	{
		result = nonblocking_search(frame);
		m_dont_get_search_result = false;	// it's now ok to get the search result

		// if search succeeded
		if (result)
		{
			// if we are to block until the search finishes, then wait here (this is bad, don't do this!)
			// This is only here for backward compatibility!
			if (block_until_search_finishes)
			{
				unsigned int cur_time = refresh_ms_time();
				unsigned int uLastTime = cur_time;	// used to compute m_uBlockedMsSincePlay
				int ldp_stat = -1;

				// we know we may be waiting for a while, so we pause the cpu timer to avoid getting a flood after the seek completes
				cpu_pause();

				// wait for player to change its status or for us to timeout
				while (elapsed_ms_time(cur_time) < 7000)
				{
					ldp_stat = get_status();	// get_status does some stuff that needs to get done, so we don't just read m_status instead

					// if the search is finished
					if (ldp_stat != LDP_SEARCHING)
					{
						break;
					}

					// Since the cpu is paused, we should not use think_delay
					// Also, blocking seeking may be used for skipping in noldp mode for cpu games like super don,
					//  so we cannot use think_delay here.
					MAKE_DELAY(1);

					// VLDP relies on our timers for its timing, so we need to keep the time moving forward during
					//  this period where we're paused. (and we can't use think_delay or pre_think because it can
					//  cause vblank events which can cause irqs while the cpu is supposed to be paused)
					unsigned int uCurTimeTmp = refresh_ms_time();
					m_uBlockedMsSincePlay += (uCurTimeTmp - uLastTime);	// since we're blocked, the blockmssinceplay must increase
					uLastTime = uCurTimeTmp;
					think();
				}

				cpu_unpause();	// done with the delay, so we can unpause

				// if we didn't succeed, then return an error
				if (ldp_stat != LDP_PAUSED)
				{
					printline("LDP : blocking search didn't succeed");
					result = false;
				}
			} // end if we were doing a blocking styled search
		} // if the initial search command was accepted

		// else if search failed immediately
		else
		{
			printline("LDP : search failed immediately");
			m_status = LDP_ERROR;
		}

	} // end regular search (instead of skipping)

	return(result);
}

// don't call this function directly, call pre_search instead
bool ldp::nonblocking_search(char *new_frame)
{
	m_noldp_timer = refresh_ms_time();
	return(true);
}

// don't call this function directly, call get_status() instead
int ldp::get_search_result()
{
	int result = SEARCH_BUSY;
	
	// stall for a couple of seconds to simulate search delay
	if (elapsed_ms_time(m_noldp_timer) > 2000)
	{
		printline("Search success!");
		result = SEARCH_SUCCESS;
	}
	return result;
}

// prepare to skip forward a certain # of frames and continue playing
// Call this function, do not call skip_forward directly
bool ldp::pre_skip_forward(Uint16 frames_to_skip)
{
	bool result = false;

	// only skip if the LDP is playing
	if (m_status == LDP_PLAYING)
	{
		Uint16 target_frame = (Uint16) (m_uCurrentFrame + frames_to_skip);
		unsigned int uOldCurrentFrame = m_uCurrentFrame;

		m_iSkipOffsetSincePlay += frames_to_skip;

		result = skip_forward(frames_to_skip, target_frame);

		char s[160];
		snprintf(s, sizeof(s), "Skipped forward %d frames (from %u to %u)", frames_to_skip, uOldCurrentFrame, target_frame);
		printline(s);
	}
	else
	{
		printline("LDP ERROR: Skip forward command was called when the disc wasn't playing");
	}

	return(result);
}

// prepare to skip forward a certain # of frames backward and continue playing forward
// do not call skip_backward directly
bool ldp::pre_skip_backward(Uint16 frames_to_skip)
{
	bool result = false;

	// only skip if the LDP is playing
	if (m_status == LDP_PLAYING)
	{
		Uint16 target_frame = (Uint16) (m_uCurrentFrame - frames_to_skip);
		unsigned int uOldCurrentFrame = m_uCurrentFrame;

		m_iSkipOffsetSincePlay -= frames_to_skip;

		result = skip_backward(frames_to_skip, target_frame);

		char s[81];
		snprintf(s, sizeof(s), "Skipped backward %d frames (from %u to %u)", frames_to_skip, uOldCurrentFrame, target_frame);
		printline(s);
	}
	else
	{
		printline("LDP ERROR: Skip backward command was called when the disc wasn't playing");
	}

	return(result);
}

// steps forward 1 frame
void ldp::pre_step_forward()
{
	// NOTE : right now we don't have LDP-specific implementations of this function since it is rarely used
	char frame[6];
	Uint16 new_frame = m_uCurrentFrame;

	// bounds check (if we haven't overflowed)
	if (new_frame < ((Uint16) -1))
	{
		framenum_to_frame(m_uCurrentFrame + 1, frame);
		printline("LDP : Stepping forward one frame");
		g_ldp->pre_search(frame, true);
	}
	else
	{
		printline("LDP: pre_step_forward failed bounds check");
	}
}

// steps backward 1 frame
void ldp::pre_step_backward()
{
	// NOTE : right now we don't have LDP-specific implementations of this function since it is rarely used
	char frame[6];
	Uint16 new_frame = m_uCurrentFrame;

	// don't step backward before the first frame on the disc
	if (new_frame > 0)
	{
		new_frame--;
	}

	framenum_to_frame(new_frame, frame);
	printline("LDP : Stepping backward one frame");
	g_ldp->pre_search(frame, true);
}

// skips forward a certain number of frames and continues playing
// Do not call this function directly!  Call pre_skip_forward instead
// NOTE : this function handles skipping for real laserdisc players that don't support it
bool ldp::skip_forward(Uint16 frames_to_skip, Uint16 target_frame)
{
	bool result = false;
	
	char frame[6] = {0};
	sprintf(frame, "%05d", target_frame);

	result = pre_search(frame, true);
	pre_play();

	return(result);
}

// DO NOT CALL THIS FUNCTION DIRECTLY
// call pre_skip_backward instead
// NOTE : this function handles skipping for real laserdisc players that don't support it
bool ldp::skip_backward(Uint16 frames_to_skip, Uint16 target_frame)
{
	bool result = false;
	
	char frame[6] = {0};
	sprintf(frame, "%05d", target_frame);

	result = pre_search(frame, true);
	pre_play();

	return(result);
}

// prepares to play the disc
void ldp::pre_play()
{
//	Uint32 cpu_hz;	// used to calculate elapsed cycles

	// safety check, if they try to play without checking the search result ...
	// THIS SAFETY CHECK CAN BE REMOVED ONCE ALL LDP DRIVERS HAVE BEEN CONVERTED OVER TO NON-BLOCKING SEEKING
	if (m_status == LDP_SEARCHING)
	{
		printline("LDP : tried to play without checking to see if we were still seeking! that's bad!");
		
		// if this ever happens, it is a bug in Daphne, so log it
		m_bug_log.push_back("LDP.CPP, pre_play() : tried to play without checking to see if we're still seeking!");
		
		return;
	}

	// we only want to refresh the frame calculation stuff if the disc is
	// not already playing
	if (m_status != LDP_PLAYING)
	{
		m_uElapsedMsSincePlay = 0;	// by definition, this must be reset since we are playing
		m_uBlockedMsSincePlay = 0;	// " " "
		m_iSkipOffsetSincePlay = 0;	// by definition, this must be reset since we are playing
		m_uCurrentOffsetFrame = 0;	// " " "
		m_uMsFrameBoundary = 1000000 / g_game->get_disc_fpks();	// how many ms must elapse before the first frame ends, 2nd frame begins

		// VLDP needs its timer reset with the rest of these timers before its play command is called.
		// Otherwise, it will think it's way behind and will try to catch up a bunch of frames.
		think();

		// if the disc may need to take a long time to spin up, then pause the cpu timers while the disc spins up
		if (m_status == LDP_STOPPED)
		{
			cpu_pause();
			m_play_time = play();
			cpu_unpause();
		}
		else
		{
			m_play_time = play();
		}

		m_bWaitingForVblankToPlay = true;	// don't start counting frames until the next vsync
		m_status = LDP_PLAYING;
	}
	else
	{
		printline("LDP : disc is already playing, play command ignored");
	}

	printline("Play");	// moved to the end of the function so as to not cause lag before play command could be issued
}

// starts playing the laserdisc
// returns the timer value that indicates when the playback actually started (used to calculate current frame)
unsigned int ldp::play()
{
	return refresh_ms_time();
}

// prepares to pause
void ldp::pre_pause()
{
	// only send pause command if disc is playing
	// some games (Super Don) repeatedly flood with a pause command and this doesn't work well with the Hitachi
	if (m_status == LDP_PLAYING)
	{
#ifdef DEBUG
		string s = "m_uMsFrameBoundary is " + numstr::ToStr(m_uMsFrameBoundary) + ", elapsed ms is " +
			numstr::ToStr(m_uElapsedMsSincePlay);
		printline(s.c_str());
#endif
		m_last_seeked_frame = m_uCurrentFrame;
		m_iSkipOffsetSincePlay = m_uCurrentOffsetFrame = m_uMsFrameBoundary = 0;
		pause();
		m_status = LDP_PAUSED;
		printline("Pause");
	}
	else
	{
		printline("LDP : Received pause while disc was not playing, ignoring");
	}
}

// pauses the disc
void ldp::pause()
{
}

// prepares to stop the disc
// "stop" is defined as going to frame 0 and stopping the motor so that
// the player has to spin up again to begin playing
void ldp::pre_stop()
{
	m_last_seeked_frame = m_uCurrentFrame = 0;
	stop();
	m_status = LDP_STOPPED;
	printline("Stop");
}

// stops the disc
void ldp::stop()
{
}

bool ldp::pre_change_speed(unsigned int uNumerator, unsigned int uDenominator)
{
	string strMsg;

	// if this is >= 1X
	if (uDenominator == 1)
	{
		m_uFramesToStallPerFrame = 0;	// don't want to stall at all

		// if this isn't 0 ...
		if (uNumerator > 0)
		{
			m_uFramesToSkipPerFrame = uNumerator - 1;	// show 1, skip the rest
		}
		// else it's 0, which is illegal (use pause() instead unless the game driver specifically wants to do this, in which case more coding is needed)
		else
		{
			m_uFramesToSkipPerFrame = 0;
			printline("ERROR : uNumerator of 0 sent to pre_change_speed, this isn't supported, going to 1X");
		}
	}
	// else if this is < 1X
	else if (uNumerator == 1)
	{
		m_uFramesToSkipPerFrame = 0;	// don't want to skip any ...

		// protect against divide by zero
		if (uDenominator > 0)
		{
			m_uFramesToStallPerFrame = uDenominator - 1;	// show 1, stall for the rest
		}
		// divide by zero situation
		else
		{
			m_uFramesToStallPerFrame = 0;
			printline("ERROR : uDenominator of 0 sent to pre_change_speed, this is undefined, going to 1X");
		}
	}
	// else it's a non-standard speed, so do some kind of error
	else
	{
		strMsg = "ERROR : unsupported speed specified (" + numstr::ToStr(uNumerator) +
			"/" + numstr::ToStr(uDenominator) + "), setting to 1X";
		uNumerator = uDenominator = 1;
	}

	bool bResult = change_speed(uNumerator, uDenominator);

	if (bResult) strMsg = "Successfully changed ";
	else strMsg = "Unable to change ";
	strMsg += "speed to " + numstr::ToStr(uNumerator) + "/" + numstr::ToStr(uDenominator) +
		"X";
	printline(strMsg.c_str());
	return bResult;
}

bool ldp::change_speed(unsigned int uNumerator, unsigned int uDenominator)
{
	return true;
}

void ldp::think_delay(unsigned int uMsDelay)
{
	bool bEmulatedCpu = (get_cpu_hz(0) != 0);	// whether we've got an emulated cpu

	// safety check: make sure that we're not using an emulated CPU
	if (bEmulatedCpu)
	{
		printline("think_delay() should not be used with an emulated CPU. Don't use blocking seeking maybe?");
		set_quitflag();
	}
	// safety check: make sure pre_init has already been called so that m_start_time has been initialized
	else if (!m_bPreInitCalled)
	{
		printline("think_delay() should not be called until pre_init() has been called.");
		set_quitflag();
	}

	// call pre_think the same # of times that uMsDelay tells us to,
	//  to ensure that we aren't caught sleeping during an important event
	for (unsigned int uMs = 0; uMs < uMsDelay; ++uMs)
	{
		pre_think();

		unsigned int uElapsedMs = elapsed_ms_time(m_start_time);

		// if we're ahead of where we need to be, then it's ok to stall ...
		if (uElapsedMs < m_uElapsedMsSinceStart)
		{
			/*
			string msg = "uElapsedMs is " + numstr::ToStr(uElapsedMs) + " and m_uElapsedMssinceStart is " +
				numstr::ToStr(m_uElapsedMsSinceStart);
			printline(msg.c_str());	// REMOVE ME
			*/

			MAKE_DELAY(1);
		}
		// otherwise we're caught up or behind, so just loop so we can make sure we're caught up
	}
}

// TODO: in the future, we may want to support vblank of 50 hz for the PAL laserdisc games
const unsigned int VBLANKS_PER_KILOSECOND = (unsigned int) ((29.97 * 2) * 1000);

void ldp::pre_think()
{
	// IMPORTANT : By definition, this function must be called every millisecond to update the timer (it is called by our cpu emulation every 1 ms)
	// If it cannot be called every 1 ms (if we are not emulating a cpu for example), then ldp::think_delay() can be
	//  (and should be) used instead of calling this function directly.
	// IMPORTANT : this must be updated no matter what because vldp relies on this timer when we're paused

	// START VBLANK COUNT

	// at the end of this function, if vblank gets asserted, we need to call an event handler in the game class, but we
	//  need to do that after the frame number has been recalculated (if need be), hence the purpose of this variable.
	bool bVblankAsserted = false;

	++m_uElapsedMsSinceStart;

	// if it's time to increase the vblank count
	if (m_uElapsedMsSinceStart >= m_uMsVblankBoundary)
	{
		++m_uVblankCount;

		// compute the next boundary
		m_uMsVblankBoundary = (unsigned int) ((((Uint64) (m_uVblankCount+1)) * 1000000) / VBLANKS_PER_KILOSECOND);

		// only increment the mini count if we aren't waiting for vblank to start counting frames
		if (!m_bWaitingForVblankToPlay)
		{
			++m_uVblankMiniCount;
		}
		// else if we have been waiting for vblank to play, then make sure we reset the vblankminicount
		else
		{
			// we want to start at the beginning of a new frame boundary
			m_uVblankMiniCount = 0;

			// we just got a vblank, so we're not waiting ...
			m_bWaitingForVblankToPlay = false;
		}

		bVblankAsserted = true;
	}
	// END VBLANK COUNT

	// if we're not waiting to sync up with vblank, then increase timer
	// (this must be done even if we're paused so VLDP will update our video overlay)
	if (!m_bWaitingForVblankToPlay)
	{
		++m_uElapsedMsSincePlay;
	}

	// If the disc is supposed to be playing then compute which frame # we're on
	// IMPORTANT:
	// ldp-vldp.cpp's nonblocking_seek() RELIES on this function NOT calling get_status()!!!
	// Be very careful about changing this 'm_status' to a get_status()
	if (m_status == LDP_PLAYING)
	{
		unsigned int uDiscFPKS = g_game->get_disc_fpks();

		// if our frame counter is in sync with vblank, then just increment frame every 2 vblanks ...
		if ((uDiscFPKS << 1) == VBLANKS_PER_KILOSECOND)
		{
			// if we've had 2 vblanks, then 1 frame has elapsed
			if (m_uVblankMiniCount > 1)
			{
				increment_current_frame();
				m_uVblankMiniCount = 0;
			}
			// else we've not had enough vblanks, so don't do anything ...
		}

		// else if we're not in sync with the vblank (for example if we're dragon's lair with a 24 FPS disc),
		//  then use the elapsed ms since play to determine if the frame has changed
		else if (m_uElapsedMsSincePlay >= m_uMsFrameBoundary)
		{
			increment_current_frame();

			// compute the next boundary
			m_uMsFrameBoundary = (unsigned int) ((((Uint64) (m_uCurrentOffsetFrame+1)) * 1000000) / uDiscFPKS);
		}

		// else the current frame has not changed

#ifdef DEBUG
#ifndef GP2X	// GP2X is slow enough that we don't want to do this safety check by default
		// SAFETY CHECK
		// This ensures that the number that is calculated is not far off what it should be.
		// (this test is only meaningful if we are emulating a cpu, because otherwise we may deliberately
		//  be calling this function slower than every 1 ms, such as ffr() or vldp's internal tests )
		if (get_cpu_hz(0))
		{
			unsigned int uElapsedMS = elapsed_ms_time(m_play_time); // compute milliseconds
			unsigned int time_result = m_last_seeked_frame + m_iSkipOffsetSincePlay +
				(unsigned int) ((((Uint64) uElapsedMS) * g_game->get_disc_fpks()) / 1000000);
			Uint16 diff = 0;

			// this isn't necessarily the same as m_uCurrentFrame due to potential skips that can occur
			// (this is updated immediately, m_uCurrentFrame is updated on a time boundary)
			unsigned int uCurrentFrame = m_last_seeked_frame + m_iSkipOffsetSincePlay + m_uCurrentOffsetFrame;

			if (uCurrentFrame > time_result) diff = uCurrentFrame - time_result;
			else diff = time_result - uCurrentFrame;

			// if the difference is noticeable, then alert the developer
			if (diff > 5)
			{
				static unsigned int last_warning = 0;

				if (elapsed_ms_time(last_warning) > 1000)
				{
					string s = "WARNING : cycle frame is ";
					s += numstr::ToStr(uCurrentFrame) + " but time frame is ";
					s += numstr::ToStr(time_result) + " which has a diff of ";
					s += numstr::ToStr(diff);
					printline(s.c_str());
					last_warning = refresh_ms_time();
				}
			}
		}
#endif // not GP2X
#endif // DEBUG

	}
	// otherwise the disc is idle, so we need not change the current frame

	think();	// call implementation-specific function

	// If vblank was asserted, let game know about it...
	// NOTE : this should probably come at the end of this function
	if (bVblankAsserted)
	{
		// let game know about vblank ...
		g_game->OnVblank();
	}
}

// DO NOT CALL THIS FUCTION DIRECTLY!  THIS FUCTION IS JUST A HELPER FOR PRE_THINK!
void ldp::increment_current_frame()
{
	++m_uCurrentOffsetFrame;

	// FOR PLAYING AT SLOWER THAN 1X (such as 1/2X)
	// if we have no frames to stall, then check to see if we need some frames to stall
	if (m_uStallFrames == 0)
	{
		m_uStallFrames = m_uFramesToStallPerFrame;
	}
	// otherwise, we do have frames to stall, so adjust the frame number accordingly
	else
	{
		--m_iSkipOffsetSincePlay;
		--m_uStallFrames;
	}
	// END PLAYING AT SLOWER THAN 1X

	// At 1X speed, m_uFramesToSkipPerFrame will be 0.
	// If we're playing faster, we need to adjust the current frame accordingly
	// I decided to use the m_iSkipOffsetSincePlay because I'm afraid to mess
	//  up the timing by modifying m_uCurrentOffsetFrame
	m_iSkipOffsetSincePlay += m_uFramesToSkipPerFrame;

	// NOTE : This must be re-calculated every time here (instead of just incremented)
	// Because if we skipped, we want to finish the current frame we are on and then go immediately
	//  to the new frame.  We do NOT want to change frames immediately when skipping.
	m_uCurrentFrame = m_last_seeked_frame + m_iSkipOffsetSincePlay + m_uCurrentOffsetFrame;

#if 0
	{
		static unsigned int uOldFrame = 0;
		static unsigned int uOldTime = 0;

		if (m_uCurrentFrame != uOldFrame)
		{
			unsigned int uTimer = refresh_ms_time();
			unsigned int uDiff = uTimer - uOldTime;
			string strMsg = "[" + numstr::ToStr(m_uCurrentFrame) + "] Time since last frame change: " + numstr::ToStr(uDiff) + " ms";
			printline(strMsg.c_str());
			uOldTime = uTimer;
			uOldFrame = m_uCurrentFrame;
		}
	}
#endif
}

void ldp::think()
{
}

// (see .h for description)
unsigned int ldp::get_current_frame()
{
	return m_uCurrentFrame;
}

unsigned int ldp::get_adjusted_current_frame()
{
#ifdef DEBUG
	// this function assumes that the disc's FPS is in line with vblank
	assert((g_game->get_disc_fpks() << 1) == VBLANKS_PER_KILOSECOND);
#endif // DEBUG

	// because get_current_frame() is a virtual function
	unsigned int uResult = get_current_frame();

	// if the disc is playing and we've already displayed the 2nd field of the frame, then advance the current frame
	if ((m_status == LDP_PLAYING) && (m_uVblankMiniCount != 0))
	{
		++uResult;
	}

	return uResult;
}

unsigned int ldp::get_vblank_mini_count()
{
	return m_uVblankMiniCount;
}

bool ldp::is_vldp()
{
	return m_bIsVLDP;
}

// returns value of blitting_allowed.  VLDP does not allow blitting!
bool ldp::is_blitting_allowed()
{
	return (blitting_allowed);
}

void ldp::set_blitting_allowed(bool bVal)
{
	blitting_allowed = bVal;
}

// returns the current LDP status
// This should ONLY be called if we are waiting for a search to complete, or
// if we don't have read access to 'm_status'
int ldp::get_status()
{
	// if we are in the middle of a search, find out if the search has completed
	if (m_status == LDP_SEARCHING)
	{
#ifdef DEBUG
		// verify that get_search_result() is not being called when it ought not be
		assert(!m_dont_get_search_result);
#endif
		int stat = get_search_result();

		// if the search was successful
		if (stat == SEARCH_SUCCESS)
		{
			m_dont_get_search_result = true;
			m_last_seeked_frame = m_uCurrentFrame = m_last_try_frame;
			m_status = LDP_PAUSED;

			// Update sram after every search if user desires it
			//  (so that the DAPHNE can be improperly terminated, if it's inside a cab and powered off, for example)
			// This is the best place to do this because it's right after a seek which can take a second or two anyway.
			if (m_sram_continuous_update)
			{
				g_game->save_sram();
			}

		}

		// if the search failed
		else if (stat == SEARCH_FAIL)
		{
			m_status = LDP_ERROR;
		}
		// else the search is busy and still going, so we needn't act ...
	}

	return m_status;
}

// converts an integer frame number into ASCII (with leading zeroes)
// NOTE : 'f' should be an array of 6 characters (5 numbers plus null terminator)
void ldp::framenum_to_frame(Uint16 num, char *f)
{
    sprintf(f, "%05d", num);
}

Uint32 ldp::get_search_latency()
{
	return(search_latency);
}

void ldp::set_search_latency(Uint32 value)
{
	search_latency = value;
}

void ldp::set_stop_on_quit(bool value)
{
	m_stop_on_quit = value;
}

// causes LDP to blank video while searching
void ldp::set_search_blanking(bool enabled)
{
	printline("NOTE : Search blanking cannot be modified with this laserdisc player!");
}

// causes LDP to blank video while skipping
void ldp::set_skip_blanking(bool enabled)
{
	printline("NOTE : Skip blanking cannot be modified with this laserdisc player!");
}

void ldp::set_seek_frames_per_ms(double value)
{
	printline("NOTE : Seek delay is not supported with this laserdisc player!");
}

void ldp::set_min_seek_delay(unsigned int value)
{
	printline("NOTE : Seek delay is not supported with this laserdisc player!");
}

// causes sram to be saved after every seek
void ldp::set_sram_continuous_update(bool value)
{
	m_sram_continuous_update = value;
}

void ldp::enable_audio1()
{
	printline("Audio1 enable received (ignored)");
}

void ldp::enable_audio2()
{
	printline("Audio2 enable received (ignored)");
}

void ldp::disable_audio1()
{
	printline("Audio1 disable received (ignored)");
}

void ldp::disable_audio2()
{
	printline("Audio2 disable received (ignored)");
}

// asks LDP to take a screenshot if that's possible
// it's only possible with VLDP as of this time
void ldp::request_screenshot()
{
	printline("NOTE: current laserdisc player does not support taking screenshots, sorry");
}

// returns the width of the laserdisc video (only meaningful with mpeg)
Uint32 ldp::get_discvideo_width()
{
	return m_discvideo_width;
}

// returns the height of the laserdisc video (only meaningful with mpeg)
Uint32 ldp::get_discvideo_height()
{
	return m_discvideo_height;
}

// ordinarily does nothing unless we're VLDP
bool ldp::lock_overlay(Uint32 timeout)
{
	return true;
}

// does nothing unless we're VLDP
bool ldp::unlock_overlay(Uint32 timeout)
{
	return true;
}

// sets the value of this boolean
void ldp::set_use_nonblocking_searching(bool val)
{
	m_use_nonblocking_searching = val;
}

// returns the value of this boolean
bool ldp::get_use_nonblocking_searching()
{
	return m_use_nonblocking_searching;
}

unsigned int ldp::get_elapsed_ms_since_play()
{
	return m_uElapsedMsSincePlay;
}

// this is called by cmdline.cpp if it gets any cmdline parameters that it doesn't recognize
// returns true if this argument was recognized and processed,
// or false is this argument wasn't recognized (and therefore the command line is bad on a whole)
bool ldp::handle_cmdline_arg(const char *arg)
{
	return false;
}

void ldp::get_bug_log(list<string> &log)
{
	log = m_bug_log;
	m_bug_log.clear();
}

void ldp::print_frame_info()
{
	string s = "Current frame is " + numstr::ToStr(m_uCurrentFrame);
	printline(s.c_str());

	unsigned int u = m_uMsVblankBoundary - m_uElapsedMsSinceStart;
	s = "Virtual milliseconds until next vblank: " + numstr::ToStr(u);
	printline(s.c_str());

	s = "Vblanks since frame changed: " + numstr::ToStr(m_uVblankMiniCount);
	printline(s.c_str());
}

//////////////////

bool fast_noldp::nonblocking_search(char *new_frame)
{
	return(true);
}

int fast_noldp::get_search_result()
{
	return SEARCH_SUCCESS;
}

bool fast_noldp::skip_forward(Uint16 frames_to_skip, Uint16 target_frame)
{
	return true;
}

bool fast_noldp::skip_backward(Uint16 frames_to_skip, Uint16 target_frame)
{
	return true;
}

