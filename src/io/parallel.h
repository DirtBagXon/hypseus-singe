/*
 * parallel.h
 *
 * Copyright (C) 2001-2009 Matt Ownby
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

#include "logger.h"

// This is now a static class to hide the global member variable(s) that it uses.
class par
{
public:
	static bool init(unsigned int port, ILogger *pLogger);
	static void base0(unsigned char data);
	static void base2(unsigned char data);
	static void close(ILogger *pLogger);

private:
	// index into the port arrays for the purpose of getting the port addresses
	static unsigned int m_uPortIdx;

	// base port address (last value reserved for custom address)
	static short m_base0[3];

	// base+2 port address (last value reserved for custom address)
	static short m_base2[3];

};

