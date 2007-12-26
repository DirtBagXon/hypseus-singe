/*
 * seektest.h
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

// seektest.h
// by Matt Ownby

#include "game.h"

class seektest : public game
{
public:
	seektest();
	void start();
	void go(Uint16 target_frame);
	void input_enable(Uint8);
	void input_disable(Uint8);
	void set_preset(int);
	bool handle_cmdline_arg(const char *arg);
	void palette_calculate();
	void video_repaint();

private:
	Uint16 m_early1, m_early2;	// two early frames we can alternate between
	Uint16 m_late1, m_late2;	// two late frames we can alternate between
	char m_name[81];	// name of the game
	
	bool m_locked;	// if the seektest in "locked" mode?
	bool m_overlay;	// video overlay enabled?
	Sint32 m_frame_offset;	// how much to adjust current frame by
	bool m_multimpeg;	// whether the mpeg is expected to be split up into multiple parts
	int *m_multimpeg_frames;	// array that holds each frame (if m_multimpeg is true), terminated with negative number
};

