/*
 * vp932.h
 *
 * Copyright (C) 2005-2007 Mark Broadhead
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


#ifndef VP932_H
#define VP932_H

// these functions are the only ones to be called externally
void vp932_write(Uint8 data);	// write a byte to the vp932
Uint8 vp932_read();	// read a byte from the vp932
							// make sure that vp932_data_available() returns true before reading data
bool vp932_data_available(); // check to see if data is available to read


// internal functions - don't call these
void vp932_process_command();
void restoreAudio();

#endif
