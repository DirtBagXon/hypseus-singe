/*
 * pr7820.h
 *
 * Copyright (C) 2003 Warren Ondras
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


// PR7820.H

#ifndef PR7820_H
#define PR7820_H

bool read_pr7820_ready();
void write_pr7820 (unsigned char value);
void pr7820_clear(void);
Uint16 pr7820_get_buffered_frame(void);
void pr7820_add_digit(char);
void pr7820_pre_audio1();
void pr7820_pre_audio2();
void reset_pr7820();

#endif
