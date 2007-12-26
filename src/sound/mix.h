/*
 * mix.h
 *
 * Copyright (C) 2007 Matt Ownby
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

// mix.h

#ifndef MIX_H
#define MIX_H

#ifdef MAC_OSX
#include "mmxdefs.h"
#endif

#include <SDL.h>	// for datatype defs

// IMPORTANT: the MMX code depends on this structure beginning as it is displayed here!
// Be very cautious about changing this struct!
struct mix_s
{
	void *pMixBuf;
	struct mix_s *pNext;
};

// TO USE THE MIX FUNCTIONS:
// 1 - set g_pMixBufs to a pointer to a populated mix_s struct (that contains all your streams)
// 2 - set g_pSampleDst to the destination stream
// 3 - set g_uBytesToMix to how many bytes long all lines are (they all must be the same length).
//	IMPORTANT : g_uBytesToMix must be a multiple of 8, and must be >= 8!
// 4 - run g_mix_func() and you're done!

// we always want this function defined for the purpose of testing (releasetest.cpp)
void mix_c();

// Here we make some definitions so that the MMX/C code use identical syntax and variables
#ifdef USE_MMX

// For some reason, OSX was being retarded and insisted on putting _'s in front of all
//  assembly language variables.  So we've compensated for this insane behavior.

#if defined(MAC_OSX) || defined(WIN32)
extern "C" void mix_mmx();
extern "C" mix_s *asm_pMixBufs;
extern "C" Uint8 *asm_pSampleDst;
extern "C" Uint32 asm_uBytesToMix;
#define g_pMixBufs asm_pMixBufs
#define g_pSampleDst asm_pSampleDst
#define g_uBytesToMix asm_uBytesToMix
#else
extern "C" void _mix_mmx();
extern "C" mix_s *_asm_pMixBufs;
extern "C" Uint8 *_asm_pSampleDst;
extern "C" Uint32 _asm_uBytesToMix;
#define g_pMixBufs _asm_pMixBufs
#define g_pSampleDst _asm_pSampleDst
#define g_uBytesToMix _asm_uBytesToMix
#define mix_mmx _mix_mmx
#endif // MAC_OSX

// debug version gets some extra (slower) error checking
#ifdef DEBUG
#define g_mix_func debug_mix_mmx
void debug_mix_mmx();
#else
// here is the fast release version
#define g_mix_func mix_mmx
#endif

#else

// here is where we go if we're not using MMX

extern mix_s *g_pMixBufs;
extern Uint8 *g_pSampleDst;
extern Uint32 g_uBytesToMix;
#define g_mix_func mix_c
#endif // USE_MMX

/////////////////////////////

#endif
