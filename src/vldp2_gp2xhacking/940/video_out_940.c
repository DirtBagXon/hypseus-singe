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
#include "mpo_lib.h"

extern struct libmpeg2_shared_vars_s *g_pShared;

// where we will work with the image
static unsigned char *g_pOutBuf[3][3];

// size of Y, U, and V buffers
static unsigned int g_uYSize, g_uUVSize;

// width and height of image
static unsigned int g_uWidth, g_uHeight;

static unsigned char g_pTwoLineBuf[720*2*2];	// max line length (720) * 2 lines * 2 bytes per pixel

// we expect this function to get called 3 times
static void vo940_setup_fbuf (vo_instance_t * _instance,
			    uint8_t ** buf, void ** id)
{
	static unsigned int uWhichBuf = 0;

	/*
    buf[0] = g_pShared->outBuf[uWhichBuf][0];
    buf[1] = g_pShared->outBuf[uWhichBuf][1];
    buf[2] = g_pShared->outBuf[uWhichBuf][2];
	*/

	buf[0] = g_pOutBuf[uWhichBuf][0];
	buf[1] = g_pOutBuf[uWhichBuf][1];
	buf[2] = g_pOutBuf[uWhichBuf][2];

	// id will refer to the buffer index
	*id = (void *) uWhichBuf;

	uWhichBuf++;

	// wraparound (this should never happen, because this function should only be called 3 times)
	uWhichBuf = uWhichBuf % 3;
}

// converts YV12 surface to YUY2 surface
void YV12_to_YUY2(void *dst, unsigned int uID)
{
	unsigned int uDstPitch = g_uWidth << 1;	// 2 bytes per pixel, so we multiply by 2
	unsigned int uDstPitchX2 = uDstPitch << 1;	// used more than once
	unsigned char *dst_ptr = dst;
	unsigned char *Y = (unsigned char *) g_pOutBuf[uID][0];
	unsigned char *Y2 = Y + g_uWidth;
	unsigned char *U = (unsigned char *) g_pOutBuf[uID][1];
	unsigned char *V = (unsigned char *) g_pOutBuf[uID][2];
	int col, row;

	// do 2 rows at a time
	for (row = 0; row < (g_uHeight >> 1); row++)
	{
		// do 4 bytes at a time, but only iterate for w_half
		for (col = 0; col < (g_uWidth << 1); col += 4)
		{
			unsigned int Y_chunk = *((unsigned short *) Y);
			unsigned int Y2_chunk = *((unsigned short *) Y2);
			unsigned int V_chunk = *V;
			unsigned int U_chunk = *U;
			unsigned char *p2 = g_pTwoLineBuf + uDstPitch;	// we want to only do 1 memcpy for hopefully a speed improvement

			//Little-Endian (PC)
			*((unsigned int *) (g_pTwoLineBuf + col)) = (Y_chunk & 0xFF) | (U_chunk << 8) |
				((Y_chunk & 0xFF00) << 8) | (V_chunk << 24);
			*((unsigned int *) (p2 + col)) = (Y2_chunk & 0xFF) | (U_chunk << 8) |
				((Y2_chunk & 0xFF00) << 8) | (V_chunk << 24);
			
			Y += 2;
			Y2 += 2;
			U++;
			V++;
		}

		// copy 2 lines in one shot (copying to slow memory, which is why we do it all at once)
		mpo_memcpy(dst_ptr, g_pTwoLineBuf, uDstPitchX2);
		
		dst_ptr += uDstPitchX2;	// we've done 2 rows, so skip a row
		Y += g_uWidth;	// we've done 2 vertical Y pixels, so skip a row
		Y2 += g_uWidth;
	}
}

// this function exists solely to work around arm gcc optimization bug
int get_available_frame_buffers()
{
	return (OUTBUFCOUNT - (g_pShared->uAckFrameCount - g_pShared->uReqFrameCount));
}

int uDummy2 = 0;	// to workaround arm optimization bug

static void vo940_draw_frame (vo_instance_t * _instance,
			    uint8_t * const * buf, void * id)
{
	unsigned int uID = (unsigned int) id;
	unsigned int uOutBufIdx;

	// if we don't have to wait for cpu1 (good)
	if (get_available_frame_buffers())
	{
		g_pShared->uAckFrameNoStalls++;
	}
	// else if we have to wait for cpu1 (bad)
	else
	{
		g_pShared->uAckFrameStalls++;
	}

	// If we have no available frame buffers because CPU1 is too slow, then block
	// (we really don't want this to happen because it slows down our decoding)
	do
	{
		// this is written this way to workaround gcc optimizer bug
		uDummy2 += 3;
	} while (get_available_frame_buffers() == 0);

	// compute which buffer to overwrite now
	// (don't increment AckFrameCount until the frame is actually where we want it)
	uOutBufIdx = (g_pShared->uAckFrameCount + 1) & OUTBUFMASK;

	// if we are supposed to return a YUY2 surface instead of a YV12 one
	if (g_pShared->uReqYUY2)
	{
		// NOTE : we are doing something slightly dangerous by departing from the outBuf array structure here, but it should be ok
		YV12_to_YUY2(g_pShared->outBuf[uOutBufIdx][0], uID);
	}
	else
	{
		// copy frame into (slow) shared memory so cpu1 can access it
		mpo_memcpy(g_pShared->outBuf[uOutBufIdx][0], g_pOutBuf[uID][0], g_uYSize);
		mpo_memcpy(g_pShared->outBuf[uOutBufIdx][1], g_pOutBuf[uID][1], g_uUVSize);
		mpo_memcpy(g_pShared->outBuf[uOutBufIdx][2], g_pOutBuf[uID][2], g_uUVSize);
	}

	// indicate which buffer has the new frame
	g_pShared->uAckOutBuf = uOutBufIdx;

	// tell cpu1 that we have a new frame
	g_pShared->uAckFrameCount = (g_pShared->uAckFrameCount + 1);

}

static int vo940_setup (vo_instance_t * _instance, int width, int height,
		      vo_setup_result_t * result)
{
	int i = 0;

	// the goal here is to only allocate these buffers once because we don't support de-allocation
	const unsigned int uFixedSize = 720 * 480;	// the max size that we will support

	g_uWidth = width;
	g_uHeight = height;
	g_uYSize = width * height;
	g_uUVSize = g_uYSize >> 2;

	// if our buffers haven't been allocated yet
	if (g_pOutBuf[0][0] == 0)
	{
		for (i = 0; i < 3; i++)
		{
			g_pOutBuf[i][0] = mpo_memalign(16, uFixedSize);
			g_pOutBuf[i][1] = mpo_memalign(16, uFixedSize >> 2);
			g_pOutBuf[i][2] = mpo_memalign(16, uFixedSize >> 2);
		}
	}
	// else it's already been initialized, don't do it again because we don't have a working de-allocator

	g_pShared->uOutX = width;
	g_pShared->uOutY = height;
	g_pShared->uOutYSize = width * height;
	g_pShared->uOutUVSize = (width * height) >> 2;
    result->convert = NULL;

    return 0;
}

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
	g_pOutBuf[0][0] = 0;	// make sure this gets initialized

    return (vo_instance_t *) instance;
}

