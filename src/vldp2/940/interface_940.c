/*
 * vldp_940_interface.c
 *
 * Copyright (C) 2007 Matt Ownby
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

// contains the "thread" that will eventually run on another cpu (940)

#ifdef WIN32
#include "../vc++/inttypes.h"
#else
#include <inttypes.h>
#endif

#include "mpeg2.h"
#include "video_out.h"
#include "interface_940.h"

////////////////////////////////////////////////

#ifndef NULL
#define NULL 0
#endif // NULL

static mpeg2dec_t *g_mpeg_data = 0;	// structure for libmpeg2's state
static vo_instance_t *s_video_output = 0;

static struct libmpeg2_shared_vars_s *g_pShared = 0;

////////////////////////////////////////////////

void mpo_strncpy(char *dst, const char *src, unsigned int uLimit)
{
	unsigned int u = 0;

	for (;;)
	{
		dst[u] = src[u];

		// if we've reached the null terminator, then break
		if (src[u] == 0)
		{
			break;
		}

		u++;

		// if we've reached our buffer limit
		if (u >= uLimit)
		{
			// terminate string, then bail
			dst[u-1] = 0;
			break;
		}
	}
}

void do_error(const char *s)
{
	mpo_strncpy(g_pShared->logStr, s, sizeof(g_pShared->logStr));
	g_pShared->uAckLogCount++;

	// wait until cpu1 has received this log message, because we don't want log messages to get lost
	while (g_pShared->uReqLogCount != g_pShared->uAckLogCount);
}

/////////////////

// decode_mpeg2 function taken from mpeg2dec.c and optimized a bit
static void decode_mpeg2 (uint8_t * current, uint8_t * end)
{
    const mpeg2_info_t * info;
    int state;
    vo_setup_result_t setup_result;

    mpeg2_buffer (g_mpeg_data, current, end);

    info = mpeg2_info (g_mpeg_data);
	// loop until we return (state is -1)
	for (;;)
    {
		state = mpeg2_parse (g_mpeg_data);
		switch (state)
		{
		case -1:
		    return;
		case STATE_SEQUENCE:
		    /* might set nb fbuf, convert format, stride */
		    /* might set fbufs */
		    if (s_video_output->setup (s_video_output, info->sequence->width,
				       info->sequence->height, &setup_result))
			{
				do_error("display setup failed");
		    }
	
		    if (setup_result.convert)
				mpeg2_convert (g_mpeg_data, setup_result.convert, NULL);
					    
		    // if this driver offers a setup_fbuf callback ...
//		    else if (s_video_output->setup_fbuf)
// we KNOW we offer a setup_fbuf function, so get rid of this conditional
		    {
				uint8_t * buf[3];
				void * id;
	
				s_video_output->setup_fbuf (s_video_output, buf, &id);
				mpeg2_set_buf (g_mpeg_data, buf, id);
				s_video_output->setup_fbuf (s_video_output, buf, &id);
				mpeg2_set_buf (g_mpeg_data, buf, id);
				s_video_output->setup_fbuf (s_video_output, buf, &id);
				mpeg2_set_buf (g_mpeg_data, buf, id);
		    }
		    break;
		case STATE_PICTURE:
		    /* might skip */
		    /* might set fbuf */

			// all possible stuff we could've done here was removed because the null driver
			// doesn't do any of it
		    
		    break;
		case STATE_PICTURE_2ND:
		    /* should not do anything */
		    break;
		case STATE_SLICE:
		case STATE_END:
		    /* draw current picture */
		    /* might free frame buffer */
			// if the init hasn't been called yet, this may fail so we have to put the conditional
		    if (info->display_fbuf)
		    {
				s_video_output->draw (s_video_output, info->display_fbuf->buf,
				      info->display_fbuf->id);
		    }
		    
		    break;
		} // end switch
    } // end endless for loop
}

/////////////////

// returns C940_NO_COMMAND is no command is waiting
int mp2_940_peek_command()
{
	int iResult = C940_NO_COMMAND;

	// if a new command is waiting
	if (g_pShared->uAckCount != g_pShared->uReqCount)
	{
		iResult = g_pShared->uReqCmd;
	}

	return iResult;
}

// tell cpu1 that cpu2 has received the command
void mp2_940_ack_command()
{
	g_pShared->uAckCount = g_pShared->uReqCount;
}

void mp2_940_do_decode()
{
	// copy variables we need before we ack
	unsigned int uWhichBuf = g_pShared->uReqInBuf;
	unsigned int uBufSize = g_pShared->uInBufSize[uWhichBuf];
	unsigned char *pBufStart = g_pShared->inBuf[uWhichBuf];
	unsigned char *pBufEnd = pBufStart + uBufSize;

	if (uBufSize == 0)
	{
		do_error("mp2_940_do_decode: uBufSize is 0!  This can't be right...");
	}

	// tell CPU1 that we will start decoding the proper buffer
	mp2_940_ack_command();

	decode_mpeg2(pBufStart, pBufEnd);

	// tell cpu1 that we're done with this buffer
	g_pShared->uAckBufCount = g_pShared->uReqBufCount;
}

void mp2_940_do_reset()
{
	mpeg2_partial_init(g_mpeg_data);
	mp2_940_ack_command();
}

// when building on x86/linux to test this stuff ...
extern struct libmpeg2_shared_vars_s g_SharedVars;

void mp2_940_setup_g_pShared()
{
#ifdef GP2X
	// TODO : g_pShared should point to a static memory location
#else
	g_pShared = &g_SharedVars;
#endif // GP2X
}

// this is the main function that starts on the 940
void *mp2_940_start(void *ignored)
{
	int done = 0;

	mp2_940_setup_g_pShared();

	s_video_output = (vo_instance_t *) vo940_open();	// open 'null' driver (we just pass decoded frames to parent thread)
	if (s_video_output)
	{
		g_mpeg_data = mpeg2_init();
	}
	else
	{
		do_error("VLDP : Error opening LIBVO!");
		done = 1;
	}

	do_error("CPU2 Running!");	// not really an error :)

	// wait for commands from CPU1
	while (!done)
	{
		int iCmd = mp2_940_peek_command();
		switch (iCmd)
		{
		default:	// no command
			break;
		case C940_QUIT:
			done = 1;
			mp2_940_ack_command();
			break;
		case C940_LIBMPEG2_DECODE:
			mp2_940_do_decode();
			break;
		case C940_LIBMPEG2_RESET:
			mp2_940_do_reset();
			break;
		}
	} // end while we have not received a quit command

	do_error("CPU2 shutting down ...");

	mpeg2_close(g_mpeg_data);	// shutdown libmpeg2

	if (s_video_output->close)
	{
		s_video_output->close(s_video_output);		// shutdown video driver
	}

	return 0;
}

