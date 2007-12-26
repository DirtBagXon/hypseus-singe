/*
 * vldp_internal.h
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

// should only be used by the vldp private thread!

#ifndef VLDP_INTERNAL_H
#define VLDP_INTERNAL_H

#include "vldp.h"	// for the VLDP_BOOL definition and SDL.h

// this is which version of the .dat file format we are using
#define DAT_VERSION 2

// header for the .DAT files that are generated
struct dat_header
{
	Uint8 version;	// which version of the DAT file this is
	Uint8 finished;	// whether the parse finished parsing or was interrupted
	Uint8 uses_fields;	// whether the stream uses fields or frames
	Uint32 length;	// length of the m2v stream
};

struct precache_entry_s
{
	void *ptrBuf;	// buffer that holds precached file
	unsigned int uLength;	// length (in bytes) of the buffer
	unsigned int uPos;	// our current position within the stream
};

int idle_handler(void *surface);
void blank_video();
void erase_yuv_overlay(SDL_Overlay *dst);
int ivldp_got_new_command();
void ivldp_ack_command();
void ivldp_lock_handler();
void paused_handler();
void play_handler();
void ivldp_set_framerate(Uint8 frame_rate_code);
void vldp_process_sequence_header();
void idle_handler_open();
void idle_handler_precache();
void idle_handler_play();
void ivldp_respond_req_play();
void ivldp_respond_req_pause_or_step();
void ivldp_respond_req_speedchange();
void ivldp_render();
void idle_handler_search(int skip);
VLDP_BOOL ivldp_get_mpeg_frame_offsets(char *mpeg_name);
VLDP_BOOL ivldp_parse_mpeg_frame_offsets(char *datafilename, Uint32 mpeg_size);
void ivldp_update_progress_indicator(SDL_Surface *indicator, double percentage_completed);

VLDP_BOOL io_open(const char *cpszFilename);
VLDP_BOOL io_open_precached(unsigned int uIdx);
unsigned int io_read(void *buf, unsigned int uBytesToRead);
VLDP_BOOL io_seek(unsigned int uPos);
void io_close();
VLDP_BOOL io_is_open();
unsigned int io_length();

///////////////////////////////////////

extern Uint8 s_old_req_cmdORcount;	// the last value of the command byte we received
extern int s_paused;	// whether the video is to be paused
extern int s_blanked;	// whether the mpeg video is to be blanked
extern int s_frames_to_skip;	// how many frames to skip before rendering the next frame (used for P and B frames seeking)
extern int s_frames_to_skip_with_inc;	// how many frames to skip while increasing the frame number (for multi-speed playback)
extern int s_skip_all;	// skip all subsequent frames.  Used to bail out of the middle of libmpeg2, back to vldp
extern unsigned int s_uSkipAllCount;	// how many frames we've skipped when s_skip_all is enabled.
extern int s_step_forward;	// if this is set, we step forward 1 frame
extern Uint32 s_timer;	// FPS timer used by the blitting code to run at the right speed
extern Uint32 s_extra_delay_ms;	// any extra delay that null_draw_frame() will use before drawing a frame (intended for laserdisc seek delay simulation)
extern Uint32 s_uFramesShownSinceTimer;	// how many frames should've been rendered (relative to s_timer) before we advance
extern int s_overlay_allocated;	// whether the SDL overlays have been allocated

// Which frame we've skipped to (0 if we haven't skipped)
// Used in order to maintain the current frame number until the skip actually occurs.
extern unsigned int s_uPendingSkipFrame;

extern SDL_Overlay *s_hw_overlay;	// if the game uses video overlay, we can't modify our buffers, so we have to
							// copy to the extra overlay and let that get displayed

extern unsigned int s_skip_per_frame;	// how many frames to skip per frame (for playing at 2X for example)
extern unsigned int s_stall_per_frame;	// how many frames to stall per frame (for playing at 1/2X for example)

#endif
