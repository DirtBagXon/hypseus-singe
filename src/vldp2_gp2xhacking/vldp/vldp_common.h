/*
 * vldp_common.h
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

// shared by the public and private VLDP threads
// nothing else should include this

#ifndef VLDP_COMMON_H
#define VLDP_COMMON_H

#include <SDL.h>
#include <SDL_thread.h>

#define VLDP_REQ_NONE	0x0
#define VLDP_REQ_OPEN	0x10
#define VLDP_REQ_SEARCH	0x20
#define VLDP_REQ_PLAY	0x30
#define VLDP_REQ_PAUSE	0x40
#define VLDP_REQ_STEP_FORWARD	0x50
#define VLDP_REQ_STOP	0x60
#define VLDP_REQ_STATUS	0x70
#define VLDP_REQ_QUIT	0x80
#define VLDP_REQ_SKIP	0x90
#define VLDP_REQ_LOCK	0xA0
#define VLDP_REQ_UNLOCK	0xB0
#define VLDP_REQ_SPEEDCHANGE 0xC0
#define VLDP_REQ_PRECACHE 0xD0

// these are defined to emphasize the importance of the same value being initialized in more than one place
#define CMDORCOUNT_INITIAL 0
#define ACK_COUNT_INITIAL 0

// how big all our character arrays will be
// (needs to be able to accomodate huge paths)
#define STRSIZE 320

extern Uint16 g_req_frame;	// which frame to seek to
extern Uint32 g_req_min_seek_ms;	// minimum # of milliseconds that this seek can take
extern Uint32 g_req_timer;
extern unsigned int g_req_idx;	// multipurpose index
extern unsigned int g_req_precache;
extern char g_req_file[];	// which file to open
extern Uint8 g_req_cmdORcount;	// the current command count OR'd with the current command of parent thread
								// made 8-bit to ensure that it's atomic
extern unsigned int g_ack_count;	// how many times we've acknowledged a command

extern unsigned int g_ack_init_done;	// child thread notifies parent thread that it's ok to return from vldp_init

extern struct vldp_out_info g_out_info;	// contains info that the parent thread should have access to
extern const struct vldp_in_info *g_in_info;	// contains info from parent thread that VLDP should have access to

extern unsigned int g_req_skip_per_frame;	// how many frames to skip per frame (for playing at 2X for example)
extern unsigned int g_req_stall_per_frame;	// how many frames to stall per frame (for playing at 1/2X for example)

int idle_handler(void *);

// how ms to wait for responses from the private thread before we give up and return an error
// NOTE : increased from 5000 now that artificial seek delay functionality is added
#define VLDP_TIMEOUT	7500

#endif
