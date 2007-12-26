/*
 * esh.h
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

// esh.h
// by Matt Ownby

#include "game.h"

#define ESH_OVERLAY_W 256	// width of overlay
#define ESH_OVERLAY_H 256	// height of overlay

// # of colors in the esh's color palette
#define ESH_COLOR_COUNT 256

#define ESH_GAMMA 4.0	

class esh : public game
{
public:
	esh();
	void do_nmi();		// does an NMI tick
	void do_irq(unsigned int);		// does an IRQ tick
	void cpu_mem_write(Uint16 addr, Uint8 value);		// memory write routine
	Uint8 port_read(Uint16 port);
	void port_write(Uint16 port, Uint8 value);
	void input_enable(Uint8);
	void input_disable(Uint8);
	void palette_calculate();
	void video_repaint();	// function to repaint video
	void patch_roms();
	void set_version(int);

protected:
	Uint8 character[0x8000];		
	Uint8 miscprom[0x200];		
	Uint8 color_prom[0x200];		

private:
	bool m_needlineblink, m_needcharblink;
	int blank_count;
	Uint8 palette_high_bit;
	Uint8 banks[4];				// esh's banks
		// bank 1 is switches
		// bank 2 is switches
		// bank 3 is dip switch 1
		// bank 4 is dip switch 2
};
