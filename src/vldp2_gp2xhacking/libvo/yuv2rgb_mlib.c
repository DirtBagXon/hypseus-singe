/*
 * yuv2rgb_mlib.c
 * Copyright (C) 2000-2002 Håkan Hjort <d95hjort@dtek.chalmers.se>
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

#include "config.h"

#ifdef LIBVO_MLIB

#include <stddef.h>
#include <mlib_types.h>
#include <mlib_status.h>
#include <mlib_sys.h>
#include <mlib_video.h>
#include <inttypes.h>

#include "convert.h"
#include "convert_internal.h"

static void mlib_YUV2ARGB420_32 (void * _id, uint8_t * const * src,
				 unsigned int v_offset)
{
    convert_rgb_t * id = (convert_rgb_t *) _id;

    mlib_VideoColorYUV2ARGB420 (id->rgb_ptr + id->rgb_stride * v_offset,
				src[0], src[1], src[2],
				id->width, 16, id->rgb_stride,
				id->uv_stride << 1, id->uv_stride);
}

static void mlib_YUV2ABGR420_32 (void * _id, uint8_t * const * src,
				 unsigned int v_offset)
{
    convert_rgb_t * id = (convert_rgb_t *) _id;

    mlib_VideoColorYUV2ABGR420 (id->rgb_ptr + id->rgb_stride * v_offset,
				src[0], src[1], src[2],
				id->width, 16, id->rgb_stride,
				id->uv_stride << 1, id->uv_stride);
}

static void mlib_YUV2RGB420_24 (void * _id, uint8_t * const * src,
				unsigned int v_offset)
{
    convert_rgb_t * id = (convert_rgb_t *) _id;

    mlib_VideoColorYUV2RGB420 (id->rgb_ptr + id->rgb_stride * v_offset,
			       src[0], src[1], src[2],
			       id->width, 16, id->rgb_stride,
			       id->uv_stride << 1, id->uv_stride);
}

yuv2rgb_copy * yuv2rgb_init_mlib (int order, int bpp)
{
    if ((order == CONVERT_RGB) && (bpp == 24))
	return mlib_YUV2RGB420_24;
    else if ((order == CONVERT_RGB) && (bpp == 32))
	return mlib_YUV2ARGB420_32;
    else if ((order == CONVERT_BGR) && (bpp == 32))
	return mlib_YUV2ABGR420_32;
    return NULL;	/* Fallback to C */
}

#endif
