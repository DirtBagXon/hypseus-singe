/*
 * ldp.h
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

#ifndef LDP_PRE_H
#define LDP_PRE_H

// different states that the laserdisc player could be in
enum
{
	LDP_ERROR, LDP_SEARCHING, LDP_STOPPED, LDP_PLAYING, LDP_PAUSED
};

// different non-blocking search results
enum
{
	SEARCH_FAIL,	// search failed
	SEARCH_SUCCESS,	// search succeeded
	SEARCH_BUSY		// search is still taking place, no change yet, my frenid
};

#include <SDL.h>	// needed for datatypes

// for bug logging
#include <string>
#include <list>

using namespace std;

#ifdef WIN32
#pragma warning (disable:4786) // disable warning about truncating to 255 in debug info
#endif

#define FRAME_SIZE 5

// the size to make your frame array (the frame + the NULL terminator)
#define FRAME_ARRAY_SIZE FRAME_SIZE + 1

class ldp
{
public:
	ldp();
	virtual ~ldp();
	
	// Call this function to initialize the ldp (the constructor does _not_ call it).
	// It's safe to call this function mulitple times.
	bool pre_init();
	
	// player-specific init stuff
	// NOTE : this function should not be called directly, call pre_init instead
	virtual bool init_player();
	
	// Call this function to shutdown the ldp (the destructor also calls it).
	// It's safe to call this function multiple times.
	void pre_shutdown();

	// player-specific shutdown stuff	
	// NOTE : this function should not be called directly, call pre_shutdown instead
	virtual void shutdown_player();
	
	void clear();	// clears any received frames

	// searches to the 5-digit ASCII frame specified by 'pszFrame'
	// if 'block_until_search_finished' is true, this function will return after the search has completed and will return
	//    true if the search was successful or false on error.
	// if 'block_until_search_finished' is false, this function will return after the search has _begun_ and will return true
	//    if the player is currently executing the search or false if there was an error
	// Call this instead of calling the search function directly!
	// WARNING : if 'block_until_search_finished' is false, then the calling function
	// MUST also call get_status() to determine when a seek has finished.
	// If you aren't prepared to do this, make 'block_until_search_finished' true.
	bool pre_search(const char *, bool block_until_search_finished);

	// does the actual search. This func returns once the search has begun and you need to call get_search_result
	// until the search isn't busy anymore.
	// This function returns true if the LDP acknowledged the search
	virtual bool nonblocking_search(char *);

	// do not call this function directly! (get get_status instead)
	// This is the LDP-specific function that checks to see if a search (issued previously) finished
	// it returns one of the SEARCH_* enumerations ...
	virtual int get_search_result();

	bool pre_skip_forward(Uint16);
	bool pre_skip_backward(Uint16);

	// steps 1 frame forward
	void pre_step_forward();

	// steps 1 frame backward
	void pre_step_backward();

	virtual bool skip_forward(Uint16 frames_to_skip, Uint16 target_frame);
	// NOTE : frames_to_skip and target_frame are both included for your convenience
	// use frames_to_skip only if you cannot use target_frame (which is more accurate)

	virtual bool skip_backward(Uint16 frames_to_skip, Uint16 target_frame);
	// NOTE : frames_to_skip and target_frame are both included for your convenience
	// use frames_to_skip only if you cannot use target_frame (which is more accurate)
	
	void pre_play();
	void pre_pause();
	void pre_stop();

	// Function all 'ldp-in' drivers should call to change the playback speed
	// Input is a fraction (numerator/denominator) to avoid using a floating point value,
	//  which doesn't work well on the GP2X.
	// Common speeds are 1X (1/1), 2X (2/1), 3X (3/1), 4X (4/1), 8X (8/1),
	//  1/2X (1/2), 1/3X (1/3), 1/4X (1/4), 1/8X (1/8)
	// Any weird speeds such as 7/2 have undefined behavior for now.
	// Returns true if the speed has changed or false if the speed cannot be changed
	bool pre_change_speed(unsigned int uNumerator, unsigned int uDenominator);

	// LASERDISC-PLAYER-SPECIFIC IMPLEMENTATIONS OF VARIOUS FUNCTIONALITY
	virtual unsigned int play();
	virtual void pause();
	virtual void stop();

	// Implementation-specific version of pre_change_speed
	// Returns true if the speed has changed or false if the speed cannot be changed
	virtual bool change_speed(unsigned int uNumerator, unsigned int uDenominator);

	// function that can replace make_delay when no cpu is present and thinking is required
	void think_delay(unsigned int);

	// When emulating a cpu, this function MUST be called EVERY 1 ms (according to cpu's reckoning) to keep us in sync w/ the cpu.
	// This allows the cpu to do all timing calculations, which saves us from redundantly doing them again.
	// (when not emulating a cpu, this function can be called less often, every frame for example, and we will do the calculations ourselves)
	// This function updates the current frame so you cannot rely on get_current_frame to know when to call this function!!!
	void pre_think();

	// pre_think() calls think() for ldp-specific stuff
	virtual void think();

	// returns the current frame number that the disc is on
	// this is a generic function which computes the current frame number using the elapsed time
	// and the framerate of the disc.  Obviously querying the laserdisc player would be preferable
	// if possible (some laserdisc players don't like to be queried too often)
	// SEEKING NOTES:
	// if get_current_frames() is called during a NON-BLOCKING seek,
	// it returns the frame the LDP was at right 
	// when seek was begun. It DOES NOT check to see if a seek is finished, which
	// means that any driver that uses non-blocking seeking MUST also call get_status()
	// to check for the completion of that seek.
	// A laserdisc driver that supports non-blocking seeking (LD-V1000) SHOULD call
	// get_status() to check to see if a search is complete.  If an LDP isn't designed
	// to return its status (ie the PR-8210) then either blocking seeking must be used
	// or each game driver which uses this player must call get_status().
	virtual unsigned int get_current_frame();

	// returns m_uCurrentFrame or m_uCurrentFrame+1 if the disc is playing and we've already displayed the 2nd field of the frame
	// (this is in an effort to fix overrun problems on super don)
	unsigned int get_adjusted_current_frame();

	// returns 0 if this is the first vblank of the frame (assuming vblanks and frames line up),
	//  or 1 if it's the second vblank of the frame
	unsigned int get_vblank_mini_count();

	// causes sram to be saved after every seek
	virtual void set_sram_continuous_update(bool value);

	virtual void enable_audio1();
	virtual void enable_audio2();
	virtual void disable_audio1();
	virtual void disable_audio2();
	virtual void request_screenshot();
	virtual void set_search_blanking(bool enabled);
	virtual void set_skip_blanking(bool enabled);
	virtual void set_seek_frames_per_ms(double value);
	virtual void set_min_seek_delay(unsigned int value);

	// END LDP-SPECIFIC SECTION

	bool is_vldp();	// returns true if our ldp type is VLDP
	bool is_blitting_allowed();	// returns value of blitting_allowed
	void set_blitting_allowed(bool bVal);
	int get_status();	// returns status of laserdisc player
	void framenum_to_frame(Uint16, char *);	// converts int to 5-digit string
	Uint32 get_search_latency();
	void set_search_latency(unsigned int);
	void set_stop_on_quit(bool);	// enables the stop_on_quit bool flag

	Uint32 get_discvideo_height();	// gets the height of the laserdisc video (only meaningful with mpeg)
	Uint32 get_discvideo_width();	// gets the width of the laserdisc video (only meaningful with mpeg)
	virtual bool lock_overlay(Uint32);	// prevents yuv callback from being called (only meaningful with mpeg)
	virtual bool unlock_overlay(Uint32);

	// sets the value of this boolean
	void set_use_nonblocking_searching(bool);

	// returns the value of this boolean
	bool get_use_nonblocking_searching();

	unsigned int get_elapsed_ms_since_play();

	// handles LDP-specific command-line arguments
	virtual bool handle_cmdline_arg(const char *arg);
	
	// Copies m_bug_log into 'log' and clears m_bug_log.
	// Used by releasetest.
	void get_bug_log(list<string> &log);

	// debug function used by the cpu debugger
	void print_frame_info();

protected:
	// helper function, shouldn't be called directly
	void increment_current_frame();

	bool need_serial;	// whether this LDP driver needs the serial port initialized
	bool serial_initialized; // whether serial has been initialized
	bool player_initialized; // whether the LDP has been properly initialized
	bool m_bIsVLDP;	// this is true if our LDP type is VLDP
	bool blitting_allowed;	// whether it's ok to blit directly to the screen (SMPEG forbids this)
	bool skipping_supported;	// whether the laserdisc player supports skipping
	bool skip_instead_of_search;	// whether we should skip instead of search if searching forward a short distance
	Uint16 max_skippable_frames;	// maximum # of frames that player can skip (if skipping is supported)
	Uint16 m_last_try_frame;	// the last frame we _tried_ to seek to
	Uint16 m_last_seeked_frame;	// the last frame we successfully seeked to (used with m_play_time to calculate current frame)
// UPDATE : we aren't using cycles anymore (see pre_think())
//	Uint64 m_play_cycles;	// # of elapsed cpu cycles from when we last issued a play command
	Uint32 m_play_time;	// current time when we last issued a play command
	unsigned int m_start_time;	// time when ldp() class was instantiated (only used when not using a cpu)
	int m_status;	// the current status of the laserdisc player
	Uint32 search_latency;	// how many ms to stall before searching (to simulate slow laserdisc players)
	bool m_stop_on_quit;	// should the LDP stop when it quits?
	Uint32 m_discvideo_width;	// width of laserdisc video (only meaningful with mpeg)
	Uint32 m_discvideo_height;	// height of laserdisc video (only meaningful with mpeg)
	bool m_use_nonblocking_searching;	// true if ldp-in drivers should use pre_search in non-blocking mode (as of now, blocking mode is more stable but non-blocking mode is more accurate)
	bool m_dont_get_search_result;	// if we should not be calling get_search_result()
	bool m_sram_continuous_update;  // if sram is to be updated on a regular basis

	// timer to be used to simulate search delay when in 'noldp' mode (for debugging)
	Uint32 m_noldp_timer;
	
	// used by 'releasetest' to do automatic self-testing
	list<string> m_bug_log;

	unsigned int m_uCurrentFrame;	// the current frame, as calculated by pre_think(), returned by get_current_frame()

	// current frame - m_last_seeked_frame (for VLDP's usage)
	// For example, the first frame displayed after playing is 0.
	unsigned int m_uCurrentOffsetFrame;

	// How many milliseconds have elapsed since we started playing the disc.
	// This value is changed by pre_think(), which must get called every 1 ms by the cpu loop
	// This allows us to keep in sync w/ the cpu w/o doing extra expensive calculations.
	unsigned int m_uElapsedMsSincePlay;

	// how many milliseconds have elapsed while we've been stuck doing blocking seeking
	// (blocking seeking is discouraged because it isn't proper emulation, but some real laserdisc players
	//  need to use it for skipping, and a few functions like step-forward use it,
	//  and it's probably not worth it to change that behavior as those functions are very rarely used)
	unsigned int m_uBlockedMsSincePlay;

	// if this is true, we won't increment m_uElapsedMsSincePlay until we get a vblank
	bool m_bWaitingForVblankToPlay;

	// the total offset of all frames we've skipped since play
	// (for example if I skipped +5 and then -10, this number would be -5)
	int m_iSkipOffsetSincePlay;

	// how many ms we have to surpass before increasing m_uCurrentOffsetFrame and m_uCurrentFrame
	unsigned int m_uMsFrameBoundary;

	// How many milliseconds have elapsed since Daphne started
	// This value is changed by pre_think() which is called every 1 ms by the cpu loop
	// This value is used to calculate how many emulated vblanks have occurred.
	unsigned int m_uElapsedMsSinceStart;

	// how many vblanks have occurred since Daphne started, as calculated by pre_think()
	unsigned int m_uVblankCount;

	// how many vblanks have occurred since last frame changed
	// (used if laserdisc FPS is half of vblank rate which is usually true)
	unsigned int m_uVblankMiniCount;

	// how many ms we have to surpass before increasing m_uVblankCount
	unsigned int m_uMsVblankBoundary;

	// How many frames to skip after displaying 1 frame (for playing at faster than 1X)
	unsigned int m_uFramesToSkipPerFrame;

	// How many frames to stall for after displaying 1 frame (for playing at slower than 1X)
	unsigned int m_uFramesToStallPerFrame;

	// State variable to keep track of whether we're stalling (according to m_uFramesToStallPerFrame)
	unsigned int m_uStallFrames;

private:
	// set to true if pre_init has been called (used to error checking)
	bool m_bPreInitCalled;

};

// same as regular ldp class but has no seek or skip delay (testing skipping games with skip delay is super annoying)
class fast_noldp : public ldp
{
	// see ldp.h for comments on how to use these functions
	bool nonblocking_search(char *);
	int get_search_result();
	bool skip_forward(Uint16 frames_to_skip, Uint16 target_frame);
	bool skip_backward(Uint16 frames_to_skip, Uint16 target_frame);
};

extern ldp *g_ldp;	// our global ldp class.  Defined here so that every .cpp file doesn't have to define it.

#endif
