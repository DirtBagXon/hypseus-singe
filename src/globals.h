/*
 * globals.h
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

#ifndef GLOBALS_H
#define GLOBALS_H

// Some global data is stored here
// *** Try to avoid using globals if possible!!!!! ***

game *g_game = NULL;	// pointer to the game class that emulator will use
ldp *g_ldp = NULL;		// pointer to the ldp class that emulator will use

Uint8 quitflag = 0;	// 1 = we are ready to quit the program

Uint8 serial_port = 0;	// which serial port to use to control LDP
int baud_rate = 9600;	// which baud rate they want the serial port to function at
int search_offset = 0;	// Value to add to every frame we search for (0 unless we have a PAL Dragon's Lair disc and the NTSC roms)
						// PAL disc starts at frame 1, NTSC disc starts at frame 153.  So the search_offset for PAL would
						// be -152

Uint8 frame_modifier = 0;	// what type of frame modifications to be used
							// the types of modifications possible are enumerated, and they
							// includ Space Ace 91 and PAL

Uint8 joystick_detected = 0;	// 0 = no joystick, 1 = joystick present

Uint8 realscoreboard = 0;	// 1 = external scoreboard is attached
unsigned int rsb_port = 0;		// 0 = LPT1, 1 = LPT2 or address of real scoreboard 

unsigned int idleexit; // added by JFA for -idleexit
unsigned char startsilent = 0;

#endif
