/*
 * cop.h
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

/****************************************************************************/
/*                                                                          */
/*         Portable National Semiconductor COP420/421 MCU emulator          */
/*                                                                          */
/*	   Copyright 2001 Mark Broadhead, released under the GPL 2 License		*/
/*																			*/
/* The COP MCU has an internal clock divider that is mask programmed into	*/
/* the device. Make sure it's set correctly in the frequency to call		*/
/* cop421_execute(unsigned int cycles). Thayer's Quest uses a 32 divider	*/
/* and Looping uses a 4 divider.											*/
/*																			*/
/* Some of the functions of the 420	aren't implemented, including the inil	*/
/* and inin commands. Also some	functions of the EN register aren't			*/
/* implemented.																*/
/*                                                                          */
/****************************************************************************/

#include "copintf.h"

// Execute functions (call these for core execution)
unsigned int cop421_execute(unsigned int); // Main function, pass number of cycles to execute
void cop421_reset(); // Reset COP
void cop421_setmemory(unsigned char *); // Pass in pointer to internal COP ROM

// Interface functions (these need to be written for the application)
extern void write_d_port(unsigned char); // Write to D port
extern void write_l_port(unsigned char); // Write L port
extern unsigned char read_l_port(void); // Read L port
extern void write_g_port(unsigned char); // Write G I/O Port
extern unsigned char read_g_port(void); // Read to G I/O port
extern void write_so_bit(unsigned char); // Write to SO
extern unsigned char read_si_bit(void); // Read to SI

