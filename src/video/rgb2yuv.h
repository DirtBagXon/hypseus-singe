/*
 * rgb2yuv.h
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

#ifndef RGB2YUV_H
#define RGB2YUV_H

#ifdef MAC_OSX
#include "mmxdefs.h"
#endif

#ifdef USE_MMX

#define rgb2yuv asm_rgb2yuv
#define rgb2yuv_input asm_rgb2yuv_input
#define rgb2yuv_result_y asm_rgb2yuv_result_y
#define rgb2yuv_result_u asm_rgb2yuv_result_u
#define rgb2yuv_result_v asm_rgb2yuv_result_v

#if defined(MAC_OSX) || defined(WIN32)
extern "C" void asm_rgb2yuv();
extern "C" unsigned short asm_rgb2yuv_input[3];	// this must be a short due to the way the MMX code is laid out
extern "C" unsigned char asm_rgb2yuv_result_y;
extern "C" unsigned char asm_rgb2yuv_result_u;
extern "C" unsigned char asm_rgb2yuv_result_v;
#else
extern "C" void _asm_rgb2yuv();
extern "C" unsigned short _asm_rgb2yuv_input[3];	// this must be a short due to the way the MMX code is laid out
extern "C" unsigned char _asm_rgb2yuv_result_y;
extern "C" unsigned char _asm_rgb2yuv_result_u;
extern "C" unsigned char _asm_rgb2yuv_result_v;
#define asm_rgb2yuv _asm_rgb2yuv
#define asm_rgb2yuv_input _asm_rgb2yuv_input
#define asm_rgb2yuv_result_y _asm_rgb2yuv_result_y
#define asm_rgb2yuv_result_u _asm_rgb2yuv_result_u
#define asm_rgb2yuv_result_v _asm_rgb2yuv_result_v
#endif // OSX/WIN32

#else

// the C version of this routine
void rgb2yuv();
extern unsigned int rgb2yuv_input[3];	// 8-bit
extern unsigned int rgb2yuv_result_y;	// 8-bit
extern unsigned int rgb2yuv_result_u;	// 8-bit
extern unsigned int rgb2yuv_result_v;	// 8-bit

#endif

/////////////////////////////

#endif
