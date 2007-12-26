/*
 * video_out_sdl.c
 *
 * Copyright (C) 2000-2002 Ryan C. Gordon <icculus@lokigames.com> and
 *                         Dominik Schnitzer <aeneas@linuxvideo.org>
 *
 * SDL info, source, and binaries can be found at http://www.libsdl.org/
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

//#include "config.h"


#include <inttypes.h>

#include "video_out.h"
#include "interface_940.h"

extern struct libmpeg2_shared_vars_s *g_pShared;

// we expect this function to get called 3 times
static void vo940_setup_fbuf (vo_instance_t * _instance,
			    uint8_t ** buf, void ** id)
{
	static unsigned int uWhichBuf = 0;

    buf[0] = g_pShared->outBuf[uWhichBuf][0];
    buf[1] = g_pShared->outBuf[uWhichBuf][1];
    buf[2] = g_pShared->outBuf[uWhichBuf][2];

	// id will refer to the buffer index
	*id = (void *) uWhichBuf;

	uWhichBuf++;

	// wraparound (this should never happen, because this function should only be called 3 times)
	uWhichBuf = uWhichBuf % OUTBUFCOUNT;
}

static void vo940_draw_frame (vo_instance_t * _instance,
			    uint8_t * const * buf, void * id)
{
	// indicate which buffer has the new frame
	g_pShared->uAckOutBuf = (unsigned int) id;

	// tell cpu1 that we have a new frame
	g_pShared->uAckFrameCount = (g_pShared->uAckFrameCount + 1);

	// wait for CPU1 to get the new frame
	while (g_pShared->uReqFrameCount != g_pShared->uAckFrameCount)
	{
	}
}

static int vo940_setup (vo_instance_t * _instance, int width, int height,
		      vo_setup_result_t * result)
{
	g_pShared->uOutX = width;
	g_pShared->uOutY = height;
    result->convert = NULL;
    return 0;
}

// don't want to use malloc, because the 2nd cpu doesn't have this function
vo_instance_t g_vo940_instance;

vo_instance_t * vo940_open (void)
{
	vo_instance_t *instance = &g_vo940_instance;

    instance->setup = vo940_setup;
    instance->setup_fbuf = vo940_setup_fbuf;
    instance->set_fbuf = NULL;
    instance->start_fbuf = NULL;
    instance->discard = NULL;
    instance->draw = vo940_draw_frame;
    instance->close = NULL;

    return (vo_instance_t *) instance;
}

