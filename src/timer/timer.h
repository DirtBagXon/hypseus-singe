#ifndef TIMER_H
#define TIMER_H

/*
 * timer.h
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


// header file for timestuff.c

#include <SDL.h>

#ifndef GP2X
// not GP2X code ...
#define GET_TICKS SDL_GetTicks
#define MAKE_DELAY SDL_Delay
#else

// GP2X CODE HERE

extern "C"
{
unsigned int SDL_GP2X_GetMiniTicks();
}

unsigned int GP2X_GetTicks(unsigned int uMiniTicks);
#define GET_TICKS(dummy) GP2X_GetTicks(SDL_GP2X_GetMiniTicks())
#define MAKE_DELAY SDL_Delay
#endif // GP2X

unsigned int elapsed_ms_time(unsigned int previous_time);

// wrapper function to refer to GET_TICKS macro (in case the macro does not do a single function call!)
unsigned int GetTicksFunc();

// legacy functions
#define refresh_ms_time GET_TICKS
#define make_delay MAKE_DELAY

#endif // TIMER_H

