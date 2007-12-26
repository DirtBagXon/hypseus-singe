/*
* tonegen.cpp
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

#ifndef TONEGEN_H
#define TONEGEN_H

#define VOICES 4

int tonegen_initialize(Uint32);
void tonegen_writedata(Uint32, Uint32, int index);
void tonegen_stream(Uint8* stream, int length, int index);

struct tonegen
{
	int bytes_per_switch[VOICES];
	int flip[VOICES];
	int bytes_to_go[VOICES];
	Sint16 amplitude[VOICES];
};

#endif
