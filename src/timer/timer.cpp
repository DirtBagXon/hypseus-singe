/*
 * timer.cpp
 *
 * Copyright (C) 1999-2006 Matt Ownby
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

// by Matt Ownby, 11 August 1999

// Purpose: To provide a way for our emulator to track very precise time
// Second purpose: To keep this source code portable

#include "timer.h"

// returns the elapsed time (in milliseconds) since the current
// time and previous_time (which is also in milliseconds)
unsigned int elapsed_ms_time(unsigned int previous_time)
{
	return(GET_TICKS() - previous_time);
}

unsigned int GetTicksFunc()
{
	return GET_TICKS();
}

#ifdef GP2X
unsigned int g_uLastTicks = 0;
unsigned int g_uExtraMs = 0;

unsigned int GP2X_GetTicks(unsigned int uMiniTicks)
{
	// gp2x's mini ticks can be converted to ms by dividing by 7372.8, but we used 7373 for speed
	const unsigned int DIVIDER = 7373;

	// # of milliseconds in one cycle before 32-bit timer wraps around
	const unsigned int CYCLE_MS = 0xFFFFFFFF / DIVIDER;

	unsigned int uTicks = uMiniTicks / DIVIDER;

	// has wraparound occurred?
	if (uMiniTicks < g_uLastTicks)
	{
		g_uExtraMs += CYCLE_MS;	// add one cycle to the extra
	}

	g_uLastTicks = uTicks;
	uTicks += g_uExtraMs;	// add whatever extra we've accumulated

	return uTicks;
}
#endif // GP2X
