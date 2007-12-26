/*
 * mach3.h
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

// timetrav.h

#ifndef TIMETRAV_H
#define TIMETRAV_H

#include "game.h"

#define TIMETRAV_CPU_HZ 5000000		// speed of cpu (5 MHz ?)

class timetrav : public game
{
public:
	timetrav();
	void do_nmi();
	Uint8 cpu_mem_read(Uint32 addr);
	void cpu_mem_write(Uint32 addr, Uint8 value);
	Uint8 port_read(Uint16);
	void port_write(Uint16, Uint8);
	void input_enable(Uint8);
	void input_disable(Uint8);
	bool set_bank(unsigned char, unsigned char);
	void palette_calculate();

protected:
};

#endif
