/*
 * vldp.h
 *
 * Copyright (C) 2001 Matt Ownby
 *
 * This file is part of VLDP, a virtual laserdisc player.
 *
 * VLDP is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * VLDP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// API for the mpeg2 virtual laserdisc player
// by Matt Ownby, Mar 16th, 2003

#ifndef VLDP_H
#define VLDP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <SDL.h>	// only used for threading

struct yuv_buf
{
	unsigned char *Y;	// Y channel
	unsigned char *U;	// U channel
	unsigned char *V;	// V channel
	unsigned int Y_size;	// size in bytes of Y
	unsigned int UV_size;	// size in bytes of U and V
};

struct yuy2_buf
{
	unsigned char *YUY2;	// beginning of buffer (virtual)
	unsigned int uPhysAddress;	// beginning of buffer (physical, only used on gp2x)
	unsigned int uSize;	// size of buffer
};

// safe strcpy that null-terminates the end of a string
// Use this instead of strcpy always!
#define SAFE_STRCPY(dst,src,size) strncpy(dst, src, size); dst[size-1] = 0;

// since this is C and not C++, we can't use booleans ...
enum { VLDP_FALSE=0, VLDP_TRUE=1 } typedef VLDP_BOOL;

// callback functions and state information provided to VLDP from the parent thread
struct vldp_in_info
{
	unsigned int uApiVersion;	// so child thread knows if it's compatible with parent thread

	// VLDP calls this when it has a frame ready to draw, but before it has slept to maintain
	// a consistent framerate.  You should do all cpu-intensive cycles to prepare the frame
	// to be drawn here.  The remaining cycles you don't used will be used by VLDP to sleep
	// until it's time for the frame to be displayed.
	// This returns 1 if the frame was prepared successfully, or 0 on error
	int (*prepare_frame)(struct yuv_buf *buf);

	// same as prepare_frame except sends down a YUY2 buffer
	int (*prepare_yuy2_frame)(struct yuy2_buf *buf);

	// VLDP calls this when it wants the frame that was earlier prepared to be displayed
	// ASAP
	void (*display_frame)(struct yuv_buf *buf);

	// same as display_frame except is used when a yuy2 image is being sent
	void (*display_yuy2_frame)();
	
	// VLDP calls this when it is doing the time consuming process of reporting an mpeg parse
	void (*report_parse_progress)(double percent_complete);

	// VLDP calls this to tell the parent-thread what the dimensions of the mpeg are (as soon we know ourselves!)
	// This comes before draw_frame to that draw_frame can be more optimized
	void (*report_mpeg_dimensions)(int width, int height);

	// VLDP calls this when it wants the parent-thread to render a blank frame
	void (*render_blank_frame)();

	///////////

	int blank_during_searches;	// if this is non-zero, VLDP will call render_blank_frame before every search
	int blank_during_skips;	// if this is non-zero, VLDP will call render_blank_frame before every skip
	unsigned int uMsTimer;	// the timer that VLDP will use for everything (replaces SDL_GetTicks()). Calling thread is responsible for updating this timer!!

	// Callback to get an arbitrary millisecond timer (such as SDL_GetTicks)
	// (for instances when we know uMsTimer will not be updated, we will call this function instead)
	unsigned int (*GetTicksFunc)();
};

// functions and state information provided to the parent thread from VLDP
struct vldp_out_info
{
	unsigned int uApiVersion;	// so parent thread knows if it's compatible with this version of VLDP

	// shuts down VLDP, de-allocates any memory that was allocated, etc ...
	void (*shutdown)();
	
	// All of the following functions return 1 on success, 0 on failure.

	// open an mpeg file and returns immediately.  File won't actually be open until
	// 'status' is equal to STAT_STOPPED.
	int (*open)(const char *filename);

	// like 'open' except it blocks until 'status' is equal to STAT_STOPPED (until all potential parsing has finished, etc)
	// does not return on STAT_BUSY
	// returns 1 if status is STAT_STOPPED or 0 if status ended up to be something else
	int (*open_and_block)(const char *filename);

	// Precaches the indicated file to memory, so that when 'open' is called, the file will
	//  be read from memory instead of the filesystem (for increased speed).
	// IMPORTANT : file won't be precached until status == STAT_STOPPED
	// Returns VLDP_TRUE if the precaching succeeded or VLDP_FALSE on failure.
	VLDP_BOOL (*precache)(const char *filename);

	// Instructus VLDP to 'open' a file that has been precached.  It is referred to
	//  by its precache index instead of a filename.  Behavior is similar to 'open'.
	VLDP_BOOL (*open_precached)(unsigned int uIdx, const char *filename);

	// plays the mpeg that has been previously open.  'timer' is the value relative to uMsTimer that
	// we should use for the beginning of the first frame that will be displayed
	// returns 0 on failure, 1 on success, 2 on busy
	int (*play)(Uint32 timer);
	
	// searches to a frame relative to the beginning of the mpeg (0 is the first frame)
	// 'min_seek_ms' is the minimum # of milliseconds that this seek must take
	//  (for the purpose of simulating laserdisc seek delay)
	// returns immediately, but search is not complete until 'status' is STAT_PAUSED
	// returns 1 if command was acknowledged, or 0 if we timed out w/o getting acknowlegement
	int (*search)(Uint16 frame, Uint32 min_seek_ms);
	
	// like search except it blocks until the search is complete
	// 'min_seek_ms' is the minimum # of milliseconds that this seek must take
	//  (for the purpose of simulating laserdisc seek delay)
	// returns 1 if search succeeded, 2 if search is still going, 0 if search failed
	// (so does not do true blocking, we could change this later)
	int (*search_and_block)(Uint16 frame, Uint32 min_seek_ms);
	
	// skips to 'frame' and immediately begins playing.
	// the mpeg is required to be playing before skip is called, because we accept no new timer as reference
	int (*skip)(Uint16 frame);
	
	// pauses mpeg playback
	int (*pause)();
	
	// steps one frame forward while paused
	int (*step_forward)();
	
	// stops playback
	int (*stop)();

	// Changes the speed of playback (while still maintaining framerate)
	// For example, to get 2X, uSkipPerFrame = 1, uStallPerFrame = 0
	// To get 3X,  uSkipPerFrame = 2, uStallPerFrame = 0
	// To get 1/2X, uSkipPerFrame = 0, uStallPerFrame = 1
	VLDP_BOOL (*speedchange)(unsigned int uSkipPerFrame, unsigned int uStallPerFrame);

	// Attempts to make sure the VLDP sits idly (mainly to prevent it from calling any callbacks)
	// Returns true if VLDP has been locked, or false if we timed out
	VLDP_BOOL (*lock)(unsigned int uTimeoutMs);
	
	// Unlocks a previous lock operation. Returns true if unlock was successful, or false if we timed out.
	VLDP_BOOL (*unlock)(unsigned int uTimeoutMs);
	
	////////////////////////////////////////////////////////////

	// State information for the parent thread's benefit
	unsigned int uFpks;	// FPKS = frames per kilosecond (FPS = uFpks / 1000.0)
	unsigned int u2milDivFpks; // (2000000) / uFpks (pre-calculated, used to determine whether to drop frames)
	Uint8 uses_fields;	// whether the video uses fields or not
	Uint32 w;	// width of the mpeg video
	Uint32 h;	// height of the mpeg video
	int status;	// the current status of the VLDP (see STAT_ enum's)
	unsigned int current_frame;	// the current frame of the opened mpeg that we are on
	unsigned int uLastCachedIndex;	// the index of the file that was last precached (if any)
};

enum
{
	STAT_ERROR, STAT_BUSY, STAT_STOPPED, STAT_PLAYING, STAT_PAUSED
};

#ifdef WIN32
// this is the only function that gets exported by this DLL
__declspec(dllexport) const struct vldp_out_info *vldp_init(const struct vldp_in_info *in_info);
#else

// initializes the VLDP
// 'in_info' contains a few callback functions from parent thread that VLDP needs to use
// returns a pointer to output functions on success or NULL on failure
const struct vldp_out_info *vldp_init(const struct vldp_in_info *in_info);

#endif

#ifdef __cplusplus
}
#endif

#endif	// VLDP_H
