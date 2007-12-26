/*
 * vldp.c
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

// see vldp.h for documentation on how to use the API

#include <stdio.h>
#include <string.h>
#include "vldp.h"
#include "vldp_common.h"

#define API_VERSION 11

//////////////////////////////////////////////////////////////////////////////////////

int vldp_cmd(int cmd);
int vldp_wait_for_status(int stat);

//////////////////////////////////////////////////////////////////////////////////////

SDL_Thread *private_thread = NULL;

int p_initialized = 0;	// whether VLDP has been initialized

Uint8 g_req_cmdORcount = CMDORCOUNT_INITIAL;	// the current command parent thread requests of the child thread
unsigned int g_ack_count = ACK_COUNT_INITIAL;	// the result returned by the internal child thread
char g_req_file[STRSIZE];	// requested mpeg filename
Uint32 g_req_timer = 0;	// requests timer value to be used for mpeg playback
Uint16 g_req_frame = 0;		// requested frame to search to
Uint32 g_req_min_seek_ms = 0;	// seek must take at least this many milliseconds (simulate laserdisc seek delay)
unsigned int g_req_precache = VLDP_FALSE;	// whether g_req_idx has any meaning
unsigned int g_req_idx = 0;	// multipurpose index (used by precaching)
unsigned int g_req_skip_per_frame = 0;	// how many frames to skip per frame (for playing at 2X for example)
unsigned int g_req_stall_per_frame = 0;	// how many frames to stall per frame (for playing at 1/2X for example)
struct vldp_out_info g_out_info;	// contains info that the parent thread should have access to
const struct vldp_in_info *g_in_info;	// contains info from parent thread that VLDP should have access to

/////////////////////////////////////////////////////////////////////

// issues a command to the internal thread and returns 1 if the internal thread acknowledged our command
// or 0 if we timed out without getting a response
// NOTE : this does not mean that the internal thread has finished executing our requested command, only
// that the command has been received
int vldp_cmd(int cmd)
{
	int result = 0;
	Uint32 cur_time = g_in_info->GetTicksFunc();
	Uint8 tmp = g_req_cmdORcount;	// we want to replace the real value atomically so we use a tmp variable first
	static unsigned int old_ack_count = ACK_COUNT_INITIAL;

	tmp++;	// increment the counter so child thread knows we're issuing a new command
	tmp &= 0xF;	// strip off old command
	tmp |= cmd;	// replace it with new command
	g_req_cmdORcount = tmp;	// here is the atomic replacement
	
	// loop until we timeout or get a response
	while ((g_in_info->GetTicksFunc() - cur_time) < VLDP_TIMEOUT)
	{
		// if the count has changed, it means the other thread has acknowledged our new command
		if (g_ack_count != old_ack_count)
		{
			result = 1;
			old_ack_count = g_ack_count;	// prepare to receive the next command
			break;
		}
		SDL_Delay(0);	// switch to other thread
	}

	// if we weren't able to communicate, notify user
	if (!result)
	{
		fprintf(stderr, "VLDP error!  Timed out waiting for internal thread to accept command!\n");
	}
	
	return result;
}

// waits until the disc status is 'stat'
// if the stat we want is received, return 1 (success)
// if we time out, or if we get an error stat, return 0 (error)
// if we are legitimately busy, we return 2 (busy)
int vldp_wait_for_status(int stat)
{
	int result = 0;	// assume error unless we explicitly
	int done = 0;
	Uint32 cur_time = g_in_info->GetTicksFunc();
	while (!done && ((g_in_info->GetTicksFunc() - cur_time) < VLDP_TIMEOUT))
	{
		if (g_out_info.status == stat)
		{
			done = 1;
			result = 1;
		}
		else if (g_out_info.status == STAT_ERROR)
		{
			done = 1;
		}
		// else keep waiting
		SDL_Delay(0);	// switch to other thread, timing is critical so we delay for 0
	}

	// if we timed out but are busy, indicate that
	if (g_out_info.status == STAT_BUSY)
	{
		result = 2;
	}

	// else if we timed out
	else if ((g_in_info->GetTicksFunc() - cur_time) >= VLDP_TIMEOUT)
	{
		fprintf(stderr, "VLDP ERROR!!!!  Timed out with getting our expected response!\n");
	}

	return result;
}

//////////////////////////////////////////////////////////

void vldp_shutdown()
{
	// only shutdown if we have previous initialized
	if (p_initialized)
	{
		vldp_cmd(VLDP_REQ_QUIT);
		SDL_WaitThread(private_thread, NULL);	// wait for private thread to terminate
	}
	p_initialized = 0;
}

// requests that we open an mpeg file
// IMPORTANT : This function returns immediately once the open command is received, but the
// file will not be open until the VLDP status is 'VLDP_STOPPED'.  Parent thread take heed.
int vldp_open(const char *filename)
{
	int result = 0;	// assume we're busy so the loop below works the first time
	FILE *F = NULL;
	
	if (p_initialized)
	{
		F = fopen(filename, "rb");
		// if file exists, we can open it
		if (F)
		{
			fclose(F);
			SAFE_STRCPY(g_req_file, filename, sizeof(g_req_file));
			g_req_precache = VLDP_FALSE;	// we're not precaching ...
			result = vldp_cmd(VLDP_REQ_OPEN);
		}
		else
		{
			fprintf(stderr, "VLDP ERROR : can't open file %s\n", filename);
		}
	}

	return result;
}

VLDP_BOOL vldp_open_precached(unsigned int uIdx, const char *filename)
{
	VLDP_BOOL bResult = VLDP_FALSE;

	if (p_initialized)
	{
		// even though we're using an index, we still need filename to compute .dat filename
		SAFE_STRCPY(g_req_file, filename, sizeof(g_req_file));
		g_req_idx = uIdx;
		g_req_precache = VLDP_TRUE;
		bResult = vldp_cmd(VLDP_REQ_OPEN);
	}

	return bResult;
}

int vldp_open_and_block(const char *filename)
{
	int result = 0;

	result = vldp_open(filename);

	// if the open message was received ...
	if (result)
	{
		result = 2;	// 2 is the code that wait_for_status returns when it is busy ...
		while (result == 2)
		{
			result = vldp_wait_for_status(STAT_STOPPED);
			SDL_Delay(1);	// timing isn't critical here, so we can delay for 1 instead of 0
		}
	}

	return result;
}

VLDP_BOOL vldp_precache(const char *filename)
{
	VLDP_BOOL bResult = VLDP_FALSE;

	if (p_initialized)
	{
		SAFE_STRCPY(g_req_file, filename, sizeof(g_req_file));
		bResult = vldp_cmd(VLDP_REQ_PRECACHE);
	}
	// else return false

	return bResult;
}

// issues search command and returns immediately to parent thread.
// Search will not be complete until the VLDP status is STAT_PAUSED
int vldp_search(Uint16 frame, Uint32 min_seek_ms)
{
	int result = 0;

	if (p_initialized)
	{
		g_req_frame = frame;
		g_req_min_seek_ms = min_seek_ms;
		result = vldp_cmd(VLDP_REQ_SEARCH);
	}
	return result;
}

// issues search command blocks until search is complete
int vldp_search_and_block(Uint16 frame, Uint32 min_seek_ms)
{
	int result = 0;

	if (p_initialized)
	{
		g_req_frame = frame;
		g_req_min_seek_ms = min_seek_ms;
		vldp_cmd(VLDP_REQ_SEARCH);
		result = vldp_wait_for_status(STAT_PAUSED);
	}
	return result;
}

int vldp_play(Uint32 timer)
{
	int result = 0;

	if (p_initialized)
	{
		g_req_timer = timer;
		vldp_cmd(VLDP_REQ_PLAY);
		result = vldp_wait_for_status(STAT_PLAYING);	// play could get an error if we're at EOF
	}
	return(result);
}

int vldp_skip(Uint16 frame)
{
	int result = 0;

	// we can only skip if the mpeg is already playing (esp. since we don't accept a timer as an argument)
	if (p_initialized && (g_out_info.status == STAT_PLAYING))
	{
		g_req_frame = frame;
		g_req_min_seek_ms = 0;	// just for safety purposes, we want to ensure that there is no minimum skip delay
		result = vldp_cmd(VLDP_REQ_SKIP);
#ifdef VLDP_DEBUG
		if (!result) fprintf(stderr, "vldp_cmd rejected SKIP request!\n");
#endif
	}

#ifdef VLDP_DEBUG
	// remove me once this bug is discovered
	else
	{
		fprintf(stderr, "VLDP skip failed because one of these happened: \n");
		fprintf(stderr, "p_initialized is %d\n", p_initialized);
		fprintf(stderr, "g_out_info.status == %d\n", g_out_info.status);
	}
#endif // VLDP_DEBUG

	return result;
}

int vldp_pause()
{
	int result = 0;
	if (p_initialized)
	{
		result = vldp_cmd(VLDP_REQ_PAUSE);
	}
	return result;
}

int vldp_step_forward()
{
	int result = 0;
		
	if (p_initialized)
	{
		result = vldp_cmd(VLDP_REQ_STEP_FORWARD);
	}
	return result;
}

int vldp_stop()
{	
	return 0;	// this is a useless function which I haven't finished yet anyway, so don't use it :)
}

VLDP_BOOL vldp_speedchange(unsigned int uSkipPerFrame, unsigned int uStallPerFrame)
{
	VLDP_BOOL result = VLDP_FALSE;

	if (p_initialized)
	{
		g_req_skip_per_frame = uSkipPerFrame;
		g_req_stall_per_frame = uStallPerFrame;
		result = vldp_cmd(VLDP_REQ_SPEEDCHANGE);
	}
	return result;
}

VLDP_BOOL vldp_lock(unsigned int uTimeoutMs)
{
	VLDP_BOOL result = VLDP_FALSE;
	
	if (p_initialized)
	{
		result = vldp_cmd(VLDP_REQ_LOCK);
	}
	return result;
}

VLDP_BOOL vldp_unlock(unsigned int uTimeoutMs)
{
	VLDP_BOOL result = VLDP_FALSE;
	
	if (p_initialized)
	{
		result = vldp_cmd(VLDP_REQ_UNLOCK);
	}
	return result;
}

///////////////////////////////////////////////////////////////////////////

// This comes at the end so I can avoid putting function declarations in vldp.h
// I want to keep these functions hidden and force the user to use the callbacks
const struct vldp_out_info *vldp_init(const struct vldp_in_info *in_info)
{
	const struct vldp_out_info *result = NULL;
	p_initialized = 0;

	g_in_info = in_info;

	// So parent thread knows if it's compatible with us
	g_out_info.uApiVersion = API_VERSION;

	// populate function pointers for parent thread's benefit
	g_out_info.shutdown = vldp_shutdown;
	g_out_info.open = vldp_open;
	g_out_info.open_precached = vldp_open_precached;
	g_out_info.open_and_block = vldp_open_and_block;
	g_out_info.precache = vldp_precache;
	g_out_info.play = vldp_play;
	g_out_info.search = vldp_search;
	g_out_info.search_and_block = vldp_search_and_block;
	g_out_info.skip = vldp_skip;
	g_out_info.pause = vldp_pause;
	g_out_info.step_forward = vldp_step_forward;
	g_out_info.stop = vldp_stop;
	g_out_info.speedchange = vldp_speedchange;
	g_out_info.lock = vldp_lock;
	g_out_info.unlock = vldp_unlock;

	private_thread = SDL_CreateThread(idle_handler, NULL);	// start our internal thread
	
	// if private thread was created successfully
	if (private_thread)
	{
		p_initialized = 1;
		result = &g_out_info;
	}

	return result;
}
