/*
 * blend.h
 *
 * Copyright (C) 2005 Matt Ownby
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

// blend.h


#ifndef BLEND_H
#define BLEND_H

#ifdef MAC_OSX
#include "mmxdefs.h"
#endif

#include <SDL.h>	// for datatype defs

// TO USE THE BLEND FUNCTIONS:
// 1 - set g_blend_line1 to the first line of bytes to be averaged
// 2 - set g_blend_line2 to the second line of bytes to be averaged
// 3 - set g_blend_dest to the line of bytes that will be the destination
// 4 - set g_blend_iterations to how many bytes long all lines are (they all must be the same length).
//	IMPORTANT : g_blend_iterations must be a multiple of 8, and must be >= 8!
// 5 - run g_blend_func() and you're done!

// we always want this function defined for the purpose of testing (releasetest.cpp)
void blend_c();

// Here we make some definitions so that the MMX/C code use identical syntax and variables
#ifdef USE_MMX

#if defined (MAC_OSX) || defined (WIN32)
extern "C" void blend_mmx();
extern "C" Uint8 *asm_line1;
extern "C" Uint8 *asm_line2;
extern "C" Uint8 *asm_dest;
extern "C" Uint32 asm_iterations;
#define g_blend_line1 asm_line1
#define g_blend_line2 asm_line2
#define g_blend_dest asm_dest
#define g_blend_iterations asm_iterations
#else
extern "C" void _blend_mmx();
extern "C" Uint8 *_asm_line1;
extern "C" Uint8 *_asm_line2;
extern "C" Uint8 *_asm_dest;
extern "C" Uint32 _asm_iterations;
#define g_blend_line1 _asm_line1
#define g_blend_line2 _asm_line2
#define g_blend_dest _asm_dest
#define g_blend_iterations _asm_iterations
#define blend_mmx _blend_mmx
#endif // MAC_OSX

// debug version gets some extra (slower) error checking
#ifdef DEBUG
#define g_blend_func debug_blend_mmx
void debug_blend_mmx();
#else
// here is the fast release version
#define g_blend_func blend_mmx
#endif

#else

extern Uint8 *g_blend_line1;
extern Uint8 *g_blend_line2;
extern Uint8 *g_blend_dest;
extern Uint32 g_blend_iterations;
#define g_blend_func blend_c
#endif // USE_MMX

/////////////////////////////

#endif
