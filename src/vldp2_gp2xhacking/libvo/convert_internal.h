/*
 * convert_internal.h
 * Copyright (C) 2000-2002 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
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

typedef struct {
    uint8_t * rgb_ptr;
    int width;
    int uv_stride, uv_stride_frame;
    int rgb_stride, rgb_stride_frame;
    void (* yuv2rgb) (uint8_t *, uint8_t *, uint8_t *, uint8_t *,
		      void *, void *, int);
} convert_rgb_t;

typedef void yuv2rgb_copy (void * id, uint8_t * const * src,
			   unsigned int v_offset);

yuv2rgb_copy * yuv2rgb_init_mmxext (int bpp, int mode);
yuv2rgb_copy * yuv2rgb_init_mmx (int bpp, int mode);
yuv2rgb_copy * yuv2rgb_init_mlib (int bpp, int mode);
