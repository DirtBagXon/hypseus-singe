/*
 * pr8210.h
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

// PR-8210.h

#include <SDL.h>	// for Uint16

int get_pr8210_playing();
void pr8210_command(unsigned int);
void pr8210_audio1();
void pr8210_audio2();
void pr8210_seek();
Uint16 pr8210_get_current_frame();
void pr8210_add_digit(char);
void pr8210_reset();

// prints a 10-bit number in binary format (for debugging)
void pr8210_print_binary(unsigned int);
