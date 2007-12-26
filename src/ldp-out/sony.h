/*
 * daphne.cpp
 *
 * Copyright (C) 2001 Robert DiNapoli
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


// sony.h header

#ifndef SONY_H
#define SONY_H

#include "ldp.h"

class sony : public ldp
{
public:
	sony();
//	bool search(char *);
	bool nonblocking_search(char *frame);
	int get_search_result();
	unsigned int play();
	void pause();
	void stop();
	void enable_audio1();
	void enable_audio2();
	void disable_audio1();
	void disable_audio2();
//	Uint16 get_current_frame();

	bool receive_status(Uint8);
	void searchforframe(char *);
	void resetplayer();
};

#endif
