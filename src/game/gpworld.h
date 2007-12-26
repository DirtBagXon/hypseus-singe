/*
 * gpworld.h
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


//gpworld.h

#ifndef GPWORLD_H
#define GPWORLD_H

#include "game.h"

#define GPWORLD_OVERLAY_W 360 // width of overlay
#define GPWORLD_OVERLAY_H 256 // height of overlay

#define GPWORLD_VID_ADDRESS 0xd000
#define GPWORLD_SPRITE_ADDRESS 0xc000

// we really need 512, but 256 is the max we can use with an 8 bit palette
#define GPWORLD_COLOR_COUNT 256

// START modified Mame code
#define SPR_Y_TOP		0
#define SPR_Y_BOTTOM	1
#define SPR_X_LO		2
#define SPR_X_HI		3
#define SPR_SKIP_LO		4
#define SPR_SKIP_HI		5
#define SPR_GFXOFS_LO	6
#define SPR_GFXOFS_HI	7
// END modified Mame code

class gpworld : public game
{
public:
	gpworld();
	void do_irq(unsigned int);		// does an IRQ tick
	void do_nmi();		// does an NMI tick
	Uint8 cpu_mem_read(Uint16 addr);			// memory read routine
	void cpu_mem_write(Uint16 addr, Uint8 value);		// memory write routine
	Uint8 port_read(Uint16 port);		// read from port
	void port_write(Uint16 port, Uint8 value);		// write to a port
	virtual void input_enable(Uint8);
	virtual void input_disable(Uint8);
	bool set_bank(Uint8, Uint8);
	void palette_calculate();
	void video_repaint();	// function to repaint video
	virtual void write_ldp(Uint8, Uint16);
	virtual Uint8 read_ldp(Uint16);
protected:
	void recalc_palette();
	void draw_sprite(int);
	Uint8 rombank[0x8000];	
	Uint8 character[0x1000];	
	Uint8 sprite[0x30000];	
	Uint8 miscprom[0x220];	
	SDL_Color palette_lookup[4096];		// all possible color entries
	int tile_color_pointer[256];
	Uint8 m_transparent_color;	// which color is to be transparent
	bool palette_modified;		// has our palette been modified?
	Uint8 ldp_output_latch;	// holds data to be sent to the LDV1000
	Uint8 ldp_input_latch;	// holds data that was retrieved from the LDV1000
	bool nmie;
	Uint8 banks[7];	
};

#endif
