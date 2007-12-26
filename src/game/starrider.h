/*
 * starrider.h
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

// starrider.h
// by Mark Broadhead

#include "game.h"

#define STARRIDER_OVERLAY_W 320	// width of overlay
#define STARRIDER_OVERLAY_H 256 // height of overlay

class starrider : public game
{
public:
	starrider();
	void do_nmi();		// does an NMI tick
	void do_irq();		// does an IRQ tick
	void do_firq();		// does a FIRQ tick
	Uint8 cpu_mem_read(Uint16 addr);			// memory read routine
	void cpu_mem_write(Uint16 addr, Uint8 value);		// memory write routine
	void input_enable(Uint8);
	void input_disable(Uint8);
	bool set_bank(unsigned char, unsigned char);
	void palette_calculate();
	void video_repaint();	// function to repaint video

private:
	int current_bank;
	bool firq_on;
	bool irq_on;
	bool nmi_on;
	Uint8 character[0x2000];		
	Uint8 rombank1[0xa000];		
	Uint8 rombank2[0x4000];		
//	SDL_Color colors[16];	// color palette
	Uint8 banks[3];				// starrider's banks
		// bank 1 is switches
		// bank 2 is dip switch 1
		// bank 3 is dip switch 2
};

