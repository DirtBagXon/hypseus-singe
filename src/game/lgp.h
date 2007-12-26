/*
* lgp.h
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


//lgp.h

#ifndef LGP_H
#define LGP_H

#include "game.h"

#define LGP_OVERLAY_W 256 // width of overlay
#define LGP_OVERLAY_H 256 // height of overlay

#define LGP_COLOR_COUNT 256

class lgp : public game
{
public:
	lgp();
	void do_irq(unsigned int);		// does an IRQ tick
	void do_nmi();		// does an NMI tick
	Uint8 cpu_mem_read(Uint16 addr);			// memory read routine
	void cpu_mem_write(Uint16 addr, Uint8 value);		// memory write routine
	Uint8 port_read(Uint16 port);		// read from port
	void port_write(Uint16 port, Uint8 value);		// write to a port
	virtual void input_enable(Uint8);
	virtual void input_disable(Uint8);
	bool set_bank(Uint8, Uint8);
	void video_repaint();	// function to repaint video
protected:
	Uint8 m_soundchip1_id;   
	Uint8 m_soundchip2_id;
	Uint8 m_soundchip3_id;   
	Uint8 m_soundchip4_id;
	Uint8 m_soundchip1_address_latch;
	Uint8 m_soundchip2_address_latch;
	Uint8 m_soundchip3_address_latch;
	Uint8 m_soundchip4_address_latch;
	Uint8 m_cpumem2[0x10000];
	void draw_sprite(int);
	Uint8 m_ldp_write_latch;
	Uint8 m_ldp_read_latch;
	Uint8 m_character[0x8000];	
	Uint8 m_transparent_color;	// which color is to be transparent
	bool palette_modified;		// has our palette been modified?
	Uint8 ldp_output_latch;	// holds data to be sent to the LDV1000
	Uint8 ldp_input_latch;	// holds data that was retrieved from the LDV1000
	bool nmie;
	Uint8 banks[7];
	void recalc_palette();
	void draw_8x8(int character_number, int xcoord, int ycoord);
};

#endif
