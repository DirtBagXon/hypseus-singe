/*
* sn_intf.cpp
*
* Copyright (C) 2005 Mark Broadhead
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

// This code is to interface the Daphne sound system with tms9919-sdl.cpp

#ifdef DEBUG
#include <assert.h>
#endif

#include "../io/conout.h"
#include "sound.h"
#include "tms9919.hpp"
#include "tms9919-sdl.hpp"

// this can be set arbitrarily large (as large as it needs to be to support all games)
#define MAX_TMS9919_CHIPS 2

cSdlTMS9919 *g_paSoundChips[MAX_TMS9919_CHIPS] = { NULL };

// how many chips have been allocated so far
int g_uTMS9919Index = 0;

// returns the internal index of the sound chip that has just been created
//  (so that it can be referenced quickly)
int tms9919_initialize(Uint32 core_frequency)
{
	int result = -1;

	// check for overflow
	if (g_uTMS9919Index < MAX_TMS9919_CHIPS)
	{
		g_paSoundChips[g_uTMS9919Index] = new cSdlTMS9919();
		g_paSoundChips[g_uTMS9919Index]->set_core_frequency(core_frequency);
		result = g_uTMS9919Index++;
	}
	return result;
}

void tms9919_writedata(Uint8 data, int index)
{
#ifdef DEBUG
	assert((index >= 0) && (index < g_uTMS9919Index));
#endif
	g_paSoundChips[index]->WriteData(data);
}

void tms9919_stream(Uint8* stream, int length, int index)
{
#ifdef DEBUG
	assert((index >= 0) && (index < g_uTMS9919Index));
#endif
	g_paSoundChips[index]->AudioCallback(stream, length);
}

void tms9919_shutdown(int index)
{
#ifdef DEBUG
	assert((index >= 0) && (index < g_uTMS9919Index));
#endif
	delete g_paSoundChips[index];
	g_paSoundChips[index] = NULL;

	// NOTE : g_uTMS9919Index cannot be decremented here because we are using an array
}
