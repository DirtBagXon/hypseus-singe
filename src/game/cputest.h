/*
 * cputest.h
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


// cputest.h

#ifndef CPUTEST_H
#define CPUTEST_H

#include "game.h"

class cputest : public game
{
public:
	cputest();
	bool init();
	void shutdown();
	Uint8 port_read(Uint16);
	void port_write(Uint16 port, Uint8 value);
	void update_pc(Uint32);
	void set_preset(int);
	void start();
private:
	Uint32 m_speedtimer;
	unsigned int m_uZeroCount;
	bool m_bStarted;
};

#endif

