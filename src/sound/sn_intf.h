/*
 * sn_intf.h
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

#ifndef SN_INTF_H
#define SN_INTF_H

int tms9919_initialize(Uint32 core_frequency);
void tms9919_writedata(Uint8, int index);
void tms9919_stream(Uint8* stream, int length, int index);
void tms9919_shutdown(int index);

#endif
