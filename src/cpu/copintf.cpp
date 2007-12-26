/*
 * copintf.cpp
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

#include "../game/game.h"
#include "../game/thayers.h"

// NOTE : these static_cast's are rather hackish, they assume that only thayer's quest will ever use the COP
// You may need to fix these later

void write_d_port(unsigned char stuff) 
{
	(static_cast<thayers*> (g_game))->thayers_write_d_port(stuff);
}

void write_l_port(unsigned char stuff) 
{
	(static_cast<thayers*> (g_game))->thayers_write_l_port(stuff);
}

unsigned char read_l_port(void)
{
	return (static_cast<thayers*> (g_game))->thayers_read_l_port();
}

void write_g_port(unsigned char stuff) 
{
	(static_cast<thayers*> (g_game))->thayers_write_g_port(stuff);
}

unsigned char read_g_port(void)
{
	return (static_cast<thayers*> (g_game))->thayers_read_g_port();
}

void write_so_bit(unsigned char stuff) 
{
	(static_cast<thayers*> (g_game))->thayers_write_so_bit(stuff);
}

unsigned char read_si_bit(void)
{
	return (static_cast<thayers*> (g_game))->thayers_read_si_bit();
}
