/*
 * dac.h
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

#ifndef DAC_H
#define DAC_H

#include <SDL.h>	// for data-type defs

// init callback
int dac_init(Uint32 unused);

// should be called from the game driver
void dac_ctrl_data(unsigned int uSamplesSinceLastChange, unsigned int uByte, int internal_id);

// called from sound mixer to get audio stream
void dac_get_stream(Uint8 *stream, int length, int internal_id);

#endif // DAC_H
