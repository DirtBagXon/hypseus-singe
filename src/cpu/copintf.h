/*
 * copintf.h
 *
 * Copyright (C) 2001 Mark Broadhead
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

#ifndef COPINTF_H
#define COPINTF_H

void write_d_port(unsigned char); // Write to D port
void write_l_port(unsigned char); // Write L port
unsigned char read_l_port(void); // Read L port
void write_g_port(unsigned char); // Write G I/O port
unsigned char read_g_port(void); // Read to G I/O port
void write_so_bit(unsigned char); // Write to SO
unsigned char read_si_bit(void); // Read to SI

#endif
